"""Synthetic-only regression tests, no original SiN DLL bytes in CI."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_original_sin_dllinit_factories as a

class FakePE:
    def __init__(self):
        self.bytes={}
    def put(self,va,raw):
        for i,v in enumerate(raw): self.bytes[va+i]=v
    def at(self,va,n):
        return bytes(self.bytes.get(va+i,0) for i in range(n))
    def section_at(self,va):
        return ".rdata" if 0x30000000<=va<0x31000000 else ".text"

def direct_fixture(record,failed_exit):
    site,name,provider,storage=record
    obj=FakePE()
    ptr=0x30004000
    obj.put(ptr,name.encode()+b"\x00")
    obj.put(site-2,b"\x6a\x00")
    call=b"\xff\xd7" if provider=="filesystem" else b"\xff\xd6"
    prefix=(b"\x68"+struct.pack("<I",ptr)+call+
            b"\x83\xc4\x08\x85\xc0\xa3"+struct.pack("<I",storage))
    je=site+len(prefix)
    obj.put(site,prefix+b"\x0f\x84"+struct.pack("<i",failed_exit-(je+6)))
    return obj

def derived_fixture(record,failed_exit):
    site,name,slot,storage=record
    obj=FakePE(); ptr=0x30004000
    obj.put(ptr,name.encode()+b"\x00")
    prefix=(b"\x68"+struct.pack("<I",ptr)+
            b"\xff\x50"+bytes([slot])+
            b"\x85\xc0\xa3"+struct.pack("<I",storage))
    obj.put(site,prefix+b"\x0f\x84"+
            struct.pack("<i",failed_exit-(site+len(prefix)+6)))
    return obj

def optional_fixture(record):
    site,name,storage=record
    obj=FakePE(); ptr=0x30004000
    obj.put(ptr,name.encode()+b"\x00")
    obj.put(site,b"\x68"+struct.pack("<I",ptr)+
            b"\xff\xd6\xa3"+struct.pack("<I",storage))
    if name=="VDebugOverlay003":
        obj.put(0x221e795f,b"\x74\x21")
        obj.put(0x221e7979,b"\xb0\x01")
        obj.put(0x221e7998,b"\xb0\x01")
    else:
        obj.put(0x240fe360,b"\x8b\x01")
    return obj

class DLLInitFactoryTests(unittest.TestCase):
    def test_all_16_original_server_requests(self):
        for record in a.SERVER_REQUIRED:
            result=a.check_direct(direct_fixture(record,0x221e79a1),
                                  record,0x221e79a1)
            self.assertEqual(result["name"],record[1])
            self.assertEqual(result["factory"],record[2])

    def test_all_21_original_client_engine_requests(self):
        for record in a.CLIENT_REQUIRED:
            result=a.check_direct(direct_fixture(record,0x240fe59e),
                                  record,0x240fe59e)
            self.assertEqual(result["name"],record[1])

    def test_both_indirect_client_material_requests(self):
        for record in a.CLIENT_DERIVED:
            result=a.check_derived(derived_fixture(record,0x240fe59e),
                                   record,0x240fe59e)
            self.assertEqual(result["name"],record[1])

    def test_optional_does_not_trigger_direct_fail(self):
        for record in a.OPTIONAL.values():
            result=a.check_optional(optional_fixture(record),record)
            self.assertFalse(result["null_directly_fails_init"])

    def test_wrong_2006_factory_name(self):
        record=a.SERVER_REQUIRED[0]; obj=direct_fixture(record,0x221e79a1)
        obj.put(0x30004000,b"VEngineServer020\x00")
        with self.assertRaisesRegex(a.AuditError,"name changed"):
            a.check_direct(obj,record,0x221e79a1)

    def test_wrong_factory_provider(self):
        record=a.SERVER_REQUIRED[10]; obj=direct_fixture(record,0x221e79a1)
        obj.put(record[0]+5,b"\xff\xd6")
        with self.assertRaisesRegex(a.AuditError,"factory callback"):
            a.check_direct(obj,record,0x221e79a1)

    def test_required_null_failure_edge_cannot_drift(self):
        record=a.SERVER_REQUIRED[4]; obj=direct_fixture(record,0x221e79a1)
        obj.put(record[0]+17,b"\x0f\x84\x00\x00\x00\x00")
        with self.assertRaisesRegex(a.AuditError,"failure target"):
            a.check_direct(obj,record,0x221e79a1)

    def test_global_interface_destination_cannot_drift(self):
        record=a.CLIENT_REQUIRED[3]; obj=direct_fixture(record,0x240fe59e)
        obj.put(record[0]+12,b"\xa3\x00\x00\x00\x00")
        with self.assertRaisesRegex(a.AuditError,"global store"):
            a.check_direct(obj,record,0x240fe59e)

    def test_missing_null_test_is_detected(self):
        record=a.SERVER_REQUIRED[2]; obj=direct_fixture(record,0x221e79a1)
        obj.put(record[0]+10,b"\x90\x90")
        with self.assertRaisesRegex(a.AuditError,"EAX null test"):
            a.check_direct(obj,record,0x221e79a1)

    def test_derived_factory_virtual_slot_cannot_change(self):
        record=a.CLIENT_DERIVED[0]; obj=derived_fixture(record,0x240fe59e)
        obj.put(record[0]+7,b"\x24")
        with self.assertRaisesRegex(a.AuditError,"method changed"):
            a.check_derived(obj,record,0x240fe59e)

    def test_original_factories_and_version_pins(self):
        self.assertEqual(len(a.SERVER_REQUIRED),16)
        self.assertEqual(len(a.CLIENT_REQUIRED),21)
        self.assertEqual(len(a.CLIENT_DERIVED),2)
        self.assertEqual(len(a.OPTIONAL),2)
        self.assertEqual(a.SERVER_REQUIRED[10][2],"filesystem")
        self.assertEqual(a.SERVER_REQUIRED[13][1],"VSoundEmitter001")
        self.assertEqual(a.CLIENT_REQUIRED[20][1],"VSoundEmitter001")
        self.assertEqual(a.ORIGINAL["server.dll"][4],16)
        self.assertEqual(a.ORIGINAL["client.dll"][4],12)

    def test_sha_mismatch_rejected_before_original_parsing(self):
        with self.assertRaisesRegex(a.AuditError,"SHA256 mismatch"):
            a.audit_file(b"fabricated PE content","server.dll")

if __name__=="__main__":
    unittest.main()
