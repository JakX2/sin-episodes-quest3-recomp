"""Original-free mutation tests of retail system vtable RTTI anchors."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_static_gamesystem_rtti import (
    validate_static_owners,audit_original,AuditError,OWNERS,BASE)

class Fake:
    base=BASE
    def __init__(self):
        self.values={}
        self.owners=[]
        for i,(name,obj,vt,col,td,slot0,size,sha,complete) in enumerate(OWNERS):
            value=bytes([i+12])*size
            self.owners.append((name,obj,vt,col,td,slot0,size,hashlib.sha256(value).hexdigest(),complete))
            self.values[obj,4]=struct.pack("<I",vt)
            self.values[vt-4,4]=struct.pack("<I",col)
            # CompleteObjectLocator's classHierarchy field is a valid
            # synthetic allocation with zero signature.
            chd=0x25000000+0x100*i
            self.values[col,20]=struct.pack("<IIIII",0,0,0,td,chd)
            self.values[chd,4]=bytes(4)
            self.values[td+8,len(name)+7]=(".?AV"+name+"@@").encode()+bytes(1)
            self.values[vt,4]=struct.pack("<I",slot0)
            self.values[slot0,size]=value
    def read(self,addr,size):return self.values[addr,size]
class AuditTests(unittest.TestCase):
    def test_positive_synthetic_original_free(self):
        m=Fake()
        r=validate_static_owners(m,m.owners)
        self.assertEqual(len(r["verified_static_systems"]),4)
        self.assertEqual([x["complete_method"] for x in r["verified_static_systems"]],[True,True,False,True])
        self.assertEqual([x["init_slot"] for x in r["verified_static_systems"]],[0]*4)
    def test_every_object_and_rtti_pointer_rejected_if_mutated(self):
        for idx,(_,obj,vt,col,td,slot0,size,sha,_) in enumerate(OWNERS):
            fake=Fake()
            for location in ((obj,4),(vt-4,4),(col,20),
                             (vt,4),(slot0,size)):
                f=Fake()
                before=f.values[location]
                f.values[location]=bytes([before[0]^1])+before[1:]
                with self.subTest(system=idx,location=hex(location[0])),self.assertRaises(AuditError):
                    validate_static_owners(f,f.owners)
            type_name=(".?AV"+OWNERS[idx][0]+"@@").encode()+bytes(1)
            fake.values[td+8,len(type_name)]=b"!" + type_name[1:]
            with self.subTest(system=idx,category="RTTI name"),self.assertRaisesRegex(AuditError,"class name"):
                validate_static_owners(fake,fake.owners)
    def test_unknown_image_base_and_file_hash_rejected(self):
        f=Fake();f.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):
            validate_static_owners(f,f.owners)
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/"client.dll";p.write_bytes(b"not original")
            with self.assertRaisesRegex(AuditError,"full SHA256"):
                audit_original(p)
if __name__=="__main__":unittest.main()
