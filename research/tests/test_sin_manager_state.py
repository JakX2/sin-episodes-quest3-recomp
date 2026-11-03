"""Original-binary-free mutation tests for Phase 77 archive evidence."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_manager_state import (SERVER_BYTES,CALLS,verify_original,
                                     VerificationError,audit,PEImage)
class FakePE:
    def __init__(self):self.data={}
    def put(self,va,raw):
        for i,value in enumerate(raw):
            key=va+i
            if key in self.data and self.data[key]!=value:
                raise AssertionError('overlapping fixture')
            self.data[key]=value
    def at(self,va,size):
        return bytes(self.data.get(va+i,0xcc) for i in range(size))
    def corrupt(self,va):
        self.data[va]^=0x10

def fixture():
    pe=FakePE()
    for va,encoded in SERVER_BYTES.items():
        pe.put(va,bytes.fromhex(encoded))
    for va,target in CALLS.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    return pe

class ManagerArchiveTests(unittest.TestCase):
    def test_original_archive_structure(self):
        report=verify_original(fixture())
        self.assertEqual(report['metrics']['written_f32'],14)
        self.assertEqual(report['success_modifiers']['written_f32'],20)
        self.assertEqual(report['success_modifiers']['max_restored_pairs'],10)
        self.assertEqual(report['success_modifiers']['selectors'],[1,3])
        self.assertEqual(report['opaque_advisors']['written_objects'],6)
        self.assertEqual(report['verified_instruction_sites'],len(SERVER_BYTES))
        self.assertEqual(report['verified_direct_call_edges'],len(CALLS))
    def test_every_instruction_rejects_mutations(self):
        for va in SERVER_BYTES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original(pe)
    def test_every_direct_call_rejects_mutations(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original(pe)
    def test_original_sha256_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')
    def test_pe32_guard(self):
        with self.assertRaisesRegex(VerificationError,'MZ'):
            PEImage(b'invalid')
if __name__=='__main__':
    unittest.main()
