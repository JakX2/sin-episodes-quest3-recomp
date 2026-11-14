#!/usr/bin/env python3
"""Binary-free mutation tests for Phase217; no original proprietary bytes."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_binary_integer_specialists as native


class FakePE:
    def __init__(self):
        self.base=native.BASE["client"]
        self.mem={}
        self.strings=set()
        self.globals={native.SYMBOL_RESOLVER_GLOBAL}
        for va,n,digest,pad,callers,ret_pop,witnesses in native.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+n-3,b"\xc2"+ret_pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*pad)
            for site in callers:
                self.write(site,native.rel_call(site,va))
        for site,target in native.NESTED_DIRECT_CALLS:
            self.write(site,native.rel_call(site,target))
        for va,literal in native.FORMATS.items():
            raw=literal.encode("ascii")+b"\0"
            self.write(va,raw)
            self.strings.update(range(va,va+len(raw)))

    def write(self,va,raw):
        for i,value in enumerate(raw):
            old=self.mem.get(va+i,value)
            if old!=value:
                raise AssertionError(f"conflicting synthetic opcode {va+i:#x}")
            self.mem[va+i]=value

    def at(self,va,n):
        return bytes(self.mem.get(va+i,0) for i in range(n))

    def section_of(self,va):
        if va in self.globals:return ".data"
        if va in self.strings:return ".rdata"
        return ".text"

    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1


class Phase217Tests(unittest.TestCase):
    @staticmethod
    def verify(fake,*,imports=False):
        return native.verify_native(fake,check_hash=False,check_imports=imports)

    def test_synthetic_data_cannot_count_as_original_recovery(self):
        fake=FakePE()
        result=self.verify(fake)
        self.assertEqual(result["new_sha_verified_native_client_functions"],0)
        self.assertEqual(result["complete_original_x86_bytes"],0)
        self.assertEqual(result["sparse_instruction_witnesses"],51)
        self.assertEqual(result["verified_original_router_call_sites"],6)
        self.assertEqual(result["verified_nested_original_direct_calls"],6)
        self.assertEqual(result["original_string_literals"],6)
        self.assertEqual(len(result["methods"]),4)
        with self.assertRaises(native.VerificationError):
            native.verify_native(fake,check_hash=True,check_imports=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_sparse_original_opcode_corruption_rejected(self):
        for name,(va,n,_,_,_,_,witnesses) in native.METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,off=hex(off)):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_all_router_and_nested_calls_verified(self):
        edges=[(caller,va) for va,_,_,_,callers,_,_ in native.METHODS.values()
               for caller in callers]+list(native.NESTED_DIRECT_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=FakePE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_all_native_ret_cleanups_and_int3_pads_pinned(self):
        for name,(va,n,_,pad,_,_,_) in native.METHODS.items():
            with self.subTest(name=name,ret="arity"):
                fake=FakePE();fake.flip(va+n-2)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for i in range(pad):
                with self.subTest(name=name,ret="padding",i=i):
                    fake=FakePE();fake.flip(va+n+i)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_every_original_format_literal_rejected_when_corrupted(self):
        for va,literal in native.FORMATS.items():
            for off in (0,len(literal)):
                with self.subTest(va=hex(va),off=off):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_named_Q_FileBase_import_and_global_section(self):
        fake=FakePE()
        with patch.object(native,"named_iat",return_value=[native.Q_FILEBASE_IAT]):
            self.assertEqual(self.verify(fake,imports=True)["original_Q_FileBase_IAT"],
                             "0x24276254")
        with patch.object(native,"named_iat",return_value=[native.Q_FILEBASE_IAT+4]):
            with self.assertRaises(native.VerificationError):
                self.verify(fake,imports=True)
        fake=FakePE();fake.globals.clear()
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_wrong_PEx86_base_and_text_sections(self):
        fake=FakePE();fake.base+=0x200000
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        fake=FakePE();fake.section_of=lambda _va:".data"
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_boundary_and_unique_target_accounting(self):
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),343)
        self.assertEqual(len({row[0] for row in native.METHODS.values()}),4)
        self.assertEqual(sum(len(row[4]) for row in native.METHODS.values()),6)
        self.assertEqual({row[5] for row in native.METHODS.values()},{12,16})
        self.assertEqual({row[3] for row in native.METHODS.values()},{2,4,8,11})


if __name__=="__main__":
    unittest.main()
