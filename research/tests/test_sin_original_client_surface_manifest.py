"""Original-free Phase181 SHA-witness mutation and manifest reference tests."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_surface_manifest as mod

class FakePE:
    base=0x24000000
    def __init__(self): self.memory={}
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,data):
        for offset,value in enumerate(data):
            where=va+offset
            if where in self.memory and self.memory[where]!=value:
                raise AssertionError('conflicting synthetic native instruction at %#x'%where)
            self.memory[where]=value
    def corrupt(self,va):self.memory[va]=self.memory.get(va,0xcc)^1
    def section_of(self,va):
        return '.rdata' if any(s<=va<s+len(v) for s,v in mod.STRINGS.items()) else '.text'

def fake_native():
    pe=FakePE()
    for va,hexcode in mod.OPCODES.items():pe.put(va,bytes.fromhex(hexcode))
    for va,target in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in mod.TAIL_JMPS.items():
        pe.put(va,b'\xe9'+struct.pack('<i',target-va-5))
    for va,s in mod.STRINGS.items():pe.put(va,s)
    pe.put(0x242FD18C,struct.pack('<I',0x242A2C5C))
    return pe

class OriginalManifestTests(unittest.TestCase):
    def test_all_sparse_original_witnesses_pass(self):
        result=mod.verify_native(fake_native(),check_hash=False)
        self.assertEqual(result['sparse_native_opcode_witnesses'],56)
        self.assertEqual(result['direct_original_call_edges'],11)
        self.assertEqual(result['original_incoming_call_edges'],1)
        self.assertEqual(result['native_tail_jump_edges'],2)
    def test_all_function_digest_lengths_and_distinctness(self):
        self.assertEqual(len(mod.REGIONS),7)
        self.assertEqual(sum(n for _,n,_ in mod.REGIONS.values()),355)
        self.assertEqual(len({va for va,n,sha in mod.REGIONS.values()}),7)
        self.assertTrue(all(len(bytes.fromhex(s))==32 for va,n,s in mod.REGIONS.values()))
    def test_corrupt_each_original_opcode(self):
        for va in mod.OPCODES:
            with self.subTest(va=hex(va)):
                f=fake_native();f.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'opcode mismatch'):
                    mod.verify_native(f,check_hash=False)
    def test_corrupt_each_direct_incoming_call(self):
        for va in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}:
            with self.subTest(va=hex(va)):
                f=fake_native();f.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'CALL relationship'):
                    mod.verify_native(f,check_hash=False)
    def test_corrupt_every_cleanup_tail_jump(self):
        for va in mod.TAIL_JMPS:
            with self.subTest(va=hex(va)):
                f=fake_native();f.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'tail JMP relationship'):
                    mod.verify_native(f,check_hash=False)
    def test_every_original_manifest_string_is_pinned(self):
        for va in mod.STRINGS:
            with self.subTest(va=hex(va)):
                f=fake_native();f.corrupt(va)
                with self.assertRaisesRegex(mod.VerificationError,'manifest string'):
                    mod.verify_native(f,check_hash=False)
    def test_manifest_data_pointer_must_match(self):
        f=fake_native();f.corrupt(0x242FD18C)
        with self.assertRaisesRegex(mod.VerificationError,'filename pointer'):
            mod.verify_native(f,check_hash=False)
    def test_whole_function_hash_blocks_synthetic_fixture(self):
        with self.assertRaisesRegex(mod.VerificationError,'body digest'):
            mod.verify_native(fake_native(),check_hash=True)
    def test_real_dll_hash_gate_rejects_synthetic_binaries(self):
        with self.assertRaisesRegex(mod.VerificationError,'SHA256 mismatch'):
            mod.audit(b'malformed-client',b'malformed-server')
    def test_bad_image_base_rejected(self):
        f=fake_native();f.base=0
        with self.assertRaisesRegex(mod.VerificationError,'image base'):
            mod.verify_native(f,check_hash=False)
    def test_original_case_insensitive_type_and_filename(self):
        self.assertEqual(mod.STRINGS[0x242A2C5C],b'scripts/surfaceproperties_manifest.txt\0')
        self.assertEqual(mod.STRINGS[0x24293B58],b'GAME\0')
        self.assertEqual(mod.STRINGS[0x24293D00],b'file\0')
        for value in (b'file',b'File',b'FILE'):
            self.assertEqual(value.lower(),mod.STRINGS[0x24293D00][:-1])
        self.assertNotEqual(b'material'.lower(),mod.STRINGS[0x24293D00][:-1])

if __name__=='__main__':unittest.main()