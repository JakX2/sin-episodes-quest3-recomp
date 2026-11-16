#!/usr/bin/env python3
"""Phase244 synthetic-only original-code witness mutation regressions."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_node_fields_list_reset as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.code=set()
        for name,va,size,sha,pad,rets,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.code.update(range(va,va+size))
            self.write(va+size,b"\xCC"*pad)
            for off,hx in rets:
                self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.e8(site,target))
            self.code.update(range(site,site+5))
    def write(self,va,data):
        for offset,value in enumerate(data):
            self.memory[va+offset]=value
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.code else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase244SyntheticTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_cannot_claim_original_review(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),8)
        self.assertEqual(report["sparse_native_opcode_witnesses"],38)
        self.assertEqual(report["exact_original_direct_E8_edges"],3)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_sparse_original_opcode_witness(self):
        for name,va,size,sha,pad,rets,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_all_native_ret_and_int3_padding_bytes(self):
        for name,va,size,sha,pad,rets,anchors in native.METHODS:
            for off,hx in rets:
                for byte in range(len(bytes.fromhex(hx))):
                    with self.subTest(name=name,ret=off,byte=byte):
                        pe=SyntheticPE()
                        pe.flip(va+off+byte)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)
            for byte in range(pad):
                with self.subTest(name=name,padding=byte):
                    pe=SyntheticPE()
                    pe.flip(va+size+byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_all_direct_call_edges(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_pe_base_and_code_boundaries(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for name,va,size,*more in native.METHODS:
            pe=SyntheticPE()
            pe.code.remove(va+size-1)
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_exact_authentic_scope(self):
        self.assertEqual(len(native.METHODS),8)
        self.assertEqual([m[2] for m in native.METHODS],
                         [18,81,46,24,23,24,17,24])
        self.assertEqual(sum(m[2] for m in native.METHODS),257)
        self.assertEqual(native.CALLS,(
            (0x24105FA5,0x24105F40),
            (0x241069B3,0x24105DD0),
            (0x241074A3,0x24105DD0)
        ))
        self.assertEqual(native.METHODS[1][3],
         "be605e319bc1bc6c2ac86feea8b724e3b9d051d4f59dc4752deb527031d106a9")

if __name__=="__main__":
    unittest.main()
