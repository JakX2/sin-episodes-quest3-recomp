#!/usr/bin/env python3
"""Phase202 synthetic-only mutation tests for original allocator dispatch SHA audit."""
from __future__ import annotations
import sys,struct,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_allocator_dispatch import (REGIONS,WITNESSES,SLOTS,
 DIRECT_CALLS,BOUNDARIES,rel_call,verify_native,audit,VerificationError)
class FakePE:
 base=0x22000000
 def __init__(self):
  self.memory={}
  for name,(va,n,_) in REGIONS.items():
   for off,hx in WITNESSES[name].items():self.put(va+off,bytes.fromhex(hx))
   for off,pointer,_ in SLOTS[name]:self.put(va+off,struct.pack('<I',pointer))
   self.put(va+n,b'\xcc'*BOUNDARIES[name])
  for site,target in DIRECT_CALLS.items():self.put(site,rel_call(site,target))
 def put(self,at,code):
  for offset,value in enumerate(code):
   pos=at+offset
   if pos in self.memory and self.memory[pos]!=value:
    raise AssertionError('conflicting sparse witness at %#x'%pos)
   self.memory[pos]=value
 def at(self,at,n):return bytes(self.memory.get(at+i,0) for i in range(n))
 def section(self,at):
  if 0x22500000<=at<0x22600000:return '.data'
  if 0x223d0000<=at<0x223e0000:return '.rdata'
  return '.text'
 def flip(self,at):self.memory[at]=self.memory.get(at,0)^1
class Tests(unittest.TestCase):
 def test_sparse_fixture_must_not_claim_original(self):
  report=verify_native(FakePE(),check_hash=False)
  self.assertEqual(report['new_sha_verified_original_functions'],0)
  self.assertEqual(report['new_native_function_bytes'],0)
  self.assertEqual(report['sparse_opcode_witnesses'],38)
  self.assertEqual(report['exact_absolute_pointers'],19)
  self.assertEqual(report['exact_direct_calls'],6)
 def test_mutate_every_sparse_instruction(self):
  for name,(va,_,_) in REGIONS.items():
   for off in WITNESSES[name]:
    with self.subTest(name=name,off=off):
     fake=FakePE();fake.flip(va+off)
     with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
 def test_mutate_every_native_absolute_pointer(self):
  for name,(va,_,_) in REGIONS.items():
   for off,_,_ in SLOTS[name]:
    with self.subTest(name=name,off=off):
     fake=FakePE();fake.flip(va+off)
     with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
 def test_mutate_every_native_direct_call(self):
  for site in DIRECT_CALLS:
   with self.subTest(site=site):
    fake=FakePE();fake.flip(site+1)
    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
 def test_mutate_each_function_boundary(self):
  for name,(va,n,_) in REGIONS.items():
   with self.subTest(name=name):
    fake=FakePE();fake.flip(va+n)
    with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
 def test_mutate_unwitnessed_code_fails_full_hash(self):
  fake=FakePE();fake.flip(0x2226e390+0x32)
  with self.assertRaises(VerificationError):verify_native(fake,check_hash=True)
 def test_wrong_image_base_or_data_section(self):
  fake=FakePE();fake.base=0x24000000
  with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
  fake=FakePE();fake.section=lambda _:'.text'
  with self.assertRaises(VerificationError):verify_native(fake,check_hash=False)
 def test_fake_full_dlls_and_methods_rejected(self):
  with self.assertRaises(VerificationError):verify_native(FakePE(),check_hash=True)
  with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')
if __name__=='__main__':unittest.main()
