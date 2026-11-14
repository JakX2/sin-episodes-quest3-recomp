#!/usr/bin/env python3
"""Phase230 synthetic-only mutation tests; no retail native body data."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_handle_dispatch as native

class SyntheticPE:
    """Sparse original opcode witnesses on artificial zero-fill bodies."""
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.data={}
        self.code=set()
        self.rdata=set()
        self.relocs=set()
        for name,(va,n,sha,pad,ret4,ret0,witnesses) in native.METHODS.items():
            body=bytearray(n)
            for offset in ret4:body[offset:offset+3]=b"\xc2\x04\x00"
            for offset in ret0:body[offset]=0xC3
            for offset,hx in witnesses.items():
                bytestring=bytes.fromhex(hx)
                for i,b in enumerate(bytestring):
                    old=body[offset+i]
                    if old not in (0,b):
                        raise AssertionError("phase230 fake native opcode overlap "+name)
                    body[offset+i]=b
            self.write(va,body)
            self.write(va+n,b"\xcc"*pad)
            self.code.update(range(va,va+n+pad))
        for slot,target in native.VIRTUAL_SLOTS:
            cell=native.CLIENT_VTABLE_VA+slot*4
            self.write(cell,struct.pack("<I",target))
            self.rdata.update(range(cell,cell+4))
            self.relocs.add(cell)
        for site,target in native.CALLS:
            self.write(site,native.rel_call(site,target))
            self.code.update(range(site,site+5))

    def write(self,addr,values):
        for i,value in enumerate(values):
            old=self.data.get(addr+i,value)
            if old!=value:
                raise AssertionError("phase230 conflicting sparse synthetic original evidence")
            self.data[addr+i]=value

    def at(self,addr,n):
        return bytes(self.data.get(addr+i,0) for i in range(n))
    def section_of(self,addr):
        if addr in self.rdata:return ".rdata"
        if addr in self.code:return ".text"
        return ""
    def flip(self,addr):
        self.data[addr]=self.data.get(addr,0)^1


class Phase230Tests(unittest.TestCase):
    @staticmethod
    def check(f):
        return native.check_native(f,check_hash=False,
            original_provenance_verified=False,relocations=f.relocs)

    def test_synthetic_evidence_never_counts_original_recovery(self):
        f=SyntheticPE()
        report=self.check(f)
        self.assertEqual(report["original_client_new_complete_SHA_methods"],0)
        self.assertEqual(report["original_client_new_whole_x86_bytes"],0)
        self.assertEqual(len(report["methods"]),7)
        self.assertEqual(report["four_original_vtable_slot_HIGHLOW_edges"],4)
        self.assertEqual(report["direct_game_CALL_edges_to_parent_helpers"],3)
        self.assertEqual(report["original_max_occupied_slot_index"],0x7FF)
        with self.assertRaises(native.VerificationError):
            native.check_native(f,check_hash=True,relocations=f.relocs)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_all_sparse_native_opcode_witnesses_reject_mutations(self):
        for name,(va,n,sha,pad,ret4,ret0,witnesses) in native.METHODS.items():
            for offset in witnesses:
                with self.subTest(method=name,offset=hex(offset)):
                    f=SyntheticPE();f.flip(va+offset)
                    with self.assertRaises(native.VerificationError):self.check(f)

    def test_every_original_early_return_and_int3_padding_byte(self):
        for name,(va,n,sha,pad,ret4,ret0,witnesses) in native.METHODS.items():
            for offset in ret4+ret0:
                with self.subTest(method=name,ret=hex(offset)):
                    f=SyntheticPE();f.flip(va+offset)
                    with self.assertRaises(native.VerificationError):self.check(f)
            for i in range(pad):
                with self.subTest(method=name,int3=i):
                    f=SyntheticPE();f.flip(va+n+i)
                    with self.assertRaises(native.VerificationError):self.check(f)

    def test_native_four_embedded_vtable_slot_pointers_and_highlow(self):
        for slot,target in native.VIRTUAL_SLOTS:
            site=native.CLIENT_VTABLE_VA+slot*4
            for i in range(4):
                with self.subTest(slot=slot,offset=i):
                    f=SyntheticPE();f.flip(site+i)
                    with self.assertRaises(native.VerificationError):self.check(f)
            f=SyntheticPE();f.relocs.remove(site)
            with self.assertRaises(native.VerificationError):self.check(f)
            f=SyntheticPE();f.rdata.remove(site)
            with self.assertRaises(native.VerificationError):self.check(f)

    def test_game_callsite_linking_and_mutation(self):
        for site,target in native.CALLS:
            for i in range(5):
                with self.subTest(call=hex(site),offset=i):
                    f=SyntheticPE();f.flip(site+i)
                    with self.assertRaises(native.VerificationError):self.check(f)

    def test_native_text_section_image_base_and_provenance(self):
        f=SyntheticPE();f.base+=0x1000
        with self.assertRaises(native.VerificationError):self.check(f)
        for name,(va,n,sha,pad,ret4,ret0,witnesses) in native.METHODS.items():
            with self.subTest(method=name):
                f=SyntheticPE();f.code.remove(va)
                with self.assertRaises(native.VerificationError):self.check(f)

    def test_method_lengths_and_architecture(self):
        self.assertEqual(len(native.METHODS),7)
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),262)
        self.assertEqual([row[3] for row in native.METHODS.values()],
                         [12,0,14,14,15,15,4])
        self.assertEqual({slot for slot,_ in native.VIRTUAL_SLOTS},{1,2,3,4})
        self.assertEqual(native.ORIGINAL_RELATIVE_SLOT_BIAS,0x1001)
        self.assertEqual(native.ORIGINAL_ENTITYLIST_EMBEDDED_OFFSET,0x10014)
        self.assertEqual(native.ORIGINAL_TABLE_STRIDE,16)

if __name__=="__main__":
    unittest.main()
