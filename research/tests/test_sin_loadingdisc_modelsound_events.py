"""Original-DLL-free mutation tests for SiN Phase94 x86 evidence."""
from __future__ import annotations
import sys,struct,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_loadingdisc_modelsound_events import (
 REGIONS,OPCODES,CALLS,STRING_SITES,ORIGINAL_LOADINGDISC_VTABLE,
 ORIGINAL_LOADINGDISC_SINGLETON,verify_one,audit,VerificationError)

class FakePE:
 def __init__(self):self.d={}
 def at(self,va,n):return bytes(self.d.get(va+i,0xcc) for i in range(n))
 def put(self,va,data):
  for i,b in enumerate(data):
   k=va+i
   if k in self.d and self.d[k]!=b:raise AssertionError('fixture overlap at %#x'%k)
   self.d[k]=b
 def corrupt(self,va):self.d[va]^=0x20

def fixture(module):
 pe=FakePE()
 for va,op in OPCODES[module].items():pe.put(va,bytes.fromhex(op))
 for va,dest in CALLS[module].items():
  pe.put(va,b'\xe8'+struct.pack('<i',dest-va-5))
 if module=='client.dll':
  pe.put(ORIGINAL_LOADINGDISC_VTABLE,struct.pack('<4I',0x241ee470,0x241ee480,0x241ee4d0,0x241ee530))
  pe.put(ORIGINAL_LOADINGDISC_SINGLETON,struct.pack('<4I',ORIGINAL_LOADINGDISC_VTABLE,0,0,0))
 else:
  for va,s in STRING_SITES.items():pe.put(va,s)
 return pe

class Phase94Tests(unittest.TestCase):
 def test_synthetic_success_both_modules(self):
  c=verify_one(fixture('client.dll'),'client.dll',False)
  s=verify_one(fixture('server.dll'),'server.dll',False)
  self.assertEqual(c['allocated_resource_bytes'],220)
  self.assertEqual(s['event_record_stride_bytes'],80)
 def test_all_five_full_function_hashes_required(self):
  self.assertEqual(sum(map(len,REGIONS.values())),5)
  for mod in REGIONS:
   with self.assertRaisesRegex(VerificationError,'SHA mismatch'):
    verify_one(fixture(mod),mod,True)
 def test_every_isa_opcode_site(self):
  for mod in OPCODES:
   for va in OPCODES[mod]:
    with self.subTest(mod=mod,site=hex(va)):
     pe=fixture(mod);pe.corrupt(va)
     with self.assertRaises(VerificationError):verify_one(pe,mod,False)
 def test_every_direct_call_edge(self):
  for mod in CALLS:
   for va in CALLS[mod]:
    with self.subTest(mod=mod,site=hex(va)):
     pe=fixture(mod);pe.corrupt(va+1)
     with self.assertRaises(VerificationError):verify_one(pe,mod,False)
 def test_client_four_original_vtable_slots(self):
  for slot in range(4):
   pe=fixture('client.dll');pe.corrupt(ORIGINAL_LOADINGDISC_VTABLE+4*slot)
   with self.assertRaisesRegex(VerificationError,'virtual methods'):
    verify_one(pe,'client.dll',False)
 def test_static_client_second_pointer_zero(self):
  pe=fixture('client.dll');pe.corrupt(ORIGINAL_LOADINGDISC_SINGLETON+8)
  with self.assertRaisesRegex(VerificationError,'static singleton'):
   verify_one(pe,'client.dll',False)
 def test_client_lazy_allocation_and_vcalls(self):
  c=verify_one(fixture('client.dll'),'client.dll',False)
  self.assertEqual(c['primary_resource_offset'],4)
  self.assertEqual(c['secondary_resource_offset'],8)
  self.assertEqual(c['secondary_configuration_virtual_offset'],0x28c)
  self.assertEqual(c['resource_toggle_virtual_offset'],0x78)
 def test_all_original_model_sound_strings(self):
  for va in STRING_SITES:
   pe=fixture('server.dll');pe.corrupt(va)
   with self.assertRaisesRegex(VerificationError,'format text'):
    verify_one(pe,'server.dll',False)
 def test_original_event_ids_and_flag(self):
  s=verify_one(fixture('server.dll'),'server.dll',False)
  self.assertEqual(s['event_id_values'],[14,1004,1008,5004,6004,6005])
  self.assertEqual(s['event_14_requires_flag_bit'],16)
  self.assertEqual(s['footstep_event_ids'],[6004,6005])
 def test_model_sound_token_sentinel(self):
  s=verify_one(fixture('server.dll'),'server.dll',False)
  self.assertEqual(s['token_not_found'],0xffffffff)
  self.assertTrue(s['deduplicates_tokens'])
 def test_no_disk_record_format_claim(self):
  s=verify_one(fixture('server.dll'),'server.dll',False)
  self.assertIn('in-memory',s['warning'])
 def test_exact_original_fingerprints_required(self):
  with self.assertRaisesRegex(VerificationError,'SHA-256'):
   audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
