#!/usr/bin/env python3
"""Phase233 synthetic only: native PVS 16-byte manager evidence mutations.

The artificial PE fixtures must never increment original recovered-function
counts. Only whole-owner-original DLL SHA256 plus whole-code body hashes do.
"""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_sixteen_list as native


class FakePE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,sha,pad,ret0,ret4,witnesses in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size+pad))
            for offset in ret0:
                self.write(va+offset,b"\xC3")
            for offset in ret4:
                self.write(va+offset,b"\xC2\x04\x00")
            for offset,hx in witnesses.items():
                self.write(va+offset,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel_call(site,target))
            self.text.update(range(site,site+5))

    def write(self,va,raw):
        for i,b in enumerate(raw):
            self.memory[va+i]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1


class Phase233Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return native.verify_native(fake,check_hash=False,
                                    authentic_provenance=False)

    def test_positive_synthetic_evidence_but_no_original_credit(self):
        fake=FakePE()
        report=self.verify(fake)
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),2)
        self.assertEqual(report["checked_original_DIRECT_CALL_edges"],4)
        self.assertEqual(report["original_record_size"],16)
        self.assertEqual(report["original_node_prev_WORD_offset"],0xC)
        self.assertEqual(report["original_node_next_WORD_offset"],0xE)
        with self.assertRaises(native.VerificationError):
            native.verify_native(fake,check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_sparse_original_opcode_instruction_witness(self):
        for name,va,size,sha,pad,ret0,ret4,witnesses in native.METHODS:
            for offset in witnesses:
                with self.subTest(method=name,offset=offset):
                    fake=FakePE()
                    fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_every_original_return_branch_and_int3_pad(self):
        for name,va,size,sha,pad,ret0,ret4,witnesses in native.METHODS:
            for offset in ret0:
                fake=FakePE()
                fake.flip(va+offset)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for offset in ret4:
                for byte in range(3):
                    with self.subTest(method=name,ret4=offset,byte=byte):
                        fake=FakePE()
                        fake.flip(va+offset+byte)
                        with self.assertRaises(native.VerificationError):
                            self.verify(fake)
            for i in range(pad):
                with self.subTest(method=name,pad=i):
                    fake=FakePE()
                    fake.flip(va+size+i)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_four_true_native_nested_and_parent_CALLs(self):
        for site,dest in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    fake=FakePE()
                    fake.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_native_PE_base_and_text_section(self):
        fake=FakePE()
        fake.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        for name,va,size,sha,pad,ret0,ret4,witnesses in native.METHODS:
            fake=FakePE()
            fake.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.verify(fake)

    def test_exact_whole_original_review_scope(self):
        self.assertEqual(len(native.METHODS),2)
        self.assertEqual(sum(m[2] for m in native.METHODS),236)
        self.assertEqual(sum(len(m[-1]) for m in native.METHODS),24)
        self.assertEqual(len(native.CALLS),4)
        self.assertEqual(native.METHODS[0][1],0x241086F0)
        self.assertEqual(native.METHODS[1][1],0x24106CB0)
        self.assertEqual(native.METHODS[0][4],0)  # contiguous next method
        self.assertEqual(native.METHODS[1][4],4)  # native CC padding
        self.assertEqual(native.METHODS[0][5],(0x47,0x5F))
        self.assertEqual(native.METHODS[1][6],(0x6F,0x89))

if __name__=="__main__":
    unittest.main()
