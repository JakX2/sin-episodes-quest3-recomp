#!/usr/bin/env python3
"""Original-free Phase226 factory singleton and PE32 relocation mutation tests."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_core_factory_thunks as native


class SyntheticPE:
    def __init__(self,module:str):
        self.base=native.ORIGINALS[module]["base"]
        self.module=module
        self.mem={}
        self.text=set()
        self.data=set()
        self.rdata=set()
        self.unbacked=set()
        self.relocations=set()
        self.registrations=[]
        for name,va,target,digest,vptr,first,is_new in native.FACTORIES[module]:
            # Synthetic opcode fixture is reconstructed from the documented
            # architecture pattern, never copied from an owner DLL.
            self.write(va,b"\xB8"+struct.pack("<I",target)+b"\xC3"+b"\xCC"*10)
            self.text.update(range(va,va+16))
            self.data.update(range(target,target+4))
            self.relocations.add(va+1)
            self.registrations.append({"interface":name,"factory_va":va})
            if vptr:
                self.write(target,struct.pack("<I",vptr))
                self.write(vptr,struct.pack("<I",first))
                self.rdata.update(range(vptr,vptr+4))
                self.text.update(range(first,first+4))
                self.relocations.update({target,vptr})
            else:self.unbacked.add(target)
        if module=="client":
            for k in range(47):
                self.registrations.append({
                    "interface":f"SyntheticProxy{k:03d}_IMaterialProxy003",
                    "factory_va":0x24000010+k,
                })

    def write(self,va,blob):
        for k,x in enumerate(blob):
            old=self.mem.get(va+k,x)
            if old!=x:raise AssertionError("synthetic factory memory collision")
            self.mem[va+k]=x

    def at(self,va,n):
        for marker in self.unbacked:
            if marker<=va<marker+4:
                raise native.VerificationError("synthetic unbacked virtual data")
        return bytes(self.mem.get(va+i,0) for i in range(n))

    def section_of(self,va):
        if va in self.data:return ".data"
        if va in self.rdata:return ".rdata"
        if va in self.text:return ".text"
        return ""

    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1


class Phase226Tests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native_factories(pe,pe.module,
                   registrations=pe.registrations,
                   relocated_sites=pe.relocations,
                   check_hash=False)

    def test_original_free_fixture_cannot_count_new_authentic_methods(self):
        for module,expected in (("server",10),("client",8)):
            pe=SyntheticPE(module)
            report=self.check(pe)
            self.assertEqual(report["checked_static_core_factories"],expected)
            self.assertEqual(report["new_whole_SHA_verified_original_methods"],0)
            self.assertEqual(report["new_whole_original_x86_bytes"],0)
            self.assertEqual(report["file_backed_vptr_and_slot0_evidence"],
                             8 if module=="server" else 4)
            # Even a synthetic byte-identical six-byte body is NOT a
            # verified original until full original-file provenance passes.
            report=native.verify_native_factories(pe,module,
                      registrations=pe.registrations,
                      relocated_sites=pe.relocations,
                      check_hash=True)
            self.assertEqual(report["new_whole_SHA_verified_original_methods"],0)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_18_original_thunk_opcode_and_immediate_byte(self):
        for module in ("server","client"):
            for name,va,target,sha,vptr,first,new in native.FACTORIES[module]:
                for off in range(6):
                    with self.subTest(module=module,name=name,offset=off):
                        pe=SyntheticPE(module)
                        pe.flip(va+off)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_all_original_10_byte_INT3_boundaries(self):
        for module in ("server","client"):
            for name,va,*_ in native.FACTORIES[module]:
                for off in range(10):
                    with self.subTest(module=module,name=name,offset=off):
                        pe=SyntheticPE(module)
                        pe.flip(va+6+off)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_each_native_highlow_relocation_is_required(self):
        for module in ("server","client"):
            for name,va,target,sha,vptr,first,new in native.FACTORIES[module]:
                for site in (va+1,target,vptr):
                    if site==0:continue
                    if site not in SyntheticPE(module).relocations:continue
                    with self.subTest(module=module,name=name,site=hex(site)):
                        pe=SyntheticPE(module)
                        pe.relocations.remove(site)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_each_file_backed_singleton_vptr_and_slot0_word(self):
        for module in ("server","client"):
            for name,va,target,sha,vptr,first,new in native.FACTORIES[module]:
                if not vptr:continue
                for site in (target,vptr):
                    with self.subTest(module=module,name=name,site=hex(site)):
                        pe=SyntheticPE(module)
                        pe.flip(site)
                        with self.assertRaises(native.VerificationError):
                            self.check(pe)

    def test_virtual_unbacked_factories_never_assume_initialized_vtable(self):
        for module in ("server","client"):
            for name,va,target,sha,vptr,first,new in native.FACTORIES[module]:
                if vptr:continue
                with self.subTest(module=module,name=name):
                    pe=SyntheticPE(module)
                    pe.unbacked.remove(target)
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)

    def test_existing_factory_manifest_is_enforced(self):
        for module in ("server","client"):
            for name,va,target,sha,vptr,first,new in native.FACTORIES[module]:
                with self.subTest(module=module,name=name):
                    pe=SyntheticPE(module)
                    row=next(x for x in pe.registrations if x["interface"]==name)
                    row["factory_va"]^=4
                    with self.assertRaises(native.VerificationError):
                        self.check(pe)
        pe=SyntheticPE("client")
        pe.registrations[-1]["interface"]="UnexpectedNonProxy"
        with self.assertRaises(native.VerificationError):
            self.check(pe)
        pe=SyntheticPE("client")
        pe.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.check(pe)

    def test_all_counts_and_alias(self):
        self.assertEqual(sum(len(v) for v in native.FACTORIES.values()),18)
        self.assertEqual(sum(sum(1 for row in v if row[-1])
                             for v in native.FACTORIES.values()),9)
        self.assertEqual(sum(sum(1 for row in v if row[4])
                             for v in native.FACTORIES.values()),12)
        s={row[0]:row for row in native.FACTORIES["server"]}
        self.assertEqual(s["PlayerInfoManager001"][2],s["PlayerInfoManager002"][2])
        self.assertNotEqual(s["PlayerInfoManager001"][1],s["PlayerInfoManager002"][1])

    def test_synthetic_PE32_highlow_parser(self):
        class RelocationPE:
            def __init__(self,entries):
                self.base=0x24000000
                self.data=bytearray(512)
                struct.pack_into("<I",self.data,0x3c,0x80)
                self.payload=struct.pack("<II",0x12000,8+len(entries)*2)+(
                    struct.pack("<"+"H"*len(entries),*entries))
                struct.pack_into("<II",self.data,0x80+24+96+5*8,
                                 0x5000,len(self.payload))
            def at(self,va,n):
                if va!=self.base+0x5000:
                    raise native.VerificationError("wrong synthetic rva")
                return self.payload[:n]
        fake=RelocationPE([0x3101,0x3202,0])
        self.assertEqual(native.pe_highlow_relocations(fake),
                         {0x24012101,0x24012202})
        fake=RelocationPE([0x1101,0])
        with self.assertRaises(native.VerificationError):
            native.pe_highlow_relocations(fake)


if __name__=="__main__":
    unittest.main()
