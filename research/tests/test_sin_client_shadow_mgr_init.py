"""Original-free synthetic CClientShadowMgr read-only auditor regressions."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_shadow_mgr_init import (
    VA,SIZE,CALLS,ANCHORS,FLOATS,LITERALS,AuditError,
    verify,audit_original)
class FakePE:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.body=b"\x23"*SIZE
        self.data[VA,SIZE]=self.body
        for addr,expected in ANCHORS.items():self.data[addr,len(expected)]=expected
        for addr,target in CALLS.items():
            self.data[addr,5]=b"\xe8"+struct.pack("<i",target-addr-5)
        for addr,value in FLOATS.items():
            if addr in (0x241188C5,0x241188D1):
                self.data[addr,5]=b"\x68"+struct.pack("<I",value)
            else:
                self.data[addr,7]=b"\xc7\x44\x24\x08"+struct.pack("<I",value)[:3]
                self.data[addr+4,4]=struct.pack("<I",value)
        for addr,text in LITERALS.items():
            self.data[addr,len(text)+1]=text.encode()+b"\0"
    def read(self,addr,size):
        return self.data[addr,size]
    def run(self):return verify(self,hashlib.sha256(self.body).hexdigest())
class Tests(unittest.TestCase):
    def test_synthetic_positive_and_metadata(self):
        out=FakePE().run()
        self.assertEqual(out["complete_method_bytes"],146)
        self.assertEqual(out["own_virtual_slots"],[24,27,28])
        self.assertEqual(out["engine_virtual_slot"],116)
        self.assertTrue(out["original_true_return"])
    def test_full_digest_rejected(self):
        with self.assertRaisesRegex(AuditError,"body SHA"):
            verify(FakePE())
    def test_witness_mutations(self):
        for addr,raw in ANCHORS.items():
            m=FakePE()
            m.data[addr,len(raw)]=bytes((raw[0]^1,))+raw[1:]
            with self.subTest(addr=hex(addr)),self.assertRaises(AuditError):m.run()
    def test_call_mutations(self):
        for addr in CALLS:
            m=FakePE()
            m.data[addr,5]=b"\xe8"+bytes(4)
            with self.subTest(addr=hex(addr)),self.assertRaisesRegex(AuditError,"direct call"):
                m.run()
    def test_scalar_mutations(self):
        for addr,value in FLOATS.items():
            m=FakePE()
            loc=addr if addr in (0x241188C5,0x241188D1) else addr+4
            m.data[loc,5 if loc==addr else 4]=bytes(5 if loc==addr else 4)
            with self.subTest(addr=hex(addr)),self.assertRaises(AuditError):
                m.run()
    def test_literal_mutations(self):
        for addr,text in LITERALS.items():
            m=FakePE();m.data[addr,len(text)+1]=b"x"+text.encode()[1:]+b"\0"
            with self.subTest(addr=hex(addr)),self.assertRaisesRegex(AuditError,"literal"):
                m.run()
    def test_reject_wrong_file_and_base(self):
        m=FakePE();m.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):m.run()
        with tempfile.TemporaryDirectory() as dirname:
            p=pathlib.Path(dirname)/"client.dll";p.write_bytes(b"fake")
            with self.assertRaisesRegex(AuditError,"file SHA"):audit_original(p)
if __name__=="__main__":unittest.main()
