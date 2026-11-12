#!/usr/bin/env python3
"""Original-free corruption tests for Phase193 native insertion-wrapper auditor."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_cache_tree_insert import (
    REGIONS,WITNESSES,CALL_TARGETS,INCOMING_CALLS,rel32,
    verify_native,audit,VerificationError)

class FakePE:
    base=0x22000000
    def __init__(self):
        self.mem={}
        for va,_,_,following in REGIONS.values():
            for off,hexcode in WITNESSES.items():self.put(va+off,bytes.fromhex(hexcode))
            self.put(va+112,bytes.fromhex(following))
            for off,dst in CALL_TARGETS[va].items():self.put(va+off,rel32(va+off,dst))
        for src,dst in INCOMING_CALLS.items():self.put(src,rel32(src,dst))
    def put(self,va,code):
        for i,value in enumerate(code):
            addr=va+i
            if addr in self.mem and self.mem[addr]!=value:
                raise AssertionError('conflicting artificial witness %#x'%addr)
            self.mem[addr]=value
    def at(self,va,size):return bytes(self.mem.get(va+i,0) for i in range(size))
    def section(self,va):return '.text'
    def mutate(self,va):self.mem[va]=self.mem.get(va,0)^1

class Tests(unittest.TestCase):
    def test_original_free_witness_count_without_native_claims(self):
        report=verify_native(FakePE(),check_hash=False)
        self.assertEqual(report['new_complete_original_function_count'],0)
        self.assertEqual(report['sparse_native_witnesses'],52)
        self.assertEqual(report['outgoing_exact_direct_calls'],6)
        self.assertEqual(report['incoming_exact_direct_calls'],2)
    def test_mutate_each_independent_opcode(self):
        for label,(va,_,_,_) in REGIONS.items():
            for off in WITNESSES:
                with self.subTest(label=label,off=off):
                    f=FakePE();f.mutate(va+off)
                    with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_mutate_each_exact_outgoing_call(self):
        for base,targets in CALL_TARGETS.items():
            for off in targets:
                with self.subTest(va=base+off):
                    f=FakePE();f.mutate(base+off+1)
                    with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_mutate_each_exact_incoming_call(self):
        for va in INCOMING_CALLS:
            with self.subTest(va=va):
                f=FakePE();f.mutate(va+1)
                with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_mutate_next_entry_prefix(self):
        for va,length,_,_ in REGIONS.values():
            f=FakePE();f.mutate(va+length)
            with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_unwitnessed_body_diff_caught_by_full_normalized_twin(self):
        f=FakePE();f.mutate(REGIONS['scene'][0]+0x12)
        with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_wrong_base_and_section(self):
        f=FakePE();f.base=0x24000000
        with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
        f=FakePE();f.section=lambda _:'.rdata'
        with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
    def test_synthetic_not_original_whole_hashes(self):
        with self.assertRaises(VerificationError):verify_native(FakePE(),check_hash=True)
        with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
