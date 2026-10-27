#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise this provider's real ABI across dates, layouts and coordinates."""
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
from check_provider import Module

manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text())
module = Module(sys.argv[1], manifest["kind"])
calls = 0
for index in range(2048):
    if manifest["kind"] == "calendar":
        day = 2305448 + (index * 89) % 182621  # 1600..2100 civil and lunar workloads.
        cases = [("fields", [day]), ("start", [day]), ("periods", [day, index % 25 - 12]),
                 ("years", [day, index % 9 - 4])]
        cases += [("calendar_format", [day, part, 512]) for part in range(6)]
    else:
        instant = (-2208988800 + index * 2304007) * 1000000 + index * 123457
        offset = (-43200, 0, 19800, 36000, 50400)[index % 5]
        latitude = (-66.0, -37.8, 0.0, 31.8, 51.5, 69.0)[index % 6]
        longitude = (-179.9, -74.0, 0.0, 35.2, 144.9, 179.9)[index % 6]
        seconds = bool(index % 2)
        cases = [("clock_format", [instant, offset, seconds, bool(index % 3), latitude, longitude,
                                    ("en_US", "ar", "ja_JP")[index % 3], 512]),
                 ("tick", [instant, offset, seconds, latitude, longitude])]
    for operation, arguments in cases:
        module.observe({"op": operation, "args": arguments})
        calls += 1
print(f"{manifest['id']}: PGO trained with {calls} actual provider calls")
