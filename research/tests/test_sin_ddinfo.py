"""Synthetic original-DLL-free tests for DDInfo field and state evidence."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_ddinfo import C,S,SCALL,CCALL,verify_ddinfo,VerificationError

class FakePE:
    def __init__(self):
        self.bytes={}
    def put(self,va,raw):
        for offset,byte in enumerate(raw):
            old=self.bytes.get(va+offset,byte)
            if old!=byte:raise AssertionError('overlapping synthetic fixture')
            self.bytes[va+offset]=byte
    def at(self,va,count):
        return bytes(self.bytes.get(va+i,0xcc) for i in range(count))
    def flip(self,va):
        self.bytes[va]^=0x40

def fixture():
    client,server=FakePE(),FakePE()
    for pe,codes in ((server,S),(client,C)):
        for site,raw in codes.items():
            pe.put(site,bytes.fromhex(raw))
    server.put(0x2245CB10,b'DDInfo\0')
    for pe,calls in ((server,SCALL),(client,CCALL)):
        for site,target in calls.items():
            pe.put(site,b'\xe8'+struct.pack('<i',target-site-5))
    return client,server

class DDInfoTests(unittest.TestCase):
    def test_all_field_operands_and_call_edges(self):
        client,server=fixture()
        found=verify_ddinfo(client,server)
        self.assertEqual(found['verified_field_bits'],57)
        self.assertEqual(found['client_indices_overridden_by_shared_flag'],[2,3])
        self.assertEqual(found['wire_fields'],['bit','bit','f32','s16','bit','bit[6]'])
    def test_wrong_width_constant_rejected(self):
        for side,site in [('server',0x2225A164),('server',0x22258F00),
                          ('client',0x240A5600),('client',0x240A5631)]:
            with self.subTest(site=hex(site)):
                c,s=fixture()
                (s if side=='server' else c).flip(site+1)
                with self.assertRaises(VerificationError):
                    verify_ddinfo(c,s)
    def test_wrong_client_flag_override_rejected(self):
        for site in (0x241F9AD2,0x241F9ADA,0x241F9ADF,0x241F9B34,
                     0x241F9B41,0x241F9B50,0x241F9B5B):
            with self.subTest(site=hex(site)):
                c,s=fixture()
                c.flip(site)
                with self.assertRaises(VerificationError):
                    verify_ddinfo(c,s)
    def test_mutated_sender_and_receiver_calls_rejected(self):
        for side,site in [('server',0x222BA603),('server',0x222BA642),
                          ('server',0x222BA64B),('server',0x222BA66E),
                          ('client',0x241F9A3A),('client',0x241F9A44)]:
            with self.subTest(site=hex(site)):
                c,s=fixture()
                (s if side=='server' else c).flip(site+1)
                with self.assertRaises(VerificationError):
                    verify_ddinfo(c,s)
    def test_mutated_message_name_rejected(self):
        c,s=fixture()
        s.flip(0x2245CB10)
        with self.assertRaises(VerificationError):
            verify_ddinfo(c,s)

if __name__=='__main__':
    unittest.main()
