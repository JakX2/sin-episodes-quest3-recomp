#!/usr/bin/env python3
"""Synthetic-only mutation tests for the original Phase220 type-5 x86 auditor."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_dword_specialists as native

class SyntheticPE:
 def __init__(self):
  self.base=native.BASE["client"]
  self.mem={}
  self.rdata=set()
  self.data={native.SYMBOL_RESOLVER_GLOBAL}
  for va,size,digest,pad,caller,pop,witnesses in native.METHODS.values():
   for off,hx in witnesses.items():self.write(va+off,bytes.fromhex(hx))
   self.write(va+size-3,b"\xc2"+pop.to_bytes(2,"little"))
   self.write(va+size,b"\xcc"*pad)
   self.write(caller,native.rel_call(caller,va))
  for site,target in native.NESTED_CALLS+(native.PHASE213_COPY,native.PHASE217_DIAGNOSTIC):
   self.write(site,native.rel_call(site,target))
  self.write(native.NEXT_VA,native.NEXT_BYTES)
  for va,s in native.FORMATS.items():
   raw=s.encode()+b"\0";self.write(va,raw)
   self.rdata.update(range(va,va+len(raw)))
 def write(self,va,raw):
  for i,v in enumerate(raw):
   old=self.mem.get(va+i,v)
   if old!=v:raise AssertionError(f"synthetic x86 fixture overlap {va+i:#x}")
   self.mem[va+i]=v
 def at(self,va,n):return bytes(self.mem.get(va+i,0) for i in range(n))
 def section_of(self,va):
  if va in self.rdata:return ".rdata"
  if va in self.data:return ".data"
  return ".text"
 def flip(self,va):self.mem[va]=self.mem.get(va,0)^1

class Phase220Tests(unittest.TestCase):
 def check(self,fake):return native.verify_native(fake,check_hash=False,check_imports=False)
 def test_synthetic_never_counts_as_original(self):
  report=self.check(SyntheticPE())
  self.assertEqual(report["new_sha_verified_original_client_functions"],0)
  self.assertEqual(report["new_complete_original_x86_bytes"],0)
  self.assertEqual(report["exact_original_type5_router_calls"],4)
  self.assertTrue(report["original_preview_adjacent_without_int3"])
  self.assertEqual(report["sparse_opcode_witnesses"],31)
  with self.assertRaises(native.VerificationError):
   native.verify_native(SyntheticPE(),check_hash=True,check_imports=False)
  with self.assertRaises(native.VerificationError):native.audit(b"MZ",b"MZ")
 def test_all_sparse_opcode_witness_mutations_fail(self):
  for name,(va,_,_,_,_,_,witnesses) in native.METHODS.items():
   for off in witnesses:
    with self.subTest(name=name,off=hex(off)):
     fake=SyntheticPE();fake.flip(va+off)
     with self.assertRaises(native.VerificationError):self.check(fake)
 def test_every_real_native_callsite_mutation_fails(self):
  edges=[(x[4],x[0]) for x in native.METHODS.values()]
  edges+=list(native.NESTED_CALLS)+(native.PHASE213_COPY,native.PHASE217_DIAGNOSTIC)
  for caller,target in edges:
   with self.subTest(site=hex(caller),target=hex(target)):
    fake=SyntheticPE();fake.flip(caller+1)
    with self.assertRaises(native.VerificationError):self.check(fake)
 def test_native_RET_pads_and_no_padding_adjacency(self):
  for name,(va,n,_,pad,_,_,_) in native.METHODS.items():
   fake=SyntheticPE();fake.flip(va+n-2)
   with self.assertRaises(native.VerificationError):self.check(fake)
   for i in range(pad):
    with self.subTest(name=name,pad=i):
     fake=SyntheticPE();fake.flip(va+n+i)
     with self.assertRaises(native.VerificationError):self.check(fake)
  for i in range(len(native.NEXT_BYTES)):
   fake=SyntheticPE();fake.flip(native.NEXT_VA+i)
   with self.assertRaises(native.VerificationError):self.check(fake)
 def test_original_format_global_import_section(self):
  for va,s in native.FORMATS.items():
   for off in (0,len(s)):
    fake=SyntheticPE();fake.flip(va+off)
    with self.assertRaises(native.VerificationError):self.check(fake)
  fake=SyntheticPE();fake.data.clear()
  with self.assertRaises(native.VerificationError):self.check(fake)
  with patch.object(native,"named_iat",return_value=[native.Q_FILEBASE_IAT]):
   native.verify_native(SyntheticPE(),check_hash=False,check_imports=True)
  with patch.object(native,"named_iat",return_value=[]):
   with self.assertRaises(native.VerificationError):
    native.verify_native(SyntheticPE(),check_hash=False,check_imports=True)
 def test_exact_native_sizes_and_boundaries(self):
  self.assertEqual(sum(v[1] for v in native.METHODS.values()),269)
  self.assertEqual({v[0] for v in native.METHODS.values()},
                   {0x24197DB0,0x241983B0})
  self.assertEqual(native.METHODS["type5_integer_symbol_aware_preview"][3],0)
  fake=SyntheticPE();fake.base+=0x1000
  with self.assertRaises(native.VerificationError):self.check(fake)
if __name__=="__main__":unittest.main()
