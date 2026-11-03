"""Synthetic, game-binary-free mutations for Phase 78 original math evidence."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_metric_math import (
    OPCODES,DIRECT_CALLS,METRIC_NAMES,METRIC_PTR_TABLE,METRIC_DATA_BASE,
    CRITICAL_FLOATS,verify_metric_math,VerificationError,audit,
)

class SyntheticPE:
    def __init__(self):self.memory={}
    def put(self,va,raw):
        for i,b in enumerate(raw):
            k=va+i
            if k in self.memory and self.memory[k]!=b:
                raise AssertionError('fixture byte conflict at %#x'%k)
            self.memory[k]=b
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0xcc) for i in range(size))
    def corrupt(self,va):self.memory[va]^=0x20

def fixture():
    p=SyntheticPE()
    for va,raw in OPCODES.items():p.put(va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for i,name in enumerate(METRIC_NAMES):
        addr=0x23000000+i*128
        p.put(METRIC_PTR_TABLE+i*4,struct.pack('<I',addr))
        p.put(addr,name.encode()+b'\0')
        p.put(METRIC_DATA_BASE+i*28,bytes(28))
    # Override zeros in critical fields using byte-level rewrite, synthetic only.
    for (row,col),v in CRITICAL_FLOATS.items():
        ptr=METRIC_DATA_BASE+row*28+col*4
        for k,b in enumerate(struct.pack('<f',v)):
            p.memory[ptr+k]=b
    return p

class OriginalMetricTests(unittest.TestCase):
    def test_complete_synthetic_metric_identity(self):
        report=verify_metric_math(fixture(),False)
        self.assertEqual(report['metric_count'],14)
        self.assertEqual(report['metric_record_stride_bytes'],28)
        self.assertEqual(report['modifier_count'],10)
        self.assertEqual(report['checked_opcode_sites'],len(OPCODES))
        self.assertEqual(report['checked_direct_call_sites'],len(DIRECT_CALLS))
        self.assertEqual(report['metric_names'],list(METRIC_NAMES))
    def test_all_opcode_sites_reject_mutation(self):
        for va in OPCODES:
            with self.subTest(site=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):verify_metric_math(p,False)
    def test_all_call_targets_reject_mutation(self):
        for va in DIRECT_CALLS:
            with self.subTest(site=hex(va)):
                p=fixture();p.corrupt(va+1)
                with self.assertRaises(VerificationError):verify_metric_math(p,False)
    def test_all_metric_name_pointers_reject_mutation(self):
        for idx in range(14):
            with self.subTest(metric_index=idx):
                p=fixture();p.corrupt(METRIC_PTR_TABLE+idx*4)
                with self.assertRaises(VerificationError):verify_metric_math(p,False)
    def test_all_original_selected_floats_reject_mutation(self):
        for row,col in CRITICAL_FLOATS:
            with self.subTest(row=row,col=col):
                p=fixture();p.corrupt(METRIC_DATA_BASE+row*28+col*4)
                with self.assertRaises(VerificationError):verify_metric_math(p,False)
    def test_region_hash_pin_rejects_synthetic_source(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            verify_metric_math(fixture(),True)
    def test_original_DLL_fingerprints_are_mandatory(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fabricated client',b'fabricated server')

if __name__=='__main__':unittest.main()
