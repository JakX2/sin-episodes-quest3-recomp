"""Phase187 owner-original-free mutation tests for server startup gates."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_server_post_emitter as audit

class FakePE:
    base=0x22000000
    def __init__(self):self.contents={}
    def at(self,va,n):return bytes(self.contents.get(va+i,0xcc) for i in range(n))
    def put(self,va,data):
        for i,b in enumerate(data):
            old=self.contents.get(va+i)
            if old is not None and old!=b:raise AssertionError('overlapping fixture evidence at %#x'%(va+i))
            self.contents[va+i]=b
    def corrupt(self,va):self.contents[va]=self.contents.get(va,0xcc)^1
    def section(self,va):
        if va in audit.GLOBALS.values():return '.data'
        if any(start<=va<start+len(s) for start,s in audit.STRINGS.items()):return '.rdata'
        return '.text'

def fixture():
    pe=FakePE()
    for va,code in audit.OPCODES.items():pe.put(va,bytes.fromhex(code))
    for va,target in audit.CALLS.items():pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in audit.JMPS.items():pe.put(va,b'\xe9'+struct.pack('<i',target-va-5))
    for va,target in audit.FAILURE_BRANCHES.items():pe.put(va,b'\x74'+struct.pack('<b',target-(va+2)))
    for va,value in audit.STRINGS.items():pe.put(va,value)
    return pe

class PostEmitterTests(unittest.TestCase):
    def test_fixture_verifies_all_evidence(self):
        o=audit.verify_native(fixture(),check_hash=False)
        self.assertEqual(o['native_opcode_witnesses'],37)
        self.assertEqual(o['native_rel32_CALL_edges'],9)
        self.assertEqual(o['native_rel32_singleton_JMP_edges'],2)
        self.assertEqual(o['native_three_shared_Init_false_branches'],3)
        self.assertEqual(o['server_interface_strings'],5)
    def test_five_original_full_hashes_and_total_355(self):
        self.assertEqual(len(audit.REGIONS),5)
        self.assertEqual(sum(length for _,length,_ in audit.REGIONS.values()),355)
        self.assertEqual(len(set(va for va,_,_ in audit.REGIONS.values())),5)
        for _,_,digest in audit.REGIONS.values():self.assertEqual(len(bytes.fromhex(digest)),32)
    def test_every_sparse_instruction_rejects_corruption(self):
        for va in audit.OPCODES:
            with self.subTest(opcode=hex(va)):
                f=fixture();f.corrupt(va)
                with self.assertRaisesRegex(audit.VerificationError,'opcode mismatch'):
                    audit.verify_native(f,check_hash=False)
    def test_every_native_call_rejects_corruption(self):
        for va in audit.CALLS:
            with self.subTest(call=hex(va)):
                f=fixture();f.corrupt(va)
                with self.assertRaisesRegex(audit.VerificationError,'CALL edge mismatch'):
                    audit.verify_native(f,check_hash=False)
    def test_both_trampolines_reject_wrong_jmp(self):
        for va in audit.JMPS:
            with self.subTest(jmp=hex(va)):
                f=fixture();f.corrupt(va)
                with self.assertRaisesRegex(audit.VerificationError,'singleton JMP edge mismatch'):
                    audit.verify_native(f,check_hash=False)
    def test_every_fatal_gate_branch_rejects_wrong_displacement(self):
        for va in audit.FAILURE_BRANCHES:
            with self.subTest(branch=hex(va)):
                f=fixture();f.corrupt(va+1)
                with self.assertRaisesRegex(audit.VerificationError,'shared Init failure edge mismatch'):
                    audit.verify_native(f,check_hash=False)
    def test_every_original_string_rejects_mutation(self):
        for va in audit.STRINGS:
            with self.subTest(string=hex(va)):
                f=fixture();f.corrupt(va)
                with self.assertRaisesRegex(audit.VerificationError,'resource string mismatch'):
                    audit.verify_native(f,check_hash=False)
    def test_original_global_must_live_in_data(self):
        f=fixture();f.section=lambda va:'.text'
        with self.assertRaisesRegex(audit.VerificationError,'global not in data'):
            audit.verify_native(f,check_hash=False)
    def test_original_x86_image_base_must_match(self):
        f=fixture();f.base=0
        with self.assertRaisesRegex(audit.VerificationError,'image base mismatch'):
            audit.verify_native(f,check_hash=False)
    def test_synthetic_fixture_cannot_satisfy_full_native_hash(self):
        with self.assertRaisesRegex(audit.VerificationError,'entire original server method hash mismatch'):
            audit.verify_native(fixture(),check_hash=True)
    def test_original_full_dll_pair_sha_gate_rejects_fake(self):
        with self.assertRaisesRegex(audit.VerificationError,'original full server/client DLL SHA256 mismatch'):
            audit.audit(b'not-server',b'not-client')
    def test_exact_original_version_strings_and_singleton_destinations(self):
        self.assertEqual(audit.STRINGS[0x22441F34],b'developer\0')
        self.assertEqual(audit.STRINGS[0x224428F0],b'resource/gameevents.res\0')
        self.assertEqual(audit.STRINGS[0x224428CC],b'resource/ModEvents.res\0')
        self.assertEqual(audit.JMPS,{0x221AD8A5:0x221AD3E0,0x22149B65:0x221498C0})
        self.assertEqual(set(audit.FAILURE_BRANCHES.values()),{0x221E79A1})

if __name__=='__main__':unittest.main()