#!/usr/bin/env python3
"""Phase232 synthetic-only mutation tests for four original client methods."""
from __future__ import annotations
import sys
import struct
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_aux_helpers as native

class FakePE:
    """Artificial sparse PE instruction evidence, NEVER an original DLL."""
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        self.rdata=set()
        self.relocations=set()
        for label,va,n,digest,padding,rets,anchors in native.METHODS:
            self.fill(va,n,0)
            self.fill(va+n,padding,0xCC)
            self.text.update(range(va,va+n+padding))
            for offset in rets:
                self.write(va+offset,b"\xC2\x04\x00")
            for offset,hx in anchors.items():
                self.write(va+offset,bytes.fromhex(hx))
        for site,dest in native.CALLS:
            self.write(site,native.rel_call(site,dest))
        self.write(native.WARN_LITERAL_VA,native.WARN_LITERAL)
        self.rdata.update(range(native.WARN_LITERAL_VA,
                                native.WARN_LITERAL_VA+len(native.WARN_LITERAL)))
        self.write(native.WARN_PUSH_VA,b"\x68"+struct.pack("<I",native.WARN_LITERAL_VA))
        self.relocations.add(native.WARN_PUSH_VA+1)
        self.write(native.WARN_IAT_IMMEDIATE_VA-2,
                   b"\xFF\x15"+struct.pack("<I",native.WARN_IAT_TARGET))
        self.relocations.add(native.WARN_IAT_IMMEDIATE_VA)
        for site in native.PVS_GLOBAL_STORES:
            self.write(site,b"\xA3"+struct.pack("<I",native.PVS_GLOBAL_VA))
            self.relocations.add(site+1)

    def fill(self,va,n,value):
        for i in range(n):self.memory[va+i]=value
    def write(self,va,raw):
        for i,byte in enumerate(raw):self.memory[va+i]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.text:return ".text"
        return ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase232Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return native.check_native(fake,check_hash=False,
                original_provenance_verified=False,
                relocations=fake.relocations)

    def test_artificial_opcode_fixture_never_credits_original_recovery(self):
        f=FakePE()
        report=self.verify(f)
        self.assertEqual(report["new_original_complete_SHA_reviewed_methods"],0)
        self.assertEqual(report["new_original_native_x86_bytes"],0)
        self.assertEqual(len(report["original_method_evidence"]),4)
        self.assertEqual(report["direct_original_CALL_rel32_edges"],8)
        self.assertEqual(report["original_absolute_PVS_global_store_count"],2)
        self.assertEqual(report["native_warning_text"],
                         "PVS notifier not in m_PVSNotifierMap\n")
        with self.assertRaises(native.VerificationError):
            native.check_native(f,check_hash=True,
                                original_provenance_verified=False,
                                relocations=f.relocations)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_original_sparse_opcode_witness_is_enforced(self):
        for label,va,n,digest,pad,rets,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(method=label,off=hex(off)):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_return_ABI_and_trailing_INT3_bounds(self):
        for label,va,n,digest,pad,rets,anchors in native.METHODS:
            for off in rets:
                for byte in range(3):
                    with self.subTest(method=label,ret=off,byte=byte):
                        fake=FakePE();fake.flip(va+off+byte)
                        with self.assertRaises(native.VerificationError):
                            self.verify(fake)
            for off in range(pad):
                with self.subTest(method=label,pad=off):
                    fake=FakePE();fake.flip(va+n+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_all_eight_original_nested_CALL_targets(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(callsite=hex(site),byte=byte):
                    fake=FakePE();fake.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_warning_literal_push_and_tier0_import(self):
        for pos in range(len(native.WARN_LITERAL)):
            with self.subTest(text_byte=pos):
                fake=FakePE();fake.flip(native.WARN_LITERAL_VA+pos)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
        for site in (native.WARN_PUSH_VA+1,native.WARN_IAT_IMMEDIATE_VA):
            for pos in range(4):
                with self.subTest(site=hex(site),byte=pos):
                    fake=FakePE();fake.flip(site+pos)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)
            fake=FakePE();fake.relocations.remove(site)
            with self.assertRaises(native.VerificationError):
                self.verify(fake)

    def test_global_PVS_absolute_relocations(self):
        for site in native.PVS_GLOBAL_STORES:
            for byte in range(5):
                fake=FakePE();fake.flip(site+byte)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            fake=FakePE();fake.relocations.remove(site+1)
            with self.assertRaises(native.VerificationError):
                self.verify(fake)

    def test_wrong_original_PE_image_base_and_native_section(self):
        fake=FakePE();fake.base+=0x1000
        with self.assertRaises(native.VerificationError):self.verify(fake)
        for label,va,n,digest,pad,rets,anchors in native.METHODS:
            fake=FakePE();fake.text.remove(va)
            with self.assertRaises(native.VerificationError):self.verify(fake)

    def test_total_four_complete_native_bodies_and_byte_count(self):
        self.assertEqual(len(native.METHODS),4)
        self.assertEqual(sum(row[2] for row in native.METHODS),564)
        self.assertEqual(sum(len(row[-1]) for row in native.METHODS),34)
        self.assertEqual(len(native.CALLS),8)
        self.assertEqual(native.WARN_IAT_TARGET,0x242761E0)
        self.assertEqual(native.PVS_GLOBAL_VA,0x2433FDB8)
        self.assertEqual(sum(len(row[5]) for row in native.METHODS),6)

if __name__=="__main__":
    unittest.main()
