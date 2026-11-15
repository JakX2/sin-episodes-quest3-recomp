#!/usr/bin/env python3
"""Phase238 synthetic-only original PE witness mutation tests."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_map_node_growth as original

class SyntheticPE:
    def __init__(self):
        self.base=original.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,pad,ret4,anchors in original.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off in ret4:
                self.write(va+off,b"\xC2\x04\x00")
            for off,encoded in anchors.items():
                self.write(va+off,bytes.fromhex(encoded))
        for site,target in original.CALLS:
            self.write(site,original.call_bytes(site,target))
            self.text.update(range(site,site+5))
    def write(self,va,data):
        for off,byte in enumerate(data):
            self.memory[va+off]=byte
    def at(self,va,length):
        return bytes(self.memory.get(va+i,0) for i in range(length))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase238SyntheticEvidenceTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return original.verify_native(
            pe,check_hash=False,authentic_provenance=False
        )

    def test_synthetic_fixture_never_counts_original(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(report["original_sparse_opcode_witnesses"],36)
        self.assertEqual(report["original_direct_CALL_edges"],7)
        self.assertEqual(len(report["reviewed_original_methods"]),2)
        self.assertEqual(report["resolver_sentinel_header_WORDs"],
                         [0xFFFF,0xFFFF,0xFFFF,1])
        with self.assertRaises(original.VerificationError):
            original.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(original.VerificationError):
            original.audit(b"MZ",b"MZ")

    def test_every_original_sparse_opcode_is_pinned(self):
        for name,va,size,digest,pad,ret4,anchors in original.METHODS:
            for off in anchors:
                with self.subTest(method=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.mutate(va+off)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_all_native_RET4_and_INT3_boundaries_are_pinned(self):
        for name,va,size,digest,pad,ret4,anchors in original.METHODS:
            for off in ret4:
                for i in range(3):
                    with self.subTest(method=name,return_offset=hex(off),byte=i):
                        pe=SyntheticPE()
                        pe.mutate(va+off+i)
                        with self.assertRaises(original.VerificationError):
                            self.check(pe)
            for i in range(pad):
                with self.subTest(method=name,padding=i):
                    pe=SyntheticPE()
                    pe.mutate(va+size+i)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_all_native_direct_CALL_rel32_are_pinned(self):
        for site,target in original.CALLS:
            for i in range(5):
                with self.subTest(site=hex(site),byte=i):
                    pe=SyntheticPE()
                    pe.mutate(site+i)
                    with self.assertRaises(original.VerificationError):
                        self.check(pe)

    def test_original_PE_base_and_method_section_are_pinned(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(original.VerificationError):
            self.check(pe)
        for name,va,size,*rest in original.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va)
            with self.assertRaises(original.VerificationError):
                self.check(pe)

    def test_exact_original_sha_credit_scope(self):
        self.assertEqual(len(original.METHODS),2)
        self.assertEqual(sum(m[2] for m in original.METHODS),191)
        self.assertEqual(original.METHODS[0][4],15)
        self.assertEqual(original.METHODS[1][4],2)
        self.assertEqual(original.METHODS[0][5],(0x46,0x4E))
        self.assertEqual(original.METHODS[1][5],(0x56,0x6B))
        self.assertEqual(original.CALLS[-2:],
                         ((0x2410618B,0x2416F960),
                          (0x241061A0,0x2416F8F0)))

if __name__=="__main__":
    unittest.main()
