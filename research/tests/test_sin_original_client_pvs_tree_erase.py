#!/usr/bin/env python3
"""Phase241 synthetic-only PE mutation tests, never genuine native credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_tree_erase as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        va=native.ENTRY
        self.write(va,bytes(native.NATIVE_BYTES))
        self.text.update(range(va,va+native.NATIVE_BYTES))
        self.write(va+native.NATIVE_BYTES,
                   b"\xCC"*native.TRAILING_INT3_BYTES)
        self.write(va+native.RET4_OFFSET,b"\xC2\x04\x00")
        for offset,hx in native.WITNESSES.items():
            self.write(va+offset,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.native_call_bytes(site,target))
            self.text.update(range(site,site+5))
    def write(self,va,raw):
        for offset,byte in enumerate(raw):
            self.memory[va+offset]=byte
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase241SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(
            pe,check_hash=False,authentic_provenance=False)

    def test_artificial_fixture_cannot_earn_original_credit(self):
        result=self.check(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(result["original_method_bytes"],693)
        self.assertEqual(result["original_sparse_opcode_witnesses"],21)
        self.assertEqual(result["original_direct_E8_CALL_edges"],27)
        self.assertEqual(result["original_erase_fixup_VA"],"0x24107750")
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_original_sparse_native_instruction(self):
        for off in native.WITNESSES:
            with self.subTest(offset=hex(off)):
                fake=SyntheticPE()
                fake.flip(native.ENTRY+off)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_RET4_and_all_native_INT3_boundary_bytes(self):
        for i in range(3):
            with self.subTest(return_byte=i):
                fake=SyntheticPE()
                fake.flip(native.ENTRY+native.RET4_OFFSET+i)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)
        for i in range(native.TRAILING_INT3_BYTES):
            with self.subTest(trailing_INT3=i):
                fake=SyntheticPE()
                fake.flip(native.ENTRY+native.NATIVE_BYTES+i)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_all_27_original_E8_rel32_edges(self):
        for site,target in native.CALLS:
            for i in range(5):
                with self.subTest(site=hex(site),byte=i):
                    fake=SyntheticPE()
                    fake.flip(site+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_original_PE_image_base_and_code_section(self):
        fake=SyntheticPE()
        fake.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(fake)
        for off in (0,native.NATIVE_BYTES-1):
            fake=SyntheticPE()
            fake.text.remove(native.ENTRY+off)
            with self.assertRaises(native.VerificationError):
                self.check(fake)

    def test_one_original_function_scope(self):
        self.assertEqual(native.ENTRY,0x241083C0)
        self.assertEqual(native.NATIVE_BYTES,693)
        self.assertEqual(native.RET4_OFFSET,0x2B2)
        self.assertEqual(native.TRAILING_INT3_BYTES,11)
        self.assertEqual(len(native.NODE_RESOLVER_CALLS),24)
        self.assertEqual(native.CALLS[-3:],(
            (0x24108669,0x24107750),
            (0x241087D0,0x241083C0),
            (0x24108850,0x241083C0)
        ))
        self.assertEqual(native.WHOLE_ORIGINAL_CODE_SHA256,
          "887ac931d635a7b9e913bb3c68128628eb40a893b626907230f872df0304419d")

if __name__=="__main__":
    unittest.main()
