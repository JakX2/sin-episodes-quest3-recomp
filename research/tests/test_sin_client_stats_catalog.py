"""Original-free mutation tests for 2006 Stats 66-record catalog setup."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_stats_catalog import (
    VA,SIZE,CALLS,ANCHORS,verify,audit_original,AuditError)
class Fake:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.body=b"Z"*SIZE
        self.data[VA,SIZE]=self.body
        for site,target in CALLS.items():
            self.data[site,5]=b"\xe8"+struct.pack("<i",target-site-5)
        for site,raw in ANCHORS.items():
            self.data[site,len(raw)]=raw
    def read(self,site,n):return self.data[site,n]
    def run(self):return verify(self,hashlib.sha256(self.body).hexdigest())
class CatalogAudit(unittest.TestCase):
    def test_synthetic_accept_and_report(self):
        out=Fake().run()
        self.assertEqual(out["complete_original_helper_bytes"],292)
        self.assertEqual(out["original_catalog_slot_count"],66)
        self.assertEqual(out["original_entry_initial_fields"],[0,0,-1])
        self.assertEqual(out["validated_direct_call_sites"],10)
    def test_complete_function_rejected_on_bad_sha(self):
        with self.assertRaisesRegex(AuditError,"full function SHA"):
            verify(Fake())
    def test_all_native_callsites_rejected_on_mutation(self):
        for site in CALLS:
            f=Fake();f.data[site,5]=b"\xe8"+bytes(4)
            with self.subTest(site=hex(site)),self.assertRaisesRegex(AuditError,"callsite"):
                f.run()
    def test_all_native_witnesses_rejected_on_mutation(self):
        for site,data in ANCHORS.items():
            f=Fake();f.data[site,len(data)]=bytes([data[0]^1])+data[1:]
            with self.subTest(site=hex(site)),self.assertRaisesRegex(AuditError,"witness"):
                f.run()
    def test_wrong_file_and_base_rejected(self):
        f=Fake();f.base=0x400000
        with self.assertRaisesRegex(AuditError,"base"):f.run()
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/"client.dll";p.write_bytes(b"invalid")
            with self.assertRaisesRegex(AuditError,"SHA"):audit_original(p)
if __name__=="__main__":unittest.main()
