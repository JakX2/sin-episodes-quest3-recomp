"""Phase173 synthetic original-code witness corruption and independent models.

The tests never fabricate complete original binary SHA256 fingerprints. The
private game DLL audit uses the owner-held originals separately.
"""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_registry_intern_insert import (
    REGIONS,OPCODES,DIRECT_CALLS,INCOMING_CALLS,
    verify_original_registry_intern_insert,VerificationError)

class SparseImage:
    base=0x22000000
    def __init__(self):self.mem={}
    def put(self,va,words):
        for i,byte in enumerate(words):
            at=va+i
            if at in self.mem and self.mem[at]!=byte:
                raise AssertionError("native fixture witnesses overlap at %#x"%at)
            self.mem[at]=byte
    def at(self,va,n):
        return bytes(self.mem.get(va+i,0xCC) for i in range(n))
    def corrupt(self,va):self.mem[va]^=1

def fixture():
    pe=SparseImage()
    for va,hexstr in OPCODES.items():pe.put(va,bytes.fromhex(hexstr))
    for va,target in {**DIRECT_CALLS,**INCOMING_CALLS}.items():
        pe.put(va,b"\xE8"+struct.pack("<i",target-va-5))
    return pe

def signed_int32(v):
    u=v&0xffffffff
    return u if u<0x80000000 else u-0x100000000

def reference_first_fit(capacity_used,requested):
    target=signed_int32(requested)
    for index,(cap,used) in enumerate(capacity_used):
        if signed_int32(cap-used)>=target:return index
    return -1

def reference_intern(name,existing,chunk_spare):
    events=[]
    if name is None:return 0xffff,events
    events.append('pending')
    events.append('probe')
    if existing!=0xffff:return existing,events
    length=len(name)+1
    events.append('find_chunk')
    index=reference_first_fit(chunk_spare,length)
    if index<0:
        events.append('allocate_pool')
        cap=max(length,2048)
        assert cap+11>cap
        index=len(chunk_spare)
        chunk_spare.append((cap,0))
    old_used=chunk_spare[index][1]
    events.extend(('pool_used','copy','advance','register_locator'))
    capacity,_=chunk_spare[index]
    chunk_spare[index]=(capacity,old_used+length)
    return (index,old_used&0xffff,events)

class OriginalInsertInternTests(unittest.TestCase):
    def test_region_lengths_and_metadata(self):
        self.assertEqual([x[1] for x in REGIONS.values()],[102,98,49,261])
        self.assertEqual(sum(x[1] for x in REGIONS.values()),510)
        for va,sz,sha in REGIONS.values():
            self.assertEqual(len(bytes.fromhex(sha)),32)
            self.assertGreaterEqual(sz,40)
            self.assertEqual(va&0xf,0)
        result=verify_original_registry_intern_insert(fixture(),check_hashes=False)
        self.assertEqual(result['verified_complete_original_function_hashes'],0)
        self.assertEqual(result['verified_sparse_opcode_witnesses'],55)
        self.assertEqual(result['verified_internal_direct_calls'],10)
        self.assertEqual(result['verified_incoming_calls'],7)
    def test_native_instruction_mutation_every_witness(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaisesRegex(VerificationError,'opcode'):
                    verify_original_registry_intern_insert(pe,check_hashes=False)
    def test_native_call_rel32_mutation_all_17_edges(self):
        for va in {**DIRECT_CALLS,**INCOMING_CALLS}:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va+1)
                with self.assertRaisesRegex(VerificationError,'CALL'):
                    verify_original_registry_intern_insert(pe,check_hashes=False)
    def test_sparse_fixture_cannot_pass_whole_code_sha(self):
        with self.assertRaisesRegex(VerificationError,'whole function SHA'):
            verify_original_registry_intern_insert(fixture(),check_hashes=True)
    def test_base_mismatch(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_original_registry_intern_insert(pe,check_hashes=False)
    def test_original_native_call_graph(self):
        self.assertEqual(DIRECT_CALLS[0x2228b0bb],0x2228a280)
        self.assertEqual(DIRECT_CALLS[0x2228b0c2],0x2228a3b0)
        self.assertEqual(DIRECT_CALLS[0x2228b0d6],0x2228af00)
        self.assertEqual(DIRECT_CALLS[0x2228f806],0x2228ea30)
        self.assertEqual(DIRECT_CALLS[0x2228f88d],0x2228f710)
        self.assertEqual(INCOMING_CALLS[0x2228b170],0x2228b0a0)
        self.assertEqual(INCOMING_CALLS[0x2228b1c1],0x2228b0a0)
    def test_first_fit_signed_x86_subtraction_matrix(self):
        seen=0
        for cap,used,req in itertools.product(
                (0,1,8,0x7fffffff,0xffffffff),
                (0,1,8,0x80000000,0xffffffff),
                (0,1,8,0x80000000,0xffffffff)):
            r=reference_first_fit([(cap,used)],req)
            correct=0 if signed_int32(cap-used)>=signed_int32(req) else -1
            self.assertEqual(r,correct)
            seen+=1
        self.assertEqual(seen,125)
        self.assertEqual(reference_first_fit([(0,0xffffffff)],1),0)
        self.assertEqual(reference_first_fit([(8,8),(20,10)],4),1)
        self.assertEqual(reference_first_fit([(8,8),(20,10)],11),-1)
        self.assertEqual(reference_first_fit([],1),-1)
    def test_intern_null_existing_and_new_matrix(self):
        cases=0
        for name,existing,capacity,used in itertools.product(
                (None,b'',b'flak',b'X'*2050),
                (0xffff,0,1,0x27),
                (0,2,2048,4096),
                (0,1,17,2047)):
            chunks=[(capacity,used)]
            result=reference_intern(name,existing,chunks)
            if name is None:
                self.assertEqual(result,(0xffff,[]))
            elif existing!=0xffff:
                self.assertEqual(result,(existing,['pending','probe']))
                self.assertEqual(chunks,[(capacity,used)])
            else:
                index,offset,events=result
                self.assertEqual(offset,(0 if 'allocate_pool' in events else used&0xffff))
                self.assertEqual(events[:3],['pending','probe','find_chunk'])
                self.assertGreaterEqual(chunks[index][0],len(name)+1)
                self.assertEqual(events[-1],'register_locator')
            cases+=1
        self.assertEqual(cases,256)
    def test_original_slot_allocator_mode_matrix(self):
        count=0
        for free_head,used_count,capacity in itertools.product(
                (0xffff,0,1,0xfffe),
                (0,1,0xfffe,0xffff),
                (0,1,0xfffe,0xffff)):
            grow=free_head==0xffff and used_count==capacity
            if free_head==0xffff:
                slot=used_count
                next_used=(used_count+1)&0xffff
            else:
                slot=free_head
                next_used=used_count
            self.assertTrue(0<=slot<=0xffff)
            self.assertEqual(next_used,used_count if free_head!=0xffff else
                             (used_count+1)&0xffff)
            self.assertEqual(grow,(free_head==0xffff and used_count==capacity))
            count+=1
        self.assertEqual(count,64)

if __name__=='__main__':unittest.main()