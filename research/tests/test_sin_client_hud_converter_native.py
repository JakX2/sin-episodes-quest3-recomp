"""Synthetic regression coverage for original HUD converter x86 ABI audit."""
import hashlib
import importlib.util
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_hud_converter_native import (
    verify_layout, audit_original, AuditError, REGIONS,
    ANCHORS, EXPECTED_LITERALS, VTABLE_VA, STATIC_CONVERTER_VA,
    CALL_VA, IMAGE_BASE)
class FakePE:
    base=IMAGE_BASE
    def __init__(self):
        self.data={}
        self.regions={}
        for i,(key,(va,size,_)) in enumerate(REGIONS.items()):
            contents=bytes((i+7,))*size
            self.data[va,size]=contents
            self.regions[key]=(va,size,hashlib.sha256(contents).hexdigest())
        self.data[STATIC_CONVERTER_VA,4]=struct.pack("<I",VTABLE_VA)
        methods=[REGIONS[key][0] for key in (
            "hud_texture_getdata","hud_texture_setdata",
            "hud_texture_initfromdefault")]
        for i,addr in enumerate(methods):
            self.data[VTABLE_VA+i*4,4]=struct.pack("<I",addr)
        for va,payload in ANCHORS.items():self.data[va,len(payload)]=payload
        for va,payload in EXPECTED_LITERALS.items():self.data[va,len(payload)]=payload
        addr=REGIONS["property_converter_registration"][0]
        self.data[CALL_VA,5]=b"\xe8"+struct.pack("<i",addr-CALL_VA-5)
    def read(self,va,n):
        return self.data[va,n]
class ConverterAudit(unittest.TestCase):
    def test_positive_synthetic_metadata(self):
        mock=FakePE()
        result=verify_layout(mock,mock.regions)
        self.assertEqual(result["bounded_sha_regions"],4)
        self.assertEqual([x["stdcall_stack_bytes"] for x in result["converter"]["virtuals"]],[12,12,8])
        self.assertEqual(result["original_virtual_calls_executed"],0)
    def test_all_function_sha_fail_closed(self):
        for key in REGIONS:
            mock=FakePE()
            va,size,_=mock.regions[key]
            mock.data[va,size]=bytes((0,))*size
            with self.subTest(key=key),self.assertRaisesRegex(AuditError,"SHA mismatch"):
                verify_layout(mock,mock.regions)
    def test_vtable_and_static_pointer_are_not_interchangeable(self):
        for index in range(4):
            mock=FakePE()
            va=STATIC_CONVERTER_VA if index==0 else VTABLE_VA+(index-1)*4
            mock.data[va,4]=struct.pack("<I",0x12345678)
            with self.subTest(index=index),self.assertRaisesRegex(AuditError,"vtable"):
                verify_layout(mock,mock.regions)
    def test_literals_and_native_opcodes_fail_closed(self):
        for va,payload in {**EXPECTED_LITERALS,**ANCHORS}.items():
            mock=FakePE()
            mock.data[va,len(payload)]=bytes([payload[0]^1])+payload[1:]
            with self.subTest(va=hex(va)),self.assertRaises(AuditError):
                verify_layout(mock,mock.regions)
    def test_registrar_call_edge_fails_closed(self):
        mock=FakePE()
        mock.data[CALL_VA,5]=b"\xe8\x00\x00\x00\x00"
        with self.assertRaisesRegex(AuditError,"call edge"):
            verify_layout(mock,mock.regions)
    def test_reject_wrong_image_base_and_nonoriginal_input(self):
        mock=FakePE()
        mock.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):
            verify_layout(mock,mock.regions)
        with tempfile.TemporaryDirectory() as root:
            path=pathlib.Path(root)/"client.dll"
            path.write_bytes(b"not a genuine original")
            with self.assertRaisesRegex(AuditError,"full SHA256"):
                audit_original(path)
if __name__=="__main__":unittest.main()
