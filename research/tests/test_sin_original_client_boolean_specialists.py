#!/usr/bin/env python3
"""Phase219 synthetic-only mutation checks; never stores commercial DLL code."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_boolean_specialists as native


class SyntheticPE:
    def __init__(self):
        self.base=native.BASE["client"]
        self.memory={}
        self.rdata=set()
        for va,size,sha,pad,caller,ret_pop,witnesses in native.METHODS.values():
            for offset,hx in witnesses.items():
                self.write(va+offset,bytes.fromhex(hx))
            self.write(va+size-3,b"\xc2"+ret_pop.to_bytes(2,"little"))
            self.write(va+size,b"\xcc"*pad)
            self.write(caller,native.rel_call(caller,va))
        self.write(0x24199993,native.rel_call(0x24199993,0x24198A40))
        for caller,callee in native.NESTED_CALLS:
            self.write(caller,native.rel_call(caller,callee))
        for va,s in native.FORMAT_LITERALS.items():
            raw=s.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))

    def write(self,va:int,raw:bytes):
        for i,value in enumerate(raw):
            old=self.memory.get(va+i,value)
            if old!=value:
                raise AssertionError("synthetic opcode witnesses overlap inconsistently")
            self.memory[va+i]=value

    def at(self,va:int,count:int)->bytes:
        return bytes(self.memory.get(va+i,0) for i in range(count))

    def section_of(self,va:int)->str:
        return ".rdata" if va in self.rdata else ".text"

    def flip(self,va:int):
        self.memory[va]=self.memory.get(va,0)^1


class Phase219Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return native.verify_native(fake,check_hash=False)

    def test_synthetic_fixture_cannot_verify_original_sha(self):
        fake=SyntheticPE()
        report=self.verify(fake)
        self.assertEqual(report["new_complete_sha_verified_original_client_functions"],0)
        self.assertEqual(report["new_complete_original_native_bytes"],0)
        self.assertEqual(report["sparse_complete_instruction_witnesses"],44)
        self.assertEqual(report["original_direct_router_calls"],4)
        self.assertEqual(report["original_nested_formatter_calls"],3)
        self.assertEqual(report["original_format_and_boolean_literals"],5)
        with self.assertRaises(native.VerificationError):
            native.verify_native(fake,check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_mutate_each_sparse_complete_instruction(self):
        for name,(va,_,_,_,_,_,witnesses) in native.METHODS.items():
            for offset in witnesses:
                with self.subTest(method=name,at=hex(offset)):
                    fake=SyntheticPE();fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_mutate_four_router_calls_and_three_nested_calls(self):
        edges=[(row[4],row[0]) for row in native.METHODS.values()]
        edges.append((0x24199993,0x24198A40))
        edges.extend(native.NESTED_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=SyntheticPE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_mutate_each_native_ret_cleanup_and_padding(self):
        for name,(va,size,_,padding,_,_,_) in native.METHODS.items():
            with self.subTest(method=name,kind="ret"):
                fake=SyntheticPE();fake.flip(va+size-2)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for i in range(padding):
                with self.subTest(method=name,kind="int3",offset=i):
                    fake=SyntheticPE();fake.flip(va+size+i)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_mutate_all_three_formats_and_true_false_native_literals(self):
        for va,s in native.FORMAT_LITERALS.items():
            for offset in (0,len(s)):
                with self.subTest(address=hex(va),offset=offset):
                    fake=SyntheticPE();fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_reject_wrong_PE_base_and_non_code_methods(self):
        fake=SyntheticPE();fake.base+=0x10000
        with self.assertRaises(native.VerificationError):self.verify(fake)
        fake=SyntheticPE();fake.section_of=lambda va:".data"
        with self.assertRaises(native.VerificationError):self.verify(fake)

    def test_exact_original_sizes_and_instruction_counts(self):
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),237)
        self.assertEqual({row[0] for row in native.METHODS.values()},
                         {0x24197E30,0x24197C30,0x24198440})
        self.assertEqual([row[3] for row in native.METHODS.values()],
                         [14,2,3])
        self.assertEqual({row[5] for row in native.METHODS.values()},{12,16})


if __name__=="__main__":
    unittest.main()
