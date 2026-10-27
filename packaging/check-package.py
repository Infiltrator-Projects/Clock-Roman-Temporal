#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Qualify the actual Debian payload, ABI and optimization record."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text())
resource = Path("System/Resources") / ("Calendars" if manifest["kind"] == "calendar" else "Clocks") / (manifest["name"] + ".so")
documentation = Path("System/Resources/Documentation") / manifest["repository"]
package, profile = Path(sys.argv[1]).resolve(), sys.argv[2]
expected = "infiltrator-" + manifest["repository"].lower()
assert subprocess.check_output(["dpkg-deb", "-f", package, "Package"], text=True).strip() == expected
version = (ROOT / "VERSION").read_text().strip() + ("+native1" if profile != "portable" else "")
assert subprocess.check_output(["dpkg-deb", "-f", package, "Version"], text=True).strip() == version
with tempfile.TemporaryDirectory(prefix="provider-payload-") as temporary:
    root = Path(temporary)
    subprocess.run(["dpkg-deb", "--extract", package, root], check=True)
    files = [p for p in root.rglob("*") if not p.is_dir()]
    assert sum(p.suffix == ".so" for p in files) == 1
    for file in files:
        relative = file.relative_to(root)
        assert not file.is_symlink()
        assert relative == resource or relative.is_relative_to(documentation), relative
        assert file.stat().st_mode & 0o777 == 0o644, relative
    library = root / resource
    info = (root / documentation / "BUILD-INFO").read_text()
    assert f"Profile: {profile}\n" in info
    assert "LTO: enabled\n" in info
    assert f"SHA256: {hashlib.sha256(library.read_bytes()).hexdigest()}\n" in info
    if profile == "aggressive":
        assert "PGO: trained two-pass profile\n" in info
        assert "C flags: -O3 " in info
        assert "-march=native" in info or "-mcpu=native" in info
    else:
        assert "-march=native" not in info and "-mcpu=native" not in info
    symbols = subprocess.check_output(["readelf", "--dyn-syms", "--wide", library], text=True)
    assert "__gcov" not in symbols, "Instrumented training binary leaked into package"
    env = dict(os.environ, LANGUAGE="en_US", LC_ALL="C.UTF-8")
    for check in ("check_provider.py", "check_accuracy.py"):
        subprocess.run([sys.executable, ROOT / "tests" / check, library], check=True, env=env)
print(f"{expected}: one qualified {profile} module, passive documentation, no installed helpers")
