"""Synthetic original-byte-free regressions for AutoGameSystem PE audit."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_auto_gamesystem_init import (
    REGIONS, ANCHORS, IMAGE_BASE, AuditError, CALL_VA, NATIVE_MESSAGE_VA,
    NATIVE_MESSAGE, DIRECT_CALLS, REGISTER_TARGET, verify_native, audit_original)
class Fake:
    base=IMAGE_BASE
    def __init__(self):
        self.content={}
        self.regions={}
        for i,(key,(va,length,_)) in enumerate(REGIONS.items()):
            data=bytes([i+10])*length
            self.content[va,length]=data
            self.regions[key]=(va,length,hashlib.sha256(data).hexdigest())
        for va,content in ANCHORS.items():self.content[va,len(content)]=content
        self.content[NATIVE_MESSAGE_VA,len(NATIVE_MESSAGE)]=NATIVE_MESSAGE
        target=REGIONS["merge_and_init"][0]
        self.content[CALL_VA,5]=b"\xe8"+struct.pack("<i",target-CALL_VA-5)
        for addr in DIRECT_CALLS:
            self.content[addr,5]=b"\xe8"+struct.pack("<i",REGISTER_TARGET-addr-5)
    def read(self,va,length):return self.content[va,length]
class AuditTests(unittest.TestCase):
    def test_positive_metadata(self):
        mock=Fake()
        result=verify_native(mock,mock.regions)
        self.assertEqual(result["complete_function_or_callsite_regions"],4)
        self.assertEqual(result["callback_vtable_slot"],0)
        self.assertEqual(result["direct_registration_calls"],12)
        self.assertEqual(result["pending_node_next_offset_x86"],4)
    def test_mutated_functions_rejected(self):
        for key in REGIONS:
            m=Fake()
            addr,length,_=m.regions[key]
            m.content[addr,length]=bytes(length)
            with self.subTest(region=key),self.assertRaisesRegex(AuditError,"SHA"):
                verify_native(m,m.regions)
    def test_mutated_opcodes_rejected(self):
        for va,raw in ANCHORS.items():
            m=Fake()
            m.content[va,len(raw)]=bytes([raw[0]^1])+raw[1:]
            with self.subTest(va=hex(va)),self.assertRaisesRegex(AuditError,"instruction"):
                verify_native(m,m.regions)
    def test_mutated_warning_or_client_init_edge_rejected(self):
        m=Fake();m.content[NATIVE_MESSAGE_VA,len(NATIVE_MESSAGE)]=bytes(len(NATIVE_MESSAGE))
        with self.assertRaisesRegex(AuditError,"warning"):verify_native(m,m.regions)
        m=Fake();m.content[CALL_VA,5]=b"\xe8\x00\x00\x00\x00"
        with self.assertRaisesRegex(AuditError,"call"):verify_native(m,m.regions)
    def test_every_original_direct_registration_call_is_pinned(self):
        for addr in DIRECT_CALLS:
            m=Fake();m.content[addr,5]=b"\xe8"+bytes(4)
            with self.subTest(addr=hex(addr)),self.assertRaisesRegex(
                AuditError,"twelve direct"):
                verify_native(m,m.regions)
    def test_wrong_original_and_image_base_refused(self):
        m=Fake();m.base=0x400000
        with self.assertRaisesRegex(AuditError,"image base"):verify_native(m,m.regions)
        with tempfile.TemporaryDirectory() as path:
            sample=pathlib.Path(path)/"client.dll";sample.write_bytes(b"invalid original")
            with self.assertRaisesRegex(AuditError,"SHA256"):audit_original(sample)
if __name__=="__main__":unittest.main()
