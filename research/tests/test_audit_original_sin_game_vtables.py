"""Synthetic PE32 MSVC RTTI game vtable regression; NO original DLL required."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest
from unittest import mock

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sourcevr_game_module_vtables as a


class SyntheticPE:
    def __init__(self, role="server.dll"):
        self.role=role
        self.data=bytearray(0x3400)
        self.data[:2]=b"MZ"
        struct.pack_into("<I",self.data,0x3c,0x100)
        self.data[0x100:0x104]=b"PE\x00\x00"
        struct.pack_into("<H",self.data,0x104,0x14c)
        struct.pack_into("<H",self.data,0x106,3)
        struct.pack_into("<H",self.data,0x114,0xe0)
        struct.pack_into("<H",self.data,0x118,0x10b)
        struct.pack_into("<I",self.data,0x118+28,0x22000000)
        for i,(name,rva,raw) in enumerate((
            (b".text",0x1000,0x400),
            (b".rdata",0x3000,0x1400),
            (b".data",0x5000,0x2400),
        )):
            at=0x1f8+40*i
            self.data[at:at+len(name)]=name
            struct.pack_into("<IIII",self.data,at+8,0x1000,rva,0x1000,raw)
        self.name=(b".?AVCServerGameDLL@@" if role=="server.dll"
                   else b".?AVCHLClient@@")
        self.data[0x2480:0x2480+len(self.name)+1]=self.name+b"\x00"
        self.col_va=0x22003200
        self.vtable_va=0x22003104
        self.count=31 if role=="server.dll" else 48
        struct.pack_into("<IIIII",self.data,0x1600,
                         0,0,0,0x22005078,0x22003300)
        struct.pack_into("<I",self.data,0x1500,self.col_va)
        self.ptrs=[0x22001100+8*i for i in range(self.count)]
        struct.pack_into("<%dI" % self.count,self.data,0x1504,*self.ptrs)
        self.table_hash=hashlib.sha256(self.data[
            0x1504:0x1504+4*self.count]).hexdigest()
        self.sha=hashlib.sha256(self.data).hexdigest()

    def evidence(self):
        return (self.name,self.col_va,self.vtable_va,
                self.count,self.table_hash,self.ptrs[0],self.ptrs[-1])

    def examine(self):
        # The production inspector still SHA-checks before reading PE bytes.
        with mock.patch.dict(a.ORIGINAL,{self.role:(self.sha,"FakeFactory")},
                             clear=False):
            with mock.patch.dict(a.ORIGINAL_VTABLES,
                                 {self.role:self.evidence()},clear=False):
                return a.inspect_original_pe_game_table(bytes(self.data),self.role)


class OriginalGameVTableTests(unittest.TestCase):
    def test_valid_synthetic_server_31_slots(self):
        fixture=SyntheticPE("server.dll")
        result=fixture.examine()
        self.assertEqual(result["method_slots"],31)
        self.assertEqual(result["pointer_bits"],32)
        self.assertEqual(result["complete_object_locator_va"],hex(fixture.col_va))

    def test_valid_synthetic_client_48_slots(self):
        fixture=SyntheticPE("client.dll")
        result=fixture.examine()
        self.assertEqual(result["method_slots"],48)
        self.assertEqual(result["vtable_va"],hex(fixture.vtable_va))

    def test_mismatched_SHA256_rejected(self):
        fixture=SyntheticPE()
        fixture.sha="0"*64
        with self.assertRaisesRegex(a.AuditError,"SHA256 changed"):
            fixture.examine()

    def test_wrong_PE_machine(self):
        fixture=SyntheticPE()
        struct.pack_into("<H",fixture.data,0x104,0x8664)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"must be x86"):
            fixture.examine()

    def test_wrong_Microsoft_type_descriptor(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,0x160c,0x2200507c)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"TypeDescriptor"):
            fixture.examine()

    def test_wrong_CLO_header(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,0x1600,1)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"locator header"):
            fixture.examine()

    def test_duplicate_CLO_link(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,0x1700,fixture.col_va)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"locator reference ambiguous"):
            fixture.examine()

    def test_nonexecutable_x86_method_pointer(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,0x1504+3*4,0x22005080)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"non-text pointer"):
            fixture.examine()

    def test_wrong_table_fingerprint(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,0x1504+3*4,0x22001200)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"pointer table SHA256"):
            fixture.examine()

    def test_extra_executable_x86_method_rejected(self):
        fixture=SyntheticPE()
        struct.pack_into("<I",fixture.data,
                         0x1504+fixture.count*4,0x22001500)
        fixture.sha=hashlib.sha256(fixture.data).hexdigest()
        with self.assertRaisesRegex(a.AuditError,"unexpectedly extends"):
            fixture.examine()

    def test_original_witness_scope(self):
        self.assertEqual(len(a.ORIGINAL_VTABLES),2)
        self.assertEqual(a.ORIGINAL_VTABLES["server.dll"][3],31)
        self.assertEqual(a.ORIGINAL_VTABLES["client.dll"][3],48)
        self.assertEqual(a.CLASS["libserver.so"][2],47)
        self.assertEqual(a.CLASS["libclient.so"][2],78)
        self.assertEqual(47-31,16)
        self.assertEqual(78-48,30)

if __name__=="__main__":
    unittest.main()
