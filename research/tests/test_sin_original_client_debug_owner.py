"""Phase186 synthetic regression suite; does not require redistributed DLLs."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_debug_owner as mod

class FakePE:
    base=0x24000000
    def __init__(self):self.memory={};self.import_map=dict(mod.IMPORTS)
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,blob):
        for i,v in enumerate(blob):
            if va+i in self.memory and self.memory[va+i]!=v:
                raise AssertionError(f'overlapping contradictory synthetic witness at {va+i:#x}')
            self.memory[va+i]=v
    def corrupt(self,va):self.memory[va]=self.memory.get(va,0xcc)^1
    def imports(self):return self.import_map

def fixture():
    f=FakePE()
    for va,fragment in mod.OPCODES.items():f.put(va,bytes.fromhex(fragment))
    for src,dest in mod.CALLS.items():f.put(src,b'\xe8'+struct.pack('<i',dest-src-5))
    return f

class OriginalDebugOwnerTests(unittest.TestCase):
    def test_unmodified_sparse_evidence(self):
        out=mod.verify_native(fixture(),check_hash=False)
        self.assertEqual(out['original_sparse_opcode_witnesses'],48)
        self.assertEqual(out['original_rel32_call_edges'],5)
        self.assertEqual(out['original_imported_api_sites'],5)
    def test_four_complete_function_sizes(self):
        self.assertEqual([x[1] for x in mod.REGIONS.values()],[90,71,74,79])
        self.assertEqual(sum(row[1] for row in mod.REGIONS.values()),314)
        self.assertTrue(all(len(bytes.fromhex(row[2]))==32 for row in mod.REGIONS.values()))
    def test_every_sparse_original_instruction_is_guarded(self):
        for va in mod.OPCODES:
            with self.subTest(va=hex(va)):
                f=fixture();f.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'instruction witness mismatch'):
                    mod.verify_native(f,check_hash=False)
    def test_every_direct_caller_displacement_is_guarded(self):
        for src in mod.CALLS:
            with self.subTest(src=hex(src)):
                f=fixture();f.corrupt(src)
                with self.assertRaisesRegex(mod.VerificationError,'resolver CALL mismatch'):
                    mod.verify_native(f,check_hash=False)
    def test_original_api_import_names_and_addresses_all_guarded(self):
        for key in mod.IMPORTS:
            with self.subTest(key=key):
                f=fixture();f.import_map[key]+=4
                with self.assertRaisesRegex(mod.VerificationError,'imported API IAT mismatch'):
                    mod.verify_native(f,check_hash=False)
    def test_missing_original_import_rejected(self):
        f=fixture();f.import_map.pop(('kernel32.dll','VirtualQuery'))
        with self.assertRaisesRegex(mod.VerificationError,'imported API IAT mismatch'):
            mod.verify_native(f,check_hash=False)
    def test_full_original_hash_rejects_synthetic_fixture(self):
        with self.assertRaisesRegex(mod.VerificationError,'full function hash mismatch'):
            mod.verify_native(fixture(),check_hash=True)
    def test_wrong_binary_file_hash_rejected(self):
        with self.assertRaisesRegex(mod.VerificationError,'full DLL file SHA256 mismatch'):
            mod.audit(b'client not original',b'server not original')
    def test_wrong_image_base_rejected(self):
        f=fixture();f.base=0x22000000
        with self.assertRaisesRegex(mod.VerificationError,'image base'):
            mod.verify_native(f,check_hash=False)
    def test_separate_original_cache_addresses(self):
        self.assertEqual(mod.OPCODES[0x2416F855],'84050c293724')
        self.assertEqual(mod.OPCODES[0x2416F8A5],'840514293724')
        self.assertEqual(mod.OPCODES[0x2416F8F5],'84050c293724')
        self.assertEqual(mod.OPCODES[0x2416F8B8],'a310293724')
        self.assertEqual(mod.OPCODES[0x2416F908],'a308293724')
    def test_module_allocation_base_and_api_order(self):
        self.assertEqual(mod.OPCODES[0x2416F821],'6804293724')
        self.assertEqual(mod.OPCODES[0x2416F82C],'8b4c2408')
        expected=['GetProcessHeap','HeapAlloc','VirtualQuery','GetModuleFileNameA']
        imports=mod.IMPORTS
        actual=sorted([k[1] for k in imports if k[0]=='kernel32.dll'],key=lambda x:imports[('kernel32.dll',x)])
        self.assertEqual(actual,expected)

if __name__=='__main__':unittest.main()