#!/usr/bin/env python3
"""Phase236 synthetic-only PVS map provenance mutation regressions."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_map_frontends as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,digest,pad,ret8,ret4,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for offset in ret8:
                self.write(va+offset,b"\xC2\x08\x00")
            for offset in ret4:
                self.write(va+offset,b"\xC2\x04\x00")
            for offset,hx in anchors.items():
                self.write(va+offset,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.rel_call(site,target))
            self.text.update(range(site,site+5))
        for site in native.INDIRECT_COMPARATOR_CALLS:
            self.write(site,b"\xFF\x17")
            self.text.update(range(site,site+2))
    def write(self,va,raw):
        for i,b in enumerate(raw):
            self.memory[va+i]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def mutate(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase236Tests(unittest.TestCase):
    @staticmethod
    def verify(pe):
        return native.verify_native(pe,check_hash=False,
                                    authentic_provenance=False)

    def test_synthetic_fixture_never_receives_original_credit(self):
        result=self.verify(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(len(result["reviewed_original_methods"]),4)
        self.assertEqual(result["checked_original_direct_CALL_edges"],10)
        self.assertEqual(result["checked_original_indirect_comparator_CALLs"],2)
        self.assertEqual(result["map_record_payload_offset"],8)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_native_sparse_witness_mutation(self):
        for name,va,size,digest,pad,ret8,ret4,anchors in native.METHODS:
            for offset in anchors:
                with self.subTest(name=name,offset=hex(offset)):
                    pe=SyntheticPE()
                    pe.mutate(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_native_RET8_RET4_and_INT3_pad_mutations(self):
        for name,va,size,digest,pad,ret8,ret4,anchors in native.METHODS:
            for off,opcode in ((x,b"\xC2\x08\x00") for x in ret8):
                for byte in range(len(opcode)):
                    with self.subTest(name=name,ret8=off,byte=byte):
                        pe=SyntheticPE()
                        pe.mutate(va+off+byte)
                        with self.assertRaises(native.VerificationError):
                            self.verify(pe)
            for off,opcode in ((x,b"\xC2\x04\x00") for x in ret4):
                for byte in range(len(opcode)):
                    with self.subTest(name=name,ret4=off,byte=byte):
                        pe=SyntheticPE()
                        pe.mutate(va+off+byte)
                        with self.assertRaises(native.VerificationError):
                            self.verify(pe)
            for off in range(pad):
                with self.subTest(name=name,pad=off):
                    pe=SyntheticPE()
                    pe.mutate(va+size+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_ten_direct_CALLs_and_two_indirect_calls(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.mutate(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)
        for site in native.INDIRECT_COMPARATOR_CALLS:
            for byte in range(2):
                with self.subTest(indirect_site=hex(site),byte=byte):
                    pe=SyntheticPE()
                    pe.mutate(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(pe)

    def test_PE_base_and_native_text_boundary(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(pe)
        for name,va,size,*rest in native.METHODS:
            pe=SyntheticPE()
            pe.text.remove(va)
            with self.assertRaises(native.VerificationError):
                self.verify(pe)

    def test_exact_new_original_scope(self):
        self.assertEqual(len(native.METHODS),4)
        self.assertEqual(sum(m[2] for m in native.METHODS),473)
        self.assertEqual(sum(len(m[-1]) for m in native.METHODS),41)
        self.assertEqual(native.METHODS[0][5],(0x61,))
        self.assertEqual(native.METHODS[1][5],(0xEF,0xF8))
        self.assertEqual(native.METHODS[2][6],(0x3A,))
        self.assertEqual(native.METHODS[3][6],(0x3A,))
        self.assertEqual(tuple(m[4] for m in native.METHODS),(12,5,3,3))

if __name__=="__main__":
    unittest.main()
