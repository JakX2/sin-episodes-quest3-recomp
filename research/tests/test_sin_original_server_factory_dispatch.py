"""Phase171 original-free opcode, call, vtable and dispatch matrix regressions."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_factory_dispatch import (
 REGIONS,OPCODES,CALLS,VTABLE,WARNINGS,OriginalError,
 verify_native,audit)

class MockPE:
 base=0x22000000
 def __init__(self):self.memory={}
 def put(self,va,raw):
  for i,b in enumerate(raw):
   k=va+i
   if k in self.memory and self.memory[k]!=b:raise AssertionError('conflict %#x'%k)
   self.memory[k]=b
 def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def damage(self,va):self.memory[va]^=1

def fixture():
 p=MockPE()
 for va,s in OPCODES.items():p.put(va,bytes.fromhex(s))
 for va,target in CALLS.items():p.put(va,bytes((0xe8,))+struct.pack('<i',target-va-5))
 for va,target in VTABLE.items():p.put(va,struct.pack('<I',target))
 for va,raw in WARNINGS.items():p.put(va,raw)
 return p

def modeled_dispatch(slot,factory_exists,destroy):
 if slot==0xffff or not factory_exists:return 'warn_destroy' if destroy else 'warn_create'
 return 'factory_virtual4' if destroy else 'factory_virtual0'

class NativeRegistryDispatch(unittest.TestCase):
 def test_partial_evidence_totals(self):
  summary=verify_native(fixture(),check_hashes=False)
  self.assertEqual(summary['native_opcode_witnesses'],51)
  self.assertEqual(summary['native_direct_calls'],2)
  self.assertEqual(summary['native_vtable_slots'],3)
  self.assertEqual(summary['native_diagnostic_literals'],2)
  self.assertEqual(len(REGIONS),4)
 def test_opcode_mutations(self):
  for va in OPCODES:
   with self.subTest(va=hex(va)):
    p=fixture();p.damage(va)
    with self.assertRaisesRegex(OriginalError,'opcode'):verify_native(p,check_hashes=False)
 def test_call_mutations(self):
  for va in CALLS:
   with self.subTest(va=hex(va)):
    p=fixture();p.damage(va+1)
    with self.assertRaisesRegex(OriginalError,'CALL'):verify_native(p,check_hashes=False)
 def test_vtable_mutations(self):
  for va in VTABLE:
   with self.subTest(va=hex(va)):
    p=fixture();p.damage(va)
    with self.assertRaisesRegex(OriginalError,'vtable'):verify_native(p,check_hashes=False)
 def test_warning_mutations(self):
  for va in WARNINGS:
   with self.subTest(va=hex(va)):
    p=fixture();p.damage(va)
    with self.assertRaisesRegex(OriginalError,'diagnostic'):verify_native(p,check_hashes=False)
 def test_full_hash_requires_original(self):
  with self.assertRaisesRegex(OriginalError,'whole-function SHA'):
   verify_native(fixture(),check_hashes=True)
  with self.assertRaises(OriginalError):audit(b'not original',b'not original')
 def test_original_function_lengths_and_calls(self):
  self.assertEqual([x[1] for x in REGIONS.values()],[94,46,50,18])
  self.assertEqual(CALLS[0x2228B02E],0x2228F7A0)
  self.assertEqual(CALLS[0x2228B044],0x2228A760)
  self.assertEqual(VTABLE[0x22456328],0x22285550)
  self.assertEqual(VTABLE[0x2245632C],0x22285580)
  self.assertEqual(VTABLE[0x22456330],0x2228B010)
 def test_independent_wrapper_matrix(self):
  combinations=0
  for slot,has_factory,destroy in itertools.product((0,1,0xffff),(False,True),(False,True)):
   r=modeled_dispatch(slot,has_factory,destroy)
   if slot==0xffff or not has_factory:self.assertIn(r,('warn_create','warn_destroy'))
   else:self.assertIn(r,('factory_virtual0','factory_virtual4'))
   combinations+=1
  self.assertEqual(combinations,12)
 def test_original_record_size_and_16bit_index(self):
  for i in (0,1,255,256,0xfffe,0xffff):
   self.assertEqual((i&0xffff)*16+12,12+16*(i&0xffff))
if __name__=='__main__':unittest.main()