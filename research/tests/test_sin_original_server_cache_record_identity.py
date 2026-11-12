#!/usr/bin/env python3
"""Original-binary-free Phase190 native witness and mutation tests."""
from __future__ import annotations
import unittest
from audit_sin_original_server_cache_record_identity import (
    REGIONS,COMMON,DETAIL,CALLS,INCOMING,verify_native,rel_call,audit,VerificationError
)

class FakePE:
    base=0x22000000
    def __init__(self):
        self.raw={}
        for name,(va,_,_,_,_) in REGIONS.items():
            for off,code in {**COMMON,**DETAIL[name]}.items():
                for i,b in enumerate(bytes.fromhex(code)):
                    self.put(va+off+i,b)
        for src,dst in {**CALLS,**INCOMING}.items():
            for i,b in enumerate(rel_call(src,dst)):
                self.put(src+i,b)
    def put(self,va,value):
        if va in self.raw and self.raw[va]!=value:
            raise AssertionError('conflicting native witness at %#x'%va)
        self.raw[va]=value
    def at(self,va,n):
        return bytes(self.raw.get(va+i,0) for i in range(n))
    def section(self,va):
        return '.rdata' if va in (0x2242DCA0,0x2240ABC8) else '.text'

class Tests(unittest.TestCase):
    def test_synthetic_witnesses_do_not_claim_original_hashes(self):
        data=verify_native(FakePE(),verify_hash=False)
        self.assertEqual(data['new_complete_native_functions'],0)
        self.assertEqual(data['sparse_instruction_witnesses'],34)
        self.assertEqual(data['exact_rel32_call_edges'],10)
    def test_mutate_every_instruction_witness(self):
        for name,(va,_,_,_,_) in REGIONS.items():
            for off in {**COMMON,**DETAIL[name]}:
                with self.subTest(name=name,offset=off):
                    fake=FakePE();fake.raw[va+off]^=1
                    with self.assertRaises(VerificationError):
                        verify_native(fake,verify_hash=False)
    def test_mutate_every_direct_call(self):
        for src in {**CALLS,**INCOMING}:
            with self.subTest(source=src):
                fake=FakePE();fake.raw[src+1]^=1
                with self.assertRaises(VerificationError):
                    verify_native(fake,verify_hash=False)
    def test_wrong_vtable_section(self):
        fake=FakePE();fake.section=lambda _:'.text'
        with self.assertRaises(VerificationError):
            verify_native(fake,verify_hash=False)
    def test_wrong_image_base(self):
        fake=FakePE();fake.base=0x24000000
        with self.assertRaises(VerificationError):
            verify_native(fake,verify_hash=False)
    def test_synthetic_function_hash_must_fail(self):
        with self.assertRaises(VerificationError):
            verify_native(FakePE(),verify_hash=True)
    def test_synthetic_whole_files_must_fail(self):
        with self.assertRaises(VerificationError):
            audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
