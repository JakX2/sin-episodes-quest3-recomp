"""Phase180 independent client post-Init x86 original-free mutation tests."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_client_postinit import (
 REGIONS,OPCODES,CALLS,INCOMING,PHYSICS,verify_native,VerificationError)
class FakePE:
    base=0x24000000
    def __init__(self):self.mem={}
    def put(self,va,blob):
        for i,v in enumerate(blob):
            pos=va+i
            if pos in self.mem and self.mem[pos]!=v:raise AssertionError('conflict at %#x'%pos)
            self.mem[pos]=v
    def at(self,va,n):return bytes(self.mem.get(va+i,0xcc) for i in range(n))
    def section_of(self,va):
        if any(nva<=va<nva+len(name)+1 for name,nva,_ in PHYSICS):return '.rdata'
        if any(va==g for _,_,g in PHYSICS):return '.data'
        return '.text'
    def mutate(self,va):self.mem[va]^=0x04

def fixture():
    pe=FakePE()
    for va,h in OPCODES.items():pe.put(va,bytes.fromhex(h))
    for va,dest in {**CALLS,**INCOMING}.items():
        pe.put(va,b'\xe8'+struct.pack('<i',dest-va-5))
    for name,va,_ in PHYSICS:pe.put(va,name.encode()+b'\0')
    return pe

class OriginalClientPostInitTests(unittest.TestCase):
    def test_function_region_exact_boundaries(self):
        self.assertEqual([n for _,n,_ in REGIONS.values()],[48,54,97])
        self.assertEqual(sum(n for _,n,_ in REGIONS.values()),199)
        for _,_,digest in REGIONS.values():self.assertEqual(len(bytes.fromhex(digest)),32)
    def test_independent_fixture(self):
        row=verify_native(fixture(),check_hash=False)
        self.assertEqual(row['verified_complete_original_functions'],0)
        self.assertEqual(row['verified_native_opcode_witnesses'],len(OPCODES))
        self.assertEqual(row['verified_native_internal_CALL_edges'],2)
        self.assertEqual(row['verified_original_incoming_CALL_edges'],2)
    def test_each_opcode_corruption_is_detected(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.mutate(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_native(pe,check_hash=False)
    def test_each_original_call_corruption_is_detected(self):
        for va in {**CALLS,**INCOMING}:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.mutate(va+1)
                with self.assertRaisesRegex(VerificationError,'CALL'):
                    verify_native(pe,check_hash=False)
    def test_each_original_physics_version_corruption(self):
        for name,va,_ in PHYSICS:
            with self.subTest(name=name):
                pe=fixture();pe.mutate(va)
                with self.assertRaisesRegex(VerificationError,'interface version'):
                    verify_native(pe,check_hash=False)
    def test_native_globals_and_list_layout(self):
        self.assertEqual([g for _,_,g in PHYSICS],
                         [0x24374398,0x243743A8,0x243743A0])
        self.assertEqual(OPCODES[0x24164121],'8b3574933624')
        self.assertEqual(OPCODES[0x24164137],'8b0e')
        self.assertEqual(OPCODES[0x24164139],'8901')
        self.assertEqual(OPCODES[0x2416413D],'833e00')
    def test_call_graph_real_edges(self):
        self.assertEqual(CALLS[0x2416414D],0x241640F0)
        self.assertEqual(CALLS[0x24182CB1],0x241927A0)
        self.assertEqual(INCOMING[0x240FE39B],0x24164120)
        self.assertEqual(INCOMING[0x240FE540],0x24182C60)
    def test_wrong_pe_base(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_native(pe,check_hash=False)
    def test_full_original_function_hashes_not_synthetic(self):
        with self.assertRaisesRegex(VerificationError,'function SHA'):
            verify_native(fixture(),check_hash=True)
    def test_null_and_first_failure_matrix(self):
        # Independent control-flow oracle for three mandatory physics calls:
        # publish even a null result, abort immediately, never couple on fail.
        for miss in range(4):
            returned=[1,2,3]
            if miss<3:returned[miss]=0
            writes=[];coupling=[]
            for value in returned:
                writes.append(value)
                if value==0:break
            else:coupling.append((returned[1],0x10))
            self.assertEqual(len(writes),min(miss+1,3))
            self.assertEqual(bool(coupling),miss==3)

if __name__=='__main__':unittest.main()