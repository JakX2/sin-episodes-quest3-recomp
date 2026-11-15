#!/usr/bin/env python3
"""Phase237 synthetic-only PVS map placement/index audit mutation tests."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import audit_sin_original_client_pvs_map_search_index as native

class SyntheticPE:
    def __init__(self):
        self.base = native.ORIGINALS["client"]["base"]
        self.memory = {}
        self.text = set()
        for name,va,size,sha,pad,ret16,ret0,anchors in native.METHODS:
            self.write(va, bytes(size))
            self.write(va+size, b"\xCC"*pad)
            self.text.update(range(va, va+size))
            for off in ret16:
                self.write(va+off, b"\xC2\x10\x00")
            for off in ret0:
                self.write(va+off, b"\xC3")
            for off,hx in anchors.items():
                self.write(va+off, bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel_call(site,target))
            self.text.update(range(site,site+5))
        self.write(native.COMPARATOR_INDIRECT_SITE,
                   native.COMPARATOR_INDIRECT_OPCODE)
        self.text.update(range(native.COMPARATOR_INDIRECT_SITE,
                               native.COMPARATOR_INDIRECT_SITE+3))
    def write(self,va,data):
        for off,byte in enumerate(data):
            self.memory[va+off] = byte
    def at(self,va,length):
        return bytes(self.memory.get(va+i,0) for i in range(length))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va] = self.memory.get(va,0)^1

class TestPhase237SyntheticEvidence(unittest.TestCase):
    @staticmethod
    def verify(pe):
        return native.verify_native(pe,check_hash=False,
                                    authentic_provenance=False)

    def test_synthetic_only_never_earns_native_recovery_credit(self):
        report=self.verify(SyntheticPE())
        self.assertEqual(report["new_original_complete_SHA_methods"],0)
        self.assertEqual(report["new_original_native_code_bytes"],0)
        self.assertEqual(len(report["reviewed_original_methods"]),2)
        self.assertEqual(report["original_sparse_instruction_witnesses"],30)
        self.assertEqual(report["original_direct_E8_CALL_edges"],4)
        self.assertEqual(report["original_indirect_comparator_CALLs"],1)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_all_sparse_opcodes_are_pinned(self):
        for name,va,size,sha,pad,ret16,ret0,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=off):
                    pe=SyntheticPE()
                    pe.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_native_returns_and_int3_boundaries(self):
        for name,va,size,sha,pad,ret16,ret0,anchors in native.METHODS:
            for off in ret16:
                for i in range(3):
                    with self.subTest(name=name,return_off=off,byte=i):
                        pe=SyntheticPE()
                        pe.flip(va+off+i)
                        with self.assertRaises(native.VerificationError):
                            self.verify(pe)
            for off in ret0:
                with self.subTest(name=name,return_off=off):
                    pe=SyntheticPE()
                    pe.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)
            for i in range(pad):
                with self.subTest(name=name,INT3=i):
                    pe=SyntheticPE()
                    pe.flip(va+size+i)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_all_original_E8_calls_are_pinned(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_indirect_callback_is_pinned(self):
        for i in range(len(native.COMPARATOR_INDIRECT_OPCODE)):
            pe=SyntheticPE()
            pe.flip(native.COMPARATOR_INDIRECT_SITE+i)
            with self.assertRaises(native.VerificationError):
                self.verify(pe)

    def test_original_PE_base_and_text_bounds(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(pe)
        for name,va,size,*rest in native.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.verify(pe)

    def test_exact_native_scope(self):
        self.assertEqual(len(native.METHODS),2)
        self.assertEqual(sum(m[2] for m in native.METHODS),341)
        self.assertEqual(native.METHODS[0][5],(0xFC,))
        self.assertEqual(native.METHODS[1][6],(0x37,0x55))
        self.assertEqual(tuple(m[4] for m in native.METHODS),(1,10))
        self.assertEqual(native.CALLS,
                         ((0x2410671C,0x24106140),
                          (0x2410673D,0x24105F40),
                          (0x2410876B,0x24106440),
                          (0x24108772,0x24106700)))

if __name__=="__main__":
    unittest.main()
