"""Original-free mutation regressions for SiN CShortTermStatGate decomp.

CI synthetic x86 fixtures contain no commercial executable bytes.
"""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_short_term_stat_gates import (
    TYPE_INFO, VFTABLES, OPCODES, CALLS, REGIONS,
    verify_original_short_term_gates, VerificationError, audit,
)

class FakePE:
    def __init__(self):
        self.mem={}
    def put(self, at, payload):
        for i,byte in enumerate(payload):
            va=at+i
            if va in self.mem and self.mem[va]!=byte:
                raise AssertionError("fixture conflicting bytes at %#x" % va)
            self.mem[va]=byte
    def at(self, address, count):
        return bytes(self.mem.get(address+i,0xCC) for i in range(count))
    def corrupt(self, at):
        self.mem[at]^=0x10

def fixture():
    p=FakePE()
    for name,(col_cell,col,type_va,name_va) in TYPE_INFO.items():
        p.put(col_cell,struct.pack("<I",col))
        p.put(col+12,struct.pack("<I",type_va))
        p.put(name_va,(".?AV"+name+"@@\0").encode())
    for _,(start,entries) in VFTABLES.items():
        for slot,target in enumerate(entries):
            p.put(start+slot*4,struct.pack("<I",target))
    for va,raw in OPCODES.items():
        p.put(va,bytes.fromhex(raw))
    for va,target in CALLS.items():
        p.put(va,b"\xE8"+struct.pack("<i",target-va-5))
    return p

class OriginalStatGateEvidenceTests(unittest.TestCase):
    def test_synthetic_positive(self):
        meta=verify_original_short_term_gates(fixture(),False)
        self.assertEqual(len(meta["original_classes"]),2)
        self.assertEqual(len(meta["vtable_slots"]["CShortTermStatGate"]),7)
        self.assertEqual(len(meta["vtable_slots"]["CShortTermStatGateConjugate"]),7)
        self.assertEqual(meta["verified_opcode_anchors"],len(OPCODES))
        self.assertEqual(meta["verified_original_calls"],len(CALLS))

    def test_all_original_opcode_witness_mutations_rejected(self):
        for va in OPCODES:
            with self.subTest(site=hex(va)):
                p=fixture()
                p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_short_term_gates(p,False)

    def test_all_vtable_targets_rejected_when_mutated(self):
        for _,(va,entries) in VFTABLES.items():
            for slot in range(len(entries)):
                with self.subTest(va=hex(va),slot=slot):
                    p=fixture()
                    p.corrupt(va+slot*4)
                    with self.assertRaises(VerificationError):
                        verify_original_short_term_gates(p,False)

    def test_type_descriptor_and_rtti_locator_mutations_rejected(self):
        for _,(col_cell,col,_,name_va) in TYPE_INFO.items():
            for va in (col_cell,col+12,name_va):
                with self.subTest(site=hex(va)):
                    p=fixture()
                    p.corrupt(va)
                    with self.assertRaises(VerificationError):
                        verify_original_short_term_gates(p,False)

    def test_constructor_call_edges_rejected_when_mutated(self):
        for va in CALLS:
            with self.subTest(site=hex(va)):
                p=fixture()
                p.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_short_term_gates(p,False)

    def test_complete_method_hashes_and_original_dlls_required(self):
        self.assertEqual(len(REGIONS),6)
        for _,(va,length,sha) in REGIONS.items():
            self.assertGreater(length,0)
            self.assertEqual(len(bytes.fromhex(sha)),32)
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            verify_original_short_term_gates(fixture(),True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"fake client",b"fake server")

if __name__=="__main__":
    unittest.main()
