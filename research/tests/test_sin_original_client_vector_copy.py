#!/usr/bin/env python3
"""Original-free synthetic regression for Phase221 type-3/type-4 native copies."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_vector_copy as audit


class SyntheticPE:
    def __init__(self):
        self.base=audit.BASE["client"]
        self.memory={}
        for va,n,_,padding,caller,pop,anchors in audit.METHODS.values():
            for offset,hex_bytes in anchors.items():
                self.write(va+offset,bytes.fromhex(hex_bytes))
            self.write(va+n-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*padding)
            self.write(caller,audit.rel_call(caller,va))
        for address,head in audit.NEIGHBOR_ENTRIES.items():
            self.write(address,head)

    def write(self,va,raw):
        for i,value in enumerate(raw):
            before=self.memory.get(va+i,value)
            if before!=value:
                raise AssertionError("incompatible synthetic original opcode witness")
            self.memory[va+i]=value

    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))

    def section_of(self,va):
        return ".text"

    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1


class Phase221Tests(unittest.TestCase):
    def test_synthetic_fixtures_do_not_claim_authentic_native_functions(self):
        result=audit.verify_native(SyntheticPE(),check_hash=False)
        self.assertEqual(result["new_sha_verified_original_client_functions"],0)
        self.assertEqual(result["new_whole_original_native_bytes"],0)
        self.assertEqual(result["sparse_native_opcode_witnesses"],34)
        self.assertEqual(result["original_exact_router_CALLs"],2)
        self.assertEqual(result["original_stride_bytes"],{"type3":12,"type4":16})
        with self.assertRaises(audit.VerificationError):
            audit.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(audit.VerificationError):
            audit.audit(b"MZ",b"MZ")

    def test_every_original_sparse_complete_instruction_witness(self):
        for name,(va,_,_,_,_,_,anchors) in audit.METHODS.items():
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(audit.VerificationError):
                        audit.verify_native(fake,check_hash=False)

    def test_every_native_router_CALL_site(self):
        for name,(va,_,_,_,caller,_,_) in audit.METHODS.items():
            with self.subTest(name=name,site=hex(caller)):
                fake=SyntheticPE();fake.flip(caller+1)
                with self.assertRaises(audit.VerificationError):
                    audit.verify_native(fake,check_hash=False)

    def test_every_native_ret_byte_and_int3_pad(self):
        for name,(va,n,_,pad,_,_,_) in audit.METHODS.items():
            for off in range(3):
                with self.subTest(name=name,kind="ret",off=off):
                    fake=SyntheticPE();fake.flip(va+n-3+off)
                    with self.assertRaises(audit.VerificationError):
                        audit.verify_native(fake,check_hash=False)
            for off in range(pad):
                with self.subTest(name=name,kind="padding",off=off):
                    fake=SyntheticPE();fake.flip(va+n+off)
                    with self.assertRaises(audit.VerificationError):
                        audit.verify_native(fake,check_hash=False)

    def test_both_following_original_function_entrypoints(self):
        for va,head in audit.NEIGHBOR_ENTRIES.items():
            for off in range(len(head)):
                with self.subTest(entry=hex(va),byte=off):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(audit.VerificationError):
                        audit.verify_native(fake,check_hash=False)

    def test_wrong_image_base_and_nontext_section(self):
        fake=SyntheticPE();fake.base+=0x20000
        with self.assertRaises(audit.VerificationError):
            audit.verify_native(fake,check_hash=False)
        fake=SyntheticPE();fake.section_of=lambda _va:".data"
        with self.assertRaises(audit.VerificationError):
            audit.verify_native(fake,check_hash=False)

    def test_exact_complete_original_function_sizes_and_splits(self):
        self.assertEqual(sum(x[1] for x in audit.METHODS.values()),102)
        self.assertEqual({x[0] for x in audit.METHODS.values()},
                         {0x24198DA0,0x24198DE0})
        self.assertEqual({x[3] for x in audit.METHODS.values()},{11,15})
        self.assertEqual({x[5] for x in audit.METHODS.values()},{16})


if __name__=="__main__":
    unittest.main()
