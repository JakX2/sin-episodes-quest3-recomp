#!/usr/bin/env python3
"""Phase194 original-free mutation tests; no original DLL binary data."""
from __future__ import annotations
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_cache_slot_allocation import (
    REGIONS,CALLS,INCOMING,WITNESSES,rel32,verify_native,audit,VerificationError)

class SyntheticPE:
 base=0x22000000
 def __init__(self):
  self.memory={}
  for va,n,_ in REGIONS.values():
   for off,code in WITNESSES.items():self.place(va+off,bytes.fromhex(code))
   for off,target in CALLS[va].items():self.place(va+off,rel32(va+off,target))
   self.place(va+n,b'\xcc'*9)
  for site,dest in INCOMING.items():self.place(site,rel32(site,dest))
 def place(self,addr,b):
  for i,value in enumerate(b):
   at=addr+i
   if at in self.memory and self.memory[at]!=value:
    raise AssertionError('witness overlap mismatch %#x'%at)
   self.memory[at]=value
 def at(self,va,size):return bytes(self.memory.get(va+i,0) for i in range(size))
 def section(self,va):return '.text'
 def mutate(self,at):self.memory[at]=self.memory.get(at,0)^1

class Tests(unittest.TestCase):
 def test_fixture_witnesses_do_not_claim_original_bytes(self):
  p=verify_native(SyntheticPE(),check_hash=False)
  self.assertEqual(p['new_original_sha_verified_functions'],0)
  self.assertEqual(p['original_witnesses'],40)
  self.assertEqual(p['exact_direct_incoming_calls'],2)
 def test_each_original_opcode_is_independently_mutation_checked(self):
  for label,(va,_,_) in REGIONS.items():
   for off in WITNESSES:
    with self.subTest(label=label,off=off):
     f=SyntheticPE();f.mutate(va+off)
     with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_each_native_outgoing_call_is_checked(self):
  for va,calls in CALLS.items():
   for off in calls:
    with self.subTest(site=va+off):
     f=SyntheticPE();f.mutate(va+off+1)
     with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_each_native_incoming_call_is_checked(self):
  for at in INCOMING:
   with self.subTest(site=at):
    f=SyntheticPE();f.mutate(at+1)
    with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_int3_function_boundary(self):
  for va,n,_ in REGIONS.values():
   f=SyntheticPE();f.mutate(va+n)
   with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_non_witness_code_difference_rejected_by_full_normalized_twin(self):
  f=SyntheticPE();f.mutate(REGIONS['scene'][0]+0x11)
  with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_wrong_image_base_or_section(self):
  f=SyntheticPE();f.base=0x24000000
  with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
  f=SyntheticPE();f.section=lambda _:'.rdata'
  with self.assertRaises(VerificationError):verify_native(f,check_hash=False)
 def test_synthetic_bytes_cannot_match_original_native_hashes(self):
  with self.assertRaises(VerificationError):verify_native(SyntheticPE(),check_hash=True)
  with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
