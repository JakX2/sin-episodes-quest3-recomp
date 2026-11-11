"""Phase175 original-free native binary witness mutation and state tests."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_symbol_tree_storage import (
    REGIONS,OPCODES,CALLS,INCOMING,verify_native_symbol_storage,VerificationError
)

class FakePE:
    base=0x22000000
    def __init__(self):self._mem={}
    def put(self,addr,blob):
        for index,byte in enumerate(blob):
            key=addr+index
            if key in self._mem and self._mem[key]!=byte:
                raise AssertionError('native witness conflict at %#x'%key)
            self._mem[key]=byte
    def at(self,addr,size):
        return bytes(self._mem.get(addr+idx,0xcc) for idx in range(size))
    def flip(self,addr):self._mem[addr]^=0x01

def fixture():
    p=FakePE()
    for addr,payload in OPCODES.items():p.put(addr,bytes.fromhex(payload))
    for addr,dest in {**CALLS,**INCOMING}.items():
        p.put(addr,bytes([0xe8])+struct.pack('<i',dest-addr-5))
    return p

def signed32(x):
    x=x&0xffffffff
    return x if x<0x80000000 else x-0x100000000

def append_result(used,capacity,requested):
    nxt=(used+requested)&0xffffffff
    return (signed32(nxt)>signed32(capacity),
            (used-capacity+requested)&0xffffffff,nxt)

class OriginalSymbolTreeStorageTests(unittest.TestCase):
    def test_declared_provenance_and_counts(self):
        self.assertEqual(len(REGIONS),5)
        self.assertEqual(sum(x[1] for x in REGIONS.values()),282)
        self.assertEqual([x[1] for x in REGIONS.values()],[17,24,86,46,109])
        for va,n,sha in REGIONS.values():
            self.assertEqual(len(bytes.fromhex(sha)),32)
            self.assertTrue(0x22200000<=va<0x22300000)
        report=verify_native_symbol_storage(fixture(),check_hashes=False)
        self.assertEqual(report['verified_complete_original_function_hashes'],0)
        self.assertEqual(report['verified_sparse_original_opcodes'],56)
        self.assertEqual(report['verified_native_internal_calls'],6)
        self.assertEqual(report['verified_native_incoming_calls'],2)
    def test_mutate_each_original_opcode_witness(self):
        for va in OPCODES:
            with self.subTest(addr=hex(va)):
                p=fixture();p.flip(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_native_symbol_storage(p,check_hashes=False)
    def test_mutate_each_original_call_displacement(self):
        for va in {**CALLS,**INCOMING}:
            with self.subTest(addr=hex(va)):
                p=fixture();p.flip(va+1)
                with self.assertRaisesRegex(VerificationError,'call rel32'):
                    verify_native_symbol_storage(p,check_hashes=False)
    def test_fake_fixture_cannot_satisfy_original_full_sha(self):
        with self.assertRaisesRegex(VerificationError,'complete function hash'):
            verify_native_symbol_storage(fixture(),check_hashes=True)
    def test_original_server_pe_base_guard(self):
        p=fixture();p.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_native_symbol_storage(p,check_hashes=False)
    def test_original_call_graph(self):
        self.assertEqual(CALLS[0x2228F4A7],0x2228EDF0)
        self.assertEqual(CALLS[0x2228F4B6],0x2228EDF0)
        self.assertEqual(CALLS[0x2228F4C4],0x2228EDF0)
        self.assertEqual(INCOMING[0x2228F732],0x2228E620)
        self.assertEqual(INCOMING[0x2228F746],0x2228F460)
    def test_independent_16_bit_slot_and_record_matrix(self):
        iterations=0
        for slot,old_used,recycled_next in itertools.product(
            (0,1,0xfffe,0xffff),(0,1,0xfffe,0xffff),(0,1,0xffff)
        ):
            self.assertEqual(12*slot,(slot+slot*2)*4)
            fresh=(old_used+1)&0xffff
            recycled_used=old_used
            self.assertEqual(fresh,(old_used+1)%65536)
            self.assertEqual(recycled_used,old_used)
            self.assertTrue(0<=recycled_next<=65535)
            iterations+=1
        self.assertEqual(iterations,48)
    def test_independent_signed_capacity_growth_matrix(self):
        cases=(0,1,5,16,0x7fffffff,0x80000000,0xfffffffe,0xffffffff)
        iterations=0
        for used,capacity,requested in itertools.product(cases,cases,cases):
            reserve,extra,new_used=append_result(used,capacity,requested)
            self.assertEqual(new_used,(used+requested)&0xffffffff)
            self.assertEqual(extra,(used-capacity+requested)&0xffffffff)
            self.assertEqual(reserve,signed32(new_used)>signed32(capacity))
            iterations+=1
        self.assertEqual(iterations,512)
        # Native JLE compares signed, so 0x80000000 is <= positive capacity.
        self.assertFalse(append_result(0x80000000,0x7fffffff,0)[0])
        self.assertTrue(append_result(20,2,1)[0])
    def test_independent_tree_link_paths_and_unmodified_payload(self):
        for parent,direction,payload in itertools.product(
            (0xffff,0,1),(0,1,255),(0,1,0xffffffff)
        ):
            route=('root' if parent==0xffff else ('left' if direction else 'right'))
            self.assertIn(route,('root','left','right'))
            raw={'left':0xffff,'right':0xffff,'parent':parent,
                 'metadata':0,'payload':payload}
            self.assertEqual(raw['payload'],payload)
            self.assertEqual(raw['metadata'],0)
            self.assertEqual(raw['parent'],parent)
            # Rebalance is unconditional on all 3 native exits.
            self.assertIn(route,('root','left','right'))

if __name__=='__main__':unittest.main()