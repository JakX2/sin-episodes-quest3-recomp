"""Synthetic, original DLL-free Phase93 regressions."""
import struct
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_cache_slot2_hud_ctors as module
from audit_sin_cache_slot2_hud_ctors import (
 HUD_CTORS,SERVER_REGIONS,SERVER_ANCHORS,SERVER_CALLS,SERVER_INDIRECT,
 audit_client,audit_server,audit,VerificationError)

class FakePE:
 def __init__(self):
  self.memory={}
  self.start=0x24000000
  self.text=bytearray(0x300000)
 def at(self,va,n):
  return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def code(self):
  return self.start,bytes(self.text)
 def put(self,va,raw):
  for i,b in enumerate(raw):
   if va+i in self.memory and self.memory[va+i]!=b:
    raise AssertionError('fixture overlap at %#x'%(va+i))
   self.memory[va+i]=b
 def corrupt(self,va):
  self.memory[va]^=0x40

def client_fixture():
 p=FakePE()
 for name,va,vt in HUD_CTORS:
  raw=(b'\x8b\xc1\xc7\x00'+struct.pack('<I',vt)
       +b'\xc7\x40\x04\x00\x00\x00\x00\xc3')
  p.put(va,raw)
  p.text[va-p.start+4:va-p.start+8]=struct.pack('<I',vt)
 return p

def server_fixture():
 p=FakePE()
 for va,raw in SERVER_ANCHORS.items():
  p.put(va,bytes.fromhex(raw))
 for va,dest in SERVER_CALLS.items():
  p.put(va,b'\xe8'+struct.pack('<i',dest-va-5))
 for va,raw in SERVER_INDIRECT.items():
  p.put(va,bytes.fromhex(raw))
 p.put(0x221ad817,b'\xcc')
 p.put(0x22145515,b'\xcc')
 return p

class Phase93Tests(unittest.TestCase):
 def test_all_eight_client_constructors(self):
  r=audit_client(client_fixture())
  self.assertEqual(r['count'],8)
  self.assertEqual([x['class_name'] for x in r['original_hud_constructors']],
                   [x[0] for x in HUD_CTORS])
 def test_each_original_client_instruction_mutation(self):
  for name,va,vt in HUD_CTORS:
   for offset in (0,3,4,8,10,15):
    with self.subTest(name=name,offset=offset):
     p=client_fixture();p.corrupt(va+offset)
     with self.assertRaises(VerificationError):audit_client(p)
 def test_client_vtable_occurrence_guard(self):
  p=client_fixture()
  p.text[0x500:0x504]=struct.pack('<I',HUD_CTORS[0][2])
  with self.assertRaisesRegex(VerificationError,'reference count'):
   audit_client(p)
 def test_client_original_rtti_crosscheck(self):
  original=module.PRIOR_HUD_RTTI
  changed=list(original)
  changed[0]=tuple([original[0][0],original[0][1],0x1234,*original[0][3:]])
  with patch.object(module,'PRIOR_HUD_RTTI',tuple(changed)):
   with self.assertRaisesRegex(VerificationError,'RTTI cross-phase'):
    audit_client(client_fixture())
 def test_server_synthetic_verifier(self):
  r=audit_server(server_fixture(),False)
  self.assertEqual(r['original_opcode_anchors'],len(SERVER_ANCHORS))
  self.assertEqual(r['direct_calls'],len(SERVER_CALLS))
  self.assertEqual(r['indirect_calls'],len(SERVER_INDIRECT))
 def test_all_server_opcode_mutations(self):
  for va in SERVER_ANCHORS:
   with self.subTest(site=hex(va)):
    p=server_fixture();p.corrupt(va)
    with self.assertRaises(VerificationError):audit_server(p,False)
 def test_all_server_call_mutations(self):
  for va in SERVER_CALLS:
   with self.subTest(site=hex(va)):
    p=server_fixture();p.corrupt(va+1)
    with self.assertRaises(VerificationError):audit_server(p,False)
 def test_all_indirect_dispatch_mutations(self):
  for va in SERVER_INDIRECT:
   with self.subTest(site=hex(va)):
    p=server_fixture();p.corrupt(va)
    with self.assertRaises(VerificationError):audit_server(p,False)
 def test_both_original_method_boundaries(self):
  for va in (0x221ad817,0x22145515):
   p=server_fixture();p.corrupt(va)
   with self.assertRaises(VerificationError):audit_server(p,False)
 def test_three_original_code_regions_sha_guarded(self):
  self.assertEqual(len(SERVER_REGIONS),3)
  with self.assertRaisesRegex(VerificationError,'SHA256'):
   audit_server(server_fixture(),True)
 def test_distinct_scene_and_sound_requirements(self):
  r=audit_server(server_fixture(),False)
  self.assertEqual(r['scene_requires_kind'],5)
  self.assertEqual(r['modelsounds_requires_kind'],3)
  self.assertEqual(r['scene_string_capacity'],64)
 def test_exact_original_dll_sha_chained(self):
  with self.assertRaisesRegex(VerificationError,'SHA-256'):
   audit(b'synthetic client',b'synthetic server')

if __name__=='__main__':
 unittest.main()
