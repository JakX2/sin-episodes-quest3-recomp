"""Original-DLL-free x86 tampering tests for Phase 80's ConVars and presets."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_challenge_advisor_presets import (
    OPCODES,DIRECT_CALLS,STRINGS,REGION_HASHES,PRESET_BITS,
    PRESET_BASE,PRESET_STRIDE,verify_challenge_advisors,audit,VerificationError
)
class FakePE:
    def __init__(self): self.memory={}
    def put(self,address,raw):
        for i,byte in enumerate(raw):
            key=address+i
            if key in self.memory and self.memory[key]!=byte:
                raise AssertionError('overlap at %#x' % key)
            self.memory[key]=byte
    def at(self,address,count):
        return bytes(self.memory.get(address+i,0xcc) for i in range(count))
    def corrupt(self,address): self.memory[address]^=0x20

def fixture():
    pe=FakePE()
    for va,raw in OPCODES.items():pe.put(va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,raw in STRINGS.items():pe.put(va,raw)
    for row,bits in enumerate(PRESET_BITS):
        pe.put(PRESET_BASE+row*PRESET_STRIDE,struct.pack('<5I',*bits))
    pe.put(0x223d6314,struct.pack('<f',100.0))
    return pe

class ChallengePresetEvidenceTests(unittest.TestCase):
    def test_schema(self):
        report=verify_challenge_advisors(fixture(),False)
        self.assertEqual(report['advisor_count'],6)
        self.assertEqual(report['advisor_preset_stride_bytes'],20)
        self.assertEqual(report['challenge_cvar']['name'],'dd_challenge')
        self.assertEqual(report['challenge_cvar']['default_text'],'0.2f')
        self.assertEqual(report['forgiveness_cvar']['name'],'dd_forgiveness')
        self.assertEqual(report['original_opcode_site_count'],len(OPCODES))
        self.assertEqual(report['original_callsite_count'],len(DIRECT_CALLS))
    def test_every_opcode_mutation(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_challenge_advisors(p,False)
    def test_every_call_edge_mutation(self):
        for va in DIRECT_CALLS:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_challenge_advisors(p,False)
    def test_every_original_cvar_string_mutation(self):
        for va in STRINGS:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_challenge_advisors(p,False)
    def test_all_30_binary_preset_values(self):
        for row in range(6):
            for field in range(5):
                with self.subTest(row=row,field=field):
                    p=fixture();p.corrupt(PRESET_BASE+row*PRESET_STRIDE+field*4)
                    with self.assertRaises(VerificationError):
                        verify_challenge_advisors(p,False)
    def test_100_constant_mutation(self):
        p=fixture();p.corrupt(0x223d6314)
        with self.assertRaises(VerificationError):
            verify_challenge_advisors(p,False)
    def test_real_code_regions_not_fakeable_in_fixture(self):
        self.assertEqual(len(REGION_HASHES),8)
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            verify_challenge_advisors(fixture(),True)
    def test_owner_original_dll_hashes_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'not original client',b'not original server')

if __name__=='__main__':unittest.main()
