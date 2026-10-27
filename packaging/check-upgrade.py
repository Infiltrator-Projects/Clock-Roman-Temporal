#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prove dpkg ownership transfer/removal against an isolated legacy package."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text())
package = Path(sys.argv[1]).resolve()
name = "infiltrator-" + manifest["repository"].lower()
resource = Path("System/Resources") / ("Calendars" if manifest["kind"] == "calendar" else "Clocks") / (manifest["name"] + ".so")
if os.geteuid() != 0:
    raise SystemExit("Run this isolated dpkg integration test as root")
with tempfile.TemporaryDirectory(prefix="provider-upgrade-") as temporary:
    work = Path(temporary)
    stage, system = work / "legacy", work / "system"
    (stage / "DEBIAN").mkdir(parents=True)
    (stage / "DEBIAN/control").write_text("Package: infiltrator-calendar-resources\nVersion: 1.19.78\nArchitecture: amd64\nMaintainer: Infiltrator Projects <noreply@infiltrator-projects.invalid>\nDescription: isolated legacy ownership fixture\n")
    (stage / resource).parent.mkdir(parents=True)
    (stage / resource).write_text("legacy module")
    other = resource.parent / "Other Provider.so"
    (stage / other).write_text("unrelated module")
    subprocess.run(["dpkg-deb", "--root-owner-group", "--build", stage, work / "legacy.deb"], check=True)
    dpkg = ["dpkg", "--root", str(system)]
    subprocess.run([*dpkg, "--unpack", work / "legacy.deb"], check=True)
    subprocess.run([*dpkg, "--unpack", package], check=True)
    query = ["dpkg-query", "--admindir", str(system / "var/lib/dpkg")]
    ownership = subprocess.check_output([*query, "--search", "/" + str(resource)], text=True).strip()
    assert ownership == name + ": /" + str(resource), ownership
    assert (system / resource).read_bytes().startswith(b"\x7fELF")
    assert (system / other).read_text() == "unrelated module"
    subprocess.run([*dpkg, "--remove", name], check=True)
    assert not (system / resource).exists()
    assert (system / other).exists()
    subprocess.run([*dpkg, "--unpack", package], check=True)
    subprocess.run([*dpkg, "--remove", "infiltrator-calendar-resources"], check=True)
    assert (system / resource).read_bytes().startswith(b"\x7fELF")
    assert not (system / other).exists()
print(name + ": isolated legacy ownership transfer and independent removal passed")
