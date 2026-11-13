#!/usr/bin/env python3
"""Phase211 synthetic-only regression; no original game DLL bytes in fixtures."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_prediction_snapshot_dispatch import (
    METHODS,CALL_EDGES,GLOBAL_COUNTER_VA,relative_call,
    verify_native,audit,VerificationError
)

class FakePE:
    def __init__(self):
        self.base=0x24000000
        self.words={}
        for name,(addr,size,_sha,padding,witnesses) in METHODS.items():
            self.put(addr,bytes(size))
            # We need to overlay the sparse authentic opcode witnesses onto
            # independently synthesized, otherwise blank, PE32 text cells.
            for offset,hexbytes in witnesses.items():
                self.put(addr+offset,bytes.fromhex(hexbytes),overwrite=True)
            self.put(addr+size,b"\xcc"*padding)
        for _,(site,target) in CALL_EDGES.items():
            self.put(site,relative_call(site,target),overwrite=True)
    def put(self,addr,data,*,overwrite=False):
        for i,byte in enumerate(data):
            key=addr+i
            if key in self.words and not overwrite and self.words[key]!=byte:
                raise AssertionError("contradictory independent fixture")
            self.words[key]=byte
    def at(self,addr,count):
        return bytes(self.words.get(addr+i,0) for i in range(count))
    def section_of(self,addr):
        return ".data" if addr==GLOBAL_COUNTER_VA else ".text"
    def flip(self,addr):
        self.words[addr]=self.words.get(addr,0)^1

class OriginalFreeTests(unittest.TestCase):
    @staticmethod
    def fixture():return FakePE()
    @staticmethod
    def check(pe):return verify_native(pe,check_hash=False)
    def test_fixture_is_never_accepted_as_sha_verified_original(self):
        pe=self.fixture()
        result=self.check(pe)
        self.assertEqual(result["new_sha_verified_client_functions"],0)
        self.assertEqual(result["new_sha_verified_original_x86_bytes"],0)
        self.assertEqual(result["short_original_opcode_witnesses"],62)
        self.assertEqual(result["exact_original_direct_call_edges"],13)
        self.assertEqual(result["original_dispatcher_global_addresses_checked"],2)
        with self.assertRaises(VerificationError):verify_native(pe,check_hash=True)
        with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
    def test_mutate_all_62_individual_original_opcode_witnesses(self):
        for name,(va,_,_,_,witnesses) in METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,off=off):
                    pe=self.fixture();pe.flip(va+off)
                    with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_every_one_of_13_direct_native_call_edges(self):
        for name,(site,_) in CALL_EDGES.items():
            with self.subTest(name=name):
                pe=self.fixture();pe.flip(site+1)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_mutate_all_three_native_RET_INT3_boundaries(self):
        for name,(va,size,_,_,_) in METHODS.items():
            with self.subTest(name=name):
                pe=self.fixture();pe.flip(va+size)
                with self.assertRaises(VerificationError):self.check(pe)
    def test_wrong_image_base_rejected(self):
        pe=self.fixture();pe.base=0x26000000
        with self.assertRaises(VerificationError):self.check(pe)
    def test_wrong_native_method_section_rejected(self):
        pe=self.fixture();pe.section_of=lambda _addr:".rdata"
        with self.assertRaises(VerificationError):self.check(pe)
    def test_process_global_must_be_in_data_not_text(self):
        pe=self.fixture();pe.section_of=lambda _addr:".text"
        with self.assertRaises(VerificationError):self.check(pe)
    def test_original_global_address_embedded_twice(self):
        pe=self.fixture()
        self.assertEqual(struct.unpack("<I",pe.at(0x24199ba2,4))[0],
                         GLOBAL_COUNTER_VA)
        self.assertEqual(struct.unpack("<I",pe.at(0x24199bd2,4))[0],
                         GLOBAL_COUNTER_VA)
        for operand in (0x24199ba2,0x24199bd2):
            mutated=self.fixture()
            mutated.flip(operand)
            with self.assertRaises(VerificationError):self.check(mutated)
    def test_four_complete_constructor_dispatch_caller_pairs(self):
        caller_pairs=(
            (0x240c5a13,0x240c5a34),
            (0x240c6987,0x240c699f),
            (0x240cc4e3,0x240cc504),
            (0x24154a42,0x24154a5a)
        )
        pe=self.fixture()
        for construct,dispatch in caller_pairs:
            self.assertEqual(pe.at(construct,5),
                             relative_call(construct,0x241978a0))
            self.assertEqual(pe.at(dispatch,5),
                             relative_call(dispatch,0x24199ba0))
    def test_native_stack_cleanup_and_record_next_offsets(self):
        self.assertEqual(METHODS["argpack"][4][0x65],"c22800")
        self.assertEqual(METHODS["dispatch"][4][0x44],"c20c00")
        self.assertEqual(METHODS["record_walk"][4][0x3e],"c20800")
        self.assertEqual(METHODS["record_walk"][4][0x1d],"8b760c")
        self.assertEqual(METHODS["record_walk"][4][0x34],"8b760c")
        self.assertEqual(METHODS["dispatch"][4][0x0d],"807f1000")
    def test_call_edge_provenance_is_closed_at_internal_native_sites(self):
        pe=self.fixture()
        for name in ("dispatch_preprocess","dispatch_prepare",
                     "dispatch_record_walk","walk_first_record","walk_next_records"):
            site,target=CALL_EDGES[name]
            self.assertEqual(pe.at(site,5),relative_call(site,target))

if __name__=="__main__":
    unittest.main()
