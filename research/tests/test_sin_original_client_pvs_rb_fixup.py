#!/usr/bin/env python3
"""Phase240 synthetic-only PE mutation tests; never original recovery credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_rb_fixup as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,pad,rets,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off in rets:
                self.write(va+off,b"\xC2\x04\x00")
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel32_call(site,target))
            self.text.update(range(site,site+5))
    def write(self,va,raw):
        for off,value in enumerate(raw):
            self.memory[va+off]=value
    def at(self,va,length):
        return bytes(self.memory.get(va+i,0) for i in range(length))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase240Tests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(
            pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_evidence_never_earns_original_method_credit(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),3)
        self.assertEqual(report["original_sparse_opcode_witnesses"],19)
        self.assertEqual(report["original_E8_rel32_CALL_edges"],11)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_original_sparse_native_witness(self):
        for name,va,size,digest,pad,rets,anchors in native.METHODS:
            for offset in anchors:
                with self.subTest(name=name,offset=hex(offset)):
                    pe=SyntheticPE()
                    pe.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_original_RET4_and_INT3_boundaries(self):
        for name,va,size,digest,pad,rets,anchors in native.METHODS:
            for off in rets:
                for i in range(3):
                    with self.subTest(name=name,ret=off,byte=i):
                        pe=SyntheticPE()
                        pe.flip(va+off+i)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)
            for i in range(pad):
                with self.subTest(name=name,padding=i):
                    pe=SyntheticPE()
                    pe.flip(va+size+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_four_original_rotation_calls_three_attach_calls_and_resolvers(self):
        self.assertEqual(native.CALLS[:4],(
            (0x24107137,0x241067C0),
            (0x241071EF,0x24106D40),
            (0x24107313,0x24106D40),
            (0x241073CB,0x241067C0)))
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_original_PE_base_and_code_section(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for name,va,size,*remaining in native.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_exact_new_original_scope(self):
        self.assertEqual(len(native.METHODS),3)
        self.assertEqual(sum(m[2] for m in native.METHODS),1947)
        self.assertEqual(tuple(m[4] for m in native.METHODS),(9,6,6))
        self.assertEqual(tuple(m[5] for m in native.METHODS),
                         ((0xE4,),(0xE7,),(0x5C7,)))
        self.assertEqual(native.METHODS[2][3],
          "f778c9c8b49e5a2f40cb79694d3c0a7191da19137edea216d456c31705775e81")

if __name__=="__main__":
    unittest.main()
