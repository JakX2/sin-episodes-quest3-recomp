"""Synthetic original-DLL-free tests for SiN LoadingDiscPanel and sound emitter."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_loading_panel_emitter as a

class FakePE:
 def __init__(self):self.mem={};self.names={}
 def at(self,va,n):return bytes(self.mem.get(va+i,0xcc) for i in range(n))
 def put(self,va,raw):
  for i,b in enumerate(raw):
   key=va+i
   if key in self.mem and self.mem[key]!=b:
    raise AssertionError('synthetic fixture overlap at %#x'%key)
   self.mem[key]=b
 def cstring(self,va):return self.names[va]
 def word(self,va,value):self.put(va,struct.pack('<I',value))
 def corrupt(self,va):self.mem[va]^=0x20

def client_fixture():
 p=FakePE()
 for va,hexbytes in a.CLIENT_ANCHORS.items():p.put(va,bytes.fromhex(hexbytes))
 for va,target in a.CLIENT_CALLS.items():
  p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
 p.names.update(a.CLIENT_STRING_VAS)
 for vt,locator,descriptor,name in (
    (0x242ac5c4,0x242eadd4,0x2431dde4,'.?AVCLoadingDiscPanel@@'),
    (0x242d4d64,0x242f0740,0x24322a70,'.?AVLabel@vgui@@')):
  p.word(vt-4,locator);p.put(locator,bytes(12))
  p.word(locator+12,descriptor);p.names[descriptor+8]=name
 p.put(0x241ee42e,b'\xcc')
 return p
def server_fixture():
 p=FakePE()
 for va,hexbytes in a.SERVER_ANCHORS.items():p.put(va,bytes.fromhex(hexbytes))
 p.names[0x2244293c]='VSoundEmitter001'
 return p

class OriginalPanelEmitterTests(unittest.TestCase):
 def test_both_synthetic_models(self):
  c=a.inspect_client(client_fixture(),False)
  s=a.inspect_server(server_fixture(),False)
  self.assertEqual(c['nested_label_allocation_bytes'],240)
  self.assertEqual(s['interface_version'],'VSoundEmitter001')
 def test_all_three_original_sha256s_enforced(self):
  self.assertEqual(len(a.CLIENT_REGIONS)+len(a.SERVER_REGIONS),3)
  with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
   a.inspect_client(client_fixture())
  with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
   a.inspect_server(server_fixture())
 def test_all_client_instruction_mutations(self):
  for va in a.CLIENT_ANCHORS:
   with self.subTest(site=hex(va)):
    p=client_fixture();p.corrupt(va)
    with self.assertRaises(a.VerificationError):a.inspect_client(p,False)
 def test_all_client_call_targets_mutations(self):
  for va in a.CLIENT_CALLS:
   with self.subTest(site=hex(va)):
    p=client_fixture();p.corrupt(va+1)
    with self.assertRaises(a.VerificationError):a.inspect_client(p,False)
 def test_client_original_strings_mutations(self):
  for va in a.CLIENT_STRING_VAS:
   p=client_fixture();p.names[va]='BAD'
   with self.assertRaises(a.VerificationError):a.inspect_client(p,False)
 def test_original_panel_rtti_locator_mutation(self):
  p=client_fixture();p.corrupt(0x242ac5c4-4)
  with self.assertRaises((a.VerificationError,KeyError)):a.inspect_client(p,False)
 def test_original_nested_label_rtti_class_mutation(self):
  p=client_fixture();p.names[0x24322a70+8]='.?AVWrong@@'
  with self.assertRaisesRegex(a.VerificationError,'RTTI type'):
   a.inspect_client(p,False)
 def test_constructor_terminal_boundary_guard(self):
  p=client_fixture();p.corrupt(0x241ee42e)
  with self.assertRaisesRegex(a.VerificationError,'boundary'):
   a.inspect_client(p,False)
 def test_all_server_instruction_mutations(self):
  for va in a.SERVER_ANCHORS:
   with self.subTest(site=hex(va)):
    p=server_fixture();p.corrupt(va)
    with self.assertRaises(a.VerificationError):a.inspect_server(p,False)
 def test_emitter_interface_name_must_be_exact(self):
  p=server_fixture();p.names[0x2244293c]='VSoundEmitter002'
  with self.assertRaisesRegex(a.VerificationError,'VSoundEmitter001'):
   a.inspect_server(p,False)
 def test_token_method_offsets_are_directional(self):
  r=a.inspect_server(server_fixture(),False)
  self.assertEqual(r['name_to_token_virtual_offset'],12)
  self.assertEqual(r['token_to_name_virtual_offset'],24)
  self.assertEqual(len(r['name_to_token_call_sites']),3)
  self.assertEqual(len(r['token_to_name_call_sites']),2)
 def test_chained_original_dll_fingerprints_required(self):
  with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
   a.audit(b'fake client',b'fake server')
if __name__=='__main__':unittest.main()
