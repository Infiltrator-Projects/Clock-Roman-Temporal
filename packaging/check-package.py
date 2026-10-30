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
    assert len(files) == 2, [str(p.relative_to(root)) for p in files]
    assert sum(p.suffix == ".so" for p in files) == 1
    guide = documentation / (manifest["name"] + ".pdf")
    for file in files:
        relative = file.relative_to(root)
        assert not file.is_symlink()
        assert relative in (resource, guide), relative
        assert file.stat().st_mode & 0o777 == 0o644, relative
    library = root / resource
    assert (root / guide).read_bytes().startswith(b"%PDF-")
    def field(name):
        return subprocess.check_output(["dpkg-deb", "-f", package, name], text=True).strip()
    assert field("X-Infiltrator-Native-Profile") == profile
    assert field("X-Infiltrator-LTO") == "enabled"
    assert field("X-Infiltrator-Resource-SHA256") == hashlib.sha256(library.read_bytes()).hexdigest()
    if profile == "aggressive":
        assert field("X-Infiltrator-PGO") == "trained-two-pass"
        assert field("X-Infiltrator-Cflags").startswith("-O3 ")
        assert "-march=native" in field("X-Infiltrator-Cflags") or "-mcpu=native" in field("X-Infiltrator-Cflags")
    else:
        assert field("X-Infiltrator-PGO") == "disabled"
        assert "-march=native" not in field("X-Infiltrator-Cflags")
        assert "-mcpu=native" not in field("X-Infiltrator-Cflags")
    symbols = subprocess.check_output(["readelf", "--dyn-syms", "--wide", library], text=True)
    assert "__gcov" not in symbols, "Instrumented training binary leaked into package"
    env = dict(os.environ, LANGUAGE="en_US", LC_ALL="C.UTF-8")
    for check in ("check_provider.py", "check_accuracy.py"):
        subprocess.run([sys.executable, ROOT / "tests" / check, library], check=True, env=env)
print(f"{expected}: one qualified {profile} module, passive documentation, no installed helpers")
