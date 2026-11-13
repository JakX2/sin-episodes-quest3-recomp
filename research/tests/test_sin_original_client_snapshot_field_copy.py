#!/usr/bin/env python3
"""Phase213 synthetic-only native PE32 mutation tests; no retail game data."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_snapshot_field_copy import (
    METHODS,relative_call,verify_native,audit,VerificationError,
    CASE_DESTINATIONS,TYPE_CASES,CASE_TABLE_VA,SELECTOR_VA,
)

class SyntheticClientPE:
    def __init__(self):
        self.base=0x24000000
        self.memory={}
        for _name,(va,length,_sha,padding,_site,_type,witnesses) in METHODS.items():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+length,b"\xcc"*padding)
        for _name,(va,_len,_sha,_padding,site,_type,_witnesses) in METHODS.items():
            self.write(site,relative_call(site,va))
        self.write(CASE_TABLE_VA,struct.pack("<13I",*CASE_DESTINATIONS))
        self.write(SELECTOR_VA,bytes(TYPE_CASES))
    def write(self,addr,data):
        for i,b in enumerate(data):
            if addr+i in self.memory and self.memory[addr+i]!=b:
                raise AssertionError("synthetic fixture conflicting byte")
            self.memory[addr+i]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0x90) for i in range(n))
    def section_of(self,_addr):
        return ".text"
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0x90)^1

class SyntheticTests(unittest.TestCase):
    @staticmethod
    def fake():return SyntheticClientPE()
    @staticmethod
    def check(pe,*,switch=True):
        return verify_native(pe,check_hash=False,check_switch=switch)

    def test_fake_original_is_never_counted(self):
        report=self.check(self.fake())
        self.assertEqual(report["new_sha_verified_original_client_functions"],0)
        self.assertEqual(report["new_complete_original_native_bytes"],0)
        self.assertEqual(report["sparse_opcode_witnesses"],35)
        self.assertEqual(report["exact_original_type_case_call_edges"],4)
        self.assertEqual(report["verified_original_duplicate_dword_function_pairs"],1)
        self.assertTrue(report["original_switch_cross_check"])
    def test_mutate_every_native_opcode_witness(self):
        for name,(va,_,_,_,_,_,witnesses) in METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,off=hex(off)):
                    pe=self.fake();pe.flip(va+off)
                    with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_all_four_original_type_specialist_call_edges(self):
        for name,(_va,_length,_sha,_padding,site,_type,_witnesses) in METHODS.items():
            with self.subTest(method=name,site=hex(site)):
                pe=self.fake();pe.flip(site+1)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_all_original_ret_alignment_boundaries(self):
        for name,(va,length,_sha,_padding,_site,_type,_witnesses) in METHODS.items():
            with self.subTest(name=name):
                pe=self.fake();pe.flip(va+length)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_original_duplicate_dword_native_body_identity(self):
        left=METHODS["type5_dword_copy"][0]
        right=METHODS["type1_dword_copy"][0]
        pe=self.fake()
        self.assertEqual(pe.at(left,34),pe.at(right,34))
        pe.flip(left+0x11)
        with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_each_case_jump_table_destination(self):
        for index in range(len(CASE_DESTINATIONS)):
            with self.subTest(case=index):
                pe=self.fake();pe.flip(CASE_TABLE_VA+index*4)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_each_phase212_type_case_selector(self):
        for index in range(len(TYPE_CASES)):
            with self.subTest(type=index):
                pe=self.fake();pe.flip(SELECTOR_VA+index)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_original_full_hash_checks_reject_synthetic_code(self):
        pe=self.fake()
        with self.assertRaises(VerificationError):
            verify_native(pe,check_hash=True,check_switch=False)
        with self.assertRaises(VerificationError):
            audit(b"MZ",b"MZ")
    def test_bad_image_base_and_original_code_section_fail(self):
        pe=self.fake();pe.base=0x23000000
        with self.assertRaises(VerificationError):self.check(pe)
        pe=self.fake();pe.section_of=lambda _va:".rdata"
        with self.assertRaises(VerificationError):self.check(pe)
    def test_type1_and_type5_native_entry_distinct_but_byte_identical(self):
        a,b=METHODS["type1_dword_copy"],METHODS["type5_dword_copy"]
        self.assertNotEqual(a[0],b[0])
        self.assertEqual(a[1],34)
        self.assertEqual(a[2],b[2])
        self.assertEqual(a[6],b[6])
        self.assertEqual(a[5],1)
        self.assertEqual(b[5],5)
    def test_string_copies_nul_inclusive_and_cleans_three_arguments(self):
        c=METHODS["type2_null_terminated_copy"]
        self.assertEqual(c[1],61)
        self.assertEqual(c[6][0x25],"2bc2")
        self.assertEqual(c[6][0x27],"8d4801")
        self.assertEqual(c[6][0x3a],"c20c00")
        self.assertEqual(METHODS["type6_byte_copy"][6][0x27],"f3a4")
        self.assertEqual(METHODS["type5_dword_copy"][6][0x1b],"f3a5")

if __name__=="__main__":unittest.main()
