#!/usr/bin/env python3
"""Phase225 original-free mutation verification of common native wrappers."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_shared_trace_formatters as native

class FakePE:
    def __init__(self):
        self.base=native.BASE["client"]
        self.mem={}
        self.rdata=set()
        self.data=set()
        for va,n,_sha,padding,witnesses in native.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+n-1,b"\xc3")
            self.write(va+n,b"\xcc"*padding)
        for site,dest in native.PROBE_CALLS+native.NESTED_CALLS:
            self.write(site,native.rel_call(site,dest))
        for va,word in native.LITERALS.items():
            raw=word.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))
        table=[]
        for i,label in enumerate(native.FIELD_NAMES):
            va=0x242A5000+i*0x40
            raw=label.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))
            table.append(va)
        table.append(0)
        self.write(native.TYPE_NAME_TABLE_VA,struct.pack("<27I",*table))
        self.data.update(range(native.TYPE_NAME_TABLE_VA,native.TYPE_NAME_TABLE_VA+27*4))
        self.data.add(native.PREVIEW_GLOBAL_PTR_VA)
        for va,head in native.NEXT_ENTRIES.items():
            self.write(va,head)

    def write(self,va,raw):
        for i,byte in enumerate(raw):
            old=self.mem.get(va+i,byte)
            if old!=byte:
                raise AssertionError(f"synthetic witness collision {va+i:#x}")
            self.mem[va+i]=byte
    def at(self,va,n):
        return bytes(self.mem.get(va+i,0) for i in range(n))
    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.data:return ".data"
        return ".text"
    def flip(self,va):
        self.mem[va]=self.mem.get(va,0)^1

class Phase225Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return native.verify_native(fake,check_hash=False,check_imports=False)

    def test_synthetic_never_authenticates_complete_native_originals(self):
        report=self.verify(FakePE())
        self.assertEqual(report["new_whole_SHA_verified_client_methods"],0)
        self.assertEqual(report["new_complete_original_native_bytes"],0)
        self.assertEqual(report["sparse_original_opcode_witnesses"],53)
        self.assertEqual(report["original_stack_probe_calls"],3)
        self.assertEqual(report["verified_nested_serializer_calls"],21)
        self.assertEqual(report["original_field_type_name_entries"],26)
        self.assertTrue(report["original_field_type_table_terminal_null"])
        self.assertEqual(report["original_format_literals"],5)
        with self.assertRaises(native.VerificationError):
            native.verify_native(FakePE(),check_hash=True,check_imports=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_original_sparse_instruction_witness(self):
        for name,(va,n,digest,padding,witnesses) in native.METHODS.items():
            for off in witnesses:
                with self.subTest(name=name,off=hex(off)):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_probe_and_serializer_CALL_sites(self):
        edges=native.PROBE_CALLS+native.NESTED_CALLS
        for site,dest in edges:
            with self.subTest(site=hex(site),dest=hex(dest)):
                fake=FakePE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_native_C3_RET_and_INT3_boundary_bytes(self):
        for name,(va,n,sha,pad,witnesses) in native.METHODS.items():
            with self.subTest(name=name,kind="RET"):
                fake=FakePE();fake.flip(va+n-1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for off in range(pad):
                with self.subTest(name=name,kind="padding",off=off):
                    fake=FakePE();fake.flip(va+n+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_header_and_preview_format_literals(self):
        for va,literal in native.LITERALS.items():
            for off in (0,len(literal)):
                with self.subTest(string=hex(va),off=off):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_all_original_type_names_and_table_terminal(self):
        for i,original in enumerate(native.FIELD_NAMES):
            va=0x242A5000+i*0x40
            for offset in (0,len(original)):
                with self.subTest(type=i,offset=offset):
                    fake=FakePE();fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)
            fake=FakePE();fake.flip(native.TYPE_NAME_TABLE_VA+i*4)
            with self.assertRaises(native.VerificationError):
                self.verify(fake)
        fake=FakePE();fake.flip(native.TYPE_NAME_TABLE_VA+26*4)
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_import_names_and_data_global(self):
        fake=FakePE()
        fake.data.remove(native.PREVIEW_GLOBAL_PTR_VA)
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        with patch.object(native,"named_iat",
            side_effect=lambda pe,dll,symbol: [
                native.Q_VSNPRINTF_IAT if symbol=="Q_vsnprintf" else native.TIER0_MSG_IAT]):
            native.verify_native(FakePE(),check_hash=False,check_imports=True)
        with patch.object(native,"named_iat",return_value=[]):
            with self.assertRaises(native.VerificationError):
                native.verify_native(FakePE(),check_hash=False,check_imports=True)

    def test_next_function_entries_and_PE_base(self):
        for va,head in native.NEXT_ENTRIES.items():
            for off in range(len(head)):
                with self.subTest(va=hex(va),off=off):
                    fake=FakePE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)
        fake=FakePE();fake.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_three_original_body_lengths(self):
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),444)
        self.assertEqual([row[3] for row in native.METHODS.values()],[10,7,3])
        self.assertEqual({row[0] for row in native.METHODS.values()},
                         {0x24197910,0x241979C0,0x24197FE0})

if __name__=="__main__":
    unittest.main()
