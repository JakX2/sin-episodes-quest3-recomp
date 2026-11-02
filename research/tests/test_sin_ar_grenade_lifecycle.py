"""Original-free 2006 PE32 weapon/projectile byte-evidence regression tests."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_ar_grenade_lifecycle import (
    assert_original_evidence, decode_rel_call, check_bytes, read_named_exports,
    WITNESSES, CALL_EDGES, NUMERIC_FIELDS, OBJECT_FIELD_INIT,
    CAST_SOURCE, CAST_TARGET, GRENADE_TYPE_DESCRIPTOR, RTTI_DECORATED_NAME,
    MODEL_GLOBAL, MODEL_STRING, MODEL_NAME, VerificationError,
)
from test_gameplay_entity_registry import fake_registered_server
from verify_gameplay_entity_registry import PEImage

BASE=0x22000000


def make_fake_evidence():
    data=bytearray(fake_registered_server())
    def write(va, raw):
        data[va-BASE:va-BASE+len(raw)]=raw
    for va,raw in WITNESSES.items():write(va,raw)
    for va,target in CALL_EDGES.items():
        write(va,b"\xe8"+struct.pack("<i",target-va-5))
    for desc,name in (CAST_SOURCE,CAST_TARGET,
                      (GRENADE_TYPE_DESCRIPTOR,RTTI_DECORATED_NAME)):
        write(desc+8,name)
    write(MODEL_GLOBAL,struct.pack("<I",MODEL_STRING))
    write(MODEL_STRING,MODEL_NAME)
    for va,value in NUMERIC_FIELDS.items():
        write(va,struct.pack("<f",value))
    for va,raw in OBJECT_FIELD_INIT.items():
        write(va,raw)
    write(0x220017E0,b"\x30\xc0\xc3")
    write(0x22253C30,b"\xb0\x01\xc3")
    return bytes(data)


class GrenadeLifecycleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fake=make_fake_evidence()

    def test_original_x86_evidence_patterns_pass_in_fake(self):
        report=assert_original_evidence(self.fake,check_hash=False)
        self.assertEqual(report["code_witnesses"],len(WITNESSES))
        self.assertEqual(report["verified_direct_calls"],len(CALL_EDGES))

    def test_real_binary_sha_required(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            assert_original_evidence(self.fake)

    def test_corrupt_owner_type_rtti_fails(self):
        data=bytearray(self.fake)
        data[CAST_TARGET[0]+8-BASE]=ord("X")
        with self.assertRaisesRegex(VerificationError,"witness mismatch"):
            assert_original_evidence(bytes(data),check_hash=False)

    def test_corrupt_ammo_decrement_fails(self):
        data=bytearray(self.fake)
        data[0x22313F35-BASE+1]=0x02
        with self.assertRaisesRegex(VerificationError,"witness mismatch"):
            assert_original_evidence(bytes(data),check_hash=False)

    def test_corrupt_actual_projectile_model_fails(self):
        data=bytearray(self.fake)
        data[MODEL_STRING-BASE]=ord("X")
        with self.assertRaisesRegex(VerificationError,"witness mismatch"):
            assert_original_evidence(bytes(data),check_hash=False)

    def test_corrupt_projectile_spawn_state_fails(self):
        data=bytearray(self.fake)
        data[0x22314BFB-BASE+7]^=0x40
        with self.assertRaisesRegex(VerificationError,"witness mismatch"):
            assert_original_evidence(bytes(data),check_hash=False)

    def test_bad_x86_call_opcode_target(self):
        pe=PEImage(self.fake)
        site=0x22313E2A
        with self.assertRaisesRegex(VerificationError,"not an x86 CALL"):
            decode_rel_call(pe,site)
        code=b"\xe8"+struct.pack("<i",0x22146C40-site-5)
        self.assertEqual(site+5+struct.unpack_from("<i",code,1)[0],0x22146C40)

    def test_read_real_pe_style_named_exports(self):
        # A complete original-free PE32 EAT/Name/Ordinal fixture.
        data=bytearray(self.fake)
        def write(va,raw):data[va-BASE:va-BASE+len(raw)]=raw
        peoff=struct.unpack_from("<I",data,0x3C)[0]
        opt=peoff+24
        eat=BASE+0x3D7000
        name=BASE+0x3D7100
        ordinals=BASE+0x3D7200
        directory=BASE+0x3D7300
        strva=BASE+0x3D7400
        write(directory+16,struct.pack("<IIIIII",1,1,1,
                                        eat-BASE,name-BASE,ordinals-BASE))
        write(eat,struct.pack("<I",0x0030f4e0))
        write(name,struct.pack("<I",strva-BASE))
        write(ordinals,struct.pack("<H",0))
        write(strva,b"?Detonate@CGrenadeHomer@@UAEXXZ\x00")
        struct.pack_into("<II",data,opt+96,directory-BASE,0x100)
        named=read_named_exports(PEImage(bytes(data)))
        self.assertEqual(named["?Detonate@CGrenadeHomer@@UAEXXZ"],0x2230F4E0)


if __name__=="__main__":
    unittest.main()
