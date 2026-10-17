#!/usr/bin/env python3
"""End-to-end byte-level SUT using an independent Python test-device codec."""
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
from ek7_device import command,encode,decode,FIELDS
from run_sil import read_model,rotate,qinv,PAD,ROOT

def euler(q):
    # Change both coordinate bases before extracting Euler Rz*Ry*Rx; never permute Euler angles.
    w,z,x,y=q
    return [math.degrees(math.atan2(2*(w*x+y*z),1-2*(x*x+y*y))),
            math.degrees(math.asin(max(-1,min(1,2*(w*y-z*x))))),
            math.degrees(math.atan2(2*(w*z+x*y),1-2*(y*y+z*z)))]

def make_data(at,t):
    ft=max(0,t-PAD);d=at(ft);q=tuple(d['q'+k] for k in 'wxyz')
    force=(0,0,d['gravity']) if t<PAD else (d['ax_world'],d['ay_world'],d['az_world']+d['gravity'])
    a=rotate(qinv(q),force)
    return [970+d['altitude_m'],d['pressure_pa']/100,a[2],a[0],a[1],*euler(q)]

def run(name,at,apo,variant,hz=10):
    dest=ROOT/'results/ek7'/name;dest.mkdir(parents=True,exist_ok=True)
    events=[(0,'ARM','0' if variant=='unarmed' else '1'),(0,'RX',command(0x22).hex())]
    source=[]
    period=round(1e6/hz)
    for tick in range(3501):
        now=tick*10000
        if now>=1010000 and (now-1010000)%period==0:
            t=(now-1010000)/1e6;d=make_data(at,t);source.append(d.copy())
            if variant=='pressure_spike' and abs(t-13)<.001:d[1]-=40
            if variant=='false_descent' and 13<=t<=14:d[1]=at(8)['pressure_pa']/100+2*(t-13)
            if variant=='accel_spike' and abs(t-6)<.001:d[2]=16*9.797
            if variant=='nan_pressure' and abs(t-13)<.001:d[1]=float('nan')
            if variant=='pressure_frozen' and t>=14:d[1]=at(9)['pressure_pa']/100
            if variant=='altitude_frozen' and t>=14:d[0]=970+at(9)['altitude_m']
            if variant=='altitude_spike' and abs(t-13)<.001:d[0]+=1000
            if variant=='long_gap' and 13<=t<=14.2:continue
            packet=encode(d)
            if variant=='bad_checksum' and abs(t-13)<.001:packet=packet[:33]+bytes((packet[33]^1,))+packet[34:]
            if variant=='jitter':events.append((now+((tick//10)%3)*7000,'RX',packet.hex()))
            elif variant=='fragmented':
                events.append((now,'RX',packet[:15].hex()));events.append((now+1000,'RX',packet[15:].hex()))
            else:events.append((now,'RX',packet.hex()))
            if variant=='real_sensor_noise':events.append((now+100,'REAL','30000,1000,1000,1000,1000,1000,1000,1'))
            if variant=='arm_lost' and t>=14:events.append((now-1,'ARM','0'))
        events.append((now,'POLL',''))
    events.extend([(35000001,'RX',command(0x24).hex()),(35100000,'POLL','')])
    events.sort(key=lambda e:e[0])
    with (dest/'events.csv').open('w') as f:
        for time,action,payload in events:f.write(f'{time},{action},{payload}\n')
    if name=='nominal':
        with (ROOT/'data/ek7_nominal_10hz.csv').open('w',newline='') as f:
            writer=csv.writer(f,lineterminator='\n');writer.writerow(FIELDS);writer.writerows(source)
    subprocess.run([str(ROOT/'build/ek7_replay'),str(dest/'events.csv'),str(dest/'trace.csv'),str(hz)],check=True)
    with (dest/'trace.csv').open() as f:rows=list(csv.DictReader(f))
    packets=[(int(r['time_us'])/1e6,decode(bytes.fromhex(r['tx']))) for r in rows if r['tx']]
    releases=[r for r in rows if r['release']=='1'];release=int(releases[0]['time_us'])/1e6 if releases else None
    expected='inhibit' if variant in ('unarmed','accel_spike','long_gap','pressure_frozen','altitude_frozen','arm_lost') else 'deploy'
    apogee=1.01+PAD+apo['time_s']
    checks=dict(valid_status_packets=bool(packets),reserved_and_drogue_zero=all(not (p['status']&32) for _,p in packets),
                start_delay=bool(packets) and packets[0][0]==1.0,
                ten_hz=all(abs(b[0]-a[0]-.1)<1e-8 for a,b in zip(packets,packets[1:])) if variant!='long_gap' else True,
                stop_clears=rows[-1]['mode']=='0' and rows[-1]['milestones']=='0' and not rows[-1]['tx'],
                decision=(release is None) if expected=='inhibit' else release is not None and apogee<=release<=apogee+3)
    if release is not None:
        at_command=int(releases[0]['milestones']);checks['gates_recorded']=(at_command&0x97)==0x97
        checks['main_status_reported']=any(p['main'] and t>=release for t,p in packets)
    if variant=='long_gap':checks['timeout_normal']=any(r['mode']=='0' and 15<int(r['time_us'])/1e6<16 for r in rows)
    result=dict(name=name,expected=expected,passed=all(checks.values()),checks=checks,release_time_s=release,
                apogee_delay_s=release-apogee if release else None,status_packets=len(packets),input_hz=hz,
                milestones_seen=sorted(set(p['status'] for _,p in packets)))
    print('PASS' if result['passed'] else 'FAIL',name,result['apogee_delay_s'])
    return result

def plot_nominal(at,apo):
    os.environ.setdefault('MPLCONFIGDIR',str(ROOT/'build/matplotlib'))
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:return
    with (ROOT/'results/ek7/nominal/trace.csv').open() as f:rows=list(csv.DictReader(f))
    t=[int(r['time_us'])/1e6-1.01-PAD for r in rows]
    release=next((x for x,r in zip(t,rows) if r['release']=='1'),None)
    fig,axes=plt.subplots(2,1,figsize=(11,7),sharex=True,layout='constrained')
    truth_t=[i*.1 for i in range(270)]
    axes[0].plot(truth_t,[at(x)['altitude_m'] for x in truth_t],label='OpenRocket AGL')
    axes[0].plot(t,[float(r['altitude_m']) for r in rows],label='SUT: ortak filtre',alpha=.8)
    axes[0].set_ylabel('Yerden irtifa (m)');axes[0].legend()
    names=['Kalkış','Motor önlemi','Minimum irtifa','Açı / yanal ivme','Alçalma','Sürüklenme (kullanılmıyor)','Alt irtifa','Tek ayrılma isteği']
    for bit,label in enumerate(names):
        axes[1].step(t,[bit+.65 if int(r['milestones'])&(1<<bit) else bit for r in rows],where='post',label=label)
    axes[1].set_yticks([i+.3 for i in range(8)],names);axes[1].set_ylabel('Durum bitleri (0–7)')
    axes[1].set_xlabel('Ateşlemeden sonraki süre (s)')
    for ax in axes:
        ax.axvline(apo['time_s'],color='gray',ls='--');ax.grid(alpha=.2);ax.set_xlim(0,26)
        if release is not None:ax.axvline(release,color='crimson',ls=':')
    fig.suptitle('EK-7 yerel SUT — gerçek ikili paketlerle karar ve durum izlemesi\nGri: tepe noktası | Kırmızı: mantıksal istek | Fiziksel çıkış kapalı')
    fig.savefig(ROOT/'results/ek7/nominal.png',dpi=150);plt.close(fig)

def main():
    subprocess.run(['make','all'],cwd=ROOT,check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/test_ek7_device.py')],check=True)
    unit=subprocess.run([str(ROOT/'build/ek7_tests')],capture_output=True,text=True)
    print(unit.stdout,end='');print(unit.stderr,end='',file=sys.stderr)
    if unit.returncode:return unit.returncode
    target=ROOT/'results/ek7';target.mkdir(exist_ok=True)
    (target/'invariants.log').write_text(unit.stdout)
    at,apo=read_model()
    variants=['nominal','fragmented','real_sensor_noise','pressure_spike','false_descent','nan_pressure','bad_checksum',
              'unarmed','accel_spike','long_gap','pressure_frozen','arm_lost','altitude_frozen','altitude_spike','jitter']
    results=[run(name,at,apo,name) for name in variants]
    results.extend(run(f'nominal_{hz}hz',at,apo,'nominal',hz) for hz in (20,50,100))
    plot_nominal(at,apo)
    baseline=results[0]['release_time_s']
    isolation=results[2];isolation['checks']['decision_independent_of_real_sensors']=isolation['release_time_s']==baseline
    isolation['passed']=all(isolation['checks'].values())
    summary=dict(profile=dict(floats='little',data_status_checksum='sum(header+payload) mod256',commands='literal Table1',
                              synthetic_input_hz=10,official_compatibility_verified=False),
                 invariant_groups=sum(s.startswith('PASS ') for s in unit.stdout.splitlines()),
                 total=len(results),passed=sum(r['passed'] for r in results),cases=results,
                 sources_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('core','tools','tests')
                     for p in (ROOT/d).glob('*') if p.suffix in ('.hpp','.cpp','.py')},
                 hardware_tested=False,physical_release_enabled=False)
    (target/'summary.json').write_text(json.dumps(summary,indent=2))
    lines=['# EK-7 yerel protokol ve SUT sonuçları','',f"**{summary['invariant_groups']} test grubu; {summary['passed']}/{summary['total']} uçtan uca senaryo geçti.**",'',
           'Aynı C++ oturum/karar koduna bağımsız Python kodlayıcısının gerçek ikili paketleri verildi. Bu, resmî cihazın veya elektriksel RS232 hattının testi değildir.',
           '', '![SUT karar ve durum bitleri](nominal.png)',
           '', '| Senaryo | Beklenti | Sonuç | Tepeden komuta gecikme |','|---|---|---|---|']
    for r in results:
        delay='—' if r['apogee_delay_s'] is None else f"{r['apogee_delay_s']:.3f} s"
        lines.append(f"| {r['name']} | {r['expected']} | {'GEÇTİ' if r['passed'] else 'KALDI'} | {delay} |")
    lines+=['','Giriş 10 Hz, çıkış 10 Hz; başlatma gecikmesi 1 s, çerçeve timeout 100 ms, SUT timeout 1000 ms. SUT giriş hızı resmî belgede belirtilmediğinden 10 Hz yerel profil seçimidir.',
            '', '288 ölçüm ve 40 komut tek-bit bozulması; iki float sırası; checksum seçenekleri; eksen/yönelim; mod tekrarı; bozuk ve parçalı paketler; gerçek sensör izolasyonu ve 200.000 bayt rastgele akış sınandı.',
            '', 'Yerel varsayımlar: komutlarda Tablo 1 sabitleri, veri/durumda header+payload toplamı, little-endian float, Euler RzRyRx, A1 tek ayrılma bit7. Resmî checksum, float/Euler ve A1 çıkış eşlemesi doğrulanmalıdır.',
            '', '`inhibit` kurtarmanın gerçekleşmesi anlamına gelmez. Kalıcı IMU/basınç arızası ve test bağlantı kaybında mantıksal komut engellenir. GPIO25 fiziksel olarak sürülmez; gerçek çıkış/EMI/yük/sensör testleri bekliyor.']
    (target/'SONUCLAR.md').write_text('\n'.join(lines)+'\n')
    return 0 if summary['passed']==summary['total'] else 1

if __name__=='__main__':sys.exit(main())
