#!/usr/bin/env python3
"""Phase251 synthetic-only sparse original-PE ABI mutation tests.

Never award original code review credit from artificial fixtures: only the
whole original retail DLL pair + all full authentic native SHA256s can do so.
"""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_recvprop_proxies_factories as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.sections={}
        for name,va,size,sha,padding,exit_spec,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.label(va,size,".text")
            self.write(va+size,b"\xCC"*padding)
            off,hx=exit_spec
            self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for opcode,edges in ((0xE8,native.DIRECT_CALLS),
                             (0xE9,native.DIRECT_TAIL_JMPS)):
            for site,target in edges:
                self.write(site,native.rel32(opcode,site,target))
                self.label(site,5,".text")
        for site,target in native.RELOCATED_POINTERS:
            self.write(site,target.to_bytes(4,"little"))
            self.label(site,4,".text")
    def write(self,va,content):
        for i,b in enumerate(content):
            self.memory[va+i]=b
    def label(self,va,length,section):
        for i in range(length):
            self.sections[va+i]=section
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return self.sections.get(va,"")
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase251SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(pe,check_hash=False,
                                    authentic_provenance=False,
                                    check_relocations=False)

    def test_synthetic_cannot_claim_original_retail_review(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),12)
        self.assertEqual(report["original_sparse_opcode_witnesses"],61)
        self.assertEqual(report["original_native_return_paths"],12)
        self.assertEqual(report["original_INT3_alignment_bytes"],94)
        self.assertEqual(report["original_direct_E8_edges"],6)
        self.assertEqual(report["original_tail_E9_edges"],1)
        self.assertEqual(report["original_absolute_proxy_pointers"],8)
        self.assertEqual(report["original_HIGHLOW_proxy_relocations"],0)
        self.assertTrue(report["original_float_proxy_is_bitwise_DWORD_copy"])
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True,
                                 check_relocations=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_sparse_original_native_opcode_witness(self):
        for name,va,size,sha,pad,exit_spec,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_every_original_return_and_INT3_boundary_byte(self):
        for name,va,size,sha,pad,exit_spec,anchors in native.METHODS:
            off,hx=exit_spec
            for j in range(len(bytes.fromhex(hx))):
                with self.subTest(name=name,exit_byte=j):
                    pe=SyntheticPE()
                    pe.flip(va+off+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
            for j in range(pad):
                with self.subTest(name=name,int3_byte=j):
                    pe=SyntheticPE()
                    pe.flip(va+size+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_all_exact_original_phase250_direct_CALL_and_tail_JMP_edges(self):
        for opcode,edges in ((0xE8,native.DIRECT_CALLS),
                             (0xE9,native.DIRECT_TAIL_JMPS)):
            for site,target in edges:
                for j in range(5):
                    with self.subTest(site=hex(site),byte=j):
                        pe=SyntheticPE()
                        pe.flip(site+j)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_all_genuine_proxy_function_pointer_immediates(self):
        for site,target in native.RELOCATED_POINTERS:
            for j in range(4):
                with self.subTest(site=hex(site),byte=j):
                    pe=SyntheticPE()
                    pe.flip(site+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
            pe=SyntheticPE()
            pe.sections[site]=".data"
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_original_image_base_and_whole_method_text_boundaries(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for name,va,size,*rest in native.METHODS:
            pe=SyntheticPE()
            pe.sections[va+size-1]=""
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_exact_original_inventory_and_precise_abi_claims(self):
        self.assertEqual([m[1] for m in native.METHODS],[
            0x24126A20,0x24126A30,0x24126C40,0x24126C50,
            0x24126C70,0x24126C80,0x24126C90,0x24126D00,
            0x24126D30,0x24126D50,0x24126D80,0x24126DF0])
        self.assertEqual([m[2] for m in native.METHODS],[
            1,34,14,26,14,16,14,37,22,36,106,146])
        self.assertEqual(sum(m[2] for m in native.METHODS),466)
        self.assertEqual(sum(m[4] for m in native.METHODS),94)
        self.assertEqual(native.DIRECT_TAIL_JMPS,
                         ((0x242736B5,0x24126A20),))
        self.assertEqual(native.RELOCATED_POINTERS[:5],(
            (0x24126D04,0x24126C70),
            (0x24126D0B,0x24126C80),
            (0x24126D12,0x24126C90),
            (0x24126D19,0x24126C40),
            (0x24126D20,0x24126C50)))
        self.assertEqual(native.METHODS[10][3],
            "bd1fa7c7ccdc0a278ee0ad889437803cc195d10aa2bd53040b5b66f5f8b97257")

if __name__=="__main__":
    unittest.main()
