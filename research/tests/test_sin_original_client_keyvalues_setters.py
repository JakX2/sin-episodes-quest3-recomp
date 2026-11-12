"""Phase183: original-free native x86 witness and typed setter regression tests."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_keyvalues_setters as mod

class FakePE:
    base=0x24000000
    def __init__(self):self.mem={}
    def at(self,va,n):return bytes(self.mem.get(va+i,0xcc) for i in range(n))
    def put(self,va,raw):
        for i,b in enumerate(raw):
            if va+i in self.mem and self.mem[va+i]!=b:
                raise AssertionError(f'conflicting fixture byte {va+i:#x}')
            self.mem[va+i]=b
    def corrupt(self,va):self.mem[va]=self.mem.get(va,0xcc)^1
    def section_of(self,va):
        if any(pos<=va<pos+len(s) for pos,s in mod.DATA_STRINGS.items()):return '.rdata'
        return '.text'

def fixture():
    pe=FakePE()
    for va,hexb in mod.OPCODES.items():pe.put(va,bytes.fromhex(hexb))
    for va,target in {**mod.CALLS,**mod.INCOMING}.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,s in mod.DATA_STRINGS.items():pe.put(va,s)
    return pe

class OriginalKeyValuesTests(unittest.TestCase):
    def test_standalone_sparse_native_fixture(self):
        result=mod.verify_native(fixture(),check_hash=False)
        self.assertEqual(result['sparse_instruction_witnesses'],54)
        self.assertEqual(result['direct_CALL_witnesses'],16)
        self.assertEqual(result['incoming_CALL_witnesses'],4)
    def test_original_whole_body_metadata(self):
        self.assertEqual(len(mod.REGIONS),6)
        self.assertEqual(sum(n for va,n,sha in mod.REGIONS.values()),609)
        self.assertEqual(len({va for va,n,sha in mod.REGIONS.values()}),6)
        self.assertTrue(all(len(bytes.fromhex(sha))==32 for va,n,sha in mod.REGIONS.values()))
    def test_every_opcode_corruption_detected(self):
        for va in mod.OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'instruction witness mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_every_native_direct_call_corruption(self):
        for va in mod.CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'rel32 CALL edge mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_all_incoming_native_call_corruption(self):
        for va in mod.INCOMING:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'rel32 CALL edge mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_all_original_default_literals_pinned(self):
        for va in mod.DATA_STRINGS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'empty value string mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_synthetic_pe_fails_entire_function_hash(self):
        with self.assertRaisesRegex(mod.VerificationError,'complete original x86 function SHA mismatch'):
            mod.verify_native(fixture(),check_hash=True)
    def test_wrong_original_image_base_rejected(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(mod.VerificationError,'image base'):
            mod.verify_native(pe,check_hash=False)
    def test_wrong_pair_hash_is_rejected(self):
        with self.assertRaisesRegex(mod.VerificationError,'complete original client/server DLL SHA256 mismatch'):
            mod.audit(b'invalid client',b'invalid server')
    def test_wrapper_types_and_layout(self):
        addrs=[0x24165BE0,0x24165C10,0x24165C40]
        for addr,tag in zip(addrs,(2,3,4)):
            self.assertEqual(bytes.fromhex(mod.OPCODES[addr+0x17]), b'\xc7\x40\x10'[:3])
            self.assertEqual(mod.INCOMING[0x241667CF],0x241656E0)
        self.assertEqual([mod.REGIONS[n][1] for n in (
            'keyvalues_set_type2_integer','keyvalues_set_type3_float',
            'keyvalues_set_type4_pointer_bits')],[33,33,33])
    def test_native_calls_include_both_recursive_find_key_sites(self):
        self.assertEqual(mod.CALLS[0x24165777],0x241656E0)
        self.assertEqual(mod.CALLS[0x241657D2],0x241656E0)
        self.assertEqual(mod.INCOMING[0x2416681A],0x24165AE0)

if __name__=='__main__':unittest.main()