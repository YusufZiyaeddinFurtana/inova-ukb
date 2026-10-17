#!/usr/bin/env python3
"""Stimulate register-level sensor surrogates, run real drivers, inspect EK-7 bytes."""
import csv
import json
import math
import os
import subprocess
import sys
from pathlib import Path
from ek7_device import command,decode,encode
from run_sil import ROOT,read_model,make_profile,rotate,qinv,qmul,modify,PAD
from run_ek7 import make_data
from source_manifest import source_hashes
import hashlib

FIELDS='time_s,pressure_pa,temperature_c,ax,ay,az,gx,gy,gz,arm,command,imu_fault,baro_fault'.split(',')
DEST=ROOT/'results/sensors'

def pressure_at(h):
    # Physical stimulus uses the .ork site's lapse-rate atmosphere.
    return 90209.744*pow(1-h*.0065/281.845,1/.190294957)

def sit_profile(kind):
    rows=[]
    for tick in range(1001):
        t=tick*.01;h=0;temp=20;a=(0,0,9.797);g=(0,0,0);imuf=barof=0
        if kind=='lift_3m':h=3 if t>=5 else 0
        if kind=='lift_return':h=3*max(0,min(t-5,1,8-t))
        if kind.startswith('rotate_'):
            axis='xyz'.index(kind[-1]);angle=min(max(t-5,0),2)*math.pi/4
            xyz=[0,0,0];xyz[axis]=math.sin(angle/2);q=(math.cos(angle/2),*xyz)
            a=rotate(qinv(q),(0,0,9.797));g=list(g);g[axis]=math.pi/4 if 5<=t<7 else 0
        if kind=='temperature':temp=35 if t>=5 else 20
        if kind=='pad_jolt' and 5<=t<5.04:a=(0,0,4*9.797)
        if kind=='imu_nack' and 5<=t<6.2:imuf=1
        if kind=='imu_short' and 5<=t<6.2:imuf=2
        if kind=='imu_stopped' and t>=5:imuf=3
        if kind=='bmp_nack' and 5<=t<6.2:barof=1
        if kind=='bmp_short' and 5<=t<6.2:barof=2
        if kind=='bmp_stopped' and t>=5:barof=3
        pressure=pressure_at(h)-(4000 if kind=='pressure_spike' and 5<=t<5.02 else 0)
        rows.append(dict(zip(FIELDS,[t+.2,pressure,temp,*a,*g,1,
            command(0x20).hex() if tick==0 else command(0x24).hex() if tick==1000 else '',imuf,barof])))
    return rows

def flight_profile(at,kind,noise=False):
    raw=make_profile(at,noise=noise);rows=[]
    for r in raw:
        t=r['time_s'];imuf=3 if kind=='imu_stopped' and t>=14 else 0
        barof=3 if kind=='bmp_stopped' and t>=14 else 0
        a=[r[k] for k in ('ax','ay','az')];g=[r[k] for k in ('gx','gy','gz')]
        if kind=='saturation' and 6<=t<6.03:a[0]=200
        pressure=r['pressure_pa']-(4000 if kind=='pressure_spike' and 13<=t<13.02 else 0)
        rows.append(dict(zip(FIELDS,[t+.2,pressure,20,*a,*g,int(t>=3 and kind!='unarmed'),'',imuf,barof])))
    return rows

def sut_profile(at,kind):
    rows=[]
    for tick in range(3601):
        t=tick*.01;packet=''
        if tick==0:packet=command(0x22).hex()
        if tick>=101 and (tick-101)%10==0:
            ft=t-1.01
            if kind!='stream_gap' or ft<13:packet=encode(make_data(at,ft)).hex()
        if tick==3600:packet=command(0x24).hex()
        a=(0,0,9.797);g=(0,0,0);pressure=90209.744;imuf=barof=0
        if kind=='disturbed':
            a=(300,-300,300);g=(50,-50,50);pressure=40000;imuf=1 if tick%20<10 else 0;barof=1
        rows.append(dict(zip(FIELDS,[t+.2,pressure,20,*a,*g,1,packet,imuf,barof])))
    return rows

def run_case(name,profile):
    target=DEST/name;target.mkdir(parents=True,exist_ok=True)
    with (target/'stimulus.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=FIELDS,lineterminator='\n');writer.writeheader();writer.writerows(profile)
    subprocess.run([str(ROOT/'build/sensor_replay'),str(target/'stimulus.csv'),str(target/'trace.csv')],check=True)
    with (target/'trace.csv').open() as f:trace=list(csv.DictReader(f))
    packets=[]
    for r in trace:
        if r['tx']:packets.append((float(r['time_s']),decode(bytes.fromhex(r['tx']))))
    return trace,packets

def main():
    DEST.mkdir(exist_ok=True)
    verify_driver_copies()
    subprocess.run(['make','build/sensor_tests','build/sensor_replay'],cwd=ROOT,check=True)
    unit=subprocess.run([str(ROOT/'build/sensor_tests')],check=True,capture_output=True,text=True)
    print(unit.stdout,end='');(DEST/'invariants.log').write_text(unit.stdout)
    results=[];traces={}
    kinds=['pad','lift_3m','lift_return','rotate_x','rotate_y','rotate_z','temperature','pad_jolt','pressure_spike',
           'imu_nack','imu_short','imu_stopped','bmp_nack','bmp_short','bmp_stopped']
    for kind in kinds:
        name='sit_'+kind;trace,packets=run_case(name,sit_profile(kind));traces[name]=trace
        readings=[(t,p) for t,p in packets if t>3.5]
        final=readings[-1][1];checks=dict(no_release=not any(int(r['release']) for r in trace),
            ten_hz=all(abs(b[0]-a[0]-.1)<1e-7 for a,b in zip(packets,packets[1:])),
            stop=trace[-1]['mode']=='0' and not trace[-1]['tx'])
        metrics={}
        if kind in ('pad','lift_3m','lift_return','temperature','pad_jolt','pressure_spike'):
            expected=973 if kind=='lift_3m' else 970
            metrics['final_altitude_msl_m']=final['altitude_msl_m'];checks['height_response']=abs(final['altitude_msl_m']-expected)<.15
        if kind=='lift_return':
            metrics['peak_altitude_msl_m']=max(p['altitude_msl_m'] for _,p in readings)
            checks['saw_three_metre_rise']=abs(metrics['peak_altitude_msl_m']-973)<.15
        if kind=='pressure_spike':checks['spike_filtered']=max(abs(p['altitude_msl_m']-970) for _,p in readings)<.5
        if kind=='temperature':
            metrics['final_temperature_c']=float(trace[-2]['temperature_c']);checks['temperature_response']=abs(metrics['final_temperature_c']-35)<.02
        if kind.startswith('rotate_'):
            q=packet_attitude(final);axis='xyz'.index(kind[-1]);target=[math.sqrt(.5),0,0,0];target[axis+1]=math.sqrt(.5)
            err=math.degrees(2*math.acos(min(1,abs(sum(x*y for x,y in zip(q,target))))))
            metrics['orientation_error_deg']=err;checks['ninety_degree_response']=err<2
            metrics['reported_angles_deg']=[final[f'angle_{k}_deg'] for k in 'xyz']
        if kind.startswith(('imu_','bmp_')):
            field='accel_x' if kind.startswith('imu_') else 'altitude_msl_m'
            checks['missing_measurement_is_NaN']=any(math.isnan(p[field]) for t,p in readings if 5.6<t<6.2)
            if kind.endswith('stopped'):checks['still_unavailable']=math.isnan(final[field])
            else:checks['recovers']=math.isfinite(final[field])
        results.append(result(name,'SİT',checks,metrics))
    at,apo=read_model()
    for kind,noise in [('nominal',False),('noise',True),('unarmed',False),('pressure_spike',False),
                       ('imu_stopped',False),('bmp_stopped',False),('saturation',False)]:
        name='flight_'+kind;trace,packets=run_case(name,flight_profile(at,kind,noise));traces[name]=trace
        release=next((float(r['time_s']) for r in trace if r['release']=='1'),None)
        inhibit=kind in ('unarmed','imu_stopped','bmp_stopped','saturation')
        delay=release-(.2+PAD+apo['time_s']) if release is not None else None
        checks=dict(decision=release is None if inhibit else delay is not None and 0<=delay<=3)
        if kind=='imu_stopped':checks['fault_visible']=any(int(r['health'])&4 for r in trace)
        if kind=='bmp_stopped':checks['fault_visible']=any(int(r['health'])&1 for r in trace)
        if kind=='saturation':checks['fault_visible']=any(int(r['health'])&8 for r in trace)
        results.append(result(name,'Uçuş',checks,dict(release_time_s=release,apogee_delay_s=delay)))
    sut_release={}
    for kind in ('clean','disturbed','stream_gap'):
        name='sut_'+kind;trace,packets=run_case(name,sut_profile(at,kind));traces[name]=trace
        release=next((float(r['time_s']) for r in trace if r['release']=='1'),None);sut_release[kind]=release
        delay=release-(.2+1.01+PAD+apo['time_s']) if release is not None else None
        checks=dict(decision=(release is None) if kind=='stream_gap' else delay is not None and 0<=delay<=3,
                    status_packets=bool(packets) and all('status' in p for _,p in packets),
                    stop_clears=trace[-1]['mode']=='0' and trace[-1]['release']=='0')
        if kind=='disturbed':checks['real_sensors_ignored']=release==sut_release['clean']
        if kind=='stream_gap':checks['timeout']=any(r['mode']=='0' for r in trace if 15.5<float(r['time_s'])<16)
        results.append(result(name,'SUT',checks,dict(release_time_s=release,apogee_delay_s=delay)))
    write_report(results,unit.stdout,traces,apo)
    return 0 if all(r['passed'] for r in results) else 1

def packet_attitude(p):
    x,y,z=(math.radians(p[f'angle_{k}_deg'])/2 for k in 'xyz')
    q=qmul(qmul((math.cos(z),0,0,math.sin(z)),(math.cos(y),0,math.sin(y),0)),(math.cos(x),math.sin(x),0,0))
    return q[0],q[2],q[3],q[1]

def result(name,mode,checks,metrics):
    passed=all(checks.values());parts=['PASS' if passed else 'FAIL',name,str(metrics)]
    if not passed:parts.append(str(checks))
    print(*parts)
    return dict(name=name,mode=mode,passed=passed,checks=checks,metrics=metrics)

def write_report(results,units,traces,apo):
    summary=dict(total=len(results),passed=sum(r['passed'] for r in results),driver_invariant_groups=units.count('PASS '),cases=results,
                 real_drivers=True,register_level_sensor_surrogates=True,hardware_executed=False,
                 sensor_model=dict(imu='MPU9255 signed16, FS16g/2000dps, 100Hz, first-order tau4ms surrogate',
                                   bmp='BMP388 unsigned24, affine synthetic trim fixture, vendor compensation, forced conversion timing, 50Hz host polling'),
                 sources_sha256=source_hashes(),
                 limitations=['Not a silicon/digital-twin qualification','ESP32 scheduler/electrical I2C/RS232/RF/actuator not emulated',
                              'No parachute deployment fed back into trajectory','Factory calibration/noise/temperature coupling not characterized'])
    (DEST/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    plot_results(traces,apo)
    lines=['# Sensör uyarımlı SİT/SUT sonuçları','',f"**{summary['passed']}/{summary['total']} senaryo; {summary['driver_invariant_groups']} sürücü test grubu geçti.**",'',
           'Uyarım → sensör register modeli → aynı MPU9255 / Adafruit BMP3XX / Bosch / BusIO sürücüleri → uçuş çekirdeği → EK-7 ikili paketi yolu çalıştırıldı. Fiziksel kart çalıştırılmadı.',
           '', '![Sensör uyarımı ve yazılım tepkisi](sensor_response.png)',
           '', '| Senaryo | Mod | Sonuç | Ölçüm |','|---|---|---|---|']
    for r in results:lines.append(f"| {r['name']} | {r['mode']} | {'GEÇTİ' if r['passed'] else 'KALDI'} | {json.dumps(r['metrics'],ensure_ascii=False)} |")
    lines+=['','BMP388 dönüşümü durduğunda eski değer yeni örnek diye raporlanıyordu: veri-hazır bayraklarıyla giderildi. Uzun kesinti sonrası tamamlanmış eski dönüşüm de 100 ms yaş kontrolüyle atılıyor.',
            '', 'BMP388 trim değerleri gerçek modülden alınmış değildir: P=131072−rawP/128 ve T=(rawT−5120000)/65536 olacak şekilde seçilmiş test fixture’ıdır. Basınç/sıcaklık bayt sırası, kalibrasyon CRC ve gerçek vendor compensation yolu test edilir; gerçek sensörün bütün transfer fonksiyonu doğrulanmış olmaz.',
            '', 'MPU9255 için ±16g/±2000°/s, ADC nicemlemesi, 100 Hz ve DRDY davranışı modellenir. 4 ms birinci dereceden filtre ölçülmüş sensör dağılımı değildir. Manyetometre, GNSS ve LoRa silisyumu modellenmez.',
            '', '`inhibit` arızada kurtarma yapıldığı anlamına gelmez. GPIO25 kapalı; ana Sky Logic, güç, EMI, analog hatlar, gerçek zamanlayıcı ve mekanik ayrılma test edilmedi. Resmî EK-7 profil belirsizlikleri sürer.']
    (DEST/'SONUCLAR.md').write_text('\n'.join(lines)+'\n')

def verify_driver_copies():
    for library in json.loads((ROOT/'third_party/LOCK.json').read_text()):
        for name,expected in library['files_sha256'].items():
            local=ROOT/library['local_path']/name
            assert hashlib.sha256(local.read_bytes()).hexdigest()==expected,local
            installed=ROOT/'.pio/libdeps/esp32_bench'/library['name']/name
            if installed.is_file():assert hashlib.sha256(installed.read_bytes()).hexdigest()==expected,installed
    print('PASS pinned native and ESP32 sensor driver source hashes')

def plot_results(traces,apo):
    os.environ.setdefault('MPLCONFIGDIR',str(ROOT/'build/matplotlib'))
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:return
    fig,axes=plt.subplots(3,1,figsize=(11,10),layout='constrained')
    lift=traces['sit_lift_return'];rot=traces['sit_rotate_z'];flight=traces['flight_nominal']
    t=[float(r['time_s'])-.2 for r in lift]
    axes[0].plot(t,[3*max(0,min(x-5,1,8-x)) for x in t],label='Uygulanan yükseklik')
    decoded=[(float(r['time_s'])-.2,decode(bytes.fromhex(r['tx']))) for r in lift if r['tx']]
    axes[0].plot([x for x,_ in decoded],[p['altitude_msl_m']-970 for _,p in decoded],label='SİT paketindeki ölçüm',marker='.',ms=3)
    axes[0].set_ylabel('Yerden yükseklik (m)');axes[0].set_xlim(4.5,9)
    t=[float(r['time_s'])-.2 for r in rot]
    axes[1].plot(t,[45*min(max(x-5,0),2) for x in t],label='Uygulanan burun ekseni dönüşü')
    decoded=[(float(r['time_s'])-.2,decode(bytes.fromhex(r['tx']))) for r in rot if r['tx']]
    axes[1].plot([x for x,_ in decoded],[p['angle_x_deg'] for _,p in decoded],label='SİT paketindeki açı',marker='.',ms=3)
    axes[1].set_ylabel('Açı (derece)');axes[1].set_xlim(4.5,9)
    at,_=read_model();t=[float(r['time_s'])-.2-PAD for r in flight]
    axes[2].plot(t,[at(max(x,0))['altitude_m'] if x>=0 else 0 for x in t],label='OpenRocket')
    axes[2].plot(t,[float(r['altitude_m']) for r in flight],label='Sanal sensör + gerçek sürücü + filtre')
    release=next(x for x,r in zip(t,flight) if r['release']=='1')
    axes[2].axvline(apo['time_s'],color='gray',ls='--',label='Tepe noktası')
    axes[2].axvline(release,color='crimson',ls=':',label=f'Ayrılma isteği (+{release-apo["time_s"]:.2f} s)')
    axes[2].set_ylabel('Yerden irtifa (m)');axes[2].set_xlim(0,26)
    for i,ax in enumerate(axes):
        ax.grid(alpha=.2);ax.legend(fontsize=8);ax.set_xlabel('Ateşlemeden sonraki süre (s)' if i==2 else 'Test senaryosu süresi (s)')
    fig.suptitle('Sanal BMP388 / MPU9255 uyarımı → gerçek sürücüler → UKB tepkisi\nFiziksel çıkış kapalı; paraşüt açılması yörüngeye geri beslenmez')
    fig.savefig(DEST/'sensor_response.png',dpi=150);plt.close(fig)

if __name__=='__main__':sys.exit(main())
