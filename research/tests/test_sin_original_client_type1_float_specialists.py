#!/usr/bin/env python3
"""Original-free synthetic native x86 mutation tests for Phase216."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_type1_float_specialists import (
    METHODS,NESTED_CALLS,STRINGS,FLOAT_ZERO_VA,BASE,VerificationError,
    verify_native,audit,rel_call,
)


class FakeOriginalPE:
    def __init__(self):
        self.base=BASE["client"]
        self.memory={}
        self.rdata=set()
        for va,length,_sha,pad,caller,pop,witnesses in METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+length-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+length,b"\xcc"*pad)
            self.write(caller,rel_call(caller,va))
        for caller,callee in NESTED_CALLS:
            self.write(caller,rel_call(caller,callee))
        for va,value in STRINGS.items():
            data=value.encode("ascii")+b"\0"
            self.write(va,data)
            self.rdata.update(range(va,va+len(data)))
        self.write(FLOAT_ZERO_VA,b"\0"*4)
        self.rdata.update(range(FLOAT_ZERO_VA,FLOAT_ZERO_VA+4))

    def write(self,va,raw):
        for i,byte in enumerate(raw):
            old=self.memory.get(va+i,byte)
            if old!=byte:
                raise AssertionError("conflicting synthetic native witnesses")
            self.memory[va+i]=byte

    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))

    def section_of(self,va):
        return ".rdata" if va in self.rdata else ".text"

    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1


class Phase216Tests(unittest.TestCase):
    def test_synthetic_cannot_claim_original_methods(self):
        fake=FakeOriginalPE()
        doc=verify_native(fake,check_hash=False)
        self.assertEqual(doc["new_original_verified_client_functions"],0)
        self.assertEqual(doc["new_complete_original_native_bytes"],0)
        self.assertEqual(doc["sparse_opcode_witnesses"],37)
        self.assertEqual(doc["original_router_calls"],3)
        self.assertEqual(doc["original_nested_trace_calls"],3)
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=True)
        with self.assertRaises(VerificationError):
            audit(b"MZ",b"MZ")

    def test_every_native_method_witness_mutation(self):
        for name,(va,_,_,_,_,_,witnesses) in METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,off=hex(off)):
                    fake=FakeOriginalPE();fake.flip(va+off)
                    with self.assertRaises(VerificationError):
                        verify_native(fake,check_hash=False)

    def test_original_internal_and_router_call_mutations(self):
        edges=[(x[4],x[0]) for x in METHODS.values()]+list(NESTED_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=FakeOriginalPE();fake.flip(site+1)
                with self.assertRaises(VerificationError):
                    verify_native(fake,check_hash=False)

    def test_every_native_ret_and_padding(self):
        for name,(va,n,_,pad,_,_,_) in METHODS.items():
            with self.subTest(name=name,kind="ret"):
                fake=FakeOriginalPE();fake.flip(va+n-2)
                with self.assertRaises(VerificationError):
                    verify_native(fake,check_hash=False)
            for i in range(pad):
                with self.subTest(name=name,kind="int3",i=i):
                    fake=FakeOriginalPE();fake.flip(va+n+i)
                    with self.assertRaises(VerificationError):
                        verify_native(fake,check_hash=False)

    def test_float_zero_and_format_literal_mutations(self):
        for va,value in STRINGS.items():
            for off in (0,len(value)):
                with self.subTest(va=hex(va),off=off):
                    fake=FakeOriginalPE();fake.flip(va+off)
                    with self.assertRaises(VerificationError):
                        verify_native(fake,check_hash=False)
        for i in range(4):
            fake=FakeOriginalPE();fake.flip(FLOAT_ZERO_VA+i)
            with self.assertRaises(VerificationError):
                verify_native(fake,check_hash=False)

    def test_wrong_sections_and_original_image_base(self):
        fake=FakeOriginalPE();fake.base+=0x1000
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)
        fake=FakeOriginalPE();fake.section_of=lambda _va:".data"
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)

    def test_real_method_boundaries_and_padding_lengths(self):
        self.assertEqual(sum(row[1] for row in METHODS.values()),281)
        self.assertEqual(
            {row[0] for row in METHODS.values()},
            {0x24198AA0,0x24197CA0,0x24198470})
        self.assertEqual(
            {row[3] for row in METHODS.values()},
            {8,4,11})


if __name__=="__main__":
    unittest.main()
