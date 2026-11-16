#!/usr/bin/env python3
"""Phase249 synthetic-only original-PE mutation regressions.

Synthetic fixtures are never authentic original code and cannot earn native
method inventory credit, even when sparse opcode witnesses happen to match.
"""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_intrusive_list_vtable as original

class SyntheticPE:
    def __init__(self):
        self.base=original.ORIGINALS["client"]["base"]
        self.memory={}
        self.sections={}
        for name,va,size,digest,padding,exits,anchors in original.METHODS:
            self.write(va,bytes(size))
            for i in range(size):
                self.sections[va+i]=".text"
            self.write(va+size,b"\xCC"*padding)
            for off,hx in exits:
                self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in original.VTABLE_EDGES:
            self.write(site,target.to_bytes(4,"little"))
            for i in range(4):
                self.sections[site+i]=".rdata"
    def write(self,va,content):
        for i,b in enumerate(content):
            self.memory[va+i]=b
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return self.sections.get(va,"")
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase249SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return original.verify_native(
            pe,check_hash=False,authentic_provenance=False,
            check_relocations=False)

    def test_no_authentic_method_credit_from_synthetic_fixture(self):
        result=self.check(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(len(result["reviewed_original_methods"]),9)
        self.assertEqual(result["original_sparse_opcode_witnesses"],29)
        self.assertEqual(result["original_native_exit_paths"],12)
        self.assertEqual(result["original_INT3_alignment_bytes"],89)
        self.assertEqual(result["original_vtable_pointers_verified"],6)
        self.assertEqual(result["original_HIGHLOW_vtable_relocations_verified"],0)
        with self.assertRaises(original.VerificationError):
            original.verify_native(
                SyntheticPE(),check_hash=True,check_relocations=False)
        with self.assertRaises(original.VerificationError):
            original.audit(b"MZ",b"MZ")

    def test_every_sparse_owner_original_opcode_witness_mutation(self):
        for name,va,size,sha,padding,exits,anchors in original.METHODS:
            for off in anchors:
                with self.subTest(method=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.mutate(va+off)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_each_original_return_byte_and_all_trailing_INT3(self):
        for name,va,size,sha,padding,exits,anchors in original.METHODS:
            for off,hx in exits:
                for i in range(len(bytes.fromhex(hx))):
                    with self.subTest(method=name,exit=hex(off),byte=i):
                        pe=SyntheticPE()
                        pe.mutate(va+off+i)
                        with self.assertRaises(original.VerificationError):
                            self.check(pe)
            for i in range(padding):
                with self.subTest(method=name,INT3=i):
                    pe=SyntheticPE()
                    pe.mutate(va+size+i)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_all_original_vtable_entries_and_pointer_bytes(self):
        for site,target in original.VTABLE_EDGES:
            for i in range(4):
                with self.subTest(entry=hex(site),byte=i):
                    pe=SyntheticPE()
                    pe.mutate(site+i)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)
            pe=SyntheticPE()
            pe.sections[site]=".text"
            with self.assertRaises(original.VerificationError):
                self.check(pe)

    def test_original_image_base_and_text_boundaries(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(original.VerificationError):
            self.check(pe)
        for name,va,size,*rest in original.METHODS:
            pe=SyntheticPE()
            pe.sections[va+size-1]=""
            with self.assertRaises(original.VerificationError):
                self.check(pe)

    def test_original_method_inventory_exact_and_no_direct_E8(self):
        self.assertEqual([m[1] for m in original.METHODS],[
            0x24127C80,0x24127C90,0x24127CA0,
            0x24127CC0,0x24127CD0,0x24127CE0,
            0x24127CF0,0x24127D40,0x24127DB0])
        self.assertEqual([m[2] for m in original.METHODS],
                         [7,16,17,3,3,3,67,106,105])
        self.assertEqual(sum(m[2] for m in original.METHODS),327)
        self.assertEqual([m[4] for m in original.METHODS],
                         [9,0,15,13,13,13,13,6,7])
        self.assertEqual(original.VTABLE_EDGES[:4],(
            (0x24298EB4,0x24127CC0),
            (0x24298EB8,0x24127CD0),
            (0x242973B8,0x241089D0),
            (0x242973BC,0x24108A70)))
        self.assertEqual(original.METHODS[6][3],
            "74eccc13557d320aae9d1ea67f0f030c3180423ae617a9582ee385c3402cbe44")

if __name__=="__main__":
    unittest.main()
