"""Phase167: original-free native witness mutation and 384-case branch matrix."""
import itertools
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_scattergun_secondary_fire import (
 REGIONS, OPCODES, CALLS, CONSTANTS, verify_original_secondary,
 VerificationError)

class MockPE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,va,blob):
        for off,byte in enumerate(blob):
            prior=self.memory.get(va+off)
            if prior is not None and prior!=byte:
                raise AssertionError("sparse byte overlap %#x"%(va+off))
            self.memory[va+off]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xCC) for i in range(n))
    def damage(self,va):
        self.memory[va]^=1

def fixture():
    pe=MockPE()
    for va,opcode in OPCODES.items():
        pe.put(va,bytes.fromhex(opcode))
    for va,target in CALLS.items():
        pe.put(va,bytes((0xE8,))+struct.pack("<i",target-va-5))
    for va,original in CONSTANTS.items():
        pe.put(va,original)
    return pe

def independent_outcome(owner,mode,vectors,uses_clip,clip):
    if not owner:return "missing_owner"
    if mode==3:return "mode_cooldown"
    if not vectors:return "missing_vectors"
    if uses_clip and (clip==0 or clip&0x80000000):return "empty_clip"
    return "fired"

class OriginalSecondaryTests(unittest.TestCase):
    def test_sparse_evidence(self):
        report=verify_original_secondary(fixture(),check_hashes=False)
        self.assertEqual(report["verified_complete_original_function_hashes"],0)
        self.assertEqual(report["original_opcode_witnesses"],54)
        self.assertEqual(report["verified_direct_call_edges"],18)
        self.assertEqual(report["verified_original_constants"],3)
        self.assertEqual(report["secondary_fire_native_bytes"],612)
        self.assertEqual(report["vector_helper_native_bytes"],225)
        self.assertEqual(len(REGIONS),2)
    def test_all_opcode_mutations(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaises(VerificationError):
                    verify_original_secondary(pe,check_hashes=False)
    def test_all_call_mutations(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_secondary(pe,check_hashes=False)
    def test_constant_mutations(self):
        for va in CONSTANTS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaises(VerificationError):
                    verify_original_secondary(pe,check_hashes=False)
    def test_full_regions_not_faked_by_witness_fixture(self):
        for name,(va,n,digest) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(n,200)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"whole-region"):
            verify_original_secondary(fixture(),check_hashes=True)
    def test_wrong_image_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_secondary(pe,check_hashes=False)
    def test_independent_gate_matrix(self):
        count=0
        for owner,mode,vectors,uses_clip,clip in itertools.product(
            (False,True),(0,1,2,3,4,5,6,7),(False,True),(False,True),
            (0,1,7,0x7fffffff,0x80000000,0xffffffff)):
            result=independent_outcome(owner,mode,vectors,uses_clip,clip)
            if not owner:self.assertEqual(result,"missing_owner")
            if owner and mode==3:self.assertEqual(result,"mode_cooldown")
            if result=="fired":
                self.assertTrue(owner and mode!=3 and vectors)
                self.assertTrue(not uses_clip or 0<clip<0x80000000)
            count+=1
        self.assertEqual(count,384)
    def test_native_call_edges_and_signed_clip(self):
        self.assertEqual(CALLS[0x2231C964],0x2231C740)
        self.assertEqual(CALLS[0x2231C889],0x222CBC20)
        self.assertEqual(CALLS[0x2231CA5C],0x22141A00)
        self.assertEqual(independent_outcome(1,0,1,1,0xffffffff),"empty_clip")
        self.assertEqual(independent_outcome(1,0,1,0,0xffffffff),"fired")

if __name__=="__main__":
    unittest.main()
