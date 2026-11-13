#!/usr/bin/env python3
"""Phase206 original-free mutation tests for paired tier0 allocator functions."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_tier0_zero_allocation as native
from audit_sin_original_tier0_zero_allocation import (
    REGIONS, IAT, DATA, BASE, verify_native, audit, VerificationError
)

class FakePE:
    def __init__(self,role:str):
        self.role=role
        self.base=BASE[role]
        self.memory={}
        number=0 if role=="server" else 1
        for name,record in REGIONS.items():
            entries,length,_,_,slots,_,padding,witnesses=record
            va=entries[number]
            raw=bytearray(b"\x90"*length)
            for offset,hx in witnesses.items():
                code=bytes.fromhex(hx)
                raw[offset:offset+len(code)]=code
            for offset,field in slots:
                target=IAT[role].get(field,DATA[role].get(field))
                struct.pack_into("<I",raw,offset,target)
            if name=="zero_alloc":
                callsite=va+0x1e
                target=(0x2226e290,0x2416f7f0)[number]
                raw[0x1e:0x23]=b"\xe8"+struct.pack("<i",target-callsite-5)
            self.put(va,raw)
            self.put(va+length,bytes([0xcc])*padding)
    def put(self,where:int,raw:bytes)->None:
        for j,b in enumerate(raw):
            p=where+j
            if p in self.memory and self.memory[p]!=b:
                raise AssertionError("synthetic fixture overlap")
            self.memory[p]=b
    def at(self,va:int,n:int)->bytes:
        return bytes(self.memory.get(va+j,0) for j in range(n))
    def section_of(self,va:int)->str:
        if 0x22500000<=va<0x22600000 or 0x24300000<=va<0x24400000:
            return ".data"
        if 0x223d0000<=va<0x223e0000 or 0x24270000<=va<0x24280000:
            return ".rdata"
        return ".text"
    def flip(self,va:int)->None:
        self.memory[va]=self.memory.get(va,0)^1

class Tests(unittest.TestCase):
    @staticmethod
    def fresh():
        return FakePE("server"),FakePE("client")
    @staticmethod
    def verify(a,b):
        return verify_native(a,b,check_hash=False,check_imports=False)
    def test_synthetic_fixture_never_claims_original_hashes(self):
        a,b=self.fresh()
        report=self.verify(a,b)
        self.assertEqual(report["new_sha_verified_original_functions"],0)
        self.assertEqual(report["new_original_native_function_bytes"],0)
        self.assertEqual(report["full_original_function_pair_count"],3)
        self.assertEqual(report["exact_original_embedded_absolute_operands"],16)
        self.assertEqual(report["sparse_opcode_witnesses"],26)
        self.assertEqual(report["exact_direct_owner_helper_calls"],2)
    def test_every_sparse_instruction_witness(self):
        for n,role in enumerate(("server","client")):
            for name,record in REGIONS.items():
                for off in record[7]:
                    with self.subTest(role=role,name=name,off=off):
                        a,b=self.fresh()
                        (a,b)[n].flip(record[0][n]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_every_embedded_static_and_iat_operand(self):
        for n,role in enumerate(("server","client")):
            for name,record in REGIONS.items():
                for off,_ in record[4]:
                    with self.subTest(role=role,name=name,off=off):
                        a,b=self.fresh()
                        (a,b)[n].flip(record[0][n]+off)
                        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_original_direct_owner_context_call(self):
        for n in (0,1):
            a,b=self.fresh()
            (a,b)[n].flip(REGIONS["zero_alloc"][0][n]+0x1f)
            with self.assertRaises(VerificationError):self.verify(a,b)
    def test_exact_native_boundaries(self):
        for n in (0,1):
            for name,record in REGIONS.items():
                with self.subTest(module=n,name=name):
                    a,b=self.fresh()
                    (a,b)[n].flip(record[0][n]+record[1])
                    with self.assertRaises(VerificationError):self.verify(a,b)
    def test_unwitnessed_mirror_difference(self):
        for name,record in REGIONS.items():
            with self.subTest(name=name):
                a,b=self.fresh()
                # This is not a witness/relocation field or a call operand.
                offset={"zero_alloc":0x2f,
                        "opaque_vtable_58_tailjump":7,
                        "opaque_vtable_20_call":8}[name]
                b.flip(record[0][1]+offset)
                with self.assertRaises(VerificationError):self.verify(a,b)
    def test_bad_pe_base_and_section(self):
        a,b=self.fresh();a.base=0x23000000
        with self.assertRaises(VerificationError):self.verify(a,b)
        a,b=self.fresh();b.section_of=lambda _:".text"
        with self.assertRaises(VerificationError):self.verify(a,b)
    def test_dll_hash_and_complete_native_function_hash_gate(self):
        a,b=self.fresh()
        with self.assertRaises(VerificationError):
            verify_native(a,b,check_hash=True,check_imports=False)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_imported_tier0_name_and_slot_verified(self):
        for role in ("server","client"):
            with self.subTest(role=role):
                a,b=self.fresh()
                with patch.object(native,"imports_with_iat",
                                  return_value={"g_pMemAlloc":("wrong.dll",IAT[role]["g_pMemAlloc"])}):
                    with self.assertRaises(VerificationError):
                        verify_native(a,b,check_hash=False,check_imports=True)
    def test_native_vtable_bridge_offsets_remain_opaque(self):
        self.assertEqual(bytes.fromhex(REGIONS["opaque_vtable_58_tailjump"][7][9]),b"\xff\x62\x58")
        self.assertEqual(bytes.fromhex(REGIONS["opaque_vtable_20_call"][7][0xe]),b"\xff\x52\x20")
        self.assertEqual(REGIONS["zero_alloc"][1],107)

if __name__=="__main__":
    unittest.main()
