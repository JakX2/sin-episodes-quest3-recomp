#!/usr/bin/env python3
"""Original-free mutation tests for Phase195 server cache node resolution."""
from __future__ import annotations
import sys
import struct
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_cache_node_resolver import (
 REGIONS,ADDRESS_OFFSETS,WITNESSES,INCOMING_CALLS,rel_call,
 verify_native,audit,VerificationError)

class FakePE:
 base=0x22000000
 def __init__(self):
  self.memory={}
  for va,n,_,sentinel in REGIONS.values():
   for offset,hexcode in WITNESSES.items():self.place(va+offset,bytes.fromhex(hexcode))
   for offset,delta in ADDRESS_OFFSETS.items():self.place(va+offset,struct.pack('<I',sentinel+delta))
   self.place(va+n,b'\xcc'*12)
  for source,dest in INCOMING_CALLS.items():self.place(source,rel_call(source,dest))
 def place(self,at,code):
  for i,b in enumerate(code):
   pos=at+i
   if pos in self.memory and self.memory[pos]!=b:
    raise AssertionError('overlapping native witness conflict %#x'%pos)
   self.memory[pos]=b
 def at(self,va,n):return bytes(self.memory.get(va+i,0) for i in range(n))
 def section(self,va):return '.data' if 0x22500000<=va<0x22600000 else '.text'
 def corrupt(self,va):self.memory[va]=self.memory.get(va,0)^1

class Tests(unittest.TestCase):
 def test_synthetic_without_original_hash_claim(self):
  result=verify_native(FakePE(),check_hash=False)
  self.assertEqual(result['new_original_functions'],0)
  self.assertEqual(result['original_opcode_witnesses'],32)
  self.assertEqual(result['absolute_dword_references'],14)
 def test_independent_opcode_corruption(self):
  for name,(va,_,_,_) in REGIONS.items():
   for off in WITNESSES:
    with self.subTest(name=name,off=off):
     p=FakePE();p.corrupt(va+off)
     with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_independent_sentinel_operand_corruption(self):
  for name,(va,_,_,_) in REGIONS.items():
   for off in ADDRESS_OFFSETS:
    with self.subTest(name=name,off=off):
     p=FakePE();p.corrupt(va+off)
     with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_incoming_exact_call_corruption(self):
  for source in INCOMING_CALLS:
   with self.subTest(site=source):
    p=FakePE();p.corrupt(source+1)
    with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_native_int3_boundary_corruption(self):
  for va,length,_,_ in REGIONS.values():
   p=FakePE();p.corrupt(va+length)
   with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_unwitnessed_body_difference_rejected(self):
  p=FakePE();p.corrupt(REGIONS['scene'][0]+0x40)
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_wrong_image_base_and_sections(self):
  p=FakePE();p.base=0x24000000
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
  p=FakePE();p.section=lambda _:'.text'
  with self.assertRaises(VerificationError):verify_native(p,check_hash=False)
 def test_fake_full_hash_rejected(self):
  with self.assertRaises(VerificationError):verify_native(FakePE(),check_hash=True)
  with self.assertRaises(VerificationError):audit(b'MZ',b'MZ')

if __name__=='__main__':unittest.main()
