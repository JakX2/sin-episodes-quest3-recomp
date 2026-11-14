#!/usr/bin/env python3
"""Original-free synthetic mutation tests for Phase222 vec3 native auditor."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_vec3_specialists as phase


class SyntheticPE:
    def __init__(self):
        self.base=phase.BASE["client"]
        self.memory={}
        self.rdata=set()
        for va,n,sha,pad,caller,pop,witnesses in phase.METHODS.values():
            for offset,hx in witnesses.items():
                self.write(va+offset,bytes.fromhex(hx))
            self.write(va+n-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*pad)
            self.write(caller,phase.call_bytes(caller,va))
        for site,dest in (phase.TYPE3_COPY_CALL,)+phase.NESTED_CALLS:
            self.write(site,phase.call_bytes(site,dest))
        for va,s in phase.FORMATS.items():
            raw=s.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))
        self.write(phase.FLOAT_ZERO_VA,b"\0"*4)
        self.rdata.update(range(phase.FLOAT_ZERO_VA,phase.FLOAT_ZERO_VA+4))

    def write(self,va:int,data:bytes):
        for i,value in enumerate(data):
            previous=self.memory.get(va+i,value)
            if previous!=value:
                raise AssertionError("conflicting synthetic PE opcode witness")
            self.memory[va+i]=value

    def at(self,va:int,count:int)->bytes:
        return bytes(self.memory.get(va+i,0) for i in range(count))

    def section_of(self,va:int)->str:
        return ".rdata" if va in self.rdata else ".text"

    def mutate(self,va:int):
        self.memory[va]=self.memory.get(va,0)^1


class Phase222Tests(unittest.TestCase):
    def verify(self,fake):
        return phase.verify_native(fake,check_hash=False)

    def test_synthetic_can_never_claim_original_SHA_recovery(self):
        report=self.verify(SyntheticPE())
        self.assertEqual(report["new_whole_original_SHA_verified_methods"],0)
        self.assertEqual(report["new_whole_original_native_bytes"],0)
        self.assertEqual(report["sparse_original_x86_instruction_witnesses"],37)
        self.assertEqual(report["original_case3_direct_calls"],4)
        self.assertEqual(report["original_nested_formatter_CALLs"],3)
        self.assertEqual(report["original_format_literals"],3)
        with self.assertRaises(phase.VerificationError):
            phase.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(phase.VerificationError):
            phase.audit(b"MZ",b"MZ")

    def test_every_sparse_original_instruction_guard(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in phase.METHODS.items():
            for off in witnesses:
                with self.subTest(method=name,offset=hex(off)):
                    fake=SyntheticPE()
                    fake.mutate(va+off)
                    with self.assertRaises(phase.VerificationError):
                        self.verify(fake)

    def test_all_router_and_formatter_calls(self):
        edges=[(record[4],record[0]) for record in phase.METHODS.values()]
        edges += [phase.TYPE3_COPY_CALL]
        edges += list(phase.NESTED_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=SyntheticPE()
                fake.mutate(site+1)
                with self.assertRaises(phase.VerificationError):
                    self.verify(fake)

    def test_original_RET_and_all_INT3_boundaries(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in phase.METHODS.items():
            with self.subTest(method=name,kind="ret"):
                fake=SyntheticPE()
                fake.mutate(va+n-2)
                with self.assertRaises(phase.VerificationError):
                    self.verify(fake)
            for i in range(pad):
                with self.subTest(method=name,kind="padding",offset=i):
                    fake=SyntheticPE()
                    fake.mutate(va+n+i)
                    with self.assertRaises(phase.VerificationError):
                        self.verify(fake)

    def test_native_format_literals_and_positive_tolerance_constant(self):
        for va,s in phase.FORMATS.items():
            for off in (0,len(s)):
                with self.subTest(address=hex(va),offset=off):
                    fake=SyntheticPE()
                    fake.mutate(va+off)
                    with self.assertRaises(phase.VerificationError):
                        self.verify(fake)
        for off in range(4):
            fake=SyntheticPE()
            fake.mutate(phase.FLOAT_ZERO_VA+off)
            with self.assertRaises(phase.VerificationError):
                self.verify(fake)

    def test_original_base_and_text_range(self):
        fake=SyntheticPE()
        fake.base+=0x10000
        with self.assertRaises(phase.VerificationError):
            self.verify(fake)
        fake=SyntheticPE()
        fake.section_of=lambda _va: ".data"
        with self.assertRaises(phase.VerificationError):
            self.verify(fake)

    def test_exact_original_native_lengths(self):
        self.assertEqual(sum(x[1] for x in phase.METHODS.values()),559)
        self.assertEqual({x[0] for x in phase.METHODS.values()},
                         {0x24198E20,0x241985A0,0x241986C0})
        self.assertEqual([x[3] for x in phase.METHODS.values()],[15,5,13])
        self.assertEqual({x[5] for x in phase.METHODS.values()},{12,16})


if __name__=="__main__":
    unittest.main()
