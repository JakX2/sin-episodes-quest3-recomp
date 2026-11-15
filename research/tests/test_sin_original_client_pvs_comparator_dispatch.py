#!/usr/bin/env python3
"""Phase243 synthetic-only native PE mutation tests, no original credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_comparator_dispatch as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,pad,returns,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off,hx in returns.items():
                self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.call_bytes(site,target))
            self.text.update(range(site,site+5))
        for site,hx in native.EXTERNAL_WITNESSES:
            data=bytes.fromhex(hx)
            self.write(site,data)
            self.text.update(range(site,site+len(data)))
    def write(self,va,raw):
        for i,x in enumerate(raw):
            self.memory[va+i]=x
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase243Tests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(
            pe,check_hash=False,authentic_provenance=False,
            check_relocations=False)

    def test_synthetic_never_receives_original_credit(self):
        v=self.check(SyntheticPE())
        self.assertEqual(v["new_original_complete_SHA_methods"],0)
        self.assertEqual(v["new_original_native_code_bytes"],0)
        self.assertEqual(len(v["reviewed_original_methods"]),4)
        self.assertEqual(v["native_sparse_opcode_witnesses"],26)
        self.assertEqual(v["native_external_owner_witnesses"],5)
        self.assertEqual(v["native_exact_E8_CALLs"],1)
        self.assertEqual(v["native_PE32_HIGHLOW_sites"],0)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_relocations=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_original_code_witness(self):
        for name,va,size,sha,pad,returns,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    f=SyntheticPE()
                    f.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.check(f)

    def test_each_original_return_and_alignment(self):
        for name,va,size,sha,pad,returns,anchors in native.METHODS:
            for off,hx in returns.items():
                for i in range(len(bytes.fromhex(hx))):
                    with self.subTest(name=name,ret=off,byte=i):
                        f=SyntheticPE()
                        f.flip(va+off+i)
                        with self.assertRaises(native.VerificationError):
                            self.check(f)
            for i in range(pad):
                with self.subTest(name=name,pad=i):
                    f=SyntheticPE()
                    f.flip(va+size+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(f)

    def test_original_owner_constructor_global_stores_and_link(self):
        for site,hx in native.EXTERNAL_WITNESSES:
            for i in range(len(bytes.fromhex(hx))):
                with self.subTest(site=hex(site),byte=i):
                    f=SyntheticPE()
                    f.flip(site+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(f)
        for site,dest in native.CALLS:
            for i in range(5):
                f=SyntheticPE()
                f.flip(site+i)
                with self.assertRaises(native.VerificationError):
                    self.check(f)

    def test_section_and_base_gates(self):
        f=SyntheticPE()
        f.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(f)
        for name,va,size,*rest in native.METHODS:
            f=SyntheticPE()
            f.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.check(f)

    def test_entire_original_method_inventory_and_relocations(self):
        self.assertEqual([m[2] for m in native.METHODS],[17,6,20,54])
        self.assertEqual(sum(m[2] for m in native.METHODS),97)
        self.assertEqual([m[4] for m in native.METHODS],[15,10,12,10])
        self.assertEqual(native.CALLS,((0x2410882B,0x24108750),))
        self.assertEqual(native.HIGHLOW_SITES,(
            0x24108235,0x24108279,0x24105CF2,
            0x24105769,0x2410576F,0x24108820,
            0x241088EB,0x2410894F))
        self.assertEqual(native.METHODS[0][3],
         "77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4")

if __name__=="__main__":
    unittest.main()
