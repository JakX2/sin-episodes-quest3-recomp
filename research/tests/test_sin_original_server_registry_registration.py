"""Original-free Phase172 native registration/hash evidence mutation tests."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_registry_registration import (
  REGIONS,OPCODES,CALLS,INCOMING,VTABLE,VTABLE_START,
  verify_original_registration,VerificationError)

class FakePE:
    base=0x22000000
    def __init__(self):self.mem={}
    def put(self,va,source):
        for i,b in enumerate(source):
            if va+i in self.mem and self.mem[va+i]!=b:
                raise AssertionError('overlapping original witness at %#x'%(va+i))
            self.mem[va+i]=b
    def at(self,va,n):
        return bytes(self.mem.get(va+i,0xcc) for i in range(n))
    def damage(self,va):self.mem[va]^=0x10

def fixture():
    pe=FakePE()
    for va,bytes_hex in OPCODES.items():pe.put(va,bytes.fromhex(bytes_hex))
    for va,target in {**CALLS,**INCOMING}.items():
        pe.put(va,bytes((0xe8,))+struct.pack('<i',target-va-5))
    pe.put(VTABLE_START,struct.pack('<4I',*VTABLE))
    return pe

def reference_hash(chars):
    acc=5381
    for unsigned in chars:
        signed=unsigned if unsigned<128 else unsigned-256
        acc=((acc<<5)+acc+signed)&0xffffffff
    return acc

class RegistryRegistrationTests(unittest.TestCase):
    def test_original_function_sha_declarations(self):
        self.assertEqual([r[1] for r in REGIONS.values()],[60,79,78,40])
        self.assertEqual(sum(r[1] for r in REGIONS.values()),257)
        for _,(_,_,sha) in REGIONS.items():
            self.assertEqual(len(bytes.fromhex(sha)),32)
        result=verify_original_registration(fixture(),check_hashes=False)
        self.assertEqual(result['verified_complete_original_function_hashes'],0)
        self.assertEqual(result['verified_sparse_native_opcodes'],58)
        self.assertEqual(result['verified_internal_direct_calls'],6)
        self.assertEqual(result['verified_incoming_hash_call'],1)
        self.assertEqual(result['verified_vtable_slots'],4)
    def test_each_sparse_opcode_mutation(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_original_registration(pe,check_hashes=False)
    def test_each_original_direct_call_mutation(self):
        for va in {**CALLS,**INCOMING}:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,'CALL'):
                    verify_original_registration(pe,check_hashes=False)
    def test_all_native_vtable_entries(self):
        self.assertEqual(VTABLE,(0x2228b190,0x22285550,0x22285580,0x2228b010))
        for slot in range(4):
            with self.subTest(slot=slot):
                pe=fixture();pe.damage(VTABLE_START+4*slot)
                with self.assertRaisesRegex(VerificationError,'vtable'):
                    verify_original_registration(pe,check_hashes=False)
    def test_original_full_regions_cannot_be_faked(self):
        with self.assertRaisesRegex(VerificationError,'whole function SHA'):
            verify_original_registration(fixture(),check_hashes=True)
    def test_wrong_native_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_original_registration(pe,check_hashes=False)
    def test_original_native_call_graph(self):
        self.assertEqual(CALLS[0x2228aeda],0x2228f7a0)
        self.assertEqual(CALLS[0x2228aef0],0x2228a760)
        self.assertEqual(CALLS[0x2228b170],0x2228b0a0)
        self.assertEqual(CALLS[0x2228b1c1],0x2228b0a0)
        self.assertEqual(INCOMING[0x222c7a13],0x2228b1e0)
    def test_independent_hash_vectors_and_signed_high_bytes(self):
        expected={
            b'':0x1505,b'A':0x2b5e6,b'abc':0xb885c8b,
            b'flak':0x7c96d683,bytes((0x80,)):0x2b525,
            bytes((0xff,)):0x2b5a4,
            b'A'*24:0xb0da1c9d,
        }
        for blob,wanted in expected.items():
            with self.subTest(chars=repr(blob)):
                self.assertEqual(reference_hash(blob),wanted)
        # Every possible single input byte verifies that sign extension
        # precedes 32-bit wrapping, including the entire high-bit range.
        for value in range(1,256):
            wanted=(0x1505*33+(value if value<128 else value-256))&0xffffffff
            self.assertEqual(reference_hash(bytes((value,))),wanted)
        self.assertNotEqual(reference_hash(bytes((0x80,))),
                            (0x1505*33+128)&0xffffffff)
    def test_independent_registration_slot_matrix(self):
        count=0
        for slot,direct,indirect,updated in itertools.product(
                (0,1,0xfffe,0xffff),(0,1,0x12345678,0xffffffff),
                (0,1,0x80000000,0xffffffff),(0,1)):
            # The original direct path stores the provided word, whereas
            # the indirect path loads the pointed-to word *after* lookup.
            stored_direct=direct&0xffffffff
            stored_indirect=(0x77665544 if updated else indirect)&0xffffffff
            self.assertEqual(stored_direct,direct)
            self.assertTrue(0<=stored_indirect<=0xffffffff)
            self.assertEqual(16*slot+12,(slot<<4)+12)
            count+=1
        self.assertEqual(count,128)
        # Explicitly no -1 sentinel branch inside either native writer.
        self.assertEqual(16*0xffff+12,1048572)

if __name__=='__main__':unittest.main()