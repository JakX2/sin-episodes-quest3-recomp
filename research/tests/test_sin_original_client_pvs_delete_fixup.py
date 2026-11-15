#!/usr/bin/env python3
"""Phase242 synthetic-only PE mutation regressions: no original-code credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_delete_fixup as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        va=native.ENTRY
        self.write(va,bytes(native.LENGTH))
        self.write(va+native.LENGTH,b"\xCC"*native.TRAILING_INT3)
        self.text.update(range(va,va+native.LENGTH))
        for off in native.RET4_OFFSETS:
            self.write(va+off,b"\xC2\x04\x00")
        for off,hexbytes in native.WITNESSES.items():
            self.write(va+off,bytes.fromhex(hexbytes))
        for site,target in native.CALLS:
            self.write(site,native.native_e8(site,target))
            self.text.update(range(site,site+5))
    def write(self,va,raw):
        for i,x in enumerate(raw):
            self.memory[va+i]=x
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase242SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_pe_never_earns_original_credit(self):
        report=self.check(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(report["sparse_opcode_witnesses"],25)
        self.assertEqual(report["native_E8_CALL_edges"],7)
        self.assertEqual(report["rotation_calls_left"],3)
        self.assertEqual(report["rotation_calls_right"],3)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_sparse_original_instruction_is_required(self):
        for off in native.WITNESSES:
            with self.subTest(offset=hex(off)):
                pe=SyntheticPE()
                pe.mutate(native.ENTRY+off)
                with self.assertRaises(native.VerificationError):
                    self.check(pe)

    def test_exact_RET4_and_all_15_padding_bytes(self):
        for off in native.RET4_OFFSETS:
            for i in range(3):
                with self.subTest(ret=hex(off),byte=i):
                    pe=SyntheticPE()
                    pe.mutate(native.ENTRY+off+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
        for i in range(native.TRAILING_INT3):
            with self.subTest(INT3=i):
                pe=SyntheticPE()
                pe.mutate(native.ENTRY+native.LENGTH+i)
                with self.assertRaises(native.VerificationError):
                    self.check(pe)

    def test_all_original_E8_rel32_destination_bytes(self):
        for site,target in native.CALLS:
            for i in range(5):
                with self.subTest(site=hex(site),byte=i):
                    pe=SyntheticPE()
                    pe.mutate(site+i)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_PE_base_and_code_section_required(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for va in (native.ENTRY,native.ENTRY+native.LENGTH-1):
            pe=SyntheticPE()
            pe.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_complete_original_code_scope(self):
        self.assertEqual(native.ENTRY,0x24107750)
        self.assertEqual(native.LENGTH,2609)
        self.assertEqual(native.TRAILING_INT3,15)
        self.assertEqual(native.RET4_OFFSETS,(0xA2E,))
        self.assertEqual(native.CALLS,(
            (0x241078E6,0x241067C0),
            (0x24107B81,0x24106D40),
            (0x24107CBF,0x241067C0),
            (0x24107D70,0x24106D40),
            (0x2410800A,0x241067C0),
            (0x24108146,0x24106D40),
            (0x24108669,0x24107750)))
        self.assertEqual(native.CODE_SHA256,
             "7bed48e41a436fb3a0e2fc69e69b3cfe83c42aee02be58f5fd6872e4c7f0c12a")

if __name__=="__main__":
    unittest.main()
