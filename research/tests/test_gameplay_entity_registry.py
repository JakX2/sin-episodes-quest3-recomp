"""Synthetic PE mutation regression and optional user-owned original validation."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from verify_gameplay_entity_registry import (FACTORIES,PEImage,VerificationError,
                                             REGISTRY_GETTER,verify_registrations)

ORIGINAL = pathlib.Path('/mnt/data/sin_phase73_work/original/server.dll')


def fake_registered_server() -> bytes:
    # Minimal, independently authored PE32 with the 2006 layout of the three
    # relevant sections. No original executable bytes are included in tests.
    data=bytearray(0x53e000)
    data[:2]=b'MZ';struct.pack_into('<I',data,0x3c,0x80)
    data[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',data,0x84,0x14c,3,0,0,0,0xe0,0x210e)
    opt=0x98;struct.pack_into('<H',data,opt,0x10b)
    struct.pack_into('<I',data,opt+28,0x22000000)
    for i,(name,addr,size) in enumerate((('.text',0x1000,0x3d1000),
                                         ('.rdata',0x3d2000,0x104000),
                                         ('.data',0x4d6000,0x68000))):
        s=opt+0xe0+i*40
        data[s:s+8]=name.encode().ljust(8,b'\0')
        struct.pack_into('<IIII',data,s+8,size,addr,size,addr)
    for name,category,register_va,name_va,factory_va in FACTORIES:
        off=register_va-0x22000000
        rel=REGISTRY_GETTER-(register_va+5)
        code=(b'\xe8'+struct.pack('<i',rel)+b'\x8b\x10\x68'+
              struct.pack('<I',name_va)+b'\x68'+struct.pack('<I',factory_va)+
              b'\x8b\xc8\xff\x12\xc3')
        assert len(code)==22
        data[off:off+22]=code
        data[name_va-0x22000000:name_va-0x22000000+len(name)+1]=name.encode()+b'\0'
    return bytes(data)


class RegistryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.fixture=fake_registered_server()

    def test_registry_has_selected_factories_only(self):
        self.assertEqual(len(FACTORIES),32)
        self.assertEqual(len({x[0] for x in FACTORIES}),32)
        self.assertIn('trigger_dynamic_difficulty',{x[0] for x in FACTORIES})
        self.assertIn('logic_auto',{x[0] for x in FACTORIES})

    def test_non_pe_rejected(self):
        with self.assertRaises(VerificationError):verify_registrations(b'MZ'+bytes(128),check_hash=False)

    def test_all_32_synthetic_factories(self):
        rows=verify_registrations(self.fixture,check_hash=False)
        self.assertEqual(len(rows),32)
        self.assertEqual(rows[0]['classname'],'logic_auto')

    def test_mutated_name_rejected(self):
        b=bytearray(self.fixture);pe=PEImage(b);name=FACTORIES[0]
        b[pe.offset(name[3],1)]^=1
        with self.assertRaisesRegex(VerificationError,'classname'):verify_registrations(bytes(b),check_hash=False)

    def test_mutated_factory_pointer_rejected(self):
        b=bytearray(self.fixture);pe=PEImage(b);name=FACTORIES[14]
        b[pe.offset(name[2]+13,1)]^=1
        with self.assertRaisesRegex(VerificationError,'target/name/factory'):verify_registrations(bytes(b),check_hash=False)

    def test_mutated_getter_rejected(self):
        b=bytearray(self.fixture);pe=PEImage(b);name=FACTORIES[26]
        b[pe.offset(name[2]+1,1)]^=1
        with self.assertRaisesRegex(VerificationError,'target/name/factory'):verify_registrations(bytes(b),check_hash=False)

    def test_mutated_registration_pattern_rejected(self):
        b=bytearray(self.fixture);pe=PEImage(b);name=FACTORIES[5]
        b[pe.offset(name[2]+19,1)]^=1
        with self.assertRaisesRegex(VerificationError,'call shape'):verify_registrations(bytes(b),check_hash=False)

    def test_hash_verification_rejects_synthetic(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):verify_registrations(self.fixture)

    def test_original_pinned_server_if_local(self):
        if not ORIGINAL.exists():self.skipTest('proprietary DLL not present in CI')
        rows=verify_registrations(ORIGINAL.read_bytes())
        self.assertEqual(len(rows),32)

if __name__=='__main__':unittest.main()
