#!/usr/bin/env python3
"""Synthetic-only negative tests of Phase201 original growth/init evidence."""
from __future__ import annotations
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_cache_array_growth import (
 GROW,INIT,GROW_LEN,INIT_LEN,GROW_WITNESSES,INIT_WITNESSES,
 CALLS,INCOMING,relcall,verify_native,audit,VerificationError)
class SyntheticPE:
 base=0x22000000
 def __init__(self):
  self.mem={}
  for va,_ in GROW.values():
   for off,word in GROW_WITNESSES.items():self.place(va+off,bytes.fromhex(word))
   for off,target in CALLS.items():self.place(va+off,relcall(va+off,target))
   self.place(va+GROW_LEN,b'\xcc'*9)
  for va,_ in INIT.values():
   for off,word in INIT_WITNESSES.items():self.place(va+off,bytes.fromhex(word))
   self.place(va+INIT_LEN,b'\xcc'*12)
  for src,dest in INCOMING.items():self.place(src,relcall(src,dest))
 def place(self,addr,data):
  for i,byte in enumerate(data):
   pos=addr+i
   if pos in self.mem and self.mem[pos]!=byte:raise AssertionError('synthetic sparse witness overlap')
   self.mem[pos]=byte
 def at(self,addr,n):return bytes(self.mem.get(addr+i,0) for i in range(n))
 def section(self,addr):return '.text'
 def flip(self,addr):self.mem[addr]=self.mem.get(addr,0)^1
class Tests(unittest.TestCase):
 def test_sparse_synthetic_never_counts_as_original(self):
  r=verify_native(SyntheticPE(),check_hash=False)
  self.assertEqual(r['new_sha_verified_functions'],0)
  self.assertEqual(r['sparse_opcode_witnesses'],52)
  self.assertEqual(r['exact_outgoing_allocator_calls'],4)
  self.assertEqual(r['exact_incoming_slot_allocator_calls'],2)
 def test_each_growth_instruction_witness_mutation(self):
  for name,(va,_) in GROW.items():
   for off in GROW_WITNESSES:
    with self.subTest(method=name,off=off):
     p=SyntheticPE();p.flip(va+off)
     with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_each_key_initializer_instruction_witness_mutation(self):
  for name,(va,_) in INIT.items():
   for off in INIT_WITNESSES:
    with self.subTest(method=name,off=off):
     p=SyntheticPE();p.flip(va+off)
     with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_all_native_allocator_calls_mutated(self):
  for name,(va,_) in GROW.items():
   for off in CALLS:
    with self.subTest(method=name,off=off):
     p=SyntheticPE();p.flip(va+off+1)
     with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_all_native_callers_mutated(self):
  for src in INCOMING:
   with self.subTest(src=src):
    p=SyntheticPE();p.flip(src+1)
    with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_all_native_method_boundaries_mutated(self):
  for va,_ in GROW.values():
   p=SyntheticPE();p.flip(va+GROW_LEN)
   with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
  for va,_ in INIT.values():
   p=SyntheticPE();p.flip(va+INIT_LEN)
   with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_sparse_unwitnessed_byte_breaks_normalized_twin(self):
  p=SyntheticPE();p.flip(GROW['scene'][0]+0x1a)
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
  p=SyntheticPE();p.flip(INIT['scene'][0]+0x11)
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_wrong_pe_base_section_and_fabricated_hashes(self):
  p=SyntheticPE();p.base=0x24000000
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
  p=SyntheticPE();p.section=lambda _:'.rdata'
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
  with self.assertRaises(VerificationError):verify_native(SyntheticPE(),check_hash=True)
  with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')
if __name__=='__main__':unittest.main()
