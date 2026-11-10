"""Phase168 original-free sparse x86 evidence and independent state matrices."""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_flak_origin import (
    REGIONS,OPCODES,CALLS,CALLER,LITERALS,
    verify_original_flak_origin,VerificationError)

class MockPE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,va,blob):
        for off,byte in enumerate(blob):
            if va+off in self.memory and self.memory[va+off]!=byte:
                raise AssertionError("original x86 sparse fixture conflict at %#x"%(va+off))
            self.memory[va+off]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xCC) for i in range(n))
    def damage(self,va):
        self.memory[va]^=1

def fixture():
    p=MockPE()
    for va,hex_bytes in OPCODES.items():
        p.put(va,bytes.fromhex(hex_bytes))
    for va,target in {**CALLS,**CALLER}.items():
        p.put(va,bytes((0xE8,))+struct.pack("<i",target-va-5))
    for va,raw in LITERALS.items():
        p.put(va,raw)
    return p

def changed_native(a,b):
    return any(x!=y for x,y in zip(a,b))

def original_parent_outcome(old,new,parent_handle,parent_valid,flags):
    if not changed_native(old,new):return ("unchanged",0,False,False)
    if parent_handle==0xffffffff or not parent_valid:
        return ("direct_local",4,False,False)
    return ("relative_local",4,bool(flags&0x1000),bool(flags&0x800))

def native_spawn_binding(source,virtual_fc,lookup_record,raw_value):
    # Original helper leaves constructor-initialized binding alone when
    # source null or its first +0xFC callback yields null.
    if not source or not virtual_fc:return 0xffffffff
    return raw_value if lookup_record else 0xffffffff

class NativeFlakOriginEvidenceTests(unittest.TestCase):
    def test_evidence_totals(self):
        x=verify_original_flak_origin(fixture(),check_hashes=False)
        self.assertEqual(x["original_verified_full_function_hashes"],0)
        self.assertEqual(x["original_sparse_opcode_witnesses"],62)
        self.assertEqual(x["original_function_direct_call_edges"],10)
        self.assertEqual(x["verified_original_scattergun_caller_edges"],2)
        self.assertEqual(x["original_flak_classname_literal"],"flak")
        self.assertEqual(x["flak_spawn_length"],134)
        self.assertEqual(x["entity_origin_setter_length"],307)
    def test_sparse_native_opcode_mutations(self):
        for va in OPCODES:
            with self.subTest(opcode=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,"opcode"):
                    verify_original_flak_origin(pe,check_hashes=False)
    def test_direct_call_mutations(self):
        for va in {**CALLS,**CALLER}:
            with self.subTest(call=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,"CALL"):
                    verify_original_flak_origin(pe,check_hashes=False)
    def test_flak_literal_mutation(self):
        for va in LITERALS:
            pe=fixture();pe.damage(va)
            with self.assertRaisesRegex(VerificationError,"classname"):
                verify_original_flak_origin(pe,check_hashes=False)
    def test_original_full_hashes_require_real_dlls(self):
        for name,(va,raw_len,digest) in REGIONS.items():
            with self.subTest(region=name):
                self.assertGreater(raw_len,100)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"full function SHA"):
            verify_original_flak_origin(fixture(),check_hashes=True)
    def test_image_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_flak_origin(pe,check_hashes=False)
    def test_native_rel32_target_and_owner_pointer_unchanged(self):
        self.assertEqual(CALLER[0x2231CA0B],0x2231B9F0)
        self.assertEqual(CALLER[0x2231CA5C],0x22141A00)
        self.assertEqual(CALLS[0x22141B12],0x22268E90)
        self.assertEqual(CALLS[0x2231BA32],0x222AA640)
    def test_x87_changed_and_parent_gates_independent_matrix(self):
        vectors=(
          (0.0,0.0,0.0),(0.0,1.0,0.0),
          (-0.0,0.0,0.0),(math.nan,0.0,0.0),
          (math.inf,-1.0,2.0),
        )
        count=0
        for old,new,handle,valid,flags in itertools.product(
            vectors,vectors,(0xffffffff,0x1234),(False,True),
            (0,0x800,0x1000,0x1800)):
            outcome,tag,refresh12,refresh11=original_parent_outcome(
                old,new,handle,valid,flags)
            if outcome=="unchanged":
                self.assertEqual(tag,0)
            elif outcome=="relative_local":
                self.assertTrue(valid and handle!=0xffffffff)
                self.assertEqual(tag,4)
                self.assertEqual(refresh12,bool(flags&0x1000))
                self.assertEqual(refresh11,bool(flags&0x800))
            else:
                self.assertEqual(tag,4)
            count+=1
        self.assertEqual(count,400)
        self.assertFalse(changed_native((0.0,-0.0,1.0),(-0.0,0.0,1.0)))
        self.assertTrue(changed_native((math.nan,0.0,0.0),(math.nan,0.0,0.0)))
    def test_optional_binding_matrix(self):
        for source,fc,record in itertools.product((0,1),(0,1),(0,1)):
            self.assertEqual(native_spawn_binding(source,fc,record,9),
                9 if source and fc and record else 0xffffffff)

if __name__=="__main__":
    unittest.main()
