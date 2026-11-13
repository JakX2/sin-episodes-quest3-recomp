#!/usr/bin/env python3
"""Phase205 original-free mutation regression for six cross-DLL native twins."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_cross_dll_tier0_allocator as native
from audit_sin_original_cross_dll_tier0_allocator import (
    BASE,IAT,DATA,METHODS,CONTEXT_CALLERS,
    verify_methods,audit,VerificationError,
)

class FakePE:
    def __init__(self,role):
        self.role=role;self.base=BASE[role];self.memory={}
        n=0 if role=="server" else 1
        for name,(entries,length,_,offsets,_,padding,witnesses) in METHODS.items():
            va=entries[n];code=bytearray(b"\x90"*length)
            for off,hx in witnesses.items():
                opcode=bytes.fromhex(hx)
                code[off:off+len(opcode)]=opcode
            for off,key in offsets:
                addr=IAT[role].get(key,DATA[role].get(key))
                struct.pack_into("<I",code,off,addr)
            if name in CONTEXT_CALLERS:
                site=va+0x13;context=METHODS["context"][0][n]
                code[0x13:0x18]=b"\xe8"+struct.pack("<i",context-site-5)
            self.place(va,bytes(code))
            self.place(va+length,b"\xcc"*padding)
    def place(self,addr,code):
        for j,byte in enumerate(code):
            pos=addr+j
            if pos in self.memory and self.memory[pos]!=byte:
                raise AssertionError("conflicting synthetic fixture %#x"%pos)
            self.memory[pos]=byte
    def at(self,va,count):
        return bytes(self.memory.get(va+i,0) for i in range(count))
    def section_of(self,va):
        if 0x22500000 <= va < 0x22600000 or 0x24300000 <= va < 0x24400000:
            return ".data"
        if 0x223d0000 <= va < 0x223e0000 or 0x24270000 <= va < 0x24280000:
            return ".rdata"
        return ".text"
    def flip(self,addr):
        self.memory[addr]=self.memory.get(addr,0)^1

class Tests(unittest.TestCase):
    def fake(self):return FakePE("server"),FakePE("client")
    def check(self,a,b):
        return verify_methods(a,b,check_hash=False,check_imports=False)
    def test_synthetic_pair_never_claims_verified_originals(self):
        a,b=self.fake();report=self.check(a,b)
        self.assertEqual(report["new_sha_verified_original_functions"],0)
        self.assertEqual(report["new_sha_verified_original_bytes"],0)
        self.assertEqual(report["cross_dll_complete_twin_pairs"],6)
        self.assertEqual(report["sparse_native_opcode_witnesses"],48)
        self.assertEqual(report["exact_absolute_address_operands"],62)
        self.assertEqual(report["exact_direct_context_calls"],8)
    def test_every_original_opcode_witness_mutation(self):
        for role_index,role in enumerate(("server","client")):
            for name,spec in METHODS.items():
                for off in spec[6]:
                    with self.subTest(role=role,name=name,offset=off):
                        a,b=self.fake()
                        (a,b)[role_index].flip(spec[0][role_index]+off)
                        with self.assertRaises(VerificationError):self.check(a,b)
    def test_every_absolute_operand_mutation(self):
        for role_index,role in enumerate(("server","client")):
            for name,spec in METHODS.items():
                for off,key in spec[3]:
                    with self.subTest(role=role,name=name,offset=off,key=key):
                        a,b=self.fake()
                        (a,b)[role_index].flip(spec[0][role_index]+off)
                        with self.assertRaises(VerificationError):self.check(a,b)
    def test_all_eight_native_context_calls(self):
        for role_index,role in enumerate(("server","client")):
            for name in CONTEXT_CALLERS:
                with self.subTest(role=role,name=name):
                    a,b=self.fake()
                    (a,b)[role_index].flip(METHODS[name][0][role_index]+0x14)
                    with self.assertRaises(VerificationError):self.check(a,b)
    def test_all_twelve_post_function_boundaries(self):
        for role_index in (0,1):
            for name,spec in METHODS.items():
                with self.subTest(role_index=role_index,name=name):
                    a,b=self.fake()
                    (a,b)[role_index].flip(spec[0][role_index]+spec[1])
                    with self.assertRaises(VerificationError):self.check(a,b)
    def test_unwitnessed_body_differences_break_exact_twin(self):
        for name in METHODS:
            with self.subTest(name=name):
                a,b=self.fake()
                # All native methods have at least one safe non-witnessed byte.
                b.flip(METHODS[name][0][1]+(2 if name=="free" else 0x10))
                with self.assertRaises(VerificationError):self.check(a,b)
    def test_wrong_image_bases_or_memory_sections(self):
        a,b=self.fake();a.base=0x20000000
        with self.assertRaises(VerificationError):self.check(a,b)
        a,b=self.fake();b.section_of=lambda _: ".text"
        with self.assertRaises(VerificationError):self.check(a,b)
    def test_fake_original_full_hashes_rejected(self):
        a,b=self.fake()
        with self.assertRaises(VerificationError):
            verify_methods(a,b,check_hash=True,check_imports=False)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_imported_export_names_and_IAT_locations_are_not_optional(self):
        for role in ("server","client"):
            for symbol in IAT[role]:
                with self.subTest(role=role,name=symbol):
                    a,b=self.fake()
                    mapping={name:(native.EXPECTED_LIBS[name],addr)
                             for name,addr in IAT[role].items()}
                    mapping[symbol]=("wrong.dll",IAT[role][symbol])
                    with patch.object(native,"imports_with_iat",return_value=mapping):
                        with self.assertRaises(VerificationError):
                            verify_methods(a,b,check_hash=False,check_imports=True)
    def test_cached_filename_data_remains_per_module(self):
        self.assertNotEqual(DATA["server"]["powner"],DATA["client"]["powner"])
        self.assertNotEqual(IAT["server"]["g_pMemAlloc"],IAT["client"]["g_pMemAlloc"])
        self.assertEqual(DATA["server"]["pflag"]-DATA["server"]["powner"],4)
        self.assertEqual(DATA["client"]["sflag"]-DATA["client"]["sowner"],4)

if __name__=="__main__":unittest.main()
