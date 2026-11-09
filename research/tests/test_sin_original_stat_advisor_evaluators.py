"""Original-free mutation regressions for Phase 148 CStatAdvisor evaluator evidence."""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_stat_advisor_evaluators import (
    OPCODES, CALLS, VTABLE, REGIONS, verify_original_evaluators,
    VerificationError, audit,
)

class FakePE:
    def __init__(self):
        self.memory = {}
    def put(self, address, raw):
        for index, byte in enumerate(raw):
            at = address + index
            if at in self.memory and self.memory[at] != byte:
                raise AssertionError("synthetic witness overlaps at %#x" % at)
            self.memory[at] = byte
    def at(self, address, count):
        return bytes(self.memory.get(address + i, 0xCC) for i in range(count))
    def corrupt(self, at):
        self.memory[at] ^= 0x10

def fixtures():
    pe = FakePE()
    for va, raw in OPCODES.items():
        pe.put(va, bytes.fromhex(raw))
    for va, target in CALLS.items():
        pe.put(va, b"\xe8" + struct.pack("<i", target - va - 5))
    for va, target in VTABLE.items():
        pe.put(va, struct.pack("<I", target))
    pe.put(0x2245CBD0, struct.pack("<I", 0x224CA970))
    pe.put(0x22521924, b".?AVCStatAdvisor@@\0")
    return pe

# Independent x87 FCOMP condition-code emulator (AH bits C0, C2, C3).
# Tests compare the decompiled C++ boolean conditions to x87's flags.
def status(a, b):
    if math.isnan(a) or math.isnan(b): return 0x45
    if a < b: return 0x01
    if a == b: return 0x40
    return 0

def parity_even(v):
    return v.bit_count() % 2 == 0

def slot9_machine(score, c1, c2, c3):
    first = status(c2, c1)
    second = status(score, c3)
    if parity_even(first & 0x05):
        return 3 if second & 0x41 else 4
    return 4 if not parity_even(second & 0x05) else 3

def slot10_machine(score, c0, c1, c2):
    first = status(c1, c2)
    second = status(score, c0)
    if first & 0x41:
        return 1 if parity_even(second & 0x05) else 0
    return 1 if second & 0x41 else 0

class AdvisorEvaluatorTests(unittest.TestCase):
    def test_original_witnesses_accept_complete_synthetic_fixture(self):
        report = verify_original_evaluators(fixtures(), False)
        self.assertEqual(report["original_class"], "CStatAdvisor")
        self.assertEqual(report["verified_vtable_pointers"], 3)
        self.assertEqual(report["verified_instruction_sites"], len(OPCODES))
        self.assertEqual(report["verified_direct_calls"], len(CALLS))
        self.assertEqual(len(REGIONS), 5)

    def test_all_opcode_and_direct_call_tampering_is_rejected(self):
        for site in OPCODES:
            with self.subTest(opcode=hex(site)):
                pe = fixtures()
                pe.corrupt(site)
                with self.assertRaises(VerificationError):
                    verify_original_evaluators(pe, False)
        for site in CALLS:
            with self.subTest(call=hex(site)):
                pe = fixtures()
                pe.corrupt(site + 1)
                with self.assertRaises(VerificationError):
                    verify_original_evaluators(pe, False)

    def test_each_vtable_and_rtti_pointer_is_guarded(self):
        for site in list(VTABLE) + [0x2245CBD0, 0x22521924]:
            with self.subTest(pointer=hex(site)):
                pe = fixtures()
                pe.corrupt(site)
                with self.assertRaises(VerificationError):
                    verify_original_evaluators(pe, False)

    def test_exact_original_function_hashes_are_required(self):
        with self.assertRaisesRegex(VerificationError, "SHA-256"):
            verify_original_evaluators(fixtures(), True)
        for _, (_, size, digest) in REGIONS.items():
            self.assertGreater(size, 0)
            self.assertEqual(len(bytes.fromhex(digest)), 32)

    def test_full_audit_rejects_non_original_dlls(self):
        with self.assertRaises((VerificationError, ValueError)):
            audit(b"fake client", b"fake server")

    def test_x87_branch_equivalence_finite_infinite_nan_equal(self):
        samples = [-math.inf, -2.0, -1.0, -0.0, 0.0,
                   1.0, 2.0, math.inf, float("nan")]
        for score, c0, c1, c2 in itertools.product(samples, repeat=4):
            expected9 = ((4 if score < c2 else 3) if c1 < c0
                         else (4 if score > c2 else 3))
            expected10 = ((0 if score > c0 else 1) if c1 > c2
                          else (0 if score < c0 else 1))
            self.assertEqual(slot9_machine(score, c0, c1, c2), expected9)
            self.assertEqual(slot10_machine(score, c0, c1, c2), expected10)

if __name__ == "__main__":
    unittest.main()
