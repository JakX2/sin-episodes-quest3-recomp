#!/usr/bin/env python3
"""Phase212 mutation regression uses SYNTHETIC PE32 bytes, never game code."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_snapshot_record_router as native
from audit_sin_original_client_snapshot_record_router import (
    ENTRY,SIZE,SHA,WITNESSES,CALL_EDGES,CASE_TABLE_VA,SELECTOR_VA,
    FOLLOWING_VA,WARNING_STRING_VA,WARNING_IAT_VA,WARNING,
    CASE_DESTINATIONS,TYPE_CASES,rel_call,verify_native,audit,
    VerificationError
)

class SyntheticPE:
    def __init__(self):
        self.base=0x24000000
        self.memory={}
        for off,hx in WITNESSES.items():
            self.write(ENTRY+off,bytes.fromhex(hx))
        for site,dest in CALL_EDGES:
            self.write(site,rel_call(site,dest))
        self.write(CASE_TABLE_VA,struct.pack("<13I",*CASE_DESTINATIONS))
        self.write(SELECTOR_VA,bytes(TYPE_CASES))
        self.write(SELECTOR_VA+27,b"\xcc"*5)
        self.write(WARNING_STRING_VA,WARNING.encode()+b"\0")
    def write(self,va,raw):
        for i,byte in enumerate(raw):
            where=va+i
            if where in self.memory and self.memory[where]!=byte:
                raise AssertionError("synthetic opcode fixture conflict at %#x"%where)
            self.memory[where]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        return ".rdata" if WARNING_STRING_VA<=va<WARNING_STRING_VA+len(WARNING)+1 else ".text"
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Tests(unittest.TestCase):
    @staticmethod
    def fake():return SyntheticPE()
    @staticmethod
    def check(fake,*,imports=False,tables=True):
        return verify_native(fake,check_hash=False,check_imports=imports,check_tables=tables)
    def test_original_free_fixture_can_never_verify_native_method(self):
        f=self.fake()
        r=self.check(f)
        self.assertEqual(r["new_sha_verified_original_functions"],0)
        self.assertEqual(r["new_sha_verified_native_bytes"],0)
        self.assertEqual(r["original_type_count"],27)
        self.assertEqual(r["original_case_count"],13)
        self.assertEqual(r["original_opaque_direct_calls"],39)
        self.assertEqual(r["original_opcode_witnesses"],len(WITNESSES))
        self.assertEqual(r["warning_type_values"],[20,22,23,24,25])
        with self.assertRaises(VerificationError):
            verify_native(f,check_hash=True,check_imports=False)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_mutate_each_sparse_native_instruction_witness(self):
        for offset in WITNESSES:
            with self.subTest(offset=hex(offset)):
                pe=self.fake();pe.flip(ENTRY+offset)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_all_thirty_nine_original_native_calls(self):
        for site,dest in CALL_EDGES:
            with self.subTest(site=hex(site),dest=hex(dest)):
                pe=self.fake();pe.flip(site+1)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_each_original_case_table_entry(self):
        for i in range(13):
            with self.subTest(case=i):
                pe=self.fake();pe.flip(CASE_TABLE_VA+i*4)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_each_original_type_selector(self):
        for i in range(27):
            with self.subTest(type=i):
                pe=self.fake();pe.flip(SELECTOR_VA+i)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_reject_invalid_native_table_padding(self):
        for byte in range(5):
            pe=self.fake();pe.flip(SELECTOR_VA+27+byte)
            with self.assertRaises(VerificationError):self.check(pe)
    def test_reject_wrong_original_warning_string(self):
        for i in (0,3,len(WARNING)):
            pe=self.fake();pe.flip(WARNING_STRING_VA+i)
            with self.assertRaises(VerificationError):self.check(pe)
    def test_warning_import_name_and_original_iat_are_exact(self):
        pe=self.fake()
        with patch.object(native,"named_iat",return_value=[WARNING_IAT_VA]):
            self.assertEqual(self.check(pe,imports=True)["warning_import"],
                             "tier0.dll!Warning")
        with patch.object(native,"named_iat",return_value=[WARNING_IAT_VA+4]):
            with self.assertRaises(VerificationError):self.check(pe,imports=True)
    def test_original_code_and_rdata_sections_required(self):
        pe=self.fake();pe.section_of=lambda _va:".data"
        with self.assertRaises(VerificationError):self.check(pe)
        pe=self.fake();pe.base=0x25000000
        with self.assertRaises(VerificationError):self.check(pe)
    def test_opcode_witnesses_original_filter_before_switch(self):
        self.assertEqual(WITNESSES[0x06f],"83380a")
        self.assertEqual(WITNESSES[0x074],"f6c602")
        self.assertEqual(WITNESSES[0x080],"83f901")
        self.assertEqual(WITNESSES[0x085],"84f1")
        self.assertEqual(WITNESSES[0x08d],"83f902")
        self.assertEqual(WITNESSES[0x092],"f6c601")
    def test_correct_original_table_layout_and_boundaries(self):
        self.assertEqual(ENTRY+SIZE,CASE_TABLE_VA)
        self.assertEqual(CASE_TABLE_VA+52,SELECTOR_VA)
        self.assertEqual(SELECTOR_VA+27+5,FOLLOWING_VA)
        self.assertEqual(TYPE_CASES[10],10)
        self.assertEqual(TYPE_CASES[13],11)
        self.assertEqual(CASE_DESTINATIONS[12],0x24199ac7)

if __name__=="__main__":unittest.main()
