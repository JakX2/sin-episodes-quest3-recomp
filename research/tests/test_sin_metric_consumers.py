"""Synthetic original-PE-free mutations for Phase 79 x86 evidence."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_metric_consumers import (
    OPCODES,DIRECT_CALLS,FUNCTION_POINTERS,REGION_HASHES,
    verify_consumers,VerificationError,audit)

class FakePE:
    def __init__(self):self.memory={}
    def at(self,address,count):
        return bytes(self.memory.get(address+i,0xCC) for i in range(count))
    def put(self,address,raw):
        for i,b in enumerate(raw):
            k=address+i
            if k in self.memory and self.memory[k]!=b:
                raise AssertionError('conflicting synthetic fixture at %#x'%k)
            self.memory[k]=b
    def corrupt(self,address):self.memory[address]^=0x20

def fixture():
    pe=FakePE()
    for va,raw in OPCODES.items():pe.put(va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in FUNCTION_POINTERS.items():
        pe.put(va,struct.pack('<I',target))
    return pe

class ConsumerTests(unittest.TestCase):
    def test_reference(self):
        report=verify_consumers(fixture(),False)
        self.assertEqual(report['opcode_sites'],31)
        self.assertEqual(report['call_edges'],8)
        self.assertEqual(report['function_pointer_sites'],4)
        self.assertEqual(len(report['metric_consumers']),4)
        self.assertEqual(report['source_object_identity'],'UNKNOWN')
    def test_every_opcode_mutation(self):
        for va in OPCODES:
            with self.subTest(address=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_consumers(pe,False)
    def test_every_call_mutation(self):
        for va in DIRECT_CALLS:
            with self.subTest(address=hex(va)):
                pe=fixture();pe.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_consumers(pe,False)
    def test_every_pointer_mutation(self):
        for va in FUNCTION_POINTERS:
            with self.subTest(address=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_consumers(pe,False)
    def test_actual_original_regions_mandatory(self):
        self.assertEqual(len(REGION_HASHES),6)
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            verify_consumers(fixture(),True)
    def test_owner_Dll_hashes_mandatory(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
