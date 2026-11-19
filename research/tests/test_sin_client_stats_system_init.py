"""Synthetic, original-byte-free StatsSystem Init ABI-audit mutations."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_stats_system_init import (
    INIT_VA,INIT_SIZE,CALLS,LITERALS,AuditError,
    verify_layout,audit_original)
class FakePE:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.body=b"\x55"*INIT_SIZE
        self.data[INIT_VA,INIT_SIZE]=self.body
        for a,b in CALLS:
            self.data[a,5]=b"\xe8"+struct.pack("<i",b-a-5)
        for a,b in LITERALS.items():
            self.data[a,len(b)+1]=b.encode()+b"\0"
        self.data[0x241FCB0E,4]=b"\xb0\x01\x5e\xc3"
    def read(self,a,n):return self.data[a,n]
    def verify(self):return verify_layout(self,hashlib.sha256(self.body).hexdigest())
class StatsAuditTest(unittest.TestCase):
    def test_synthetic_metadata(self):
        o=FakePE().verify()
        self.assertEqual(len(o["verified_seven_calls"]),7)
        self.assertEqual(o["registered_callback_names"],
            ["StaticStat","DynamicStat","ResetHUD"])
        self.assertEqual(o["fully_recreated_helper_bodies"],0)
    def test_method_digest_refusal(self):
        m=FakePE()
        with self.assertRaisesRegex(AuditError,"SHA"):
            verify_layout(m)
    def test_reject_every_original_callsite_mutation(self):
        for a,b in CALLS:
            m=FakePE();m.data[a,5]=b"\xe8"+bytes(4)
            with self.subTest(callsite=hex(a)),self.assertRaisesRegex(
                AuditError,"call edge"):m.verify()
    def test_reject_every_string_mutation(self):
        for a,b in LITERALS.items():
            m=FakePE()
            data=m.data[a,len(b)+1]
            m.data[a,len(b)+1]=bytes((data[0]^1,))+data[1:]
            with self.subTest(literal=b),self.assertRaisesRegex(
                AuditError,"string"):m.verify()
    def test_reject_return_or_wrong_image_base(self):
        m=FakePE();m.data[0x241FCB0E,4]=bytes(4)
        with self.assertRaisesRegex(AuditError,"return"):m.verify()
        m=FakePE();m.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):m.verify()
    def test_refuse_nonoriginal_client(self):
        with tempfile.TemporaryDirectory() as d:
            sample=pathlib.Path(d)/"client.dll"
            sample.write_bytes(b"fake PE")
            with self.assertRaisesRegex(AuditError,"Owner-original client SHA"):
                audit_original(sample)
if __name__=="__main__":unittest.main()
