"""Synthetic registry registration tests. No original SiN DLL bytes in CI."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))

from audit_sin_original_rifle_grenade_factories import (
    ORIGINAL_FACTORIES, verify_factory_pair
)
from verify_gameplay_entity_registry import REGISTRY_GETTER,PEImage,VerificationError

def fake_pe():
    # Synthetic PE32 sections model only the addresses/code SHAPES required
    # by two source-proven original factory registrations.
    b=bytearray(0x53e000)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",b,0x84,0x14c,3,0,0,0,0xe0,0x210e)
    opt=0x98
    struct.pack_into("<H",b,opt,0x10b)
    struct.pack_into("<I",b,opt+28,0x22000000)
    for i,(name,addr,size) in enumerate((
        (".text",0x1000,0x3d1000),
        (".rdata",0x3d2000,0x104000),
        (".data",0x4d6000,0x68000))):
        at=opt+0xe0+40*i
        b[at:at+8]=name.encode().ljust(8,b"\0")
        struct.pack_into("<IIII",b,at+8,size,addr,size,addr)
    for name,register,name_ptr,factory_ptr in ORIGINAL_FACTORIES:
        raw=(b"\xe8"+struct.pack("<i",REGISTRY_GETTER-register-5)+
             b"\x8b\x10\x68"+struct.pack("<I",name_ptr)+
             b"\x68"+struct.pack("<I",factory_ptr)+b"\x8b\xc8\xff\x12\xc3")
        assert len(raw)==22
        b[register-0x22000000:register-0x22000000+22]=raw
        b[name_ptr-0x22000000:name_ptr-0x22000000+len(name)+1]=(
            name.encode()+b"\0")
    return b

class OriginalRifleGrenadeRegistryTests(unittest.TestCase):
    def test_two_exact_original_names(self):
        output=verify_factory_pair(bytes(fake_pe()),check_hash=False)
        self.assertEqual(len(output["verified_original_registry_entries"]),2)
        self.assertEqual([e["classname"] for e in
                          output["verified_original_registry_entries"]],
                         ["weapon_assault_rifle","grenade_ar"])
        self.assertFalse(output["verified_original_registry_entries"][0][
            "native_sourcevr_engine_registration_proven"])

    def test_original_binary_hash_cannot_be_bypassed_by_fixture(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            verify_factory_pair(bytes(fake_pe()))

    def test_rifle_registration_getter_changed(self):
        b=fake_pe();b[ORIGINAL_FACTORIES[0][1]-0x22000000+1]^=0x01
        with self.assertRaisesRegex(VerificationError,"operands"):
            verify_factory_pair(bytes(b),check_hash=False)

    def test_grenade_registration_factory_object_changed(self):
        b=fake_pe();b[ORIGINAL_FACTORIES[1][1]-0x22000000+13]^=0x01
        with self.assertRaisesRegex(VerificationError,"operands"):
            verify_factory_pair(bytes(b),check_hash=False)

    def test_grenade_registration_literal_changed(self):
        b=fake_pe();b[ORIGINAL_FACTORIES[1][2]-0x22000000]^=0x01
        with self.assertRaisesRegex(VerificationError,"classname changed"):
            verify_factory_pair(bytes(b),check_hash=False)

    def test_rifle_registration_call_shape_changed(self):
        b=fake_pe();b[ORIGINAL_FACTORIES[0][1]-0x22000000+19]^=0x01
        with self.assertRaisesRegex(VerificationError,"call shape"):
            verify_factory_pair(bytes(b),check_hash=False)

    def test_original_factory_object_not_rdata(self):
        b=fake_pe();ptr=ORIGINAL_FACTORIES[1][3]
        b[ORIGINAL_FACTORIES[1][1]-0x22000000+13:
          ORIGINAL_FACTORIES[1][1]-0x22000000+17]=struct.pack("<I",ptr+0x1000000)
        with self.assertRaises(VerificationError):
            verify_factory_pair(bytes(b),check_hash=False)

    def test_bad_pe_header_rejected(self):
        with self.assertRaises(VerificationError):
            verify_factory_pair(b"MZ"+bytes(80),check_hash=False)

if __name__=="__main__":
    unittest.main()
