"""Phase169 synthetic x86 witnesses and independent floating-point branch tests.

The synthetic fixture cannot pass original-region SHA verification.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_entity_angles import (
    REGIONS,OPCODES,CALLS,INCOMING_CALLS,
    verify_original_angles,VerificationError)

class MockPE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,address,blob):
        for off,b in enumerate(blob):
            at=address+off
            if at in self.memory and self.memory[at]!=b:
                raise AssertionError("conflicting sparse native witness at %#x"%at)
            self.memory[at]=b
    def at(self,va,n):
        return bytes(self.memory.get(va+off,0xcc) for off in range(n))
    def damage(self,va):self.memory[va]^=1

def fixture():
    pe=MockPE()
    for va,opcode in OPCODES.items():pe.put(va,bytes.fromhex(opcode))
    for va,target in {**CALLS,**INCOMING_CALLS}.items():
        pe.put(va,bytes((0xe8,))+struct.pack("<i",target-va-5))
    return pe

def x87_ah(a,b):
    # x87 C0,C2,C3 flags in the high status byte.
    if math.isnan(a) or math.isnan(b):return 0x45
    if a<b:return 0x01
    if a>b:return 0x00
    return 0x40

def x87_equal(a,b):
    masked=x87_ah(a,b)&0x44
    return masked.bit_count()%2==1

def independent_vector_equal(a,b):
    return all(x87_equal(x,y) for x,y in zip(a,b))

def tracked_angle_update(a,b,mode,dirty,external):
    if independent_vector_equal(a,b):return a,dirty,external,False
    if mode:dirty=1
    elif external is not None:external|=1
    return b,dirty,external,True

class Tests(unittest.TestCase):
    def test_native_witness_summary(self):
        report=verify_original_angles(fixture(),check_hashes=False)
        self.assertEqual(report["native_complete_function_hashes"],0)
        self.assertEqual(report["native_sparse_instruction_witnesses"],96)
        self.assertEqual(report["native_direct_call_edges"],13)
        self.assertEqual(report["native_incoming_flak_call_edges"],1)
        self.assertEqual(len(REGIONS),4)
        self.assertEqual(report["original_abs_angles_bytes"],412)
    def test_native_opcode_mutation_rejected(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,"opcode"):
                    verify_original_angles(pe,check_hashes=False)
    def test_original_call_mutation_rejected(self):
        for va in {**CALLS,**INCOMING_CALLS}:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,"CALL"):
                    verify_original_angles(pe,check_hashes=False)
    def test_original_region_hashes_required(self):
        for name,(va,n,digest) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(n,50)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"full function SHA"):
            verify_original_angles(fixture(),check_hashes=True)
    def test_wrong_original_image_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_angles(pe,check_hashes=False)
    def test_verified_native_edges_and_distinct_comparators(self):
        self.assertEqual(CALLS[0x22143AAE],0x220E2E70)
        self.assertEqual(CALLS[0x22143B2C],0x2213A360)
        self.assertEqual(CALLS[0x22143B3D],0x22140040)
        self.assertEqual(INCOMING_CALLS[0x2231BA65],0x221439C0)
        self.assertTrue(x87_equal(0.0,-0.0))
        self.assertFalse(x87_equal(math.nan,math.nan))
        self.assertTrue(x87_equal(math.inf,math.inf))
    def test_independent_vector_matrix(self):
        values=(0.0,-0.0,1.0,-1.0,math.inf,-math.inf,math.nan)
        count=0
        for old,new in itertools.product(values,values):
            for position in range(3):
                a=[0.0,0.0,0.0];b=[0.0,0.0,0.0]
                a[position]=old;b[position]=new
                equal=independent_vector_equal(a,b)
                self.assertEqual(equal,old==new)
                count+=1
        self.assertEqual(count,147)
    def test_tracked_float_notification_matrix(self):
        values=(0.0,-0.0,1.0,-1.0,math.inf,-math.inf,math.nan)
        count=0
        for old,new,mode,dirty,flags in itertools.product(
                values,values,(0,1,255),(0,9),(None,0,0x100)):
            original=(old,0.0,2.0);proposed=(new,0.0,2.0)
            stored,newdirty,newflags,changed=tracked_angle_update(
                original,proposed,mode,dirty,flags)
            self.assertEqual(changed,not(old==new))
            if changed:
                self.assertEqual(stored,proposed if not math.isnan(new) else stored)
                if mode:
                    self.assertEqual(newdirty,1)
                    self.assertEqual(newflags,flags)
                elif flags is not None:
                    self.assertEqual(newflags,flags|1)
            else:
                self.assertEqual(newdirty,dirty)
                self.assertEqual(newflags,flags)
            count+=1
        self.assertEqual(count,882)
    def test_parent_handle_decode_branches(self):
        count=0
        for index,serial,flag,match in itertools.product(
                (0,1,4095),(0,1,0xFFFFF),(0,0x800),(False,True)):
            raw=(serial<<12)|index
            self.assertEqual(raw&0xFFF,index)
            self.assertEqual(raw>>12,serial)
            refreshes=1 if flag&0x800 else 0
            if not match and flag&0x800:refreshes+=1
            self.assertIn(refreshes,(0,1,2))
            count+=1
        self.assertEqual(count,36)

if __name__=="__main__":unittest.main()