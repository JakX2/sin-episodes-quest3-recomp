#!/usr/bin/env python3
"""Phase247 synthetic original-PE mutation tests; never authentic-method credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_base_removal as original

class SyntheticPE:
    def __init__(self):
        self.base=original.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,exits,padding,anchors in original.METHODS:
            self.write(va,bytes(size))
            self.text.update(range(va,va+size))
            self.write(va+size,b"\xCC"*padding)
            for offset,hx in exits:
                self.write(va+offset,bytes.fromhex(hx))
            for offset,hx in anchors.items():
                self.write(va+offset,bytes.fromhex(hx))
        for opcode,edges in ((0xE8,original.CALLS),
                             (0xE9,original.TAIL_JMPS)):
            for site,target in edges:
                self.write(site,original.relative(opcode,site,target))
                self.text.update(range(site,site+5))
        va,size,digest=original.CARRY_FORWARD_PHASE246
        self.write(va,bytes(size))
        self.text.update(range(va,va+size))
        self.write(va+size,b"\xCC"*9)
        self.write(va+size-2,b"\x5E\xC3")
        self.write(0x24105FF0,
                   original.relative(0xE8,0x24105FF0,0x2416F940))
    def write(self,va,content):
        for off,b in enumerate(content):
            self.memory[va+off]=b
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase247SyntheticTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return original.verify_native(
            pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_fixture_is_never_original_code_credit(self):
        result=self.check(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(result["sparse_original_opcode_witnesses"],24)
        self.assertEqual(result["original_exact_E8_CALL_edges"],3)
        self.assertEqual(result["original_exact_E9_tail_JMP_edges"],2)
        self.assertEqual(len(result["reviewed_original_methods"]),4)
        with self.assertRaises(original.VerificationError):
            original.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(original.VerificationError):
            original.audit(b"MZ",b"MZ")

    def test_every_sparse_original_x86_witness_required(self):
        for name,va,size,sha,exits,pad,anchors in original.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.mutate(va+off)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_all_original_exits_and_trailing_int3(self):
        for name,va,size,sha,exits,pad,anchors in original.METHODS:
            for off,hx in exits:
                for byte in range(len(bytes.fromhex(hx))):
                    with self.subTest(name=name,exit=hex(off),byte=byte):
                        pe=SyntheticPE()
                        pe.mutate(va+off+byte)
                        with self.assertRaises(original.VerificationError):
                            self.check(pe)
            for byte in range(pad):
                with self.subTest(name=name,padding=byte):
                    pe=SyntheticPE()
                    pe.mutate(va+size+byte)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_all_original_E8_calls_and_E9_tail_jumps(self):
        for opcode,edges in ((0xE8,original.CALLS),
                             (0xE9,original.TAIL_JMPS)):
            for site,dest in edges:
                for byte in range(5):
                    with self.subTest(site=hex(site),byte=byte):
                        pe=SyntheticPE()
                        pe.mutate(site+byte)
                        with self.assertRaises(original.VerificationError):
                            self.check(pe)

    def test_predecessor_original_phase246_boundaries_still_required(self):
        va,size,digest=original.CARRY_FORWARD_PHASE246
        for site in (va+size-2,va+size-1,va+size,va+size+8,0x24105FF0):
            pe=SyntheticPE()
            pe.mutate(site)
            with self.assertRaises(original.VerificationError):
                self.check(pe)

    def test_image_base_and_entire_text_method_bounds(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(original.VerificationError):
            self.check(pe)
        for name,va,size,*unused in original.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va+size-1)
            with self.assertRaises(original.VerificationError):
                self.check(pe)

    def test_count_and_native_abi_contract(self):
        self.assertEqual([m[1] for m in original.METHODS],[
            0x24127E20,0x24127FC0,0x24128070,0x24128180])
        self.assertEqual([m[2] for m in original.METHODS],[91,176,48,18])
        self.assertEqual(sum(m[2] for m in original.METHODS),333)
        self.assertEqual(original.TAIL_JMPS,(
            (0x2412818D,0x24127FC0),
            (0x241074C7,0x24128070)))
        self.assertEqual(original.CARRY_FORWARD_PHASE246[0],0x24105FE0)

if __name__=="__main__":
    unittest.main()
