#!/usr/bin/env python3
"""Synthetic-only Phase218 original SiN type7 helper audit mutation tests."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_short_specialists as native


class SyntheticPE:
    """Synthetic instruction witnesses, NOT original 2006 game code."""
    def __init__(self):
        self.base=native.BASE["client"]
        self.mem={}
        self.strings=set()
        for va,n,_,pad,caller,stack_pop,witnesses in native.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+n-3,b"\xc2"+stack_pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*pad)
            self.write(caller,native.rel_call(caller,va))
        self.write(native.FOLLOWING_FUNCTION_VA,native.FOLLOWING_ENTRY_WITNESS)
        for site,target in native.NESTED_CALLS:
            self.write(site,native.rel_call(site,target))
        for va,value in native.FORMAT_STRINGS.items():
            raw=value.encode("ascii")+b"\0"
            self.write(va,raw)
            self.strings.update(range(va,va+len(raw)))

    def write(self,va,raw):
        for i,value in enumerate(raw):
            old=self.mem.get(va+i,value)
            if old!=value:
                raise AssertionError("conflicting synthetic original method witness")
            self.mem[va+i]=value

    def at(self,va,n):
        return bytes(self.mem.get(va+i,0) for i in range(n))

    def section_of(self,va):
        return ".rdata" if va in self.strings else ".text"

    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1


class Phase218Tests(unittest.TestCase):
    def check(self,fake):
        return native.verify_native(fake,check_hash=False)

    def test_synthetic_does_not_count_as_original_native_recovery(self):
        fake=SyntheticPE()
        result=self.check(fake)
        self.assertEqual(result["new_sha_verified_original_client_functions"],0)
        self.assertEqual(result["new_complete_original_x86_bytes"],0)
        self.assertEqual(result["exact_type7_router_call_edges"],4)
        self.assertEqual(result["exact_native_nested_formatter_call_edges"],3)
        self.assertEqual(result["original_literal_count"],3)
        self.assertEqual(result["sparse_native_instruction_witnesses"],50)
        self.assertTrue(result["verified_adjacent_no_padding_boundary"])
        with self.assertRaises(native.VerificationError):
            native.verify_native(fake,check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_sparse_instruction_anchor_checked(self):
        for name,(va,n,_,_,_,_,witnesses) in native.METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,offset=hex(off)):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_every_original_router_and_nested_formatter_call_checked(self):
        edges=[(data[4],data[0]) for data in native.METHODS.values()]
        edges+=list(native.NESTED_CALLS)
        for site,target in edges:
            with self.subTest(callsite=hex(site),target=hex(target)):
                fake=SyntheticPE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_every_original_ret_and_int3_padding_byte(self):
        for name,(va,n,_,pad,_,_,_) in native.METHODS.items():
            with self.subTest(method=name,ret=True):
                fake=SyntheticPE();fake.flip(va+n-2)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)
            for off in range(pad):
                with self.subTest(method=name,padding=off):
                    fake=SyntheticPE();fake.flip(va+n+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_no_int3_boundary_between_original_word_and_dword_copies(self):
        self.assertEqual(native.METHODS["type7_short_word_copy"][0]+48,
                         native.FOLLOWING_FUNCTION_VA)
        self.assertEqual(native.METHODS["type7_short_word_copy"][3],0)
        for off in range(len(native.FOLLOWING_ENTRY_WITNESS)):
            with self.subTest(byte=off):
                fake=SyntheticPE();fake.flip(native.FOLLOWING_FUNCTION_VA+off)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_original_signed_short_literal_witnesses(self):
        for va,value in native.FORMAT_STRINGS.items():
            for off in (0,len(value)):
                with self.subTest(format=hex(va),byte=off):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_wrong_original_client_PE_base_and_section(self):
        fake=SyntheticPE();fake.base+=0x100000
        with self.assertRaises(native.VerificationError):
            self.check(fake)
        fake=SyntheticPE();fake.section_of=lambda va:".data"
        with self.assertRaises(native.VerificationError):
            self.check(fake)

    def test_exact_original_method_lengths_and_arity(self):
        self.assertEqual(sum(x[1] for x in native.METHODS.values()),244)
        self.assertEqual({x[0] for x in native.METHODS.values()},
                         {0x24197D50,0x241989E0,0x24197B10,0x24198380})
        self.assertEqual({x[5] for x in native.METHODS.values()},{12,16})
        self.assertEqual(sorted(x[3] for x in native.METHODS.values()),
                         [0,1,12,15])


if __name__=="__main__":
    unittest.main()
