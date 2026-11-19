"""Original-free mutation checks for DynamicDifficulty named registrations."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_dynamic_difficulty_init import (
    BINDINGS,VA,LENGTH,REGISTRY_GLOBAL,REGISTRAR,
    verify_native,audit_original,AuditError)
class Fake:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.body=bytes([19])*LENGTH
        self.data[VA,LENGTH]=self.body
        for i,(name,nameva,callback) in enumerate(BINDINGS):
            entry=VA+i*0x15
            self.data[entry,6]=b"\x8b\x0d"+struct.pack("<I",REGISTRY_GLOBAL)
            self.data[entry+6,5]=b"\x68"+struct.pack("<I",callback)
            self.data[entry+11,5]=b"\x68"+struct.pack("<I",nameva)
            call=entry+16
            self.data[call,5]=b"\xe8"+struct.pack("<i",REGISTRAR-call-5)
            self.data[nameva,len(name)+1]=name.encode()+b"\x00"
        self.data[VA+0xE7,3]=b"\xb0\x01\xc3"
    def read(self,va,n):return self.data[va,n]
class Tests(unittest.TestCase):
    def test_synthetic_positive_and_metadata(self):
        f=Fake()
        r=verify_native(f,hashlib.sha256(f.body).hexdigest())
        self.assertEqual(len(r["bindings"]),11)
        self.assertEqual(r["bindings"][0]["name"],"GamePlayVarValue")
        self.assertEqual(r["bindings"][-1]["name"],"Cheating")
        self.assertEqual(r["full_function_bytes"],234)
    def test_full_region_sha_corruption(self):
        f=Fake()
        expected=hashlib.sha256(f.body).hexdigest()
        f.data[VA,LENGTH]=bytes(LENGTH)
        with self.assertRaisesRegex(AuditError,"complete DynamicDifficulty"):
            verify_native(f,expected)
    def test_each_registration_input_mutation(self):
        for i,(name,nameva,callback) in enumerate(BINDINGS):
            start=VA+i*0x15
            keys=((start,6),(start+6,5),(start+11,5),(start+16,5),
                  (nameva,len(name)+1))
            for key in keys:
                f=Fake();before=f.data[key];f.data[key]=bytes([before[0]^1])+before[1:]
                with self.subTest(entry=i,key=key),self.assertRaises(AuditError):
                    verify_native(f,hashlib.sha256(f.body).hexdigest())
    def test_wrong_return_and_original_hash_refused(self):
        f=Fake();f.data[VA+0xE7,3]=bytes(3)
        with self.assertRaisesRegex(AuditError,"return"):
            verify_native(f,hashlib.sha256(f.body).hexdigest())
        with tempfile.TemporaryDirectory() as root:
            p=pathlib.Path(root)/"client.dll";p.write_bytes(b"wrong")
            with self.assertRaisesRegex(AuditError,"full SHA256"):
                audit_original(p)
if __name__=="__main__":unittest.main()
