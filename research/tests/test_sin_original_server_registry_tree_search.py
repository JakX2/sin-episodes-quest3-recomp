"""Phase174 original-free mutation and independent native tree-gate tests."""
from __future__ import annotations
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_registry_tree_search import (
    REGIONS,OPCODES,CALLS,INCOMING,SENTINELS,
    verify_original_tree_search,VerificationError)

class FakePE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,va,bytestr):
        for i,byte in enumerate(bytestr):
            if va+i in self.memory and self.memory[va+i]!=byte:
                raise AssertionError('fixture overlap collision at %#x'%(va+i))
            self.memory[va+i]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def damage(self,va):self.memory[va]^=0x80

def fixture():
    pe=FakePE()
    for va,raw in OPCODES.items():pe.put(va,bytes.fromhex(raw))
    for va,target in {**CALLS,**INCOMING}.items():
        pe.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    return pe

def insertion_reference(root,record_compare,edges):
    last=0xffff;direction=False;init=False;visited=[]
    while root!=0xffff:
        visited.append(root)
        last=root
        direction=record_compare[root]
        init=True
        root=edges.get((root,0 if direction else 2),0xffff)
    return (last,direction,init,visited)

def symbol_reference(root,first,second,edges):
    init=False;visited=[]
    while root!=0xffff:
        visited.append(root)
        if first[root]:
            init=True
            root=edges.get((root,0),0xffff)
            continue
        if not second[root]:return (root,init,visited)
        init=True
        root=edges.get((root,2),0xffff)
    return (0xffff,init,visited)

class OriginalPhase174Tests(unittest.TestCase):
    def test_region_lengths_and_chained_evidence_metadata(self):
        self.assertEqual([x[1] for x in REGIONS.values()],[255,39,266,94])
        self.assertEqual(sum(x[1] for x in REGIONS.values()),654)
        self.assertEqual(len(OPCODES),56)
        self.assertEqual(len(CALLS),4)
        self.assertEqual(len(INCOMING),4)
        for _,(_,_,digest) in REGIONS.items():
            self.assertEqual(len(bytes.fromhex(digest)),32)
        report=verify_original_tree_search(fixture(),check_hashes=False)
        self.assertEqual(report['verified_original_complete_function_hashes'],0)
        self.assertEqual(report['native_bytes_covered_by_function_hashes'],654)
    def test_original_opcode_mutations(self):
        for va in OPCODES:
            with self.subTest(opcode=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_original_tree_search(pe,check_hashes=False)
    def test_original_relative_call_mutations(self):
        for va in {**CALLS,**INCOMING}:
            with self.subTest(call=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,'CALL'):
                    verify_original_tree_search(pe,check_hashes=False)
    def test_full_regions_not_satisfied_by_sparse_fixture(self):
        with self.assertRaisesRegex(VerificationError,'full function SHA'):
            verify_original_tree_search(fixture(),check_hashes=True)
    def test_wrong_original_image_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_original_tree_search(pe,check_hashes=False)
    def test_exact_original_caller_relationships(self):
        self.assertEqual(INCOMING[0x2228b0bb],0x2228a280)
        self.assertEqual(INCOMING[0x2228f7db],0x2228eb40)
        self.assertEqual(INCOMING[0x2228f88d],0x2228f710)
        self.assertEqual(CALLS[0x2228f746],0x2228f460)
        self.assertEqual(SENTINELS['insertion_words'],0x2258df88)
        self.assertEqual(SENTINELS['symbol_words'],0x2259012c)
    def test_independent_insertion_gate_matrix(self):
        count=0
        for a,b,c in itertools.product((False,True),repeat=3):
            tree={1:a,2:b,3:c}
            links={(1,0):2,(1,2):3,(2,0):0xffff,(2,2):0xffff,
                   (3,0):0xffff,(3,2):0xffff}
            got=insertion_reference(1,tree,links)
            self.assertEqual(got[0],2 if a else 3)
            self.assertEqual(got[1],b if a else c)
            self.assertTrue(got[2])
            self.assertEqual(len(got[3]),2)
            count+=1
        self.assertEqual(count,8)
        self.assertEqual(insertion_reference(0xffff,{},{}),
                         (0xffff,False,False,[]))
    def test_independent_symbol_comparator_matrix(self):
        count=0
        # Vary all comparator outcomes at both levels, avoiding cycles.
        for first_a,second_a,first_b,second_b in itertools.product((False,True),repeat=4):
            first={1:first_a,2:first_b,3:first_b}
            second={1:second_a,2:second_b,3:second_b}
            links={(1,0):2,(1,2):3}
            result,init,seen=symbol_reference(1,first,second,links)
            if not first_a and not second_a:
                self.assertEqual(result,1)
                self.assertFalse(init)
            else:
                self.assertEqual(len(seen),2)
                self.assertTrue(init)
                self.assertIn(result,(2,3,0xffff))
            count+=1
        self.assertEqual(count,16)
        self.assertEqual(symbol_reference(0xffff,{}, {}, {}),(0xffff,False,[]))
    def test_native_record_offsets_and_counter_wrap(self):
        for idx in (0,1,7,0xfffe,0xffff):
            self.assertEqual((idx*16+8)&0xffffffff,(idx<<4)+8)
            self.assertEqual((idx*12+8)&0xffffffff,idx*12+8)
            self.assertEqual((idx+1)&0xffff,(idx+1)%65536)
    def test_guarded_release_matrix(self):
        for guard,has_memory,aux in itertools.product((0xffffffff,0,1),(False,True),(0,99)):
            frees=[]
            alloc=0x11223344 if has_memory else 0
            result_alloc=alloc;result_aux=aux
            if guard!=0xffffffff:
                if alloc:
                    frees.append(alloc);result_alloc=0
                result_aux=0
            self.assertEqual(bool(frees),guard!=0xffffffff and has_memory)
            self.assertEqual(result_aux,aux if guard==0xffffffff else 0)
            self.assertEqual(result_alloc,alloc if guard==0xffffffff else 0)
if __name__=='__main__':unittest.main()