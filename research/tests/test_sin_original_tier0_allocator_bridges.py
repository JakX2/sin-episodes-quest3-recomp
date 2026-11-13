#!/usr/bin/env python3
"""Original-free mutation tests for Phase208 native tier0 bridge audit."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_tier0_allocator_bridges as module
from audit_sin_original_tier0_allocator_bridges import (
    REGIONS, BASE, IAT, DATA, METHODS, ALIAS_NAMES, PREVIOUSLY_VERIFIED,
    call32, verify_native, audit, VerificationError
)

class SyntheticPE:
    def __init__(self,role):
        self.role=role
        self.base=BASE[role]
        self.memory={}
        n=0 if role=="server" else 1
        for name,(entries,size,_,fields,_,padding,witnesses) in REGIONS.items():
            va=entries[n]
            raw=bytearray(b"\x90"*size)
            for off,hexcode in witnesses.items():
                data=bytes.fromhex(hexcode)
                raw[off:off+len(data)]=data
            for off,key in fields:
                ptr=IAT[role].get(key,DATA[role].get(key))
                struct.pack_into("<I",raw,off,ptr)
            if name in ALIAS_NAMES:
                site=va+0x13
                raw[0x13:0x18]=call32(site,METHODS["context"][0][n])
            self.put(va,raw)
            self.put(va+size,b"\xcc"*padding)
        # Earlier Phase202/205 original free must have exact native equality.
        old=METHODS["free"][0][n]
        src=REGIONS["duplicate_free"][0][n]
        self.put(old,self.at(src,18))
    def put(self,va,code):
        for i,b in enumerate(code):
            if va+i in self.memory and self.memory[va+i]!=b:
                raise AssertionError("synthetic overlapping fixture evidence")
            self.memory[va+i]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if 0x22500000<=va<0x22600000 or 0x24300000<=va<0x24400000:
            return ".data"
        if 0x223d0000<=va<0x223e0000 or 0x24270000<=va<0x24280000:
            return ".rdata"
        return ".text"
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Tests(unittest.TestCase):
    @staticmethod
    def fixture():return SyntheticPE("server"),SyntheticPE("client")
    @staticmethod
    def verify(a,b):
        return verify_native(a,b,check_hash=False,check_imports=False)
    def test_fake_dlls_cannot_claim_original_hashes(self):
        a,b=self.fixture()
        report=self.verify(a,b)
        self.assertEqual(report["new_sha_verified_original_functions"],0)
        self.assertEqual(report["new_original_x86_bytes"],0)
        self.assertEqual(report["complete_original_functions_SHA_checked"],0)
        self.assertEqual(report["complete_original_server_client_pairs"],7)
        self.assertEqual(report["sparse_opcode_witnesses"],74)
        self.assertEqual(report["exact_absolute_globals_or_IAT_operands"],34)
        self.assertEqual(report["original_direct_alias_context_calls"],4)
        self.assertEqual(report["native_free_wrapper_identity_checks"],2)
        self.assertEqual(len(PREVIOUSLY_VERIFIED),4)
    def test_every_sparse_original_opcode_witness_mutation(self):
        for j,role in enumerate(("server","client")):
            for name,row in REGIONS.items():
                for off in row[6]:
                    with self.subTest(role=role,name=name,off=off):
                        a,b=self.fixture()
                        (a,b)[j].flip(row[0][j]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_every_original_absolute_operand_mutation(self):
        for j,role in enumerate(("server","client")):
            for name,row in REGIONS.items():
                for off,key in row[3]:
                    with self.subTest(role=role,name=name,off=off,key=key):
                        a,b=self.fixture()
                        (a,b)[j].flip(row[0][j]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_each_direct_context_helper_call_mutation(self):
        for j in (0,1):
            for name in ALIAS_NAMES:
                with self.subTest(role=j,name=name):
                    a,b=self.fixture()
                    (a,b)[j].flip(REGIONS[name][0][j]+0x14)
                    with self.assertRaises(VerificationError):self.verify(a,b)
    def test_all_original_method_ret_and_int3_boundaries(self):
        for j in (0,1):
            for name,row in REGIONS.items():
                with self.subTest(role=j,name=name):
                    a,b=self.fixture()
                    (a,b)[j].flip(row[0][j]+row[1])
                    with self.assertRaises(VerificationError):self.verify(a,b)
    def test_five_unwitnessed_original_machine_code_differences(self):
        # Fields below are outside absolute relocation operands and do not
        # overlap sparse witnesses. Whole-twin comparison must still reject.
        for name,off in (("slot00_three",0xf),("slot10_three",0xf),
                         ("slot08_four",0x12),("slot00_skip_middle",0xd),
                         ("cached_alias_a",0x30)):
            with self.subTest(name=name):
                a,b=self.fixture()
                b.flip(REGIONS[name][0][1]+off)
                with self.assertRaises(VerificationError):self.verify(a,b)
    def test_prior_phase202_free_must_match_byte_for_byte(self):
        for j,role in enumerate(("server","client")):
            with self.subTest(role=role):
                a,b=self.fixture()
                (a,b)[j].flip(METHODS["free"][0][j]+7)
                with self.assertRaises(VerificationError):self.verify(a,b)
    def test_wrong_pe_image_base_or_module_static_section(self):
        a,b=self.fixture()
        a.base=0x23000000
        with self.assertRaises(VerificationError):self.verify(a,b)
        a,b=self.fixture()
        b.section_of=lambda _:".text"
        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_wrong_original_tier0_import_library_and_slot(self):
        for role in ("server","client"):
            a,b=self.fixture()
            with patch.object(module,"imports_with_iat",
                 return_value={"g_pMemAlloc":("wrong.dll",IAT[role]["g_pMemAlloc"])}):
                with self.assertRaises(VerificationError):
                    verify_native(a,b,check_hash=False,check_imports=True)
    def test_full_original_bodies_and_dll_hashes_not_fabricated(self):
        a,b=self.fixture()
        with self.assertRaises(VerificationError):
            verify_native(a,b,check_hash=True,check_imports=False)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_stack_order_and_native_vtable_opcode_separation(self):
        self.assertEqual(REGIONS["slot00_three"][6][0x18],"ff12")
        self.assertEqual(REGIONS["slot10_three"][6][0x18],"ff5210")
        self.assertEqual(REGIONS["slot08_four"][6][0x1d],"ff5208")
        self.assertEqual(REGIONS["slot00_skip_middle"][6][7],"8b442410")
        self.assertEqual(REGIONS["slot00_skip_middle"][6][0x13],"8b44240c")
        self.assertEqual(REGIONS["duplicate_free"][6][0xe],"ff5214c3")

if __name__=="__main__":
    unittest.main()
