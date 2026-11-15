#!/usr/bin/env python3
"""Phase231 synthetic-only mutation tests for the parent lifecycle ABI."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_parent_lifecycle as original

class FakePE:
    """Built from sparse instructions, NOT retail native method bodies."""
    def __init__(self):
        self.base=original.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        self.rdata=set()
        self.highlow=set()
        for name,entry,size,sha,pad,ret_offsets,witnesses in original.METHODS:
            self.fill(entry,size,b"\x00")
            for at in ret_offsets:
                self.write(entry+at,b"\xc2"+(b"\x04\x00" if
                    "destructor" in name else b"\x08\x00"))
            for off,hx in witnesses.items():
                self.write(entry+off,bytes.fromhex(hx))
            self.fill(entry+size,pad,b"\xcc")
            self.text.update(range(entry,entry+size+pad))
        vptr=original.CONSTRUCTOR_FINAL_PARENT_VPTR_STORE
        self.write(vptr,b"\xc7\x06"+
                    struct.pack("<I",original.PARENT_VTABLE_VA))
        self.text.update(range(vptr,vptr+6))
        self.highlow.add(vptr+2)
        for slot,va in original.VIRTUAL_SLOTS:
            at=original.PARENT_VTABLE_VA+4*slot
            self.write(at,struct.pack("<I",va))
            self.rdata.update(range(at,at+4))
            self.highlow.add(at)
        for site,dest in original.DIRECT_CALLS:
            self.write(site,original.direct_call(site,dest))
            self.text.update(range(site,site+5))

    def fill(self,va,n,one):
        for i in range(n):self.memory[va+i]=one[0]
    def write(self,va,raw):
        for i,b in enumerate(raw):self.memory[va+i]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.text:return ".text"
        return ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase231Tests(unittest.TestCase):
    @staticmethod
    def verify(pe):
        return original.verify_native(pe,check_hash=False,
            original_provenance_verified=False,relocated_sites=pe.highlow)

    def test_original_free_fixture_never_counts_native_functions(self):
        pe=FakePE()
        result=self.verify(pe)
        self.assertEqual(result["new_original_full_SHA_methods"],0)
        self.assertEqual(result["new_original_complete_x86_bytes"],0)
        self.assertEqual(len(result["method_evidence"]),3)
        self.assertEqual(result["original_parent_vtable_HIGHLOW_slots"],3)
        self.assertEqual(result["native_direct_CALLs_into_other_methods"],7)
        with self.assertRaises(original.VerificationError):
            original.verify_native(pe,check_hash=True,relocated_sites=pe.highlow)
        with self.assertRaises(original.VerificationError):
            original.audit(b"MZ",b"MZ")

    def test_every_original_native_opcode_witness_mutation(self):
        for name,entry,size,sha,pad,ret_offsets,witnesses in original.METHODS:
            for offset in witnesses:
                with self.subTest(method=name,off=offset):
                    pe=FakePE();pe.flip(entry+offset)
                    with self.assertRaises(original.VerificationError):
                        self.verify(pe)

    def test_all_original_ret_and_int3_method_boundaries(self):
        for name,entry,size,sha,pad,ret_offsets,witnesses in original.METHODS:
            for offset in ret_offsets:
                with self.subTest(method=name,ret=offset):
                    pe=FakePE();pe.flip(entry+offset)
                    with self.assertRaises(original.VerificationError):
                        self.verify(pe)
            for i in range(pad):
                with self.subTest(method=name,pad=i):
                    pe=FakePE();pe.flip(entry+size+i)
                    with self.assertRaises(original.VerificationError):
                        self.verify(pe)

    def test_three_original_parent_vtable_slots_and_highlow_relocations(self):
        for slot,entry in original.VIRTUAL_SLOTS:
            va=original.PARENT_VTABLE_VA+4*slot
            for i in range(4):
                with self.subTest(slot=slot,byte=i):
                    pe=FakePE();pe.flip(va+i)
                    with self.assertRaises(original.VerificationError):
                        self.verify(pe)
            pe=FakePE();pe.highlow.remove(va)
            with self.assertRaises(original.VerificationError):self.verify(pe)
            pe=FakePE();pe.rdata.remove(va)
            with self.assertRaises(original.VerificationError):self.verify(pe)

    def test_native_parent_ctor_vptr_not_embedded_vptr(self):
        va=original.CONSTRUCTOR_FINAL_PARENT_VPTR_STORE
        for i in range(6):
            with self.subTest(i=i):
                pe=FakePE();pe.flip(va+i)
                with self.assertRaises(original.VerificationError):self.verify(pe)
        pe=FakePE();pe.highlow.remove(va+2)
        with self.assertRaises(original.VerificationError):self.verify(pe)
        pe=FakePE();pe.base+=0x1000
        with self.assertRaises(original.VerificationError):self.verify(pe)

    def test_seven_real_game_direct_call_edges(self):
        for site,dest in original.DIRECT_CALLS:
            for i in range(5):
                with self.subTest(site=hex(site),index=i):
                    pe=FakePE();pe.flip(site+i)
                    with self.assertRaises(original.VerificationError):self.verify(pe)

    def test_separate_primary_and_mirror_index_constants(self):
        self.assertEqual(sum(row[2] for row in original.METHODS),348)
        self.assertEqual(len(original.METHODS),3)
        self.assertEqual(len(original.DIRECT_CALLS),7)
        self.assertEqual(original.PARENT_VTABLE_VA,0x242973B8)
        self.assertEqual(original.EMBEDDED_VTABLE_VA,0x24297390)
        self.assertNotEqual(original.PARENT_VTABLE_VA,original.EMBEDDED_VTABLE_VA)
        self.assertEqual(original.NATIVE_INDEX_MASK,0xFFF)
        self.assertEqual(original.MIRROR_LIVE_SLOT_LIMIT,0x800)
        self.assertEqual(original.MIRROR_POINTER_OFFSET,0x10028)
        self.assertEqual(original.MIRROR_AUX_HANDLE_OFFSET,0x1002C)
        self.assertEqual(original.SECONDARY_FLAG_BIT,0x400)

if __name__=="__main__":
    unittest.main()
