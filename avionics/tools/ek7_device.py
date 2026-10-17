#!/usr/bin/env python3
"""Independent EK-7 codec and optional physical RS232 test-device CLI.

Defaults are the documented LOCAL profile, not a verified referee-device protocol.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import struct
import time

FIELDS = ['altitude_msl_m','pressure_mbar','accel_x','accel_y','accel_z','angle_x_deg','angle_y_deg','angle_z_deg']
COMMANDS = {0x20:0x8c,0x22:0x8e,0x24:0x90}

def checksum(prefix,scope='prefix'):
    return (sum(prefix)+(23 if scope=='prefix-footer' else 0))&255

def command(code,policy='table'):
    prefix=bytes((0xaa,code))
    c=COMMANDS[code] if policy=='table' else checksum(prefix,'prefix-footer' if policy=='sum-footer' else 'prefix')
    return prefix+bytes((c,13,10))

def encode(values,order='little',scope='prefix'):
    prefix=b'\xab'+struct.pack(('<' if order=='little' else '>')+'8f',*values)
    return prefix+bytes((checksum(prefix,scope),13,10))

def decode(packet,order='little',scope='prefix'):
    length=36 if packet[0]==0xab else 6
    if len(packet)!=length or packet[-2:]!=b'\r\n' or checksum(packet[:-3],scope)!=packet[-3]:
        raise ValueError('length, footer or checksum mismatch')
    if packet[0]==0xab:
        return dict(zip(FIELDS,struct.unpack(('<' if order=='little' else '>')+'8f',packet[1:33])))
    if packet[0]!=0xaa or packet[2]!=0:raise ValueError('header or reserved status bits')
    names=['launch','motor_guard_elapsed','minimum_height','angle_or_lateral','descending','drogue','below_height','main']
    return dict(status=packet[1],**{key:bool(packet[1]&(1<<bit)) for bit,key in enumerate(names)})

class Stream:
    def __init__(self,order='little',scope='prefix'):
        self.buffer=bytearray();self.started=None;self.order=order;self.scope=scope
    def feed(self,data,now):
        if self.started is not None and now-self.started>=.1:self.buffer.clear();self.started=None
        results=[]
        for b in data:
            if not self.buffer:self.started=now
            self.buffer.append(b)
            while self.buffer:
                if self.buffer[0] not in (0xaa,0xab):del self.buffer[0];continue
                size=36 if self.buffer[0]==0xab else 6
                if len(self.buffer)<size:break
                packet=bytes(self.buffer[:size])
                try:decoded=decode(packet,self.order,self.scope)
                except ValueError:del self.buffer[0];continue
                results.append((packet,decoded));del self.buffer[:size];self.started=None
        return results

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',required=True,help='Real USB-to-RS232 port, never USB-to-TTL')
    p.add_argument('--mode',choices=['sit','sut'],required=True)
    p.add_argument('--input',type=Path,help='SUT CSV: '+','.join(FIELDS))
    p.add_argument('--seconds',type=float,default=30)
    p.add_argument('--input-hz',type=float,default=10,help='Must match firmware Profile.synthetic_period_s')
    p.add_argument('--float-order',choices=['little','big'],default='little')
    p.add_argument('--checksum-scope',choices=['prefix','prefix-footer'],default='prefix')
    p.add_argument('--command-policy',choices=['table','sum-prefix','sum-footer'],default='table')
    p.add_argument('--output',type=Path,required=True,help='Timestamped RX/TX JSONL log')
    args=p.parse_args()
    if args.mode=='sut' and not args.input:p.error('SUT requires --input')
    if not 5<=args.input_hz<=100:p.error('--input-hz must be 5..100 and match firmware')
    if args.seconds<=0:p.error('--seconds must be positive')
    rows=[]
    if args.input:
        with args.input.open() as f:rows=[[float(r[k]) for k in FIELDS] for r in csv.DictReader(f)]
        if not rows:p.error('empty SUT input')
        if not all(math.isfinite(x) for row in rows for x in row):p.error('non-finite input; use digital fault tests for malformed frames')
    import serial
    stream=Stream(args.float_order,args.checksum_scope)
    with args.output.open('w') as log,serial.Serial(args.port,115200,bytesize=8,parity='N',stopbits=1,timeout=0,write_timeout=.1) as port:
        start=time.monotonic();next_input=1.1;index=0
        def record(direction,packet,decoded=None):
            entry=dict(time_s=round(time.monotonic()-start,6),direction=direction,hex=packet.hex(),decoded=decoded)
            log.write(json.dumps(entry,allow_nan=True)+'\n');log.flush()
            if direction=='RX':print(json.dumps(entry,allow_nan=True),flush=True)
        def send(packet):port.write(packet);record('TX',packet)
        try:
            send(command(0x20 if args.mode=='sit' else 0x22,args.command_policy))
            while time.monotonic()-start<args.seconds:
                now=time.monotonic()-start
                if args.mode=='sut' and now>=next_input:
                    if index>=len(rows):break
                    send(encode(rows[index],args.float_order,args.checksum_scope));index+=1
                    next_input=now+1/args.input_hz # No backlog burst after host scheduling stalls.
                for packet,decoded in stream.feed(port.read(port.in_waiting or 1),now):record('RX',packet,decoded)
                time.sleep(.001)
        finally:send(command(0x24,args.command_policy))

if __name__=='__main__':main()
