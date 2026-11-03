"""Synthetic original-free corruption tests for Phase 88 SiN client/server."""
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_shutdown_caches import (
 CLIENT,SERVER,REGIONS,OPCODES,CALLS,JMPS,VTABLE_SLOTS,CACHE,
 VerificationError,verify_module,audit)

class FakePE:
 def __init__(self):self.memory={}
 def put(self,va,data):
  for i,b in enumerate(data):
   key=va+i
   if key in self.memory and self.memory[key]!=b:raise AssertionError('overlap at %#x'%key)
   self.memory[key]=b
 def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def corrupt(self,va):self.memory[va]^=0x40

def fixture(module):
 p=FakePE()
 for va,raw in OPCODES[module].items():p.put(va,bytes.fromhex(raw))
 for va,target in CALLS[module].items():p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
 for va,target in JMPS[module].items():p.put(va,b'\xe9'+struct.pack('<i',target-va-5))
 for va,target in VTABLE_SLOTS[module].items():p.put(va,struct.pack('<I',target))
 if module==SERVER:
  for item in CACHE.values():
   p.put(item['instance']+0x34,struct.pack('<II',item['filename_va'],item['format_marker']))
   p.put(item['filename_va'],item['filename'])
  p.put(0x22442a74,b'exec game.cfg\n\x00')
 return p

class ShutdownAuditTests(unittest.TestCase):
 def test_synthetic_client_and_server(self):
  c=verify_module(fixture(CLIENT),CLIENT,False)
  s=verify_module(fixture(SERVER),SERVER,False)
  self.assertEqual(c['reverse_system_virtual_offset'],'0x34')
  self.assertEqual(s['slot_8_virtual_0x28_dispatch_count'],5)
  self.assertEqual(s['cache_close_order'],['modelsounds.cache','scene.cache'])
  self.assertEqual(s['cache_singletons']['scene']['format_marker'],3)
  self.assertEqual(s['cache_singletons']['model_sounds']['format_marker'],2)
 def test_every_opcode_mutation(self):
  for module in (CLIENT,SERVER):
   for va in OPCODES[module]:
    with self.subTest(module=module,site=hex(va)):
     p=fixture(module);p.corrupt(va)
     with self.assertRaises(VerificationError):verify_module(p,module,False)
 def test_every_call_mutation(self):
  for module in (CLIENT,SERVER):
   for va in CALLS[module]:
    with self.subTest(module=module,site=hex(va)):
     p=fixture(module);p.corrupt(va+1)
     with self.assertRaises(VerificationError):verify_module(p,module,False)
 def test_every_jump_mutation(self):
  for module in (CLIENT,SERVER):
   for va in JMPS[module]:
    with self.subTest(module=module,site=hex(va)):
     p=fixture(module);p.corrupt(va+1)
     with self.assertRaises(VerificationError):verify_module(p,module,False)
 def test_every_vtable_slot_mutation(self):
  for module in (CLIENT,SERVER):
   for va in VTABLE_SLOTS[module]:
    with self.subTest(module=module,site=hex(va)):
     p=fixture(module);p.corrupt(va)
     with self.assertRaises(VerificationError):verify_module(p,module,False)
 def test_both_cache_names_pinned(self):
  for item in CACHE.values():
   p=fixture(SERVER);p.corrupt(item['filename_va'])
   with self.assertRaises(VerificationError):verify_module(p,SERVER,False)
 def test_both_cache_format_markers_pinned(self):
  for item in CACHE.values():
   p=fixture(SERVER);p.corrupt(item['instance']+0x38)
   with self.assertRaises(VerificationError):verify_module(p,SERVER,False)
 def test_both_cache_filename_pointers_pinned(self):
  for item in CACHE.values():
   p=fixture(SERVER);p.corrupt(item['instance']+0x34)
   with self.assertRaises(VerificationError):verify_module(p,SERVER,False)
 def test_server_console_command_pinned(self):
  p=fixture(SERVER);p.corrupt(0x22442a74)
  with self.assertRaises(VerificationError):verify_module(p,SERVER,False)
 def test_all_seven_code_region_hashes_required(self):
  self.assertEqual(sum(map(len,REGIONS.values())),7)
  for module in (CLIENT,SERVER):
   with self.assertRaisesRegex(VerificationError,'SHA-256'):
    verify_module(fixture(module),module,True)
 def test_both_original_dll_hashes_mandatory(self):
  with self.assertRaisesRegex(VerificationError,'SHA-256'):
   audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
