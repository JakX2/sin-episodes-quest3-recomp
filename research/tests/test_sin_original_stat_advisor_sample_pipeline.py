"""Synthetic original-free regression for Phase149 x86 CStatAdvisor route/fanout."""
import itertools
import math
import pathlib
import struct
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from audit_sin_original_stat_advisor_sample_pipeline import (
    OPCODES, VTABLE, CALLS, REGION_HASHES,
    verify_sample_pipeline, VerificationError, audit,
)

class FakePE:
    def __init__(self): self.mem={}
    def put(self,va,raw):
        for i,v in enumerate(raw):
            if va+i in self.mem and self.mem[va+i]!=v:
                raise AssertionError('fixture overlap %#x' % (va+i))
            self.mem[va+i]=v
    def at(self,va,n):return bytes(self.mem.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.mem[va]^=0x10

def fixture():
    p=FakePE()
    for va, raw in OPCODES.items():p.put(va,bytes.fromhex(raw))
    for va,target in VTABLE.items():p.put(va,struct.pack('<I',target))
    for va,target in CALLS.items():p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    return p

def x87(a,b):
    if math.isnan(a) or math.isnan(b):return 0x45
    if a<b:return 0x01
    if a==b:return 0x40
    return 0

def parity_even(x):return (x&0xff).bit_count()%2==0

def original_machine_route(score,c1,c2,enabled):
    if not enabled:return 11
    first=x87(c1,c2)
    second=x87(score,c1)
    if (first&0x41)==0:
        if (second&0x41)==0:return 10
    else:
        if not parity_even(second&0x05):return 10
    third=x87(c2,c1)
    fourth=x87(score,c2)
    if not parity_even(third&0x05):
        if parity_even(fourth&0x05):return 11
    else:
        if fourth&0x41:return 11
    return 9

def readable_route(score,c1,c2,enabled):
    if not enabled:return 11
    if c1>c2:
        if score>c1:return 10
        if score<c2:return 9
    else:
        if score<c1:return 10
        if score>c2:return 9
    return 11

class Phase149Tests(unittest.TestCase):
    def test_fixture_contract(self):
        report=verify_sample_pipeline(fixture(),False)
        self.assertEqual(report['aggregate_virtual_slot'],1)
        self.assertEqual(report['route_virtual_slots'],[9,10,11])
        self.assertEqual(report['verified_opcode_sites'],len(OPCODES))
        self.assertEqual(report['verified_call_edges'],2)
    def test_every_opcode_tampering_is_rejected(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):verify_sample_pipeline(p,False)
    def test_every_virtual_pointer_and_call_tampering_is_rejected(self):
        for va in VTABLE:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):verify_sample_pipeline(p,False)
        for va in CALLS:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va+1)
                with self.assertRaises(VerificationError):verify_sample_pipeline(p,False)
    def test_full_original_function_hashes_required(self):
        self.assertEqual(len(REGION_HASHES),4)
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            verify_sample_pipeline(fixture(),True)
    def test_bad_original_dlls_rejected(self):
        with self.assertRaises((VerificationError, ValueError)):
            audit(b'fake client',b'fake server')
    def test_x87_flag_machine_and_readable_transition_equivalence(self):
        samples=[-math.inf,-2.,-1.,-0.,0.,1.,2.,math.inf,float('nan')]
        for score,c1,c2,enabled in itertools.product(samples,samples,samples,(False,True)):
            self.assertEqual(original_machine_route(score,c1,c2,enabled),
                             readable_route(score,c1,c2,enabled))

if __name__=='__main__':unittest.main()
