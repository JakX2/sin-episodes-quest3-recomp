"""Phase182 original-free native witness and 2006 surface file ABI tests."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_surface_file as mod

class FakePE:
    base=0x24000000
    def __init__(self):self.d={}
    def put(self,va,raw):
        for offset,b in enumerate(raw):
            where=va+offset
            if where in self.d and self.d[where]!=b:
                raise AssertionError('conflicting witness at %#x'%where)
            self.d[where]=b
    def at(self,va,n):return bytes(self.d.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.d[va]=self.d.get(va,0xcc)^0x01
    def section_of(self,va):
        if any(a<=va<a+len(v) for a,v in mod.STRINGS.items()):return '.rdata'
        return '.text'

def fake():
    f=FakePE()
    for va,h in {**mod.OPCODES,**mod.VTABLE_CALLS}.items():f.put(va,bytes.fromhex(h))
    for va,target in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}.items():
        f.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,s in mod.STRINGS.items():f.put(va,s)
    f.put(mod.VALUE_TAG_TABLE_VA,struct.pack('<5I',*mod.VALUE_TAG_TARGETS))
    return f

class NativeSurfaceFileTests(unittest.TestCase):
    def test_native_witnesses(self):
        report=mod.verify_native(fake(),check_hash=False)
        self.assertEqual(report['sparse_opcode_witnesses'],48)
        self.assertEqual(report['original_vtable_dispatch_witnesses'],8)
        self.assertEqual(report['internal_rel32_calls'],5)
        self.assertEqual(report['incoming_rel32_calls'],2)
    def test_regions_full_hash_metadata(self):
        self.assertEqual(len(mod.REGIONS),5)
        self.assertEqual(sum(size for va,size,sha in mod.REGIONS.values()),454)
        self.assertEqual(len(set(va for va,_,sha in mod.REGIONS.values())),5)
        self.assertTrue(all(len(bytes.fromhex(digest))==32 for _,_,digest in mod.REGIONS.values()))
        self.assertEqual(hashlib.sha256(struct.pack('<5I',*mod.VALUE_TAG_TARGETS)).hexdigest(),mod.VALUE_TAG_TABLE_SHA)
    def test_each_opcode_mutated(self):
        for va in mod.OPCODES:
            with self.subTest(va=hex(va)):
                p=fake();p.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'opcode/vtable mismatch'):
                    mod.verify_native(p,check_hash=False)
    def test_each_native_vtable_slot_call_mutated(self):
        for va in mod.VTABLE_CALLS:
            with self.subTest(va=hex(va)):
                p=fake();p.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'opcode/vtable mismatch'):
                    mod.verify_native(p,check_hash=False)
    def test_each_original_rel32_edge_mutated(self):
        for va in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}:
            with self.subTest(va=hex(va)):
                p=fake();p.corrupt(va)
                with self.assertRaises(mod.VerificationError):
                    mod.verify_native(p,check_hash=False)
    def test_each_original_string_mutated(self):
        for va in mod.STRINGS:
            with self.subTest(va=hex(va)):
                p=fake();p.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'original string mismatch'):
                    mod.verify_native(p,check_hash=False)
    def test_each_native_value_tag_slot_mutated(self):
        for i in range(5):
            with self.subTest(type_tag=i+1):
                p=fake();p.corrupt(mod.VALUE_TAG_TABLE_VA+i*4)
                with self.assertRaisesRegex(mod.VerificationError,'jump table mismatch'):
                    mod.verify_native(p,check_hash=False)
    def test_region_hash_rejects_synthetic_fixture(self):
        with self.assertRaisesRegex(mod.VerificationError,'function SHA mismatch'):
            mod.verify_native(fake(),check_hash=True)
    def test_full_file_sha_gate_rejects_fakes(self):
        with self.assertRaisesRegex(mod.VerificationError,'full client/server DLL SHA256 mismatch'):
            mod.audit(b'fakeclient',b'fakeserver')
    def test_original_base_and_file_mode_and_utf8(self):
        p=fake();p.base=0
        with self.assertRaisesRegex(mod.VerificationError,'client base mismatch'):
            mod.verify_native(p,check_hash=False)
        self.assertEqual(mod.STRINGS[0x24289C0C],b'rb\0')
        self.assertEqual(mod.STRINGS[0x242850E8],b'%f\0')
        self.assertEqual(mod.STRINGS[0x242D49F0],b'%d\0')
        self.assertEqual(mod.OPCODES[0x24166873],'68e9fd0000') # UTF-8 cp65001
        self.assertEqual(mod.VALUE_TAG_TARGETS[1],mod.VALUE_TAG_TARGETS[3])
        self.assertNotEqual(mod.VALUE_TAG_TARGETS[0],mod.VALUE_TAG_TARGETS[4])
    def test_guarded_vs_unconditional_bridge_semantics(self):
        self.assertEqual(mod.VTABLE_CALLS[0x2419206B],'ff5604')
        self.assertEqual(mod.VTABLE_CALLS[0x2419208B],'ff5608')
        self.assertEqual(mod.VTABLE_CALLS[0x241920A2],'ff500c')
        self.assertEqual(mod.OPCODES[0x24192056],'7417')
        self.assertEqual(mod.OPCODES[0x24192076],'7417')

if __name__=='__main__':unittest.main()