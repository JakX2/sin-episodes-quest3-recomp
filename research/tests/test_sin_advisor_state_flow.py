"""Mutation-based x86 fixtures for original 2006 SiN advisor state evidence."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_advisor_state_flow import (
    OPCODES,DIRECT_CALLS,DIRECT_JUMPS,VTABLE_ENTRIES,REGION_HASHES,
    verify_advisor_state_flow,VerificationError,audit)

class FakePE:
    def __init__(self):self.memory={}
    def put(self,va,raw):
        for i,byte in enumerate(raw):
            loc=va+i
            if loc in self.memory and self.memory[loc]!=byte:
                raise AssertionError('original-free fixture overlap at %#x'%loc)
            self.memory[loc]=byte
    def at(self,va,count):
        return bytes(self.memory.get(va+i,0xcc) for i in range(count))
    def corrupt(self,va):self.memory[va]^=0x10

def fixtures():
    pe=FakePE()
    for va,raw in OPCODES.items():pe.put(va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in DIRECT_JUMPS.items():
        pe.put(va,b'\xe9'+struct.pack('<i',target-va-5))
    for va,target in VTABLE_ENTRIES.items():
        pe.put(va,struct.pack('<I',target))
    pe.put(0x223d9778,struct.pack('<f',0.5))
    pe.put(0x22521924,b'.?AVCStatAdvisor@@\x00')
    return pe

class AdvisorFlowTests(unittest.TestCase):
    def test_original_metadata_contract(self):
        report=verify_advisor_state_flow(fixtures(),False)
        self.assertEqual(report['rtti_class'],'CStatAdvisor')
        self.assertEqual(report['opcode_sites'],len(OPCODES))
        self.assertEqual(report['call_edges'],len(DIRECT_CALLS))
        self.assertEqual(report['vtable_rtti_edges'],len(VTABLE_ENTRIES))
        self.assertEqual(report['four_float_member_offsets'],['0x24','0x28','0x2C','0x30'])
        self.assertEqual(report['score_member_offset'],'0x1C')
    def test_every_original_opcode_tamper_rejected(self):
        for va in OPCODES:
            with self.subTest(site=hex(va)):
                p=fixtures();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_advisor_state_flow(p,False)
    def test_every_direct_call_tamper_rejected(self):
        for va in DIRECT_CALLS:
            with self.subTest(site=hex(va)):
                p=fixtures();p.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_advisor_state_flow(p,False)
    def test_fanout_relative_jump_tamper_rejected(self):
        for va in DIRECT_JUMPS:
            p=fixtures();p.corrupt(va+1)
            with self.assertRaises(VerificationError):
                verify_advisor_state_flow(p,False)
    def test_vtable_and_rtti_pointers_tamper_rejected(self):
        for va in VTABLE_ENTRIES:
            with self.subTest(site=hex(va)):
                p=fixtures();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_advisor_state_flow(p,False)
    def test_score_half_and_class_name_tamper_rejected(self):
        for va in (0x223d9778,0x22521924):
            p=fixtures();p.corrupt(va)
            with self.assertRaises(VerificationError):
                verify_advisor_state_flow(p,False)
    def test_original_region_hash_pins(self):
        self.assertEqual(len(REGION_HASHES),8)
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            verify_advisor_state_flow(fixtures(),True)
    def test_only_exact_original_dual_dll_fingerprints(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
