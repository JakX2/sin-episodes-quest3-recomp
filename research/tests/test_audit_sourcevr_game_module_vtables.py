"""Synthetic-only tests for native SourceVR client/server full vtable evidence.

All bytes in these fixtures are FABRICATED; no SourceVR or SiN binaries in CI.
"""
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sourcevr_game_module_vtables as a

def adrp(pc, target, reg):
    displacement = (((target & ~4095) - (pc & ~4095)) // 4096) & 0x1fffff
    return (0x90000000 | ((displacement & 3) << 29) |
            (((displacement >> 2) & 0x7ffff) << 5) | reg)

def add(reg, target):
    return 0x91000000 | ((target & 4095) << 10) | (reg << 5) | reg


class FakeELF:
    def __init__(self, kind):
        self.kind=kind
        self.data=bytearray(0x3800)
        self.sections={
            ".rodata": (0,1,0x2,0x10000,0x0000,0x200,0,0,1,0),
            ".data.rel.ro": (0,1,0x3,0x20000,0x0200,0x2000,0,0,8,0),
            ".text": (0,1,0x6,0x40000,0x2200,0x1000,0,0,4,0),
        }
        self.vtable=0x20410
        self.singleton=0x60048
        self.site=0x40500
        self.store=0x40510
        self.count=a.CLASS[kind][2]
        self.rtti=0x20300
        self.base_rtti=0x20340
        cls,base,_=a.CLASS[kind]
        self.data[0x20:0x20+len(cls)+1]=cls+b"\x00"
        self.data[0x80:0x80+len(base)+1]=base+b"\x00"
        self.q(self.rtti+8,0x10020)
        self.q(self.rtti+16,self.base_rtti)
        self.q(self.base_rtti+8,0x10080)
        self.q(self.vtable-8,self.rtti)
        for i in range(self.count):
            self.q(self.vtable+i*8,0x40100+4*i)
        self.hash=hashlib.sha256(self.at(self.vtable,self.count*8)).hexdigest()
        self.i(self.site-4,adrp(self.site-4,self.singleton-0x48,8))
        self.i(self.site,adrp(self.site,self.vtable,9))
        self.i(self.site+4,add(9,self.vtable))
        self.i(self.store,0xf9000000 | ((0x48//8)<<10) | (8<<5) | 9)

    def offset(self,va,n):
        for sec in self.sections.values():
            if sec[3]<=va and va+n<=sec[3]+sec[5]:
                return sec[4]+va-sec[3]
        raise a.AuditError("invalid synthetic virtual address")

    def at(self,va,n):
        pos=self.offset(va,n)
        return bytes(self.data[pos:pos+n])

    def word(self,va):
        return struct.unpack("<I",self.at(va,4))[0]

    def q(self,va,v):
        struct.pack_into("<Q",self.data,self.offset(va,8),v)

    def i(self,va,v):
        struct.pack_into("<I",self.data,self.offset(va,4),v)


class SourceVRVTableTests(unittest.TestCase):
    def test_server_full_rtti_and_47_method_slots(self):
        elf=FakeELF("libserver.so")
        vt,methods,digest,base=a.method_table(elf,"libserver.so")
        self.assertEqual(vt,elf.vtable)
        self.assertEqual(len(methods),47)
        self.assertEqual(methods[0],0x40100)
        self.assertEqual(digest,elf.hash)
        self.assertEqual(base,elf.rtti)

    def test_client_full_rtti_and_78_method_slots(self):
        elf=FakeELF("libclient.so")
        vt,methods,digest,base=a.method_table(elf,"libclient.so")
        self.assertEqual(vt,elf.vtable)
        self.assertEqual(len(methods),78)
        self.assertEqual(base,elf.rtti)
        self.assertEqual(digest,elf.hash)

    def test_constructor_writes_factory_owned_singleton(self):
        elf=FakeELF("libserver.so")
        result=a.verify_constructor_vptr(
            elf,elf.vtable,elf.singleton,elf.site,elf.store)
        self.assertEqual(result["factory_singleton_va"],hex(elf.singleton))
        self.assertEqual(result["constructor_vptr_store_va"],hex(elf.store))

    def test_bad_factory_singleton_does_not_pass(self):
        elf=FakeELF("libclient.so")
        with self.assertRaisesRegex(a.AuditError,"factory singleton"):
            a.verify_constructor_vptr(
                elf,elf.vtable,elf.singleton+8,elf.site,elf.store)

    def test_wrong_ctor_vtable_target(self):
        elf=FakeELF("libserver.so")
        elf.i(elf.site+4,add(9,elf.vtable+8))
        with self.assertRaisesRegex(a.AuditError,"different game vtable"):
            a.verify_constructor_vptr(
                elf,elf.vtable,elf.singleton,elf.site,elf.store)

    def test_wrong_ctor_store_instruction(self):
        elf=FakeELF("libclient.so")
        elf.i(elf.store,0xd503201f)
        with self.assertRaisesRegex(a.AuditError,"STR"):
            a.verify_constructor_vptr(
                elf,elf.vtable,elf.singleton,elf.site,elf.store)

    def test_wrong_base_rtti(self):
        elf=FakeELF("libserver.so")
        elf.q(elf.rtti+16,0x20348)
        with self.assertRaisesRegex(a.AuditError,"no longer derives"):
            a.method_table(elf,"libserver.so")

    def test_wrong_top_offset(self):
        elf=FakeELF("libclient.so")
        elf.q(elf.vtable-16,8)
        with self.assertRaisesRegex(a.AuditError,"offset-to-top"):
            a.method_table(elf,"libclient.so")

    def test_non_text_method_pointer(self):
        elf=FakeELF("libserver.so")
        elf.q(elf.vtable+3*8,0x20000)
        with self.assertRaisesRegex(a.AuditError,"non-executable"):
            a.method_table(elf,"libserver.so")

    def test_extra_method_changes_boundary(self):
        elf=FakeELF("libclient.so")
        elf.q(elf.vtable+elf.count*8,0x40200)
        with self.assertRaisesRegex(a.AuditError,"terminator"):
            a.method_table(elf,"libclient.so")

    def test_duplicate_concrete_rtti_reference(self):
        elf=FakeELF("libserver.so")
        elf.q(0x20600,0x10020)
        with self.assertRaisesRegex(a.AuditError,"RTTI references changed"):
            a.method_table(elf,"libserver.so")

    def test_missing_base_class_name(self):
        elf=FakeELF("libclient.so")
        elf.data[0x80:0x80+len(a.CLASS["libclient.so"][1])]=b"X"*len(a.CLASS["libclient.so"][1])
        with self.assertRaisesRegex(a.AuditError,"missing or ambiguous"):
            a.method_table(elf,"libclient.so")

    def test_all_unique_profile_shas_covered(self):
        self.assertEqual(len(a.SERVER_TABLES),5)
        self.assertEqual(len(a.CLIENT_TABLES),6)
        self.assertEqual(set(a.SERVER_TABLES)|set(a.CLIENT_TABLES),set(a.WITNESSES))
        self.assertEqual(set(a.PROFILE_SHAS),
                         {"hl2","ep1","ep2","lostcoast","portal","portal2","entropyzero"})
        for role,definitions in a.TABLES.items():
            self.assertEqual(len(definitions),
                             5 if role=="libserver.so" else 6)
            for sha,details in definitions.items():
                self.assertEqual(len(sha),64)
                self.assertEqual(len(details[1]),64)
                self.assertLess(details[0],details[2]+0x1000000)

if __name__=="__main__":
    unittest.main()
