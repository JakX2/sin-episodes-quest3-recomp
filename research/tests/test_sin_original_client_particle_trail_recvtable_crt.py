#!/usr/bin/env python3
"""Phase250 synthetic-only original-PE mutation evidence tests.

These tests are deliberately NOT full original retail byte samples; all
synthetic checks report zero authentically reviewed native method credit.
"""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_particle_trail_recvtable_crt as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.section_map={}
        for name,va,size,sha,pad,exit_spec,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.label(va,size,".text")
            self.write(va+size,b"\xCC"*pad)
            exit_off,exit_hex=exit_spec
            self.write(va+exit_off,bytes.fromhex(exit_hex))
            for offset,hx in anchors.items():
                self.write(va+offset,bytes.fromhex(hx))
        for opcode,edges in ((0xE8,native.DIRECT_CALLS),
                             (0xE9,native.TAIL_JUMPS)):
            for site,target in edges:
                self.write(site,native.rel32(opcode,site,target))
                self.label(site,5,".text")
        for site,target in native.ABSOLUTE_OPERANDS:
            self.write(site,target.to_bytes(4,"little"))
            self.label(site,4,".data" if site in native.CRT_POINTER_SITES else ".text")
        for va,name in native.ORIGINAL_TEXT_STRINGS:
            value=name.encode("ascii")+b"\x00"
            self.write(va,value)
            self.label(va,len(value),".rdata")
    def write(self,va,payload):
        for i,b in enumerate(payload):
            self.memory[va+i]=b
    def label(self,va,count,section):
        for i in range(count):
            self.section_map[va+i]=section
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0) for i in range(size))
    def section_of(self,va):
        return self.section_map.get(va,"")
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase250SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(pe,check_hash=False,
                                    authentic_provenance=False,
                                    check_relocations=False)

    def test_synthetic_fixtures_never_earn_retail_code_credit(self):
        result=self.check(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(result["original_sparse_opcode_witnesses"],30)
        self.assertEqual(result["original_INT3_alignment_bytes"],25)
        self.assertEqual(result["original_exact_E8_CALL_edges"],8)
        self.assertEqual(result["original_exact_E9_tail_JMP_edges"],1)
        self.assertEqual(result["original_absolute_pointer_operands"],22)
        self.assertEqual(result["original_HIGHLOW_pointer_relocations"],0)
        self.assertEqual(len(result["reviewed_original_methods"]),4)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True,
                                 check_relocations=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_all_native_sparse_instruction_witnesses(self):
        for name,va,size,sha,pad,exit_spec,anchors in native.METHODS:
            for offset in anchors:
                with self.subTest(method=name,offset=hex(offset)):
                    pe=SyntheticPE()
                    pe.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_each_original_exit_byte_and_int3_boundary(self):
        for name,va,size,sha,pad,exit_spec,anchors in native.METHODS:
            off,hx=exit_spec
            for j in range(len(bytes.fromhex(hx))):
                with self.subTest(method=name,exit_byte=j):
                    pe=SyntheticPE()
                    pe.flip(va+off+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
            for j in range(pad):
                with self.subTest(method=name,int3_byte=j):
                    pe=SyntheticPE()
                    pe.flip(va+size+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_all_original_exact_e8_calls_and_e9_tail_jmp(self):
        for opcode,edges in ((0xE8,native.DIRECT_CALLS),
                             (0xE9,native.TAIL_JUMPS)):
            for site,target in edges:
                for j in range(5):
                    with self.subTest(site=hex(site),byte=j):
                        pe=SyntheticPE()
                        pe.flip(site+j)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_all_original_22_absolute_address_operands(self):
        for site,target in native.ABSOLUTE_OPERANDS:
            for j in range(4):
                with self.subTest(address=hex(site),byte=j):
                    pe=SyntheticPE()
                    pe.flip(site+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
            with self.subTest(address=hex(site),wrong_section=True):
                pe=SyntheticPE()
                pe.section_map[site]=".text" if site in native.CRT_POINTER_SITES else ".data"
                with self.assertRaises(native.VerificationError):
                    self.check(pe)

    def test_each_original_network_property_name_and_table_string(self):
        for va,name in native.ORIGINAL_TEXT_STRINGS:
            for j in range(len(name)+1):
                with self.subTest(string=name,byte=j):
                    pe=SyntheticPE()
                    pe.flip(va+j)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
            pe=SyntheticPE()
            pe.section_map[va]=".text"
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_original_image_base_and_complete_method_bounds(self):
        pe=SyntheticPE()
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        for name,va,size,*rest in native.METHODS:
            pe=SyntheticPE()
            pe.section_map[va+size-1]=""
            with self.assertRaises(native.VerificationError):
                self.check(pe)

    def test_exact_entire_original_method_inventory_and_abi_scope(self):
        self.assertEqual([m[1] for m in native.METHODS],[
            0x241281D0,0x24268960,0x24268980,0x242736B0])
        self.assertEqual([m[2] for m in native.METHODS],[151,22,16,10])
        self.assertEqual(sum(m[2] for m in native.METHODS),199)
        self.assertEqual([m[4] for m in native.METHODS],[9,10,0,6])
        self.assertEqual(native.TAIL_JUMPS,((0x242736B5,0x24126A20),))
        self.assertEqual(native.DIRECT_CALLS[-1],(0x24268982,0x241281D0))
        self.assertEqual(native.ABSOLUTE_OPERANDS[-2:],(
            (0x242F3A38,0x24268960),
            (0x242F3A3C,0x24268980)))
        self.assertEqual(native.ORIGINAL_TEXT_STRINGS[-1],
            (0x24298EF8,"DT_EntityParticleTrailInfo"))

if __name__=="__main__":
    unittest.main()
