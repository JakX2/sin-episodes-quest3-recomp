"""Phase163 synthetic x86 code/witness and independent array math tests.

No original SiN 2006 executable bytes are included: only partial, disjoint
opcode anchors and original SHA256 digests. Binary audit requires separately
owned exact original client/server DLLs; these mutation tests do not.
"""
import itertools
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_property_descriptor_array import (
    REGIONS,OPCODES,CALLS,INTERNAL_CALLS,REGISTRAR_CALLS,
    verify_native_descriptor_vector,VerificationError,audit,
)

class MockPE:
    base=0x24000000
    def __init__(self):self.memory={}
    def put(self,addr,raw):
        for i,value in enumerate(raw):
            loc=addr+i
            if loc in self.memory and self.memory[loc]!=value:
                raise AssertionError("sparse fixture conflicting byte at %#x"%loc)
            self.memory[loc]=value
    def at(self,addr,length):
        return bytes(self.memory.get(addr+i,0xcc) for i in range(length))
    def damage(self,addr):self.memory[addr]^=0x10

def fixture():
    pe=MockPE()
    for addr,hexcode in OPCODES.items():pe.put(addr,bytes.fromhex(hexcode))
    for site,target in CALLS.items():
        pe.put(site,b"\xe8"+struct.pack("<i",target-site-5))
    return pe

def oracle_grow(original_capacity,delta,grow_amount):
    # Independent imperative translation of 0x2400B140 for small valid
    # signed 32-bit values (without relying on the C++ implementation).
    if grow_amount == -1:
        return original_capacity
    minimum=original_capacity+delta
    cap=original_capacity
    while cap<minimum:
        if cap==0:cap=2
        elif grow_amount==0:cap*=2
        else:cap+=grow_amount
    return cap

def oracle_insert(records,index,six_raw_words,capacity,grow_amount):
    # Native array has 24-byte entries, not parsed Color objects.
    count=len(records)
    if count+1>capacity:
        capacity=oracle_grow(capacity,count+1-capacity,grow_amount)
    # Original memmove: copy count-index entries toward higher addresses,
    # then copy six original DWORDs at the insertion index.
    records.append([0]*6)
    for i in range(count,index,-1):
        records[i]=records[i-1].copy()
    records[index]=list(six_raw_words)
    return capacity,records

class NativeDescriptorVectorTests(unittest.TestCase):
    def test_two_complete_function_regions_and_metadata(self):
        result=verify_native_descriptor_vector(fixture(),check_hashes=False)
        self.assertEqual(result["verified_complete_function_SHA256s"],0)
        self.assertEqual(result["verified_x86_instruction_anchors"],len(OPCODES))
        self.assertEqual(result["verified_original_CALL_edges"],len(CALLS))
        self.assertEqual(result["verified_original_timer_registrar_calls"],7)
        self.assertEqual(result["native_record_size_bytes"],24)
        self.assertEqual(result["native_element_words"],6)
        self.assertEqual(len(REGIONS),2)
        self.assertEqual(len(INTERNAL_CALLS),4)
        self.assertEqual(len(REGISTRAR_CALLS),7)

    def test_every_original_opcode_anchor_must_match(self):
        for addr in OPCODES:
            with self.subTest(address=hex(addr)):
                pe=fixture();pe.damage(addr)
                with self.assertRaises(VerificationError):
                    verify_native_descriptor_vector(pe,check_hashes=False)

    def test_every_original_direct_call_target_must_match(self):
        for addr in CALLS:
            with self.subTest(call=hex(addr)):
                pe=fixture();pe.damage(addr+1)
                with self.assertRaises(VerificationError):
                    verify_native_descriptor_vector(pe,check_hashes=False)

    def test_original_hashes_not_faked_by_sparse_fixture(self):
        start,length,digest=REGIONS["native_24byte_descriptor_insert"]
        self.assertEqual((start,length),(0x2400A300,141))
        self.assertEqual(len(bytes.fromhex(digest)),32)
        start,length,digest=REGIONS["native_descriptor_capacity_growth"]
        self.assertEqual((start,length),(0x2400B140,119))
        self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"whole-function SHA"):
            verify_native_descriptor_vector(fixture(),check_hashes=True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"not original",b"not original")

    def test_wrong_base_rejected(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_native_descriptor_vector(pe,check_hashes=False)

    def test_independent_native_growth_matrix(self):
        cases=0
        for capacity,delta,grow_amount in itertools.product(
            (0,1,2,3,4,5,12,32), (0,1,2,3,7,17), (0,1,2,3,8,-1)
        ):
            new_capacity=oracle_grow(capacity,delta,grow_amount)
            self.assertGreaterEqual(new_capacity,capacity)
            if grow_amount!=-1:
                self.assertGreaterEqual(new_capacity,capacity+delta)
            else:
                self.assertEqual(new_capacity,capacity)
            if capacity==0 and grow_amount!=-1 and delta>0:
                self.assertGreaterEqual(new_capacity,2)
            cases+=1
        self.assertEqual(cases,288)
        self.assertEqual(oracle_grow(0,1,0),2)
        self.assertEqual(oracle_grow(2,1,0),4)
        self.assertEqual(oracle_grow(2,1,3),5)
        self.assertEqual(oracle_grow(5,5,3),11)

    def test_independent_memmove_and_six_word_descriptor(self):
        records=[]
        capacity=0
        for i,index in ((0,0),(1,1),(2,1),(3,0),(4,2)):
            descriptor=[(i<<16)|n for n in range(6)]
            capacity,records=oracle_insert(
                records,index,descriptor,capacity,0)
            self.assertGreaterEqual(capacity,len(records))
            self.assertEqual(records[index],descriptor)
        self.assertEqual(capacity,8)
        self.assertEqual([row[0]>>16 for row in records],
                         [3,0,4,2,1])
        self.assertTrue(all(len(rec)==6 for rec in records))

    def test_all_seven_actual_client_property_registrar_calls(self):
        expected=(0x24230E37,0x24230EA7,0x24230F17,
                  0x242364A7,0x24236517,0x24236587,0x242365F7)
        self.assertEqual(tuple(REGISTRAR_CALLS),expected)
        self.assertEqual(set(REGISTRAR_CALLS.values()),{0x2400A300})

if __name__=="__main__":
    unittest.main()
