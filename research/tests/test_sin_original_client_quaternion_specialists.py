#!/usr/bin/env python3
"""Synthetic mutation tests for Phase223 quaternion native evidence.

The fixture NEVER contains or validates full original retail method bytes.
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_quaternion_specialists as audit


class FakePE:
    def __init__(self):
        self.base=audit.BASE["client"]
        self.mem={}
        self.rdata=set()
        for va,n,_sha,pad,caller,pop,witnesses in audit.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+n-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*pad)
            self.write(caller,audit.rel_call(caller,va))
        for site,dest in (audit.COPY_CALL,)+audit.NESTED_CALLS:
            self.write(site,audit.rel_call(site,dest))
        for va,s in audit.FORMAT_STRINGS.items():
            raw=s.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))
        self.write(audit.FLOAT_ZERO,b"\0"*4)
        self.rdata.update(range(audit.FLOAT_ZERO,audit.FLOAT_ZERO+4))
        for va,head in audit.NEXT_FUNCTIONS.items():
            self.write(va,head)
        addr,opcode=audit.UNINITIALIZED_STACK_SLOT_WRITER
        self.write(addr,opcode)
        for addr in audit.DELTA_SLOT_READS:
            self.write(addr,b"\xd9")

    def write(self,va,data):
        for i,value in enumerate(data):
            old=self.mem.get(va+i,value)
            if old!=value:
                raise AssertionError(f"synthetic witness byte collision at {va+i:#x}")
            self.mem[va+i]=value

    def at(self,va,n):
        return bytes(self.mem.get(va+i,0) for i in range(n))

    def section_of(self,va):
        return ".rdata" if va in self.rdata else ".text"

    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1


class Phase223Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return audit.verify_native(fake,check_hash=False)

    def test_no_synthetic_fixture_counts_as_recovered_original_method(self):
        report=self.verify(FakePE())
        self.assertEqual(report["new_SHA_verified_original_native_methods"],0)
        self.assertEqual(report["new_complete_original_x86_bytes"],0)
        self.assertEqual(report["sparse_native_opcode_witnesses"],46)
        self.assertEqual(report["native_type4_router_direct_CALLs"],4)
        self.assertEqual(report["native_nested_formatter_CALLs"],3)
        self.assertEqual(report["format_literals"],3)
        with self.assertRaises(audit.VerificationError):
            audit.verify_native(FakePE(),check_hash=True)
        with self.assertRaises(audit.VerificationError):
            audit.audit(b"MZ",b"MZ")

    def test_every_native_opcode_witness_mutation_is_rejected(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in audit.METHODS.items():
            for off in witnesses:
                with self.subTest(method=name,offset=hex(off)):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(audit.VerificationError):
                        self.verify(fake)

    def test_all_router_and_formatter_call_mutations_rejected(self):
        edges=[(row[4],row[0]) for row in audit.METHODS.values()]
        edges+=[audit.COPY_CALL]
        edges+=list(audit.NESTED_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=FakePE();fake.flip(site+1)
                with self.assertRaises(audit.VerificationError):
                    self.verify(fake)

    def test_every_ret_and_padding_byte(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in audit.METHODS.items():
            fake=FakePE();fake.flip(va+n-2)
            with self.assertRaises(audit.VerificationError):
                self.verify(fake)
            for offset in range(pad):
                with self.subTest(method=name,padding=offset):
                    fake=FakePE();fake.flip(va+n+offset)
                    with self.assertRaises(audit.VerificationError):
                        self.verify(fake)

    def test_scratch_indexing_and_reader_instructions(self):
        addr,opcode=audit.UNINITIALIZED_STACK_SLOT_WRITER
        for i in range(len(opcode)):
            fake=FakePE();fake.flip(addr+i)
            with self.assertRaises(audit.VerificationError):
                self.verify(fake)
        for addr in audit.DELTA_SLOT_READS:
            fake=FakePE();fake.flip(addr)
            with self.assertRaises(audit.VerificationError):
                self.verify(fake)

    def test_format_strings_zero_constant_and_neighbors(self):
        for va,s in audit.FORMAT_STRINGS.items():
            for offset in (0,len(s)):
                with self.subTest(format=hex(va),offset=offset):
                    fake=FakePE();fake.flip(va+offset)
                    with self.assertRaises(audit.VerificationError):
                        self.verify(fake)
        for offset in range(4):
            fake=FakePE();fake.flip(audit.FLOAT_ZERO+offset)
            with self.assertRaises(audit.VerificationError):
                self.verify(fake)
        for va,head in audit.NEXT_FUNCTIONS.items():
            for off in range(len(head)):
                with self.subTest(neighbor=hex(va),offset=off):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(audit.VerificationError):
                        self.verify(fake)

    def test_original_PE_base_and_sections(self):
        fake=FakePE();fake.base+=0x1000
        with self.assertRaises(audit.VerificationError):
            self.verify(fake)
        fake=FakePE();fake.section_of=lambda _va: ".data"
        with self.assertRaises(audit.VerificationError):
            self.verify(fake)

    def test_exact_original_three_method_sizes(self):
        self.assertEqual(sum(row[1] for row in audit.METHODS.values()),722)
        self.assertEqual([row[3] for row in audit.METHODS.values()],[14,10,6])
        self.assertEqual({row[0] for row in audit.METHODS.values()},
                         {0x24198F50,0x241987C0,0x241989A0})
        self.assertEqual({row[5] for row in audit.METHODS.values()},{12,16})


if __name__=="__main__":
    unittest.main()
