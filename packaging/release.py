#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build this repository's one provider, native installer and portable package."""
from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text())
REPOSITORY = MANIFEST["repository"]
SLUG = REPOSITORY.lower()
PACKAGE = "infiltrator-" + SLUG
VERSION = (ROOT / "VERSION").read_text().strip()
if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", VERSION):
    raise ValueError("VERSION must contain a three-part release version")
RESOURCE = Path("System/Resources") / ("Calendars" if MANIFEST["kind"] == "calendar" else "Clocks") / (MANIFEST["name"] + ".so")
DOCUMENTATION = Path("System/Resources/Documentation") / REPOSITORY
EXCLUDED = {".git", "build", "dist", "__pycache__", ".build", ".pytest_cache"}


def run(args, *, cwd=ROOT, env=None, capture=False):
    print("+ " + shlex.join(map(str, args)), flush=True)
    return subprocess.run(list(map(str, args)), cwd=cwd, env=env, check=True,
                          text=True, stdout=subprocess.PIPE if capture else None).stdout


def source_files():
    files = []
    for folder, directories, names in os.walk(ROOT):
        directories[:] = sorted(d for d in directories if d not in EXCLUDED)
        for name in sorted(names):
            path = Path(folder) / name
            if name == ".git":
                continue
            if path.is_symlink():
                raise ValueError("Source symlinks are not supported: " + str(path))
            if name.endswith((".pyc", ".deb", ".run", ".zip", ".gcda", ".gcno")):
                continue
            files.append(path.relative_to(ROOT))
    return sorted(files)


def revision():
    saved = ROOT / "SOURCE-REVISION"
    if saved.exists():
        return saved.read_text().strip()
    return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()


def epoch():
    # Fixed archive timestamps make all three artifacts independently reproducible.
    value = os.environ.get("SOURCE_DATE_EPOCH", "1704067200")
    if not value.isdecimal():
        raise ValueError("Invalid SOURCE_DATE_EPOCH")
    return int(value)


def check_requirements():
    missing = [x for x in ("gcc", "make", "pkg-config", "dpkg", "dpkg-deb", "readelf", "strip") if not shutil.which(x)]
    if subprocess.run([sys.executable, "-c", "import reportlab"], stdout=subprocess.DEVNULL,
                      stderr=subprocess.DEVNULL).returncode != 0:
        missing.append("python3-reportlab")
    dependencies = MANIFEST["build_dependencies"]
    if not missing and dependencies:
        if subprocess.run(["pkg-config", "--exists", *dependencies]).returncode:
            missing.extend(dependencies)
    return missing


def ensure_requirements(provision):
    if not check_requirements():
        return
    if not provision:
        raise RuntimeError("Missing build prerequisites: " + ", ".join(check_requirements()))
    command = ["apt-get", "-o", "DPkg::Lock::Timeout=300"]
    if os.geteuid() != 0:
        command = ["sudo", "--", *command]
    run([*command, "update"])
    packages = ["build-essential", "binutils", "pkg-config", "python3", "python3-reportlab"]
    if "glib-2.0" in MANIFEST["build_dependencies"]:
        packages.append("libglib2.0-dev")
    if "icu-i18n" in MANIFEST["build_dependencies"]:
        packages.append("libicu-dev")
    run([*command, "install", "-y", "--no-install-recommends", *packages])
    if check_requirements():
        raise RuntimeError("Build prerequisites remain unavailable")


def compile_provider(profile, jobs):
    build = ROOT / "build/package"
    shutil.rmtree(build, ignore_errors=True)
    build.mkdir(parents=True)
    flags = ["-O3" if profile == "aggressive" else "-O2", "-g0", "-pipe", "-flto", "-DNDEBUG"]
    if profile == "portable":
        if platform.machine() != "x86_64":
            raise RuntimeError("The portable release package must be built on amd64")
        flags += ["-march=x86-64", "-mtune=generic"]
    elif platform.machine() == "x86_64":
        flags += ["-march=native", "-mtune=native"]
    else:
        flags += ["-mcpu=native"]
    library = build / (MANIFEST["name"] + ".so")
    env = dict(os.environ, LANGUAGE="en_US", LC_ALL="C.UTF-8", SOURCE_DATE_EPOCH=str(epoch()))
    # Environment CFLAGS retain the Makefile's mandatory ABI/security flags.
    # A command-line CFLAGS assignment would discard its += additions.
    def make(extra):
        if any(flag.startswith("-fprofile-use=") for flag in extra):
            shutil.rmtree(build / "common", ignore_errors=True)
        env["CFLAGS"] = shlex.join(flags + extra)
        env["LDFLAGS"] = shlex.join(["-flto", "-Wl,-z,relro,-z,now", *extra])
        env.pop("CPPFLAGS", None)
        env.pop("LDLIBS", None)
        run(["make", "--no-print-directory", f"-j{jobs}", "CC=gcc", "BUILD_DIR=build/package", "check"], env=env)
    if profile == "aggressive":
        profiles = build / "pgo"
        profiles.mkdir()
        make(["-fprofile-generate=" + str(profiles), "-fprofile-update=atomic"])
        run([sys.executable, "packaging/train.py", library], env=env)
        if not list(profiles.rglob("*.gcda")):
            raise RuntimeError("PGO training did not produce any measured profile")
        # Same output/source paths in both passes: missing/stale profiles fail.
        make(["-fprofile-use=" + str(profiles), "-fprofile-correction", "-fprofile-partial-training"])
    else:
        make([])
    run(["strip", "--strip-unneeded", library])
    # Check the exact stripped bytes that will be installed.
    run([sys.executable, "tests/check_provider.py", library], env=env)
    run([sys.executable, "tests/check_accuracy.py", library], env=env)
    return library, flags


def runtime_dependencies(library):
    text = run(["readelf", "--version-info", library], capture=True)
    versions = re.findall(r"\bGLIBC_([0-9]+(?:\.[0-9]+)+)\b", text)
    minimum = max(versions, key=lambda s: tuple(map(int, s.split(".")))) if versions else "2.34"
    needed = run(["readelf", "-d", library], capture=True)
    names = re.findall(r"\(NEEDED\).*?\[(.*?)\]", needed)
    allowed = {"libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "libglib-2.0.so.0"}
    if set(names) - allowed:
        raise RuntimeError("Unaccounted runtime libraries: " + repr(set(names) - allowed))
    dependencies = ["libc6 (>= " + minimum + ")"]
    if "libglib-2.0.so.0" in names:
        dependencies.append("libglib2.0-0 (>= 2.74) | libglib2.0-0t64 (>= 2.74)")
    if "icu-i18n" in MANIFEST["build_dependencies"]:
        dependencies.append(" | ".join("libicu" + str(v) for v in range(72, 82)))
    return ", ".join(dependencies)


def build_deb(profile, output, jobs):
    ensure_requirements(False)
    library, flags = compile_provider(profile, jobs)
    arch = run(["dpkg", "--print-architecture"], capture=True).strip()
    package_version = VERSION + ("+native1" if profile != "portable" else "")
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"{PACKAGE}_{package_version}_{arch}.deb"
    with tempfile.TemporaryDirectory(prefix="provider-package-") as temporary:
        stage = Path(temporary)
        target = stage / RESOURCE
        target.parent.mkdir(parents=True)
        shutil.copyfile(library, target)
        target.chmod(0o644)
        documentation = stage / DOCUMENTATION
        documentation.mkdir(parents=True)
        guide = documentation / (MANIFEST["name"] + ".pdf")
        run([sys.executable, "packaging/build_guide.py", guide])
        control = stage / "DEBIAN"
        control.mkdir()
        (control / "control").write_text(
            f"Package: {PACKAGE}\nVersion: {package_version}\nArchitecture: {arch}\n"
            "Section: libs\nPriority: optional\nMaintainer: Infiltrator Projects <noreply@infiltrator-projects.invalid>\n"
            f"Depends: {runtime_dependencies(library)}\n"
            "Replaces: infiltrator-calendar-resources\n"
            f"Homepage: https://github.com/Infiltrator-Projects/{REPOSITORY}\n"
            f"X-Infiltrator-Source-Revision: {revision()}\n"
            f"X-Infiltrator-Native-Profile: {profile}\n"
            f"X-Infiltrator-LTO: enabled\n"
            f"X-Infiltrator-PGO: {'trained-two-pass' if profile == 'aggressive' else 'disabled'}\n"
            f"X-Infiltrator-Cflags: {shlex.join(flags)}\n"
            f"X-Infiltrator-Resource-SHA256: {hashlib.sha256(target.read_bytes()).hexdigest()}\n"
            f"Description: {MANIFEST['name']} temporal resource\n"
            " Independently owned provider module for Infiltrator temporal consumers.\n")
        for directory, _, names in os.walk(stage):
            os.chmod(directory, 0o755)
            for name in names:
                file = Path(directory) / name
                os.chmod(file, 0o644)
                os.utime(file, (epoch(), epoch()))
            os.utime(directory, (epoch(), epoch()))
        run(["dpkg-deb", "--root-owner-group", "-Zxz", "--build", stage, destination],
            env=dict(os.environ, SOURCE_DATE_EPOCH=str(epoch())))
    return destination


def archive_sources():
    entries = {str(path): (ROOT / path).read_bytes() for path in source_files()}
    entries["SOURCE-REVISION"] = (revision() + "\n").encode()
    buffer = io.BytesIO()
    with tarfile.open(fileobj=buffer, mode="w", format=tarfile.USTAR_FORMAT) as archive:
        for name, data in sorted(entries.items()):
            item = tarfile.TarInfo("source/" + name)
            item.size, item.mtime, item.mode = len(data), epoch(), 0o644
            archive.addfile(item, io.BytesIO(data))
    return entries, gzip.compress(buffer.getvalue(), mtime=0)


def release(output, jobs):
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise RuntimeError("Release output directory must be empty")
    build_deb("portable", output, jobs)
    entries, payload = archive_sources()
    prefix = f"{SLUG}-{VERSION}"
    with zipfile.ZipFile(output / (prefix + "-source.zip"), "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(entries.items()):
            item = zipfile.ZipInfo(prefix + "/" + name, (2024, 1, 1, 0, 0, 0))
            item.external_attr = 0o100644 << 16
            item.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(item, data)
    header = (ROOT / "packaging/run-header.sh").read_text().replace("@PAYLOAD_SHA256@", hashlib.sha256(payload).hexdigest())
    installer = output / (prefix + "-native-installer.run")
    installer.write_bytes(header.encode() + payload)
    installer.chmod(0o755)
    if len(list(output.iterdir())) != 3:
        raise RuntimeError("A release must have exactly three assets")
    print("Release assets: " + ", ".join(p.name for p in sorted(output.iterdir())))


def main():
    parser = argparse.ArgumentParser(description=MANIFEST["name"] + " source builder and native installer")
    parser.add_argument("command", choices=("release", "deb", "install"))
    parser.add_argument("--profile", choices=("portable", "native", "aggressive"), default="aggressive")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--jobs", type=int, default=max(1, min(8, (os.cpu_count() or 2) - 1)))
    parser.add_argument("--system-package-mode", action="store_true", help="permit root invocation by InfiltratorOS after its package transaction")
    parser.add_argument("--build-only", action="store_true", help="build and qualify a Debian package without installing it")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.system_package_mode and os.geteuid() != 0:
        parser.error("--system-package-mode requires root")
    if args.command == "release":
        release((args.output or ROOT / "dist").resolve(), args.jobs)
        return
    if args.command == "deb":
        build_deb(args.profile, (args.output or ROOT / "dist").resolve(), args.jobs)
        return
    if os.geteuid() == 0 and not (args.system_package_mode or args.build_only):
        parser.error("use an ordinary account, or --system-package-mode for OS provisioning")
    if args.build_only and args.output is None:
        parser.error("--build-only requires --output so the qualified package is retained")
    ensure_requirements(not args.build_only)
    # Keep the finished package until dpkg returns. Compilation failures never
    # uninstall or replace a previously working provider.
    with tempfile.TemporaryDirectory(prefix="provider-native-package-") as temporary:
        output = (args.output or Path(temporary)).resolve()
        package = build_deb(args.profile, output, args.jobs)
        if not args.build_only:
            install = ["/usr/bin/dpkg", "--install", package]
            if os.geteuid() != 0:
                install = ["/usr/bin/sudo", "--", *install]
            run(install)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print("Provider build failed: " + str(error), file=sys.stderr)
        raise SystemExit(1)
