"""Synthetic original-free original 16-type Stats RTTI mutation tests."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_stats_definition_types import (
    CASES,RATES,JUMP_TABLE,DISPATCH_VA,DISPATCH_LENGTH,verify,audit_original,AuditError)
class Fake:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.body=bytes([91])*DISPATCH_LENGTH
        self.data[DISPATCH_VA,DISPATCH_LENGTH]=self.body
        for i,(entry,size,call,ctor,write,vt,td,name) in enumerate(CASES):
            self.data[JUMP_TABLE+4*i,4]=struct.pack("<I",entry)
            self.data[entry,2]=bytes((0x6a,size))
            if call:self.data[call,5]=b"\xe8"+struct.pack("<i",ctor-call-5)
            self.data[write,6]=b"\xc7\x00"+struct.pack("<I",vt)
            locator=vt+0x2000
            self.data[vt-4,4]=struct.pack("<I",locator)
            self.data[locator+12,4]=struct.pack("<I",td)
            namebytes=(".?AV"+name+"@@").encode()+b"\0"
            self.data[td+8,len(namebytes)]=namebytes
        for addr,bits in RATES.items():
            self.data[addr,5]=b"\x68"+struct.pack("<I",bits)
    def read(self,addr,n):return self.data[addr,n]
    def run(self):return verify(self,hashlib.sha256(self.body).hexdigest())
class TypeTests(unittest.TestCase):
    def test_all_original_free_types_match(self):
        out=Fake().run()
        self.assertEqual(len(out["cases"]),16)
        self.assertEqual(out["unique_native_rtti_classes"],14)
        self.assertEqual(out["cases"][0]["class"],"CIntStat")
        self.assertEqual(out["cases"][1]["class"],"CFloatStat")
        self.assertEqual(out["cases"][11]["class"],"CAveragePerTimeStat")
    def test_full_dispatch_digest_fail_closed(self):
        with self.assertRaisesRegex(AuditError,"switch body SHA"):verify(Fake())
    def test_jump_table_allocation_and_constructor_sites_fail_closed(self):
        for i,(entry,size,call,ctor,write,vt,td,name) in enumerate(CASES):
            for target,length in ((JUMP_TABLE+4*i,4),(entry,2),
                                  (call,5),(write,6)):
                if not target:continue
                sample=Fake();old=sample.data[target,length]
                sample.data[target,length]=bytes([old[0]^1])+old[1:]
                with self.subTest(i=i,site=hex(target)),self.assertRaises(AuditError):
                    sample.run()
    def test_all_rtti_and_rate_checks_fail_closed(self):
        for i,(entry,size,call,ctor,write,vt,td,name) in enumerate(CASES):
            sample=Fake()
            namebytes=(".?AV"+name+"@@").encode()+b"\0"
            target=td+8
            original=sample.data[target,len(namebytes)]
            sample.data[target,len(namebytes)]=bytes([original[0]^1])+original[1:]
            with self.subTest(type_index=i),self.assertRaisesRegex(AuditError,"RTTI"):
                sample.run()
        for site in RATES:
            sample=Fake();sample.data[site,5]=bytes(5)
            with self.subTest(site=hex(site)),self.assertRaisesRegex(AuditError,"rate"):
                sample.run()
    def test_wrong_file_or_image_base_refused(self):
        sample=Fake();sample.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):sample.run()
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/"client.dll";path.write_bytes(b"not original")
            with self.assertRaisesRegex(AuditError,"full file SHA"):audit_original(path)
if __name__=="__main__":unittest.main()
