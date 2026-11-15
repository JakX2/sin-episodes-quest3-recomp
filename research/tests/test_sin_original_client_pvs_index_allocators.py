"""Phase234 synthetic evidence mutations: never credit fake original bytes."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_pvs_index_allocators as native

class FakePE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.mem={}
        self.text=set()
        self.write(native.ERROR_STRING_VA,native.ERROR_STRING)
        for name,va,size,digest,pad,rets,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.write(va+size,b"\xCC"*pad)
            self.text.update(range(va,va+size))
            for off in rets:self.write(va+off,b"\xC2\x04\x00")
            for off,hx in anchors.items():self.write(va+off,bytes.fromhex(hx))
        for site,dest in native.CALLS:
            self.write(site,native.rel_call(site,dest))
            self.text.update(range(site,site+5))
    def write(self,va,data):
        for i,c in enumerate(data):self.mem[va+i]=c
    def at(self,va,count):
        return bytes(self.mem.get(va+i,0) for i in range(count))
    def section_of(self,va):
        if va in self.text:return ".text"
        if native.ERROR_STRING_VA<=va<native.ERROR_STRING_VA+len(native.ERROR_STRING):return ".rdata"
        return ""
    def flip(self,va):self.mem[va]=self.mem.get(va,0)^1

class TestSyntheticPhase234(unittest.TestCase):
    @staticmethod
    def run_fake(fake):
        return native.verify_native(fake,check_hash=False,authentic_provenance=False)
    def test_synthetic_not_original(self):
        report=self.run_fake(FakePE())
        self.assertEqual(report["new_original_complete_sha_methods"],0)
        self.assertEqual(report["new_original_code_bytes"],0)
        self.assertEqual(report["sparse_instruction_witnesses"],24)
        self.assertEqual(report["original_call_edges"],4)
        self.assertEqual(report["prev_next_offsets"],[[4,6],[12,14]])
        with self.assertRaises(native.VerificationError):native.verify_native(FakePE())
        with self.assertRaises(native.VerificationError):native.audit(b"MZ",b"MZ")
    def test_each_witness_mutates(self):
        for _,va,_,_,_,_,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(va=hex(va),offset=hex(off)):
                    p=FakePE();p.flip(va+off)
                    with self.assertRaises(native.VerificationError):self.run_fake(p)
    def test_return_padding_and_calls_mutate(self):
        for _,va,size,_,pad,rets,_ in native.METHODS:
            for off in rets:
                for i in range(3):
                    p=FakePE();p.flip(va+off+i)
                    with self.assertRaises(native.VerificationError):self.run_fake(p)
            for off in range(pad):
                p=FakePE();p.flip(va+size+off)
                with self.assertRaises(native.VerificationError):self.run_fake(p)
        for site,dest in native.CALLS:
            for i in range(5):
                p=FakePE();p.flip(site+i)
                with self.assertRaises(native.VerificationError):self.run_fake(p)
    def test_bad_base_section_and_error(self):
        p=FakePE();p.base+=1
        with self.assertRaises(native.VerificationError):self.run_fake(p)
        p=FakePE();p.text.remove(native.METHODS[0][1])
        with self.assertRaises(native.VerificationError):self.run_fake(p)
        p=FakePE();p.flip(native.ERROR_STRING_VA)
        with self.assertRaises(native.VerificationError):self.run_fake(p)
    def test_original_scope(self):
        self.assertEqual(len(native.METHODS),2)
        self.assertEqual(sum(m[2] for m in native.METHODS),308)
        for method in native.METHODS:
            self.assertEqual(method[4],6)
            self.assertEqual(method[5],(0x77,0x97))
if __name__=="__main__":unittest.main()
