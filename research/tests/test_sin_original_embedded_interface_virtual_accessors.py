#!/usr/bin/env python3
"""Phase229 synthetic-only evidence mutation regression tests.

The fake PE is NOT an original DLL. Whole-original SHA claims require
the private owner-original complete client/server binary provenance gate.
"""
from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_embedded_interface_virtual_accessors as audit


class SyntheticPE:
    def __init__(self,module:str):
        self.module=module
        self.base=audit.ORIGINALS[module]["base"]
        self.memory={}
        self.code=set()
        self.rdata=set()
        self.relocations=set()
        self.vtable=(audit.SERVER_VTABLE_VA if module=="server"
                     else audit.CLIENT_VTABLE_VA)
        for name,va,size,sha,pad,slot,pop,witnesses in audit.METHODS[module]:
            body=bytearray(size)
            if pop:
                body[-3:]=b"\xc2"+struct.pack("<H",pop)
            else:
                body[-1:]=b"\xc3"
            for off,hx in witnesses.items():
                fragment=bytes.fromhex(hx)
                for i,b in enumerate(fragment):
                    original=body[off+i]
                    if original not in (0,b):
                        raise AssertionError(f"synthetic anchor conflict {name} {off+i}")
                    body[off+i]=b
            self.write(va,body)
            self.write(va+size,b"\xcc"*pad)
            self.code.update(range(va,va+size+pad))
            entry=self.vtable+slot*4
            self.write(entry,struct.pack("<I",va))
            self.rdata.update(range(entry,entry+4))
            self.relocations.add(entry)
        if module=="server":
            self.write(audit.SERVER_GLOBAL_IMM_SITE,
                       struct.pack("<I",audit.SERVER_GLOBAL_ABSOLUTE_VA))
            self.relocations.add(audit.SERVER_GLOBAL_IMM_SITE)

    def write(self,va:int,b:bytes)->None:
        for i,byte in enumerate(b):
            self.memory[va+i]=byte

    def at(self,va:int,n:int)->bytes:
        return bytes(self.memory.get(va+i,0) for i in range(n))

    def section_of(self,va:int)->str:
        if va in self.rdata:return ".rdata"
        if va in self.code:return ".text"
        return ""

    def flip(self,va:int)->None:
        self.memory[va]=self.memory.get(va,0)^1


class Phase229Tests(unittest.TestCase):
    @staticmethod
    def check(pe:SyntheticPE)->dict:
        return audit.check_module(pe,pe.module,check_hash=False,
             original_provenance_verified=False,relocated_sites=pe.relocations)

    def test_fixture_never_increments_original_function_count(self)->None:
        for module,expected in (("server",7),("client",5)):
            fake=SyntheticPE(module)
            result=self.check(fake)
            self.assertEqual(result["checked_original_vtable_slots"],expected)
            self.assertEqual(result["new_whole_SHA_verified_original_methods"],0)
            self.assertEqual(result["new_original_x86_code_bytes"],0)
            with self.assertRaises(audit.VerificationError):
                audit.check_module(fake,module,check_hash=True,
                    original_provenance_verified=False,
                    relocated_sites=fake.relocations)
        with self.assertRaises(audit.VerificationError):
            audit.audit(b"MZ",b"MZ")

    def test_each_sparse_native_instruction_witness_mutation(self)->None:
        for module in audit.METHODS:
            for label,va,size,sha,pad,slot,pop,witnesses in audit.METHODS[module]:
                for off in witnesses:
                    with self.subTest(module=module,label=label,offset=off):
                        fake=SyntheticPE(module)
                        fake.flip(va+off)
                        with self.assertRaises(audit.VerificationError):
                            self.check(fake)

    def test_every_original_return_type_and_padding_mutation(self)->None:
        for module in audit.METHODS:
            for label,va,size,sha,pad,slot,pop,witnesses in audit.METHODS[module]:
                fake=SyntheticPE(module)
                fake.flip(va+size-1)
                with self.assertRaises(audit.VerificationError):
                    self.check(fake)
                for i in range(pad):
                    with self.subTest(module=module,label=label,padding=i):
                        fake=SyntheticPE(module)
                        fake.flip(va+size+i)
                        with self.assertRaises(audit.VerificationError):
                            self.check(fake)

    def test_every_original_virtual_slot_pointer_and_highlow(self)->None:
        for module in audit.METHODS:
            for label,va,size,sha,pad,slot,pop,witnesses in audit.METHODS[module]:
                slot_va=(audit.SERVER_VTABLE_VA if module=="server"
                         else audit.CLIENT_VTABLE_VA)+slot*4
                for i in range(4):
                    with self.subTest(module=module,label=label,byte=i):
                        fake=SyntheticPE(module)
                        fake.flip(slot_va+i)
                        with self.assertRaises(audit.VerificationError):
                            self.check(fake)
                fake=SyntheticPE(module)
                fake.relocations.remove(slot_va)
                with self.assertRaises(audit.VerificationError):
                    self.check(fake)
                fake=SyntheticPE(module)
                fake.rdata.remove(slot_va)
                with self.assertRaises(audit.VerificationError):
                    self.check(fake)

    def test_original_server_global_return_immediate_and_highlow(self)->None:
        for i in range(4):
            fake=SyntheticPE("server")
            fake.flip(audit.SERVER_GLOBAL_IMM_SITE+i)
            with self.assertRaises(audit.VerificationError):
                self.check(fake)
        fake=SyntheticPE("server")
        fake.relocations.remove(audit.SERVER_GLOBAL_IMM_SITE)
        with self.assertRaises(audit.VerificationError):
            self.check(fake)

    def test_pe_base_and_text_section(self)->None:
        for module in audit.METHODS:
            fake=SyntheticPE(module)
            fake.base+=0x1000
            with self.assertRaises(audit.VerificationError):
                self.check(fake)
            fake=SyntheticPE(module)
            first=audit.METHODS[module][0][1]
            fake.code.remove(first)
            with self.assertRaises(audit.VerificationError):
                self.check(fake)

    def test_totals_and_selected_interface_slots(self)->None:
        self.assertEqual(sum(len(v) for v in audit.METHODS.values()),12)
        self.assertEqual(sum(sum(f[2] for f in vals)
                             for vals in audit.METHODS.values()),112)
        self.assertEqual({x[5] for x in audit.METHODS["server"]},
                         {1,3,4,5,6,7,8})
        self.assertEqual({x[5] for x in audit.METHODS["client"]},
                         {0,5,6,7,8})
        self.assertEqual(audit.CLIENT_STRIDE,8)
        self.assertEqual(audit.CLIENT_INDEXED_BASE_OFFSET,0x14)


if __name__=="__main__":
    unittest.main()
