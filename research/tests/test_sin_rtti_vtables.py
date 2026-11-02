"""Synthetic PE32 RTTI/callshape test; no original commercial binary in CI."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_rtti_vtables import scan,VerificationError
from test_gameplay_entity_registry import fake_registered_server

BASE=0x22000000

def make_rtti(virtual_methods=2):
    b=bytearray(fake_registered_server())
    type_va=BASE+0x4d7000
    chd=BASE+0x3d4000
    pointers=BASE+0x3d4020
    bcd=BASE+0x3d4040
    locator=BASE+0x3d4100
    vftable=BASE+0x3d4200
    def put(va,data):
        i=va-BASE
        b[i:i+len(data)]=data
    put(type_va,struct.pack("<II",0x224b0660,0)+b".?AVCNPC_Synthetic@@\x00")
    put(chd,struct.pack("<4I",0,0,1,pointers))
    put(pointers,struct.pack("<I",bcd))
    put(bcd,struct.pack("<7I",type_va,1,0,0xffffffff,0,0,chd))
    put(locator,struct.pack("<5I",0,0,0,type_va,chd))
    put(vftable,struct.pack("<I",locator)+
        b"".join(struct.pack("<I",BASE+0x1000+i*16) for i in range(virtual_methods)))
    return bytes(b),locator,vftable

class RTTITests(unittest.TestCase):
    def test_one_real_msvc_constructor_relationship(self):
        blob,col,vf=make_rtti()
        report=scan(blob,"server",check_hash=False)
        self.assertEqual(report["counts"]["type_descriptors"],1)
        self.assertEqual(report["counts"]["qualified_complete_object_locators"],1)
        self.assertEqual(report["counts"]["distinct_vftables"],1)
        self.assertEqual(report["counts"]["total_vftable_slots"],2)
        self.assertEqual(report["counts"]["distinct_executable_vftable_targets"],2)
        self.assertEqual(report["hierarchies"]["CNPC_Synthetic"][0]["class"],"CNPC_Synthetic")
        self.assertEqual(report["vtables"][0]["vfptr_va"],f"0x{vf+4:08X}")
        self.assertEqual(report["vtables"][0]["this_displacement"],0)
        self.assertTrue(report["vtables"][0]["primary"])

    def test_no_vftable_when_locator_pointer_corrupted(self):
        blob,col,vf=make_rtti()
        b=bytearray(blob)
        struct.pack_into("<I",b,vf-BASE,0)
        report=scan(bytes(b),"server",check_hash=False)
        self.assertEqual(report["counts"]["qualified_complete_object_locators"],1)
        self.assertEqual(report["counts"]["distinct_vftables"],0)

    def test_invalid_hierarchy_reference_rejected(self):
        blob,col,vf=make_rtti()
        b=bytearray(blob)
        struct.pack_into("<I",b,col-BASE+16,BASE+0x1000)
        report=scan(bytes(b),"server",check_hash=False)
        self.assertEqual(report["counts"]["qualified_complete_object_locators"],0)

    def test_nonexecutable_function_pointer_rejected(self):
        blob,col,vf=make_rtti(1)
        b=bytearray(blob)
        struct.pack_into("<I",b,vf-BASE+4,BASE+0x4d7000)
        self.assertFalse(scan(bytes(b),"server",check_hash=False)["vtables"])

    def test_exact_original_hash_is_required(self):
        blob,_,_=make_rtti()
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            scan(blob,"server")

    def test_wrong_dll_kind_and_image_base_refused(self):
        blob,_,_=make_rtti()
        with self.assertRaisesRegex(VerificationError,"image base"):
            scan(blob,"client",check_hash=False)
        with self.assertRaisesRegex(VerificationError,"expected client or server"):
            scan(blob,"nonexistent",check_hash=False)

if __name__=="__main__":unittest.main()
