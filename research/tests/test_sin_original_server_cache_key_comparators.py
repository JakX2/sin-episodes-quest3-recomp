#!/usr/bin/env python3
"""Synthetic negative/mutation tests; never pretend fixture is original DLL."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_cache_key_comparators import (
    FUNCTIONS,WITNESSES,LENGTH,BOUNDARY,VerificationError,verify_native,audit)

class SyntheticPE:
    base=0x22000000
    def __init__(self):
        self.memory={}
        for va,slot in FUNCTIONS.values():
            for off,code in WITNESSES.items():
                for i,value in enumerate(bytes.fromhex(code)):
                    self.put(va+off+i,value)
            for i,value in enumerate(BOUNDARY):
                self.put(va+LENGTH+i,value)
            for i,value in enumerate(struct.pack('<I',va)):
                self.put(slot+i,value)
    def put(self,addr,value):
        if addr in self.memory and self.memory[addr]!=value:
            raise AssertionError('conflicting witness')
        self.memory[addr]=value
    def at(self,addr,length):
        return bytes(self.memory.get(addr+i,0) for i in range(length))
    def section(self,addr):
        return '.data' if 0x224d6000<=addr<0x22540000 else '.text'
    def flip(self,addr):
        self.memory[addr]=self.memory.get(addr,0)^1

class Tests(unittest.TestCase):
    def test_fixture_models_but_does_not_claim_native_bytes(self):
        report=verify_native(SyntheticPE(),check_hash=False)
        self.assertEqual(report['new_native_functions'],0)
        self.assertEqual(report['new_original_x86_bytes'],0)
        self.assertEqual(report['sparse_witnesses'],10)
    def test_mutate_every_native_witness(self):
        for name,(va,_) in FUNCTIONS.items():
            for offset in WITNESSES:
                with self.subTest(name=name,offset=offset):
                    fake=SyntheticPE();fake.flip(va+offset)
                    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_mutate_exact_static_comparator_ptr(self):
        for name,(_,slot) in FUNCTIONS.items():
            with self.subTest(name=name):
                fake=SyntheticPE();fake.flip(slot)
                with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_mutate_boundary(self):
        for name,(va,_) in FUNCTIONS.items():
            with self.subTest(name=name):
                fake=SyntheticPE();fake.flip(va+LENGTH)
                with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_detect_unwitnessed_difference_between_comparators(self):
        fake=SyntheticPE();fake.flip(FUNCTIONS['scene'][0]+4)
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_reject_wrong_image_base_or_sections(self):
        fake=SyntheticPE();fake.base=0x24000000
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
        fake=SyntheticPE();fake.section=lambda _:'.text'
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_never_accept_synthetic_full_method_hash(self):
        with self.assertRaises(VerificationError):verify_native(SyntheticPE(),check_hash=True)
    def test_never_accept_fake_whole_dlls(self):
        with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
