#!/usr/bin/env python3
"""Phase245: synthetic-only original-PE mutation regressions."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_pvs_lifecycle_wrappers as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,sha,pad,exits,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off,hx in exits:
                self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel32(0xE8,site,target))
            self.text.update(range(site,site+5))
        site,target=native.TAIL_JMP
        self.write(site,native.rel32(0xE9,site,target))
        self.text.update(range(site,site+5))
    def write(self,va,raw):
        for i,x in enumerate(raw):
            self.memory[va+i]=x
    def at(self,va,length):
        return bytes(self.memory.get(va+i,0) for i in range(length))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase245SyntheticTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(
            pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_bodies_never_receive_original_credit(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),7)
        self.assertEqual(report["sparse_original_opcode_witnesses"],68)
        self.assertEqual(report["verified_original_direct_E8_calls"],17)
        self.assertEqual(report["verified_original_tail_JMPs"],1)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_original_sparse_opcode_witness(self):
        for name,va,size,sha,pad,exits,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    pe=SyntheticPE()
                    pe.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_each_original_exit_and_trailing_INT3_byte(self):
        for name,va,size,sha,pad,exits,anchors in native.METHODS:
            for off,hx in exits:
                for byte in range(len(bytes.fromhex(hx))):
                    with self.subTest(name=name,exit=hex(off),byte=byte):
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

    def test_all_original_E8_direct_edges_and_terminal_tail_JMP(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(call=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
        site,target=native.TAIL_JMP
        for byte in range(5):
            with self.subTest(jmp=hex(site),byte=byte):
                pe=SyntheticPE()
                pe.flip(site+byte)
                with self.assertRaises(native.VerificationError):
                    self.check(pe)

    def test_original_PE_image_base_and_code_sections(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for name,va,size,*unused in native.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va+size-1)
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_no_double_counting_original_catalog_entries(self):
        self.assertEqual(
            [m[1] for m in native.METHODS],
            [0x241061E0,0x24107430,0x241082C0,
             0x241082F0,0x24108320,0x24108360,0x24108390])
        self.assertEqual(
            [m[2] for m in native.METHODS],[209,156,37,40,54,34,42])
        self.assertEqual(sum(m[2] for m in native.METHODS),572)
        self.assertEqual(native.TAIL_JMP,(0x241074C7,0x24128070))
        self.assertEqual(native.CALLS[-2:],(
            (0x2410828F,0x241061E0),
            (0x241082A3,0x24107430)))
        self.assertEqual(native.METHODS[1][3],
          "ba31e5b972b0d2a3242e636970ad553c1df5e7e72ab49f3d0ebfd48aa99b8787")

if __name__=="__main__":
    unittest.main()
