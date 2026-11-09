"""Original-free tamper tests for Phase151 CFloatShortTermStat x86 witnesses."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_float_short_term_stat import (
    REGIONS,OPCODES,CALLS,RTTI,TYPE_NAME_VA,TYPE_NAME,
    VTABLE_START,VTABLE_TARGETS,OriginalBinaryMismatch,verify,audit,
)

class FakePE:
    base=0x22000000
    def __init__(self):self.data={}
    def put(self,va,buf):
        for i,b in enumerate(buf):
            if va+i in self.data and self.data[va+i]!=b:
                raise AssertionError('overlapping synthetic code witnesses at %#x' % (va+i))
            self.data[va+i]=b
    def at(self,va,n):return bytes(self.data.get(va+i,0xcc) for i in range(n))
    def tamper(self,va):self.data[va]^=0x10

def fixture():
    pe=FakePE()
    for va,val in RTTI.items():pe.put(va,struct.pack('<I',val))
    pe.put(TYPE_NAME_VA,TYPE_NAME)
    for slot,target in enumerate(VTABLE_TARGETS):pe.put(VTABLE_START+slot*4,struct.pack('<I',target))
    for va,hexbytes in OPCODES.items():pe.put(va,bytes.fromhex(hexbytes))
    for va,target in CALLS.items():pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    return pe

class FloatShortTermStatTests(unittest.TestCase):
    def test_synthetic_evidence_accepts(self):
        report=verify(fixture(),False)
        self.assertEqual(report['virtual_slots'],13)
        self.assertEqual(report['original_class'],'CFloatShortTermStat')
        self.assertEqual(report['opcode_witnesses'],len(OPCODES))
        self.assertEqual(report['direct_calls'],4)
        self.assertEqual(report['validated_functions'],0)
    def test_every_opcode_anchor_is_required(self):
        for site in OPCODES:
            with self.subTest(address=hex(site)):
                p=fixture();p.tamper(site)
                with self.assertRaises(OriginalBinaryMismatch):verify(p,False)
    def test_each_vtable_slot_is_required(self):
        for slot in range(len(VTABLE_TARGETS)):
            with self.subTest(slot=slot):
                p=fixture();p.tamper(VTABLE_START+4*slot)
                with self.assertRaises(OriginalBinaryMismatch):verify(p,False)
    def test_rtti_pointers_and_type_name_are_required(self):
        for site in list(RTTI)+[TYPE_NAME_VA]:
            with self.subTest(address=hex(site)):
                p=fixture();p.tamper(site)
                with self.assertRaises(OriginalBinaryMismatch):verify(p,False)
    def test_each_real_x86_direct_call_is_required(self):
        for site in CALLS:
            with self.subTest(address=hex(site)):
                p=fixture();p.tamper(site+1)
                with self.assertRaises(OriginalBinaryMismatch):verify(p,False)
    def test_full_function_hash_required_without_original_bytes(self):
        self.assertEqual(len(REGIONS),9)
        for _,(va,count,digest) in REGIONS.items():
            self.assertGreater(count,0)
            self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(OriginalBinaryMismatch,'SHA256'):
            verify(fixture(),True)
    def test_wrong_dll_hash_rejected(self):
        with self.assertRaisesRegex(OriginalBinaryMismatch,'SHA256'):
            audit(b'not a client',b'not a server')

if __name__=='__main__':unittest.main()
