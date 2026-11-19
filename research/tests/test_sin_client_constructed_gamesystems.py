"""Original-free constructor/vptr/RTTI corruption tests."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_constructed_gamesystems import (
    CONSTRUCTED,store_bytes,verify_constructed,verify_detail_full,DETAIL_INIT,audit_original,AuditError,BASE)
class FakePE:
    base=BASE
    def __init__(self):
        self.regions={}
        self.specs=[]
        for (index,name,obj,delta,vt,col,td,init,store,kind,_) in CONSTRUCTED:
            code=(bytes([index+15])*16)
            if index in (1,3,6):
                code=bytes((0xb0,1,0xc3))+code[3:]
            sha=hashlib.sha256(code).hexdigest()
            self.specs.append((index,name,obj,delta,vt,col,td,init,store,kind,sha))
            instruction=store_bytes(obj,vt,kind)
            self.regions[store,len(instruction)]=instruction
            self.regions[vt-4,4]=struct.pack("<I",col)
            hierarchy=0x25000000+0x100*index
            self.regions[col,20]=struct.pack("<IIIII",0,delta,0,td,hierarchy)
            self.regions[hierarchy,4]=bytes(4)
            typename=(".?AV"+name+"@@").encode()+bytes(1)
            self.regions[td+8,len(typename)]=typename
            self.regions[vt,4]=struct.pack("<I",init)
            self.regions[init,16]=code
            if index in (1,3,6):
                self.regions[init,3]=code[:3]
    def read(self,va,n):
        return self.regions[va,n]
class ConstructorTests(unittest.TestCase):
    def test_eight_constructed_original_free(self):
        f=FakePE();r=verify_constructed(f,f.specs)
        self.assertEqual(len(r["constructor_rtti_owners"]),8)
        self.assertEqual([s["index"] for s in r["constructor_rtti_owners"]],[1,3,4,6,7,8,9,11])
        self.assertEqual(sum(x["init_complete_three_byte_true"] for x in r["constructor_rtti_owners"]),3)
        self.assertEqual(r["constructor_rtti_owners"][1]["rtti_subobject_offset"],4)
    def test_constructor_store_corruption_detected(self):
        for index,_,obj,delta,vt,col,td,init,store,kind,sha in CONSTRUCTED:
            f=FakePE()
            key=store,len(store_bytes(obj,vt,kind))
            value=f.regions[key];f.regions[key]=bytes([value[0]^1])+value[1:]
            with self.subTest(index=index),self.assertRaisesRegex(AuditError,"constructor"):
                verify_constructed(f,f.specs)
    def test_every_rtti_and_method_entry_corruption_detected(self):
        for idx,name,obj,delta,vt,col,td,init,store,kind,sha in CONSTRUCTED:
            typename=(".?AV"+name+"@@").encode()+bytes(1)
            for key in ((vt-4,4),(col,20),(td+8,len(typename)),(vt,4),(init,16)):
                f=FakePE();v=f.regions[key];f.regions[key]=bytes([v[0]^1])+v[1:]
                with self.subTest(index=idx,field=hex(key[0])),self.assertRaises(AuditError):
                    verify_constructed(f,f.specs)
    def test_complete_detail_init_sha_refusal(self):
        f=FakePE()
        va,length,_=DETAIL_INIT
        sample=bytes([31])*length
        f.regions[va,length]=sample
        h=hashlib.sha256(sample).hexdigest()
        self.assertEqual(verify_detail_full(f,h)["complete_bytes"],31)
        f.regions[va,length]=bytes([32])*length
        with self.assertRaisesRegex(AuditError,"complete DetailObjectSystem"):
            verify_detail_full(f,h)
    def test_wrong_original_refused(self):
        with tempfile.TemporaryDirectory() as root:
            p=pathlib.Path(root)/"client.dll";p.write_bytes(b"not original")
            with self.assertRaisesRegex(AuditError,"full SHA256"):
                audit_original(p)
    def test_invalid_store_kind_rejected(self):
        with self.assertRaisesRegex(AuditError,"unsupported constructor"):
            store_bytes(0,0,"fake")
if __name__=="__main__":unittest.main()
