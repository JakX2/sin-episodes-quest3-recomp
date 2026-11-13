#!/usr/bin/env python3
"""Phase198 original-free x86 rotation witness and mutation tests."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_cache_tree_rotations import (
    REGIONS,CALL_OFFSETS,WITNESSES,INCOMING,FOLLOWING_RIGHT,
    rel_call,verify_native,audit,VerificationError,
)

class FakePE:
    base=0x22000000
    def __init__(self):
        self.mem={}
        for name,(va,n,_,resolver) in REGIONS.items():
            direction=name.rsplit("_",1)[1]
            for off,hexcode in WITNESSES[direction].items():
                self.place(va+off,bytes.fromhex(hexcode))
            for off in CALL_OFFSETS[direction]:
                self.place(va+off,rel_call(va+off,resolver))
            if direction=="left":
                self.place(va+n,b"\xcc"*3)
            else:
                self.place(va+n,FOLLOWING_RIGHT[name])
        for src,dst in INCOMING.items():
            self.place(src,rel_call(src,dst))
    def place(self,va,code):
        for offset,byte in enumerate(code):
            addr=va+offset
            if addr in self.mem and self.mem[addr]!=byte:
                raise AssertionError("synthetic overlapping witness mismatch at %#x"%addr)
            self.mem[addr]=byte
    def at(self,va,length):
        return bytes(self.mem.get(va+i,0) for i in range(length))
    def section(self,va):
        return ".text"
    def mutate(self,va):
        self.mem[va]=self.mem.get(va,0)^1

class Tests(unittest.TestCase):
    def test_synthetic_witnesses_do_not_claim_original_method_recovery(self):
        report=verify_native(FakePE(),check_hash=False)
        self.assertEqual(report["new_sha_verified_original_functions"],0)
        self.assertEqual(report["new_original_native_function_bytes"],0)
        self.assertEqual(report["sparse_opcode_witnesses"],72)
        self.assertEqual(report["exact_resolver_calls"],36)
        self.assertEqual(report["exact_balancer_incoming_calls"],8)
    def test_each_sparse_x86_witness(self):
        for name,(va,_,_,_) in REGIONS.items():
            for off in WITNESSES[name.rsplit("_",1)[1]]:
                with self.subTest(name=name,offset=off):
                    fake=FakePE();fake.mutate(va+off)
                    with self.assertRaises(VerificationError):
                        verify_native(fake,check_hash=False)
    def test_every_original_resolver_call(self):
        for name,(va,_,_,_) in REGIONS.items():
            for off in CALL_OFFSETS[name.rsplit("_",1)[1]]:
                with self.subTest(name=name,offset=off):
                    fake=FakePE();fake.mutate(va+off+1)
                    with self.assertRaises(VerificationError):
                        verify_native(fake,check_hash=False)
    def test_every_original_balancer_incoming_call(self):
        for src in INCOMING:
            with self.subTest(site=src):
                fake=FakePE();fake.mutate(src+1)
                with self.assertRaises(VerificationError):
                    verify_native(fake,check_hash=False)
    def test_native_return_and_method_boundary(self):
        for name,(va,n,_,_) in REGIONS.items():
            with self.subTest(name=name):
                fake=FakePE();fake.mutate(va+n)
                with self.assertRaises(VerificationError):
                    verify_native(fake,check_hash=False)
    def test_unwitnessed_method_divergence(self):
        fake=FakePE();fake.mutate(REGIONS["scene_left"][0]+0x71)
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)
        fake=FakePE();fake.mutate(REGIONS["modelsounds_right"][0]+0x70)
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)
    def test_wrong_image_base_and_section(self):
        fake=FakePE();fake.base=0x24000000
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)
        fake=FakePE();fake.section=lambda _:".rdata"
        with self.assertRaises(VerificationError):
            verify_native(fake,check_hash=False)
    def test_fabricated_evidence_is_never_original(self):
        with self.assertRaises(VerificationError):
            verify_native(FakePE(),check_hash=True)
        with self.assertRaises(VerificationError):
            audit(b"MZ",b"MZ")

if __name__=="__main__":
    unittest.main()
