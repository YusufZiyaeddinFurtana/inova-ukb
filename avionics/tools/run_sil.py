#!/usr/bin/env python3
"""Reproducible open-loop SIL of the SAME C++ flight core used by ESP32.

Truth comes from a separate OpenRocket RK4 run. No truth fields enter the core.
This tests decisions before deployment, not parachute mechanics or RF propagation.
"""
import bisect
import copy
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import random
import subprocess
import sys
from xml.etree import ElementTree as ET
from zipfile import ZipFile
from decode_packet import decode

ROOT=Path(__file__).resolve().parents[1]
FIELDS='time_s,pressure_pa,ax,ay,az,gx,gy,gz,baro_new,imu_new,arm,truth_alt_m,truth_vz_m_s,radio_busy,gps_valid,latitude,longitude,battery_mv,reset'.split(',')
PAD=5.0

def qmul(a,b):
    w,x,y,z=a;v,i,j,k=b
    return (w*v-x*i-y*j-z*k,w*i+x*v+y*k-z*j,w*j-x*k+y*v+z*i,w*k+x*j-y*i+z*v)

def qinv(q):return(q[0],-q[1],-q[2],-q[3])
def rotate(q,v):return qmul(qmul(q,(0,*v)),qinv(q))[1:]

def read_model():
    with (ROOT/'data/openrocket_no_recovery.csv').open() as f:
        rows=[{k:float(v) for k,v in r.items()} for r in csv.DictReader(f)]
    times=[r['time_s'] for r in rows]
    # Quaternion signs must follow the same hemisphere before linear interpolation.
    for i in range(1,len(rows)):
        if sum(rows[i]['q'+k]*rows[i-1]['q'+k] for k in 'wxyz')<0:
            for k in 'wxyz':rows[i]['q'+k]*=-1
    def at(t):
        i=max(0,min(bisect.bisect_right(times,t)-1,len(rows)-2))
        a,b=rows[i],rows[i+1]
        r=max(0,min(1,(t-a['time_s'])/(b['time_s']-a['time_s'])))
        d={k:a[k]+r*(b[k]-a[k]) for k in a}
        q=tuple(d['q'+k] for k in 'wxyz');norm=math.sqrt(sum(x*x for x in q))
        for k,x in zip('wxyz',q):d['q'+k]=x/norm
        return d
    apo=max(rows,key=lambda r:r['altitude_m'])
    return at,apo

def make_profile(at,seed=0,noise=True,accel_scale=1.0,gyro_scale=1.0,lag=0.0):
    rng=random.Random(seed);profile=[]
    for tick in range(3501):
        t=tick*.01;ft=max(0,t-PAD);d=at(ft);pressure=at(max(0,ft-lag))['pressure_pa']
        q=tuple(d['q'+k] for k in 'wxyz')
        if t<PAD:
            specific=(0,0,d['gravity']);g=(0,0,0);alt=vz=0
        else:
            specific=(d['ax_world'],d['ay_world'],d['az_world']+d['gravity'])
            qa=tuple(at(max(0,ft-.005))['q'+k] for k in 'wxyz')
            qb=tuple(at(ft+.005)['q'+k] for k in 'wxyz')
            delta=qmul(qinv(qa),qb)
            if delta[0]<0:delta=tuple(-x for x in delta)
            sn=math.sqrt(sum(x*x for x in delta[1:]));angle=2*math.atan2(sn,max(0,delta[0]))
            g=tuple(x*angle/(sn*.01) for x in delta[1:]) if sn>1e-12 else (0,0,0)
            alt=d['altitude_m'];vz=d['vz_m_s']
        a=rotate(qinv(q),specific)
        # DLPF/ADC quantization and mounting errors are not modeled here.
        a=tuple(x*accel_scale+(rng.gauss(0,.025) if noise else 0) for x in a)
        g=tuple(x*gyro_scale+(rng.gauss(0,.0005) if noise else 0) for x in g)
        profile.append(dict(zip(FIELDS,[t,pressure+(rng.gauss(0,1.5) if noise else 0),*a,*g,
                       int(tick%2==0),1,int(t>=3),alt,vz,0,1,38.4,34.0,7800,0])))
    return profile

def modify(base,kind):
    p=copy.deepcopy(base)
    frozen=next(r['pressure_pa'] for r in p if r['time_s']>=14)
    fake_start=next(r['pressure_pa'] for r in p if r['time_s']>=13)
    for r in p:
        t=r['time_s']
        if kind=='unarmed':r['arm']=0
        elif kind=='gps_loss':r['gps_valid']=0;r['latitude']=r['longitude']=0
        elif kind=='radio_stuck' and t>=8:r['radio_busy']=1
        elif kind=='baro_spike' and abs(t-13)<.001:r['pressure_pa']-=4000
        elif kind=='baro_nan' and 13<=t<=13.04:r['pressure_pa']=float('nan')
        elif kind=='baro_gap_short' and 13<=t<=13.06:r['baro_new']=0
        elif kind=='baro_gap_apogee' and 24<=t<=25.2:r['baro_new']=0
        elif kind=='baro_dead' and t>=14:r['baro_new']=0
        elif kind=='baro_frozen' and t>=14:r['pressure_pa']=frozen
        elif kind=='imu_dead' and t>=14:r['imu_new']=0
        elif kind=='imu_gap_short' and 13<=t<=13.01:r['imu_new']=0
        elif kind=='imu_gap_armed' and 4<=t<=4.1:r['imu_new']=0
        elif kind=='late_arm':r['arm']=int(t>=13)
        elif kind=='accel_saturated' and 6<=t<=6.02:r['ax']=16*9.797
        elif kind=='gyro_saturated' and 13<=t<=13.02:r['gy']=2000*math.pi/180
        elif kind=='boot_invalid':r['pressure_pa']=float('nan')
        elif kind=='arm_lost' and t>=14:r['arm']=0
        elif kind=='reset_in_flight' and abs(t-14)<.001:r['reset']=1
        elif kind=='clock_reversed' and abs(t-14)<.001:r['time_s']=13.5
        elif kind=='baro_false_descent' and 13<=t<=14:
            r['pressure_pa']=fake_start+200*(t-13)
        elif kind=='accel_bias_in_flight' and t>=5:r['az']+=.02*9.797
        elif kind=='pad_only':
            r['truth_alt_m']=r['truth_vz_m_s']=0;r['ax']=r['ay']=r['gx']=r['gy']=r['gz']=0;r['az']=9.797
            r['pressure_pa']=90209.744
        elif kind=='pad_jolt':
            r['truth_alt_m']=r['truth_vz_m_s']=0;r['ax']=r['ay']=r['gx']=r['gy']=r['gz']=0
            r['az']=4*9.797 if 6<=t<=6.04 else 9.797;r['pressure_pa']=90209.744
    return p

def run_case(name,profile,expected,apogee):
    target=ROOT/'results'/name;target.mkdir(exist_ok=True)
    source=target/'input.csv'
    with source.open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=FIELDS);writer.writeheader();writer.writerows(profile)
    subprocess.run([str(ROOT/'build/replay'),str(source),str(target/'flight.csv'),str(target/'packets.hex')],check=True)
    with (target/'flight.csv').open() as f: rows=list(csv.DictReader(f))
    releases=[r for r in rows if int(r['release'])]
    release=float(releases[0]['time_s']) if releases else None
    checks={}
    if expected=='deploy':
        checks['release_in_window']=release is not None and apogee-.01<=release<=apogee+3
        checks['release_stays_latched']=not releases or all(int(r['release']) for r in rows[rows.index(releases[0]):])
    else:checks['no_release']=release is None
    decoded=[]
    for line in (target/'packets.hex').read_text().splitlines():
        time,h=line.split(',');d=decode(bytes.fromhex(h));d['source_time_s']=float(time);decoded.append(d)
    checks['telemetry_decode']=len(decoded)>0
    checks['telemetry_sequence']=all(b['sequence']==(a['sequence']+1)%65536 for a,b in zip(decoded,decoded[1:]))
    if expected=='deploy' and name!='radio_stuck':
        packet_release=next((d['source_time_s'] for d in decoded if d['release']),None)
        checks['release_in_telemetry']=release is not None and packet_release is not None and release-.002<=packet_release<=release+.21
    if expected=='inhibit':checks['no_release_in_telemetry']=not any(d['release'] for d in decoded)
    if name not in ('radio_stuck','clock_reversed'):
        checks['telemetry_5hz']=all(abs(b['time_s']-a['time_s']-.2)<.002 for a,b in zip(decoded,decoded[1:]))
    if name=='imu_dead': checks['inertial_fault_reported']=any(int(r['health'])&4 for r in rows)
    if name=='baro_dead':checks['barometer_stale_reported']=any(int(r['health'])&1 for r in rows)
    if name in ('accel_saturated','gyro_saturated'):checks['saturation_reported']=any(int(r['health'])&8 for r in rows)
    if name=='clock_reversed':checks['clock_fault_reported']=any(int(r['health'])&16 for r in rows)
    if name=='gps_loss':checks['gps_invalid_flag']=all(not d['gps_valid'] for d in decoded)
    if name=='radio_stuck':checks['radio_loss_injected']=int(rows[-1]['tx_dropped'])>0
    (target/'decoded.json').write_text(json.dumps(decoded,indent=2))
    result=dict(name=name,expected=expected,passed=all(checks.values()),checks=checks,release_time_s=release,
                apogee_delay_s=release-apogee if release is not None else None,
                altitude_at_command_m=float(releases[0]['truth_alt_m']) if releases else None,
                tx_sent=int(rows[-1]['tx_sent']),tx_dropped=int(rows[-1]['tx_dropped']))
    print(('PASS' if result['passed'] else 'FAIL'),name,'release=',release,'delay=',result['apogee_delay_s'])
    return result,rows

def plot(rows,apogee):
    os.environ.setdefault('MPLCONFIGDIR',str(ROOT/'build/matplotlib'))
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        return None
    t=[float(r['time_s'])-PAD for r in rows];release=next((x for x,r in zip(t,rows) if int(r['release'])),None)
    fig,axes=plt.subplots(3,1,figsize=(11,8),sharex=True,layout='constrained')
    axes[0].plot(t,[float(r['truth_alt_m']) for r in rows],label='OpenRocket irtifası')
    axes[0].plot(t,[float(r['altitude_m']) for r in rows],label='Filtrelenmiş barometre',alpha=.8)
    axes[0].set_ylabel('Yerden irtifa (m)');axes[0].legend()
    for key,label in [('truth_vz_m_s','OpenRocket düşey hızı'),('baro_v_m_s','Barometrik kestirim'),('inertial_v_m_s','IMU kestirimi')]:
        axes[1].plot(t,[float(r[key]) for r in rows],label=label)
    axes[1].set_ylabel('Düşey hız (m/s)');axes[1].legend()
    axes[2].plot(t,[float(r['tilt_deg']) for r in rows],label='Düşeyden sapma açısı')
    axes[2].set_ylabel('Açı (derece)');axes[2].set_xlabel('Ateşlemeden sonraki süre (s)');axes[2].legend()
    for ax in axes:
        ax.axvline(apogee-PAD,color='gray',ls='--',label='Apogee')
        if release is not None:ax.axvline(release,color='crimson',ls=':',label='Release request')
        ax.grid(alpha=.2);ax.set_xlim(0,26)
    fig.suptitle('İNOVA UKB — dijital uçuş denemesi, fiziksel çıkış kapalı\nKesikli gri: tepe noktası | Noktalı kırmızı: ayrılma komutu | Açılma yörüngeye geri beslenmez')
    path=ROOT/'results/nominal.png';fig.savefig(path,dpi=160);plt.close(fig)
    return str(path)

def main():
    subprocess.run(['make','all'],cwd=ROOT,check=True)
    subprocess.run([str(ROOT/'build/core_tests')],check=True)
    at,apo=read_model();apogee=apo['time_s']+PAD
    base=make_profile(at)
    cases=[('nominal',base,'deploy'),('no_noise',make_profile(at,noise=False),'deploy')]
    normal=['gps_loss','radio_stuck','baro_spike','baro_nan','baro_gap_short','baro_gap_apogee','imu_gap_short','baro_false_descent','accel_bias_in_flight']
    inhibit=['unarmed','baro_dead','baro_frozen','imu_dead','imu_gap_armed','late_arm','accel_saturated','gyro_saturated','boot_invalid','arm_lost','reset_in_flight','clock_reversed','pad_only','pad_jolt']
    cases.extend((k,modify(base,k),'deploy') for k in normal)
    cases.extend((k,modify(base,k),'inhibit') for k in inhibit)
    for seed in range(30):
        rng=random.Random(seed+1200)
        cases.append((f'monte_carlo_{seed:02}',make_profile(at,seed+100,accel_scale=rng.uniform(.99,1.01),gyro_scale=rng.uniform(.995,1.005),lag=rng.uniform(0,.08)),'deploy'))
    results=[];nominal_rows=None
    for name,profile,expected in cases:
        result,rows=run_case(name,profile,expected,apogee);results.append(result)
        if name=='nominal':nominal_rows=rows
    baseline=results[0]['release_time_s']
    for r in results:
        if r['name'] in ('gps_loss','radio_stuck'):
            r['checks']['decision_independent_of_telemetry']=r['release_time_s']==baseline
            r['passed']=all(r['checks'].values())
    source=ROOT/'data/roket5500ft.ork'
    with ZipFile(source) as z:root=ET.fromstring(z.read('rocket.ork'))
    saved=root.find('.//simulation/flightdata').attrib
    summary=dict(ork_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),saved_flight=saved,
                 independent_run=dict(apogee_m=apo['altitude_m'],apogee_s=apo['time_s'],seed=20261017,recovery='suppressed',duration_s=30),
                 nominal=results[0],passed=sum(x['passed'] for x in results),total=len(results),cases=results,
                 scope='Open-loop software-in-the-loop, not electronics/RF/closed-loop recovery certification')
    summary['sources_sha256']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
        for folder in ('core','tools','tests') for p in (ROOT/folder).glob('*') if p.is_file() and p.suffix in ('.hpp','.cpp','.py','.java')}
    (ROOT/'results/summary.json').write_text(json.dumps(summary,indent=2))
    plot(nominal_rows,apogee)
    passed=summary['passed'];total=summary['total']
    lines=['# Dijital UKB test sonuçları','',f'**{passed}/{total} senaryo beklenen davranışı gösterdi.**','',
           f"Kaynak: `roket5500ft.ork`; SHA-256: `{summary['ork_sha256']}`.",'',
           f"Kayıtlı koşu: {saved['maxaltitude']} m AGL. Bağımsız, kurtarma olayları kapalı koşu: {apo['altitude_m']:.2f} m, tepe noktası {apo['time_s']:.3f} s.",'',
           'Tüm replay zamanlarında ateşleme öncesi 5 saniyelik kalibrasyon/bekleme bölümü vardır. Aşağıdaki gecikmeler tepe noktasına göredir.',
           '', '| Senaryo | Beklenti | Sonuç | Tepe noktasından komuta gecikme |','|---|---|---|---|']
    for r in results:
        delay='—' if r['apogee_delay_s'] is None else f"{r['apogee_delay_s']:.3f} s"
        lines.append(f"| {r['name']} | {r['expected']} | {'GEÇTİ' if r['passed'] else 'KALDI'} | {delay} |")
    lines+=['','## Sonucun anlamı','',
        '`inhibit` senaryolarının geçmesi, arızada kurtarma yapıldığı anlamına gelmez; yanlış/kanıtsız komut üretilmediğini gösterir. Kalıcı basınç/IMU arızası ve uçuşta reset bu özgün UKB’nin kurtarma görevini engeller. Ana Sky Logic V2 bu pakette modellenmedi.',
        '', '30 Monte Carlo koşusu: sabit tohumlar, basınç/ivme/gyro gürültüsü, ±%1 ivme ölçeği, ±%0,5 gyro ölçeği ve 0–80 ms basınç gecikmesi. Bu aralıklar ölçülmüş hata dağılımları değildir.',
        '', 'Sanal sensörler analog bant genişliği, PCB, güç, montaj titreşimi, RF menzili ve elektromıknatısı doğrulamaz. OpenRocket yeniden örnekleme/enterpolasyon hatası vardır. Kurtarma açılması bu koşuya geri beslenmez. Komut sonrası yörünge gerçek iniş öngörüsü değildir.',
        '', 'ESP32 hedef derlemesi ayrıca `firmware-build.log` ile değerlendirilir; masaüstü testinin geçmesi hedef derlemesinin geçtiği anlamına gelmez. Resmî TEKNOFEST EK-7 protokolü bu sürümde uygulanmadı.']
    (ROOT/'results/SONUCLAR.md').write_text('\n'.join(lines)+'\n')
    print(f'RESULT: {passed}/{total}')
    return 0 if passed==total else 1

if __name__=='__main__':sys.exit(main())
