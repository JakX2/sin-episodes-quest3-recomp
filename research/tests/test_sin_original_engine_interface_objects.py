"""Phase178 original-free tests for nine native factory/RTTI/vtable pins."""
from __future__ import annotations
import copy
import json
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_engine_interface_objects import (
    original_manifest, verify_original_objects, VerificationError,
    MANIFEST_SHA256, MANIFEST_FILE, EXPECTED_COUNT, EXPECTED_STATIC_SLOTS,
)

class FakePE:
    def __init__(self,dll):
        self.dll=dll
        self.mem={}
    def section_of(self,va):
        if self.dll=='server':
            if 0x22001000 <= va < 0x22400000:return '.text'
            if 0x22400000 <= va < 0x22500000:return '.rdata'
            if 0x22500000 <= va < 0x22600000:return '.data'
        else:
            if 0x24001000 <= va < 0x24280000:return '.text'
            if 0x24280000 <= va < 0x242f0000:return '.rdata'
            if 0x242f0000 <= va < 0x24390000:return '.data'
        return ''
    def put(self,va,raw):
        for i,c in enumerate(raw):
            if va+i in self.mem and self.mem[va+i]!=c:
                raise AssertionError(f'overlapping witness {va+i:#x}')
            self.mem[va+i]=c
    def at(self,va,n):
        if any(va+i not in self.mem for i in range(n)):
            raise VerificationError('uninitialized virtual PE section')
        return bytes(self.mem[va+i] for i in range(n))
    def damage(self,va):self.mem[va]^=0x01


def fixture(dll):
    manifests=original_manifest()['dlls'][dll]['interfaces']
    pe=FakePE(dll)
    for rec in manifests:
        fn=int(rec['factory_va'],16);obj=int(rec['singleton_va'],16)
        pe.put(fn,bytes((0xb8,))+struct.pack('<I',obj)+bytes((0xc3,)))
        if rec['vtable_storage']=='runtime_uninitialized':continue
        vt=int(rec['static_vtable_va'],16)
        pe.put(obj,struct.pack('<I',vt))
        pe.put(vt,struct.pack('<'+'I'*len(rec['static_slots']),
                            *[int(x,16) for x in rec['static_slots']]))
        pe.put(vt+4*len(rec['static_slots']),
               struct.pack('<I',int(rec['first_noncode_word_after_prefix'],16)))
        col=int(rec['msvc_rtti_col_va'],16)
        pe.put(vt-4,struct.pack('<I',col))
        pe.put(col,struct.pack('<5I',0,0,0,int(rec['msvc_type_descriptor_va'],16),
                                   int(rec['msvc_class_hierarchy_va'],16)))
        pe.put(int(rec['msvc_type_descriptor_va'],16)+8,
               rec['msvc_type_name'].encode()+b'\0')
    lookup={rec['interface']:int(rec['factory_va'],16) for rec in manifests}
    return pe,manifests,lookup

class OriginalInterfaceObjectTests(unittest.TestCase):
    def test_manifest_is_pinned_and_counted(self):
        import hashlib
        self.assertEqual(hashlib.sha256(MANIFEST_FILE.read_bytes()).hexdigest(),MANIFEST_SHA256)
        for dll in ('server','client'):
            rows=original_manifest()['dlls'][dll]['interfaces']
            self.assertEqual(len(rows),EXPECTED_COUNT[dll])
            self.assertEqual(sum(len(r['static_slots']) for r in rows),EXPECTED_STATIC_SLOTS[dll])
            for r in rows:
                self.assertEqual(r['factory_len'],6)
                self.assertEqual(len(bytes.fromhex(r['factory_sha256'])),32)
    def test_synthetic_complete_native_fixtures(self):
        for dll in ('server','client'):
            with self.subTest(dll=dll):
                pe,rows,lookup=fixture(dll)
                result=verify_original_objects(pe,dll,rows,lookup)
                self.assertEqual(result['original_factory_body_hashes_verified'],len(rows))
                self.assertEqual(result['original_static_vtable_method_pointers'],EXPECTED_STATIC_SLOTS[dll])
    def test_every_native_factory_byte_mutation_rejected(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                for pos in range(6):
                    with self.subTest(dll=dll,name=r['interface'],byte=pos):
                        bad=copy.deepcopy(pe);bad.damage(int(r['factory_va'],16)+pos)
                        with self.assertRaises(VerificationError):
                            verify_original_objects(bad,dll,rows,reg)
    def test_all_105_vtable_slot_mutations_rejected(self):
        count=0
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='pe_initialized':continue
                vt=int(r['static_vtable_va'],16)
                for slot in range(len(r['static_slots'])):
                    with self.subTest(dll=dll,name=r['interface'],slot=slot):
                        bad=copy.deepcopy(pe);bad.damage(vt+4*slot)
                        with self.assertRaisesRegex(VerificationError,'vtable slot'):
                            verify_original_objects(bad,dll,rows,reg)
                        count+=1
        self.assertEqual(count,105)
    def test_all_rtti_col_fields_mutations_rejected(self):
        count=0
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='pe_initialized':continue
                col=int(r['msvc_rtti_col_va'],16)
                for i in range(5):
                    with self.subTest(dll=dll,name=r['interface'],field=i):
                        bad=copy.deepcopy(pe);bad.damage(col+4*i)
                        with self.assertRaises(VerificationError):
                            verify_original_objects(bad,dll,rows,reg)
                        count+=1
        self.assertEqual(count,25)
    def test_rtti_type_name_mutation_rejected(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='pe_initialized':continue
                bad=copy.deepcopy(pe);bad.damage(int(r['msvc_type_descriptor_va'],16)+8)
                with self.assertRaisesRegex(VerificationError,'RTTI type name'):
                    verify_original_objects(bad,dll,rows,reg)
    def test_vtable_boundary_mutation_rejected(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='pe_initialized':continue
                bad=copy.deepcopy(pe)
                bad.damage(int(r['static_vtable_va'],16)+4*len(r['static_slots']))
                with self.assertRaisesRegex(VerificationError,'boundary'):
                    verify_original_objects(bad,dll,rows,reg)
    def test_object_vtable_pointer_mutation_rejected(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='pe_initialized':continue
                bad=copy.deepcopy(pe);bad.damage(int(r['singleton_va'],16))
                with self.assertRaisesRegex(VerificationError,'object/vtable'):
                    verify_original_objects(bad,dll,rows,reg)
    def test_dynamic_bss_object_rejected_when_fabricated(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            for r in rows:
                if r['vtable_storage']!='runtime_uninitialized':continue
                bad=copy.deepcopy(pe);bad.put(int(r['singleton_va'],16),struct.pack('<I',0xdeadbeef))
                with self.assertRaisesRegex(VerificationError,'supposedly-uninitialized'):
                    verify_original_objects(bad,dll,rows,reg)
    def test_registration_factory_mismatch_rejected(self):
        for dll in ('server','client'):
            pe,rows,reg=fixture(dll)
            bad=reg.copy();bad[rows[0]['interface']]+=16
            with self.assertRaisesRegex(VerificationError,'registration/factory'):
                verify_original_objects(pe,dll,rows,bad)
    def test_missing_virtual_method_or_wrong_section_rejected(self):
        pe,rows,reg=fixture('client')
        r=next(x for x in rows if x['interface']=='VClient011')
        bad=copy.deepcopy(pe)
        target=int(r['static_vtable_va'],16)+4
        for i,c in enumerate(struct.pack('<I',0x00001000)):bad.mem[target+i]=c
        with self.assertRaisesRegex(VerificationError,'vtable slot'):
            verify_original_objects(bad,'client',rows,reg)

if __name__ == '__main__':unittest.main()