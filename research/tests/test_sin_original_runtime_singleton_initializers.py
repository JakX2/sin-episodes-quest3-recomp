#!/usr/bin/env python3
"""Synthetic-only Phase227 startup/teardown evidence mutation tests."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_runtime_singleton_initializers as native


class FakePE:
    """Construct only sparse synthetic opcode/section witnesses, NOT full originals."""
    def __init__(self,module):
        self.module=module
        self.base=native.ORIGINALS[module]["base"]
        self.mem={}
        self.text=set()
        self.data=set()
        self.rdata=set()
        self.unbacked=set()
        self.relocs=set()
        for name,va,n,sha,pad,kind in native.METHODS[module]:
            raw=bytearray(n)
            if kind=="teardown":raw[5]=0xE9
            else:raw[-1]=0xC3
            self.write(va,raw)
            self.write(va+n,b"\xCC"*pad)
            self.text.update(range(va,va+n+pad))
        for record in native.LIFECYCLES[module]:
            name,cell,init,object_va,ctor,shutdown,destructor,register,
                initial_vptr,final_vptr=record
            self.write(cell,struct.pack("<I",init))
            self.data.update(range(cell,cell+4))
            self.unbacked.add(object_va)
            self.data.update(range(object_va,object_va+4))
            self.relocs.add(cell)
            self.write(init,b"\xB9"+struct.pack("<I",object_va))
            self.write(init+10,b"\x68"+struct.pack("<I",shutdown))
            self.write(shutdown,b"\xB9"+struct.pack("<I",object_va))
            self.text.add(ctor)
            self.text.add(destructor)
            self.text.add(register)
            # Actual original .rdata contains a relocated pointer to
            # original .text slot-zero method.
            first={
                "server":0x222D01B0,
                "ClientLeafSystem001":0x2410D8E0,
                "GameMovement001":0x2420FC60,
                "VClientPrediction001":0x24195A50,
            }["server" if module=="server" else name]
            self.write(final_vptr,struct.pack("<I",first))
            self.rdata.update(range(final_vptr,final_vptr+4))
            self.text.add(first)
            self.relocs.add(final_vptr)
            if initial_vptr!=final_vptr:
                self.write(init+15,b"\xC7\x05"+struct.pack("<II",object_va,final_vptr))
                self.relocs.update({init+17,init+21})
            self.relocs.add(init+1)
        for site,dst in native.CALLS[module]:
            self.write(site,native.call32(site,dst))
        for site,dst in native.TEARDOWN_JMPS[module]:
            self.write(site,native.call32(site,dst,0xE9))
        for site,vptr in native.INITIAL_VPTR_WRITES[module]:
            if site==0x2410C7A6:prefix=b"\xC7\x40\x04"
            elif site==0x2410C7AD:prefix=b"\xC7\x40\x08"
            elif site in (0x24136FAE,0x221EBF6E):prefix=b"\xC7\x02"
            else:prefix=b"\xC7\x00"
            self.write(site,prefix+struct.pack("<I",vptr))
            self.text.update(range(site,site+len(prefix)+4))
            self.relocs.add(site+len(prefix))

    def write(self,va,raw):
        for off,x in enumerate(raw):
            self.mem[va+off]=x
    def at(self,va,n):
        if any(x<=va<x+4 for x in self.unbacked):
            raise native.VerificationError("synthetic unbacked virtual singleton")
        return bytes(self.mem.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.data:return ".data"
        if va in self.text:return ".text"
        return ""
    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1

class Phase227Tests(unittest.TestCase):
    @staticmethod
    def verify(pe):
        return native.verify_module(pe,pe.module,
            original_provenance_verified=False,
            check_hash=False,relocated_sites=pe.relocs)

    def test_synthetic_fixtures_never_count_as_original_recovery(self):
        for module,n in (("server",3),("client",8)):
            pe=FakePE(module)
            report=self.verify(pe)
            self.assertEqual(report["new_original_SHA_verified_methods"],0)
            self.assertEqual(report["new_original_complete_native_bytes"],0)
            self.assertEqual(len(report["complete_method_evidence"]),n)
            self.assertEqual(report["original_relocated_initializer_entries"],
                             1 if module=="server" else 3)
            with self.assertRaises(native.VerificationError):
                native.verify_module(pe,module,check_hash=True,
                    relocated_sites=pe.relocs)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_init_pointer_cell_and_highlow_relocation(self):
        for module in ("server","client"):
            for row in native.LIFECYCLES[module]:
                name,cell,*_=row
                with self.subTest(module=module,name=name):
                    pe=FakePE(module);pe.flip(cell)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                    pe=FakePE(module);pe.relocs.remove(cell)
                    with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_each_init_constructor_and_atexit_rel32_CALL(self):
        for module in ("server","client"):
            for site,dest in native.CALLS[module]:
                with self.subTest(module=module,site=hex(site)):
                    pe=FakePE(module);pe.flip(site+1)
                    with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_each_teardown_tail_jump_and_this_pointer(self):
        for module in ("server","client"):
            for site,dest in native.TEARDOWN_JMPS[module]:
                with self.subTest(module=module,site=hex(site)):
                    pe=FakePE(module);pe.flip(site+1)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
            for name,cell,init,singleton,ctor,shutdown,*_ in native.LIFECYCLES[module]:
                pe=FakePE(module);pe.flip(shutdown+1)
                with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_each_constructor_vptr_write_and_imm_relocation(self):
        for module in ("server","client"):
            for site,vptr in native.INITIAL_VPTR_WRITES[module]:
                with self.subTest(module=module,site=hex(site)):
                    pe=FakePE(module);pe.flip(site)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                    pe=FakePE(module)
                    imm=site+3 if site in (0x2410C7A6,0x2410C7AD) else site+2
                    pe.flip(imm)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                    pe=FakePE(module);pe.relocs.remove(imm)
                    with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_movement_post_ctor_final_vptr_writes_and_relocations(self):
        for module in ("server","client"):
            for row in native.LIFECYCLES[module]:
                name,cell,init,singleton,ctor,shutdown,destructor,register,
                    old_vptr,new_vptr=row
                if old_vptr==new_vptr:continue
                for offset in (15,17,21):
                    with self.subTest(module=module,offset=offset):
                        pe=FakePE(module);pe.flip(init+offset)
                        with self.assertRaises(native.VerificationError):self.verify(pe)
                for site in (init+17,init+21):
                    pe=FakePE(module);pe.relocs.remove(site)
                    with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_vtable_first_method_pointer_and_relocation(self):
        for module in ("server","client"):
            for row in native.LIFECYCLES[module]:
                final_vptr=row[-1]
                with self.subTest(module=module,vptr=hex(final_vptr)):
                    pe=FakePE(module);pe.flip(final_vptr)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                    pe=FakePE(module);pe.relocs.remove(final_vptr)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                    pe=FakePE(module);pe.rdata.remove(final_vptr)
                    with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_all_native_C3_RET_and_INT3_padding(self):
        for module in ("server","client"):
            for name,va,n,sha,pad,kind in native.METHODS[module]:
                if kind!="teardown":
                    pe=FakePE(module);pe.flip(va+n-1)
                    with self.assertRaises(native.VerificationError):self.verify(pe)
                for off in range(pad):
                    with self.subTest(module=module,name=name,pad=off):
                        pe=FakePE(module);pe.flip(va+n+off)
                        with self.assertRaises(native.VerificationError):self.verify(pe)

    def test_original_address_grouping_and_evidence_count(self):
        self.assertEqual(sum(len(rows) for rows in native.METHODS.values()),11)
        self.assertEqual(sum(sum(row[2] for row in rows)
                             for rows in native.METHODS.values()),314)
        self.assertEqual(sum(len(rows) for rows in native.LIFECYCLES.values()),4)
        self.assertEqual(len(native.INITIAL_VPTR_WRITES["client"]),5)
        self.assertEqual(len(native.INITIAL_VPTR_WRITES["server"]),1)

if __name__=="__main__":
    unittest.main()
