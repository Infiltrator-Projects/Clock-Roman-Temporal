# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent audit fixtures and explicit unresolved authority comparisons."""
from pathlib import Path
import ctypes as C
import hashlib,json,sys
from check_provider import Module,check

root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'SOURCE-MANIFEST.json').read_text())
authority='--authorities' in sys.argv
name='authority-cases.json' if authority else 'accuracy-cases.json'
path=root/'tests'/name
cases=json.loads(path.read_text()) if path.exists() else []
if authority:
    module=Module(sys.argv[1],manifest['kind']);failures=[]
    for case in cases:
        actual=module.observe(case)
        if actual!=case['expected']:failures.append(dict(case=case,actual=actual))
    for failure in failures:print(json.dumps(failure,ensure_ascii=False))
    print(manifest['id']+': '+str(len(failures))+' unresolved reference disagreements in '+str(len(cases))+' authority comparison cases')
    sys.exit(bool(failures))

check(sys.argv[1],manifest,cases)
extra=0
if manifest['kind']=='calendar':
    # ABI requires failed callbacks to preserve all caller output.
    module=Module(sys.argv[1],'calendar')
    for part in range(7):
        for capacity in [0,1,2,3,8]:
            output=C.create_string_buffer(b'Q'*511,512);before=output.raw;length=C.c_size_t(9876)
            ok=module.provider.format(module.pointer,2461341,part,output,capacity,C.byref(length))
            if not ok:assert output.raw==before and length.value==9876,(part,capacity,'failure modified caller output')
            extra+=1
print(manifest['id']+': '+str(len(cases))+' independent model fixtures and '+str(extra)+' failure-contract checks passed')
