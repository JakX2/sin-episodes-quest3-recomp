"""Original-file-free mutation tests for Stats storage helper audit."""
import hashlib
import pathlib
import struct
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from audit_sin_client_stats_storage import (
    REGIONS,CALLS,ANCHORS,DISPATCH_VA,DISPATCH_SIZE,
    verify,audit_original,AuditError)
class Fake:
    base=0x24000000
    def __init__(self):
        self.data={}
        self.regions={}
        for i,(name,(va,size,_)) in enumerate(REGIONS.items()):
            contents=bytes([20+i])*size
            self.data[va,size]=contents
            self.regions[name]=(va,size,hashlib.sha256(contents).hexdigest())
        self.dispatch=b"r"*DISPATCH_SIZE
        self.data[DISPATCH_VA,DISPATCH_SIZE]=self.dispatch
        for site,target in CALLS.items():
            self.data[site,5]=b"\xe8"+struct.pack("<i",target-site-5)
        for site,payload in ANCHORS.items():
            self.data[site,len(payload)]=payload
    def read(self,va,size):return self.data[va,size]
    def run(self):return verify(self,self.regions,
                  hashlib.sha256(self.dispatch).hexdigest())
class StatsStorageAudit(unittest.TestCase):
    def test_good_metadata(self):
        m=Fake().run()
        self.assertEqual(m["static_entry_count"],55)
        self.assertEqual(m["dynamic_entry_count"],11)
        self.assertEqual(m["static_entry_bytes"],8)
        self.assertEqual(m["dynamic_entry_bytes"],20)
        self.assertEqual(m["stat_factory_specialized_types"],16)
    def test_all_full_method_shas_refused_on_mutation(self):
        for name in REGIONS:
            p=Fake();va,size,_=p.regions[name]
            p.data[va,size]=bytes(size)
            with self.subTest(name=name),self.assertRaisesRegex(AuditError,"body SHA"):
                p.run()
        p=Fake();p.data[DISPATCH_VA,DISPATCH_SIZE]=bytes(DISPATCH_SIZE)
        with self.assertRaisesRegex(AuditError,"dispatch body SHA"):p.run()
    def test_all_callsite_mutations_fail(self):
        for site in CALLS:
            p=Fake();p.data[site,5]=b"\xe8"+bytes(4)
            with self.subTest(site=hex(site)),self.assertRaisesRegex(AuditError,"call edge"):
                p.run()
    def test_all_opcode_anchors_fail_closed(self):
        for site,raw in ANCHORS.items():
            p=Fake()
            p.data[site,len(raw)]=bytes([raw[0]^1])+raw[1:]
            with self.subTest(site=hex(site)),self.assertRaisesRegex(AuditError,"opcode"):
                p.run()
    def test_wrong_file_and_image_base_refused(self):
        p=Fake();p.base=0x400000
        with self.assertRaisesRegex(AuditError,"base"):p.run()
        with tempfile.TemporaryDirectory() as d:
            sample=pathlib.Path(d)/"client.dll";sample.write_bytes(b"not original")
            with self.assertRaisesRegex(AuditError,"full SHA"):
                audit_original(sample)
if __name__=="__main__":unittest.main()
