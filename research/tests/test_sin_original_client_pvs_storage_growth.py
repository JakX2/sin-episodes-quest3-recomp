#!/usr/bin/env python3
"""Phase235 synthetic evidence mutation tests (zero original-body credit)."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_storage_growth as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,pad,returns,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off in returns:
                self.write(va+off,b"\xC2\x04\x00")
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel_call(site,target))
            self.text.update(range(site,site+5))
    def write(self,va,raw):
        for offset,value in enumerate(raw):
            self.memory[va+offset]=value
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase235SyntheticTests(unittest.TestCase):
    @staticmethod
    def check(fake):
        return native.verify_native(fake,check_hash=False,
                                    authentic_provenance=False)

    def test_complete_synthetic_fixture_never_counts_original(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_bytes"],0)
        self.assertEqual(report["checked_sparse_opcode_witnesses"],24)
        self.assertEqual(report["checked_original_CALL_edges"],6)
        self.assertEqual(report["record_strides"],[8,16])
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_sparse_original_witness_is_required(self):
        for name,va,size,digest,pad,ret,anchors in native.METHODS:
            for offset in anchors:
                with self.subTest(name=name,offset=hex(offset)):
                    fake=SyntheticPE()
                    fake.mutate(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_original_RET4_exits_and_INT3_bounds_are_pinned(self):
        for name,va,size,digest,pad,ret,anchors in native.METHODS:
            for offset in ret:
                for byte in range(3):
                    with self.subTest(name=name,ret=hex(offset),byte=byte):
                        fake=SyntheticPE()
                        fake.mutate(va+offset+byte)
                        with self.assertRaises(native.VerificationError):
                            self.check(fake)
            for offset in range(pad):
                with self.subTest(name=name,pad=offset):
                    fake=SyntheticPE()
                    fake.mutate(va+size+offset)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_six_native_CALL_edges_are_pinned(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    fake=SyntheticPE()
                    fake.mutate(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_original_x86_base_and_text_bounds_are_pinned(self):
        fake=SyntheticPE()
        fake.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(fake)
        for name,va,size,*rest in native.METHODS:
            fake=SyntheticPE()
            fake.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.check(fake)

    def test_function_boundaries_and_count(self):
        self.assertEqual(len(native.METHODS),2)
        self.assertEqual(sum(m[2] for m in native.METHODS),226)
        self.assertEqual(native.METHODS[0][4],12)
        self.assertEqual(native.METHODS[1][4],2)
        self.assertEqual(native.METHODS[0][5],(0x59,0x71))
        self.assertEqual(native.METHODS[1][5],(0x56,0x6B))
        self.assertEqual(native.CALLS[-2:],
                         ((0x2410658A,0x24106010),
                          (0x2410662A,0x24106090)))

if __name__=="__main__":
    unittest.main()
