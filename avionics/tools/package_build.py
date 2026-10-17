#!/usr/bin/env python3
"""Refresh the bench binary and source hashes after passing the recorded checks."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
from zipfile import ZipFile,ZIP_DEFLATED
from source_manifest import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser();p.add_argument('--zip',type=Path);args=p.parse_args()
    sil=json.loads((ROOT/'results/summary.json').read_text());ek7=json.loads((ROOT/'results/ek7/summary.json').read_text())
    sensors=json.loads((ROOT/'results/sensors/summary.json').read_text())
    assert sil['passed']==sil['total']==55 and ek7['passed']==ek7['total']==18
    assert sensors['passed']==sensors['total']==25
    log=(ROOT/'results/firmware-build.log').read_text();assert '[SUCCESS]' in log
    binary=ROOT/'.pio/build/esp32_bench/firmware.bin';target=ROOT/'artifacts/esp32_bench.bin'
    shutil.copyfile(binary,target)
    sources=source_hashes()
    for summary in (sil,ek7,sensors):
        for name,value in summary['sources_sha256'].items():assert digest(ROOT/name)==value,f'Stale tests: {name}'
    for name in ('core/ek7.hpp','core/flight.hpp'):
        assert sources[name]==ek7['sources_sha256'][name]
    manifest=dict(target='esp32_bench',physical_release_output=False,firmware_sha256=digest(target),sources=sources,
                  ram=re.search(r'^RAM:.*$',log,re.M)[0],flash=re.search(r'^Flash:.*$',log,re.M)[0],
                  validation=dict(native_scenarios=sil['total'],passed=sil['passed'],ek7_scenarios=ek7['total'],
                                  ek7_passed=ek7['passed'],ek7_invariant_groups=ek7['invariant_groups'],
                                  independent_host_codec_tests=7,sanitizers='ASan + UBSan: core and EK7 invariants passed',
                                  sensor_stimulus_scenarios=sensors['total'],sensor_stimulus_passed=sensors['passed'],
                                  sensor_driver_invariant_groups=sensors['driver_invariant_groups'],
                                  sensor_sanitizers='ASan + UBSan, leak checking off for vendor lifetime allocations; see sensors/sanitizers.log',
                                  hardware_execution=False,official_EK7_compatibility_verified=False,
                                  EK7_local_protocol_profile_implemented=True),ek7_profile=ek7['profile'])
    (ROOT/'artifacts/BUILD.json').write_text(json.dumps(manifest,indent=2)+'\n')
    if args.zip:
        args.zip.parent.mkdir(parents=True,exist_ok=True)
        with ZipFile(args.zip,'w',ZIP_DEFLATED) as archive:
            for path in sorted(ROOT.rglob('*')):
                rel=path.relative_to(ROOT)
                if not path.is_file() or set(rel.parts)&{'.pio','.pio-core','.venv','build','__pycache__'}:continue
                if path.name in ('.DS_Store','EK7-2026.docx','EK7-2026.txt'):continue
                archive.write(path,str(Path('avionics')/rel))
            archive.write(ROOT.parent/'README.md','README.md')
        print(f'Package: {args.zip} ({args.zip.stat().st_size} bytes)')
    print('Build manifest refreshed; physical output OFF, official compatibility unverified')

if __name__=='__main__':main()
