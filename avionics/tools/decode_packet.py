"""Decode the project's binary telemetry, independent of the C++ encoder."""
import argparse
import binascii
import json
import struct

STATES = ['BOOT', 'READY', 'ARMED', 'BOOST', 'COAST', 'DESCENT', 'LANDED', 'FAULT']

def decode(packet):
    if len(packet) != 34 or packet[:3] != b'IN\x01':
        raise ValueError('Paket uzunluğu/başlığı geçersiz')
    if binascii.crc_hqx(packet[:32], 0xffff) != struct.unpack_from('<H', packet, 32)[0]:
        raise ValueError('CRC hatası')
    state, seq, flags, ms, alt, vb, vi, lat, lon, mv, tilt = struct.unpack_from('<BHHIihhiiHH', packet, 3)
    if state >= len(STATES):
        raise ValueError('Durum geçersiz')
    return dict(state=STATES[state], sequence=seq, flags=flags, time_s=ms/1000,
                altitude_m=alt/100, baro_v_m_s=vb/100, imu_v_m_s=vi/100,
                latitude=lat/1e7, longitude=lon/1e7, battery_mv=mv,
                tilt_deg=tilt/100, release=bool(flags & 0x4000), gps_valid=bool(flags & 0x8000))

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('hex_packet');a=p.parse_args()
    print(json.dumps(decode(bytes.fromhex(a.hex_packet)),ensure_ascii=False,indent=2))
