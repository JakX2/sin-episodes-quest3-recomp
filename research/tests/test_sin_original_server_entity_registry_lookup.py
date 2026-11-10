"""Phase171 original-free factory registry vtable and lookup branch regressions."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_entity_registry_lookup import (
 REGIONS,OPCODES,CALLS,VTABLE,VTABLE_START,LITERALS,INDIRECT_EDGES,
 verify_original_registry_lookup,VerificationError)

class FakePE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,va,data):
        for offset,byte in enumerate(data):
            old=self.memory.get(va+offset)
            if old is not None and old!=byte:
                raise AssertionError('sparse fixture overlap at %#x'%(va+offset))
            self.memory[va+offset]=byte
    def at(self,va,size):
        return bytes(self.memory.get(va+i,0xcc) for i in range(size))
    def damage(self,va):self.memory[va]^=0x10

def fixture():
    out=FakePE()
    for va,hex_bytes in OPCODES.items():out.put(va,bytes.fromhex(hex_bytes))
    for va,target in CALLS.items():
        out.put(va,bytes((0xe8,))+struct.pack('<i',target-va-5))
    out.put(VTABLE_START,struct.pack('<4I',*VTABLE))
    for va,raw in LITERALS.items():out.put(va,raw)
    for va,raw in INDIRECT_EDGES.items():out.put(va,raw)
    return out

def independent_lookup(index,provider,create_result,destroy_result):
    if index==0xffff or provider==0:
        return {'create':0,'destroy':0,'warning_create':True,'warning_destroy':True}
    return {'create':create_result,'destroy':destroy_result,
            'warning_create':False,'warning_destroy':False}

class OriginalRegistryLookupEvidenceTests(unittest.TestCase):
    def test_exact_function_fingerprints_are_declared(self):
        self.assertEqual(len(REGIONS),3)
        self.assertEqual([x[1] for x in REGIONS.values()],[46,50,94])
        for _,(_,size,digest) in REGIONS.items():
            self.assertGreater(size,0)
            self.assertEqual(len(bytes.fromhex(digest)),32)
        report=verify_original_registry_lookup(fixture(),check_hashes=False)
        self.assertEqual(report['original_full_function_hashes'],0)
        self.assertEqual(report['original_opcode_witnesses'],41)
        self.assertEqual(report['original_direct_call_edges'],2)
        self.assertEqual(report['original_vtable_slots'],4)
        self.assertEqual(report['original_parent_factory_indirect_calls'],2)
        self.assertEqual(report['original_diagnostic_literals'],2)
    def test_each_native_opcode_witness_is_mutation_guarded(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_original_registry_lookup(pe,check_hashes=False)
    def test_each_native_direct_call_is_mutation_guarded(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,'CALL'):
                    verify_original_registry_lookup(pe,check_hashes=False)
    def test_each_vtable_slot_is_mutation_guarded(self):
        self.assertEqual(VTABLE,(0x2228b190,0x22285550,0x22285580,0x2228b010))
        for i in range(4):
            with self.subTest(slot=i):
                pe=fixture();pe.damage(VTABLE_START+i*4)
                with self.assertRaisesRegex(VerificationError,'vtable'):
                    verify_original_registry_lookup(pe,check_hashes=False)
    def test_original_classname_diagnostic_mutations(self):
        for va in LITERALS:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,'literal'):
                    verify_original_registry_lookup(pe,check_hashes=False)
    def test_both_phase170_entrypoints_dispatch_registry_slot_four(self):
        for va in INDIRECT_EDGES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,'virtual dispatch'):
                    verify_original_registry_lookup(pe,check_hashes=False)
    def test_original_whole_region_cannot_be_faked_by_witnesses(self):
        with self.assertRaisesRegex(VerificationError,'full function SHA'):
            verify_original_registry_lookup(fixture(),check_hashes=True)
    def test_wrong_native_pe32_base_rejected(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_original_registry_lookup(pe,check_hashes=False)
    def test_independent_128_case_lookup_and_factory_matrix(self):
        count=0
        for index,provider,create,destroy in itertools.product(
            (0,1,0xfffe,0xffff),(0,1,2,0xffffffff),(0,3,0x7fffffff,0xffffffff),(0,7)):
            result=independent_lookup(index,provider,create,destroy)
            missing=index==0xffff or provider==0
            self.assertEqual(result['warning_create'],missing)
            self.assertEqual(result['warning_destroy'],missing)
            if missing:self.assertEqual((result['create'],result['destroy']),(0,0))
            else:self.assertEqual((result['create'],result['destroy']),(create,destroy))
            count+=1
        self.assertEqual(count,128)
    def test_pointer_record_offset_and_branch_semantics(self):
        self.assertEqual(CALLS,{0x2228b02e:0x2228f7a0,0x2228b044:0x2228a760})
        self.assertEqual(16*3+12,60)
        self.assertEqual(independent_lookup(0xffff,123,9,7)['create'],0)
        self.assertFalse(independent_lookup(0,123,9,7)['warning_create'])

if __name__=='__main__':unittest.main()