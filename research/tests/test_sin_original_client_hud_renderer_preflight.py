"""Original-free mutation tests for Phase157 native HUD renderer preflight.

No original SiN game bytes are included in the synthetic PE fixtures.
The independent x87 C0/C2/C3 flag model verifies the clamp branches,
including unordered NaN behavior.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_hud_renderer_preflight import (
    REGIONS, OPCODES, DIRECT_CALLS, JUMP_TABLE_VA, MODE_JUMP_TARGETS,
    verify_original_hud_render_preflight, VerificationError, audit,
)

class MockPE:
    base=0x24000000
    def __init__(self):self.memory={}
    def write(self,va,source):
        for i,byte in enumerate(source):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=byte:
                raise AssertionError("conflicting synthetic witness at %#x"%addr)
            self.memory[addr]=byte
    def at(self,va,count):
        return bytes(self.memory.get(va+i,0xCC) for i in range(count))
    def corrupt(self,va):
        self.memory[va]^=0x10

def fixture():
    pe=MockPE()
    for va,code in OPCODES.items():
        pe.write(va,bytes.fromhex(code))
    for va,target in DIRECT_CALLS.items():
        pe.write(va,b"\xE8"+struct.pack("<i",target-va-5))
    for mode,target in enumerate(MODE_JUMP_TARGETS):
        pe.write(JUMP_TABLE_VA+mode*4,struct.pack("<I",target))
    return pe

def original_status_ah(a,b):
    if math.isnan(a) or math.isnan(b):return 0x45
    if a<b:return 0x01
    if a==b:return 0x40
    return 0

def even_parity_of_test(ah,mask):
    return (ah&mask).bit_count()%2==0

def x87_native_clamp(value):
    first=original_status_ah(value,1.0)
    if (first&0x41)==0:
        return 1.0
    second=original_status_ah(value,0.0)
    if not even_parity_of_test(second,0x05):
        return 0.0
    return value

def readable_portable_clamp(value):
    if value>1.0:return 1.0
    if value<0.0:return 0.0
    return value

class OriginalHudRendererTests(unittest.TestCase):
    def test_all_original_metadata_and_finite_mode_targets(self):
        report=verify_original_hud_render_preflight(fixture(),check_hashes=False)
        self.assertEqual(report["original_entry"],"0x24154D50")
        self.assertEqual(report["verified_complete_original_method_hashes"],0)
        self.assertEqual(report["verified_original_call_edges"],len(DIRECT_CALLS))
        self.assertEqual(report["hud_paint_callers"],7)
        self.assertEqual(len(MODE_JUMP_TARGETS),4)
        self.assertEqual(len(REGIONS),3)

    def test_each_original_opcode_anchor_is_guarded(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture()
                pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_hud_render_preflight(pe,check_hashes=False)

    def test_each_real_native_direct_call_edge_is_guarded(self):
        for va in DIRECT_CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture()
                pe.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_hud_render_preflight(pe,check_hashes=False)

    def test_original_mode_jump_table_values_are_guarded(self):
        for mode in range(len(MODE_JUMP_TARGETS)):
            with self.subTest(mode=mode):
                pe=fixture()
                pe.corrupt(JUMP_TABLE_VA+4*mode)
                with self.assertRaises(VerificationError):
                    verify_original_hud_render_preflight(pe,check_hashes=False)

    def test_original_full_function_hashes_cannot_be_faked(self):
        for name,(va,length,sha) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(length,0)
                self.assertEqual(len(bytes.fromhex(sha)),32)
        with self.assertRaisesRegex(VerificationError,"complete code SHA256"):
            verify_original_hud_render_preflight(fixture(),check_hashes=True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"not the owner client.dll",b"not the owner server.dll")

    def test_original_x87_float_flag_clamp_vs_independent_readable_code(self):
        sample=[-math.inf,-65535.0,-2.0,-0.0,0.0,0.001,
                0.25,0.5,0.999999999,1.0,1.0000001,2.0,
                math.inf,float("nan")]
        for value in sample:
            with self.subTest(value=value):
                actual=x87_native_clamp(value)
                expected=readable_portable_clamp(value)
                if math.isnan(expected):
                    self.assertTrue(math.isnan(actual))
                else:
                    self.assertEqual(actual,expected)

    def test_wrong_executable_load_base_rejected(self):
        pe=fixture()
        pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_hud_render_preflight(pe,check_hashes=False)

if __name__=="__main__":
    unittest.main()
