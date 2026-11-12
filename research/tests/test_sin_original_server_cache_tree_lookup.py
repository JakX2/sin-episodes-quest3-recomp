#!/usr/bin/env python3
"""Original-free mutation tests for SHA-pinned Phase191 x86 tree lookups."""
from __future__ import annotations
import struct
import unittest
from audit_sin_original_server_cache_tree_lookup import (
    REGIONS,WITNESSES,SENTINEL_OFFSETS,INDIRECT_COMPARE_OFFSETS,RET8_OFFSETS,
    verify_native,audit,VerificationError)

class SyntheticPE:
    base=0x22000000
    def __init__(self):
        self.mem={}
        for name,(entry,size,_,sentinel) in REGIONS.items():
            for offset,value in WITNESSES.items():
                self.place(entry+offset,bytes.fromhex(value))
            for offset,delta in SENTINEL_OFFSETS.items():
                self.place(entry+offset,struct.pack('<I',sentinel+delta))
            for offset in INDIRECT_COMPARE_OFFSETS:
                self.place(entry+offset,b'\xff\x17')
            for offset in RET8_OFFSETS:
                self.place(entry+offset,b'\xc2\x08\x00')
            self.place(entry+size,b'\xcc\xcc')
    def place(self,addr,code):
        for offset,byte in enumerate(code):
            location=addr+offset
            if location in self.mem and self.mem[location]!=byte:
                raise AssertionError('synthetic witness overlap %#x'%location)
            self.mem[location]=byte
    def at(self,addr,length):
        return bytes(self.mem.get(addr+i,0) for i in range(length))
    def section(self,addr):
        if 0x22500000<=addr<0x22600000:return '.data'
        return '.text'
    def corrupt(self,addr):
        self.mem[addr]=self.mem.get(addr,0)^1

class Cases(unittest.TestCase):
    def test_synthetic_witnesses_pass_without_native_function_claim(self):
        report=verify_native(SyntheticPE(),check_hash=False)
        self.assertEqual(report['new_sha_verified_function_count'],0)
        self.assertEqual(report['new_sha_verified_native_bytes'],0)
        self.assertEqual(report['independent_sparse_instruction_witnesses'],72)
        self.assertEqual(report['absolute_sentinel_references'],28)
    def test_independently_mutate_every_sparse_witness(self):
        for name,(va,_,_,_) in REGIONS.items():
            for off in WITNESSES:
                with self.subTest(name=name,off=off):
                    fake=SyntheticPE();fake.corrupt(va+off)
                    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_independently_mutate_every_sentinel_reference(self):
        for name,(va,_,_,_) in REGIONS.items():
            for off in SENTINEL_OFFSETS:
                with self.subTest(name=name,off=off):
                    fake=SyntheticPE();fake.corrupt(va+off)
                    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_indirect_comparator_and_ret8_gate(self):
        for name,(va,_,_,_) in REGIONS.items():
            for off in (*INDIRECT_COMPARE_OFFSETS,*RET8_OFFSETS):
                with self.subTest(name=name,off=off):
                    fake=SyntheticPE();fake.corrupt(va+off)
                    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_boundary_marker(self):
        for name,(va,size,_,_) in REGIONS.items():
            fake=SyntheticPE();fake.corrupt(va+size)
            with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_full_normalized_equality_detects_unreviewed_difference(self):
        fake=SyntheticPE();fake.corrupt(REGIONS['scene'][0]+0x1f)
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_wrong_image_base_and_section(self):
        fake=SyntheticPE();fake.base=0x24000000
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
        fake=SyntheticPE();fake.section=lambda _:'.text'
        with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
    def test_synthetic_cannot_pass_original_whole_hashes(self):
        with self.assertRaises(VerificationError):verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
