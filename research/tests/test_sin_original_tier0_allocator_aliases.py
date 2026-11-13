#!/usr/bin/env python3
"""Phase207: original-free synthetic mutation tests of complete native aliases."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_tier0_allocator_aliases as audit_module
from audit_sin_original_tier0_allocator_aliases import (
    REGIONS, FULL_SHA, BASE, IAT, DATA, METHODS, ALLOC_ALIAS_TYPES,
    rel_call, verify_native, audit, VerificationError,
)

class SyntheticPE:
    def __init__(self,role:str):
        self.role=role
        self.base=BASE[role]
        self.mem={}
        n=0 if role=="server" else 1
        for name,(entries,size,_,absolute,padding,witnesses,_) in REGIONS.items():
            va=entries[n]
            body=bytearray(b"\x90"*size)
            for off,hx in witnesses.items():
                code=bytes.fromhex(hx)
                body[off:off+len(code)]=code
            for off,key in absolute:
                ptr=IAT[role].get(key,DATA[role].get(key))
                struct.pack_into("<I",body,off,ptr)
            if name in ALLOC_ALIAS_TYPES:
                site=va+0x13
                body[0x13:0x18]=rel_call(site,METHODS["context"][0][n])
            self.put(va,body)
            self.put(va+size,b"\xcc"*padding)
        # Existing Phase205 stack allocation body is machine-code identical
        # to both new entrypoints after normalizing one native relative CALL.
        source=REGIONS["stack_alloc_alias_a"][0][n]
        origin=METHODS["stack_alloc"][0][n]
        base=bytearray(self.at(source,79))
        base[0x13:0x18]=rel_call(origin+0x13,METHODS["context"][0][n])
        self.put(origin,base)
        # Existing Phase206 one-argument vtable+0x20 bridge:
        old=(0x2226e5c0,0x2416fb20)[n]
        self.put(old,self.at(REGIONS["repeated_virtual20"][0][n],18))
    def put(self,va:int,content:bytes)->None:
        for i,b in enumerate(content):
            if va+i in self.mem and self.mem[va+i]!=b:
                raise AssertionError("conflicting independent synthetic x86 fixture")
            self.mem[va+i]=b
    def at(self,va:int,n:int)->bytes:
        return bytes(self.mem.get(va+i,0) for i in range(n))
    def section_of(self,va:int)->str:
        if 0x22500000<=va<0x22600000 or 0x24300000<=va<0x24400000:
            return ".data"
        if 0x223d0000<=va<0x223e0000 or 0x24270000<=va<0x24280000:
            return ".rdata"
        return ".text"
    def flip(self,va:int)->None:
        self.mem[va]=self.mem.get(va,0)^1

class Tests(unittest.TestCase):
    @staticmethod
    def new_pair():return SyntheticPE("server"),SyntheticPE("client")
    @staticmethod
    def verify(a,b):
        return verify_native(a,b,check_hash=False,check_imports=False)
    def test_synthetic_only_does_not_inflate_verified_function_count(self):
        a,b=self.new_pair()
        r=self.verify(a,b)
        self.assertEqual(r["new_sha_verified_original_functions"],0)
        self.assertEqual(r["new_original_native_function_bytes"],0)
        self.assertEqual(r["new_cross_dll_pairs"],4)
        self.assertEqual(r["exact_original_embedded_static_operands"],28)
        self.assertEqual(r["sparse_opcode_witnesses"],64)
        self.assertEqual(r["original_direct_context_calls_checked"],6)
    def test_every_sparse_opcode_witness_mutation(self):
        for j,role in enumerate(("server","client")):
            for name,spec in REGIONS.items():
                for off in spec[5]:
                    with self.subTest(role=role,name=name,offset=off):
                        a,b=self.new_pair()
                        (a,b)[j].flip(spec[0][j]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_every_absolute_import_or_global_mutation(self):
        for j,role in enumerate(("server","client")):
            for name,spec in REGIONS.items():
                for off,key in spec[3]:
                    with self.subTest(role=role,name=name,offset=off,key=key):
                        a,b=self.new_pair()
                        (a,b)[j].flip(spec[0][j]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_all_six_existing_and_new_direct_context_calls(self):
        for j in (0,1):
            for name in ("stack_alloc_alias_a","stack_alloc_alias_b"):
                a,b=self.new_pair()
                (a,b)[j].flip(REGIONS[name][0][j]+0x14)
                with self.assertRaises(VerificationError):self.verify(a,b)
            a,b=self.new_pair()
            (a,b)[j].flip(METHODS["stack_alloc"][0][j]+0x14)
            with self.assertRaises(VerificationError):self.verify(a,b)
    def test_every_ret_and_int3_original_boundary(self):
        for j in (0,1):
            for name,spec in REGIONS.items():
                with self.subTest(role=j,name=name):
                    a,b=self.new_pair()
                    (a,b)[j].flip(spec[0][j]+spec[1])
                    with self.assertRaises(VerificationError):self.verify(a,b)
    def test_unwitnessed_non_relocation_byte_breaks_native_twin(self):
        a,b=self.new_pair()
        b.flip(REGIONS["stack_alloc_alias_a"][0][1]+0x40)
        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_existing_vtable20_function_must_remain_identical(self):
        a,b=self.new_pair()
        b.flip(0x2416fb20+0x0c)
        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_image_base_memory_sections_and_IAT_identity(self):
        a,b=self.new_pair();a.base=0x23000000
        with self.assertRaises(VerificationError):self.verify(a,b)
        a,b=self.new_pair();b.section_of=lambda _:".text"
        with self.assertRaises(VerificationError):self.verify(a,b)
        for role in ("server","client"):
            a,b=self.new_pair()
            with patch.object(audit_module,"imports_with_iat",
                              return_value={"g_pMemAlloc":("wrong.dll",IAT[role]["g_pMemAlloc"])}):
                with self.assertRaises(VerificationError):
                    verify_native(a,b,check_hash=False,check_imports=True)
    def test_fake_original_full_function_and_both_dll_hash_rejected(self):
        a,b=self.new_pair()
        with self.assertRaises(VerificationError):
            verify_native(a,b,check_hash=True,check_imports=False)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_native_vtable_one_and_two_argument_distinction(self):
        self.assertEqual(REGIONS["repeated_virtual20"][5][0xd],"50ff5220")
        self.assertEqual(REGIONS["opaque_virtual1c_twoargs"][5][0x12],"50ff521c")
        self.assertEqual(REGIONS["stack_alloc_alias_a"][1],79)
        self.assertEqual(REGIONS["stack_alloc_alias_b"][1],79)

if __name__=="__main__":
    unittest.main()
