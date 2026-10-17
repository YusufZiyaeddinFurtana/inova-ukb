import sys
from pathlib import Path
import struct
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from ek7_device import command,encode,decode,Stream,FIELDS

class DeviceCodecTest(unittest.TestCase):
    def test_literal_commands(self):
        self.assertEqual(command(0x20).hex(),'aa208c0d0a')
        self.assertEqual(command(0x22).hex(),'aa228e0d0a')
        self.assertEqual(command(0x24).hex(),'aa24900d0a')
    def test_both_byte_orders(self):
        data=[970,1013.25,9.8,-1,2,30,-40,50]
        for order in ('little','big'):
            packet=encode(data,order);self.assertEqual(len(packet),36)
            decoded=decode(packet,order)
            for field,value in zip(FIELDS,data):self.assertAlmostEqual(decoded[field],value,places=5)
    def test_status_and_bad_reserved(self):
        self.assertTrue(decode(bytes.fromhex('aa80002a0d0a'))['main'])
        with self.assertRaises(ValueError):decode(bytes.fromhex('aa80012b0d0a'))
    def test_bad_checksum_and_footer(self):
        for packet in ('aa14001a0d0a','aa1400be000a'):
            with self.assertRaises(ValueError):decode(bytes.fromhex(packet))
    def test_byte_stream_fragmentation_and_noise(self):
        data=[970,900,9.8,0,0,0,0,0];packet=encode(data);stream=Stream();found=[]
        for i,b in enumerate(b'\x00\x01'+packet+packet):found+=stream.feed(bytes((b,)),i*.0001)
        self.assertEqual(len(found),2);self.assertEqual(found[0][0],packet)
    def test_incomplete_frame_timeout(self):
        packet=encode([970,900,9.8,0,0,0,0,0]);stream=Stream()
        self.assertEqual(stream.feed(packet[:20],0),[])
        self.assertEqual(stream.feed(packet[20:],.101),[])
        self.assertEqual(len(stream.feed(packet,.3)),1)
    def test_marker_bytes_in_float_payload(self):
        marker=struct.unpack('<f',bytes.fromhex('aaab0d0a'))[0]
        packet=encode([970,900,9.8,marker,0,0,0,0]);stream=Stream()
        self.assertEqual(len(stream.feed(packet,0)),1)

if __name__=='__main__':unittest.main()
