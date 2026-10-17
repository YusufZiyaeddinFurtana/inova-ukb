#!/usr/bin/env python3
"""Instrument the sensor models, real drivers and session, then replay every sensor case."""
import os
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]

def main():
    target=ROOT/'build/sanitized-sensors';target.mkdir(exist_ok=True)
    report=ROOT/'results/sensors/sanitizers.log'
    includes=[f'-I{ROOT/p}' for p in ('core','firmware','sim','sim/arduino','third_party/adafruit_bmp3xx','third_party/adafruit_busio')]
    flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
    with report.open('w') as log:
        log.write('ASan + UBSan: models, Arduino/Wire compatibility, vendor drivers, sensor adapters and session.\n')
        log.write('LeakSanitizer OFF: upstream Adafruit_BMP3XX has lifetime I2C allocations without a freeing destructor; hardware uses one global instance. No leak-free claim.\n');log.flush()
        def run(args,**kwargs):subprocess.run([str(x) for x in args],check=True,stdout=log,stderr=log,**kwargs)
        objects=[]
        for path,c in [('third_party/adafruit_bmp3xx/bmp3.c',True),('third_party/adafruit_bmp3xx/Adafruit_BMP3XX.cpp',False),('third_party/adafruit_busio/Adafruit_I2CDevice.cpp',False)]:
            obj=target/(Path(path).stem+'.o');objects.append(obj)
            run(['cc' if c else 'c++','-std=c99' if c else '-std=c++17',*flags,*includes,'-c',ROOT/path,'-o',obj])
        for src,name in [('tests/sensor_tests.cpp','sensor_tests'),('tools/sensor_replay.cpp','sensor_replay')]:
            run(['c++','-std=c++17',*flags,*includes,ROOT/src,*objects,'-o',target/name])
        run([target/'sensor_tests'],env=env)
        cases=sorted((ROOT/'results/sensors').glob('*/stimulus.csv'))
        if not cases:raise RuntimeError('Run run_sensors.py first')
        for case in cases:
            run([target/'sensor_replay',case,target/(case.parent.name+'.csv')],env=env)
            log.write(f'PASS ASan/UBSan {case.parent.name}\n');log.flush()
    print(f'ASan + UBSan: driver tests and {len(cases)} sensor cases PASS; leak checking excluded')

if __name__=='__main__':main()
