#!/usr/bin/env python3
"""Original-free Phase228 native embedded-interface SHA witness mutations."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_embedded_interface_initializers as native

class FakePE:
    """Synthetic constructed opcode patterns, NOT original retail bodies."""
    def __init__(self,module):
        self.module=module
        self.base=native.ORIGINALS[module]["base"]
        self.mem={}
        self.text=set()
        self.data=set()
        self.rdata=set()
        self.unbacked=set()
        self.relocs=set()
        for label,va,n,digest,pad in native.METHODS[module]:
            body=bytearray(n)
            if label.endswith("teardown"):body[5]=0xE9
            else:body[-1]=0xC3
            self.write(va,body)
            self.write(va+n,b"\xcc"*pad)
            self.text.update(range(va,va+n+pad))
        (name,cell,initializer,parent,offset,factory,ctor,shutdown,
         destructor,registrar,final_vptr,first_method)=native.CHAINS[module]
        self.data.update(range(cell,cell+4))
        self.data.update(range(parent,parent+4))
        self.data.update(range(parent+offset,parent+offset+4))
        self.unbacked.update({parent,parent+offset})
        self.write(cell,struct.pack("<I",initializer))
        self.relocs.add(cell)
        self.write(factory,b"\xb8"+struct.pack("<I",parent+offset)+b"\xc3")
        self.relocs.add(factory+1)
        self.text.update(range(factory,factory+6))
        self.write(initializer,b"\xb9"+struct.pack("<I",parent))
        self.write(initializer+5,native.branch32(initializer+5,ctor,0xE8))
        self.write(initializer+10,b"\x68"+struct.pack("<I",shutdown))
        self.write(initializer+15,native.branch32(initializer+15,registrar,0xE8))
        self.relocs.update({initializer+1,initializer+11})
        self.text.add(registrar)
        self.text.add(destructor)
        self.write(shutdown,b"\xb9"+struct.pack("<I",parent))
        self.write(shutdown+5,native.branch32(shutdown+5,destructor,0xE9))
        self.relocs.add(shutdown+1)
        self.write(final_vptr,struct.pack("<I",first_method))
        self.rdata.update(range(final_vptr,final_vptr+4))
        self.text.add(first_method)
        self.relocs.add(final_vptr)
        for site,hx,vptr,_offset in native.VPTR_WRITES[module]:
            raw=bytes.fromhex(hx)
            self.write(site,raw)
            self.relocs.add(site+len(raw)-4)
            self.text.update(range(site,site+len(raw)))
        for site,dest in native.NESTED[module]:
            self.write(site,native.branch32(site,dest,0xE8))
            self.text.add(dest)

    def write(self,va,blob):
        for i,c in enumerate(blob):
            self.mem[va+i]=c
    def at(self,va,n):
        if any(root<=va<root+4 for root in self.unbacked):
            raise native.VerificationError("synthetic unbacked virtual runtime singleton")
        return bytes(self.mem.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.data:return ".data"
        if va in self.text:return ".text"
        return ""
    def mutate(self,va):
        self.mem[va]=self.mem.get(va,0)^1

class Phase228Tests(unittest.TestCase):
    @staticmethod
    def verify(f):
        return native.check_module(f,f.module,check_hash=False,
              full_original_provenance=False,relocated_sites=f.relocs)

    def test_synthetic_never_counts_as_original_recovery(self):
        for mod in ("server","client"):
            fake=FakePE(mod)
            report=self.verify(fake)
            self.assertEqual(report["new_SHA_verified_functions"],0)
            self.assertEqual(report["new_whole_native_bytes"],0)
            self.assertEqual(len(report["complete_method_evidence"]),3)
            self.assertEqual(report["original_vptr_write_witnesses"],
                             5 if mod=="server" else 3)
            with self.assertRaises(native.VerificationError):
                native.check_module(fake,mod,check_hash=True,
                                   relocated_sites=fake.relocs)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_initializer_pointer_and_relocation(self):
        for mod in ("server","client"):
            cell=native.CHAINS[mod][1]
            f=FakePE(mod);f.mutate(cell)
            with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.relocs.remove(cell)
            with self.assertRaises(native.VerificationError):self.verify(f)

    def test_factory_return_address_and_module_sections(self):
        for mod in ("server","client"):
            row=native.CHAINS[mod]
            factory=row[5];parent=row[3]
            for site in (factory,factory+1,factory+5):
                with self.subTest(mod=mod,site=hex(site)):
                    f=FakePE(mod);f.mutate(site)
                    with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.relocs.remove(factory+1)
            with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.unbacked.remove(parent)
            with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.base+=0x1000
            with self.assertRaises(native.VerificationError):self.verify(f)

    def test_initializer_ctor_and_registration_calls(self):
        for mod in ("server","client"):
            init=native.CHAINS[mod][2]
            for off in (0,1,5,6,10,11,15,16):
                with self.subTest(mod=mod,offset=off):
                    f=FakePE(mod);f.mutate(init+off)
                    with self.assertRaises(native.VerificationError):self.verify(f)
            for off in (1,11):
                f=FakePE(mod);f.relocs.remove(init+off)
                with self.assertRaises(native.VerificationError):self.verify(f)

    def test_teardown_uses_containing_object_not_secondary_interface(self):
        for mod in ("server","client"):
            shutdown=native.CHAINS[mod][7]
            for off in (0,1,5,6):
                with self.subTest(mod=mod,offset=off):
                    f=FakePE(mod);f.mutate(shutdown+off)
                    with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.relocs.remove(shutdown+1)
            with self.assertRaises(native.VerificationError):self.verify(f)

    def test_every_construction_vptr_instruction_and_HIGHLOW(self):
        for mod in ("server","client"):
            for site,hx,ptr,offset in native.VPTR_WRITES[mod]:
                imm=site+len(bytes.fromhex(hx))-4
                for at in (site,imm):
                    with self.subTest(mod=mod,site=hex(site),at=hex(at)):
                        f=FakePE(mod);f.mutate(at)
                        with self.assertRaises(native.VerificationError):self.verify(f)
                f=FakePE(mod);f.relocs.remove(imm)
                with self.assertRaises(native.VerificationError):self.verify(f)

    def test_final_subobject_vtable_and_nested_CALLs(self):
        for mod in ("server","client"):
            vptr=native.CHAINS[mod][10]
            f=FakePE(mod);f.mutate(vptr)
            with self.assertRaises(native.VerificationError):self.verify(f)
            f=FakePE(mod);f.relocs.remove(vptr)
            with self.assertRaises(native.VerificationError):self.verify(f)
            for site,dest in native.NESTED[mod]:
                f=FakePE(mod);f.mutate(site+1)
                with self.assertRaises(native.VerificationError):self.verify(f)

    def test_all_ret_and_INT3_native_boundaries(self):
        for mod in ("server","client"):
            for label,va,n,digest,pad in native.METHODS[mod]:
                if not label.endswith("teardown"):
                    f=FakePE(mod);f.mutate(va+n-1)
                    with self.assertRaises(native.VerificationError):self.verify(f)
                for j in range(pad):
                    with self.subTest(mod=mod,label=label,pad=j):
                        f=FakePE(mod);f.mutate(va+n+j)
                        with self.assertRaises(native.VerificationError):self.verify(f)

    def test_original_native_counts_and_embedded_offsets(self):
        self.assertEqual(sum(len(v) for v in native.METHODS.values()),6)
        self.assertEqual(sum(sum(r[2] for r in v) for v in native.METHODS.values()),486)
        self.assertEqual(native.CHAINS["server"][4],8)
        self.assertEqual(native.CHAINS["client"][4],0x10014)
        self.assertEqual(native.CHAINS["server"][3]+8,0x2257AEA0)
        self.assertEqual(native.CHAINS["client"][3]+0x10014,0x2434FDDC)

if __name__=="__main__":
    unittest.main()
