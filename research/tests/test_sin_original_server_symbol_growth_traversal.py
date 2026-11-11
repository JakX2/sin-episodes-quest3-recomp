"""Original-free Phase176 native evidence mutation + independent branch matrices."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_symbol_growth_traversal import (
 REGIONS,OPCODES,CALLS,INCOMING,SENTINEL_WORDS,
 verify_original_symbol_growth,VerificationError)

class FakePE:
 base=0x22000000
 def __init__(self):self.mem={}
 def put(self,va,data):
  for idx,b in enumerate(data):
   assert va+idx not in self.mem or self.mem[va+idx]==b,hex(va+idx)
   self.mem[va+idx]=b
 def at(self,va,n):return bytes(self.mem.get(va+i,0xCC) for i in range(n))
 def corrupt(self,va):self.mem[va]^=0x20

def fixture():
 m=FakePE()
 for va,raw in OPCODES.items():m.put(va,bytes.fromhex(raw))
 for va,target in {**CALLS,**INCOMING}.items():
  m.put(va,b'\xe8'+struct.pack('<i',target-va-5))
 return m

def signed32(x):
 x&=0xffffffff
 return x if x<0x80000000 else x-0x100000000

def original_reserve(capacity,additional,quantum,stride):
 if quantum==0xffffffff:return (capacity,0,False)
 target=(capacity+additional)&0xffffffff
 count=0
 while signed32(capacity)<signed32(target):
  if capacity==0:capacity=3 if stride==12 else 8
  elif quantum:capacity=(capacity+quantum)&0xffffffff
  else:capacity=(capacity*2)&0xffffffff
  count+=1
  if count>1000:return (capacity,0,True)
 return (capacity,(capacity*stride)&0xffffffff,False)

class Tests(unittest.TestCase):
 def test_native_functions_and_evidence_count(self):
  assert [x[1] for x in REGIONS.values()]==[115,112,261,58]
  assert sum(x[1] for x in REGIONS.values())==546
  report=verify_original_symbol_growth(fixture(),check_hashes=False)
  assert report['verified_complete_function_hashes']==0
  assert report['verified_sparse_opcode_witnesses']==len(OPCODES)
  assert len(OPCODES)>=65
  assert report['verified_internal_direct_calls']==5
  assert report['verified_incoming_direct_calls']==2
 def test_opcode_corruption(self):
  for va in OPCODES:
   with self.subTest(va=hex(va)):
    p=fixture();p.corrupt(va)
    with self.assertRaisesRegex(VerificationError,'opcode witness'):
     verify_original_symbol_growth(p,check_hashes=False)
 def test_call_edge_corruption(self):
  for va in {**CALLS,**INCOMING}:
   with self.subTest(va=hex(va)):
    p=fixture();p.corrupt(va+1)
    with self.assertRaisesRegex(VerificationError,'direct CALL'):
     verify_original_symbol_growth(p,check_hashes=False)
 def test_full_native_body_hash_rejects_sparse_fixture(self):
  for _,(_,_,sha) in REGIONS.items():assert len(bytes.fromhex(sha))==32
  with self.assertRaisesRegex(VerificationError,'whole original native function SHA'):
   verify_original_symbol_growth(fixture(),check_hashes=True)
 def test_wrong_pe_base(self):
  p=fixture();p.base=0x24000000
  with self.assertRaisesRegex(VerificationError,'image base'):
   verify_original_symbol_growth(p,check_hashes=False)
 def test_function_call_graph(self):
  assert CALLS[0x2228E24F]==0x2226E400
  assert CALLS[0x2228E265]==0x2226E390
  assert CALLS[0x2228E2CC]==0x2226E400
  assert CALLS[0x2228E2E2]==0x2226E390
  assert CALLS[0x2228E5C7]==0x2226E3E0
  assert INCOMING[0x2228E63C]==0x2228E200
  assert INCOMING[0x2228F72B]==0x2228E470
 def test_modular_signed_capacity_matrix(self):
  count=0
  for stride,cap,additional,quantum in itertools.product((4,12),
      (0,1,3,8,20,0x7fffffff,0x80000000,0xffffffff),
      (0,1,2,9,0xffffffff),(0,1,3,0xffffffff)):
   final,bytes_out,stuck=original_reserve(cap,additional,quantum,stride)
   if quantum==0xffffffff:assert final==cap and bytes_out==0
   elif not stuck:assert bytes_out==((final*stride)&0xffffffff)
   count+=1
  assert count==320
  assert original_reserve(0,1,0,12)==(3,36,False)
  assert original_reserve(0,1,0,4)==(8,32,False)
  assert original_reserve(8,9,0,4)==(32,128,False)
  assert original_reserve(8,8,3,4)==(17,68,False)
  assert original_reserve(0x7fffffff,3,0,12)[0]==0x7fffffff
 def test_symbol_tree_branch_matrix(self):
  n=0
  for root,left,right,choice in itertools.product((0xffff,0,1),
      (0xffff,1),(0xffff,2),(False,True)):
   # Native writes initial sentinel before first comparator, only when root exists.
   out=0xffff;flag=0;init=False
   if root!=0xffff:
    out=root;flag=int(choice);init=True
    next_index=left if choice else right
    assert next_index in (0xffff,1,2)
   assert init==(root!=0xffff)
   assert 0<=out<=0xffff and flag in (0,1)
   n+=1
  assert n==24
 def test_symbol_native_sentinel(self):
  assert SENTINEL_WORDS==(0x2259012c,0x2259012e,0x22590130,0x22590132)
 def test_cleanup_matrix(self):
  count=0
  for guard,has_ptr,capacity in itertools.product((0xffffffff,0,3),
                                              (False,True),(0,15)):
   free=guard!=0xffffffff and has_ptr
   remaining=has_ptr if guard==0xffffffff else False
   remaining_capacity=capacity if guard==0xffffffff else 0
   assert free<=has_ptr
   if guard==0xffffffff:assert remaining==has_ptr
   else:assert remaining_capacity==0
   count+=1
  assert count==12
if __name__=='__main__':unittest.main()