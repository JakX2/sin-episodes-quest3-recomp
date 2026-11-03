"""Original-DLL-free Phase99 mutation tests for keyboard/key-table and tokens."""
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_legacy_keymap_sound_namespace as a

class FakePE:
 def __init__(self):self.mem={}
 def put(self,va,data):
  for i,b in enumerate(data):
   at=va+i
   if at in self.mem and self.mem[at]!=b:
    raise AssertionError('fixture overlap at %#x'%at)
   self.mem[at]=b
 def at(self,va,n):return bytes(self.mem.get(va+i,0x90) for i in range(n))
 def corrupt(self,va):self.mem[va]^=0x40

def keyboard():
 p=FakePE()
 for va,op in a.ORIGINAL_KEY_OPCODE_ANCHORS.items():p.put(va,bytes.fromhex(op))
 # Use synthetic keys and values, never copy the original mappings.
 for index in range(a.OLD_VGUI_IMMEDIATE_COUNT):
  key=index if index<48 else index+1
  value=index+2
  p.put(a.OLD_VGUI_IMMEDIATE_FIRST+index*10,
        b'\xc7\x05'+struct.pack('<II',a.OLD_VGUI_KEY_TABLE_VA+key*4,value))
 return p

def sound_old():
 p=FakePE()
 for va,op in a.ORIGINAL_SOUND_OPCODE_ANCHORS.items():p.put(va,bytes.fromhex(op))
 return p

def sound_native():
 p=FakePE()
 for va,op in a.NATIVE_SOUND_OPCODE_ANCHORS.items():p.put(va,struct.pack('<I',op))
 return p

class Phase99Tests(unittest.TestCase):
 def test_reconstruct_99_legacy_key_mappings(self):
  table,meta=a.reconstruct_legacy_keymap(keyboard(),False)
  self.assertEqual((len(table),sum(bool(x) for x in table)),(256,99))
  self.assertEqual((table[48],table[0],table[49]),(1,2,50))
  self.assertEqual(meta['consecutive_immediate_stores'],98)
 def test_all_keyboard_prologue_epilogue_opcode_corruptions(self):
  for va in a.ORIGINAL_KEY_OPCODE_ANCHORS:
   with self.subTest(site=hex(va)):
    p=keyboard();p.corrupt(va)
    with self.assertRaises(a.VerificationError):a.reconstruct_legacy_keymap(p,False)
 def test_each_of_98_original_key_assignment_mutations(self):
  for i in range(98):
   with self.subTest(index=i):
    p=keyboard();p.corrupt(a.OLD_VGUI_IMMEDIATE_FIRST+i*10)
    with self.assertRaisesRegex(a.VerificationError,'MOV'):
     a.reconstruct_legacy_keymap(p,False)
 def test_key_assignment_duplicate_rejected(self):
  p=keyboard();va=a.OLD_VGUI_IMMEDIATE_FIRST+10
  for i,b in enumerate(struct.pack('<I',a.OLD_VGUI_KEY_TABLE_VA)):
   p.mem[va+2+i]=b
  with self.assertRaisesRegex(a.VerificationError,'duplicate'):
   a.reconstruct_legacy_keymap(p,False)
 def test_key_assignment_out_of_range_rejected(self):
  p=keyboard();va=a.OLD_VGUI_IMMEDIATE_FIRST
  for i,b in enumerate(struct.pack('<I',a.OLD_VGUI_KEY_TABLE_VA+256*4)):
   p.mem[va+2+i]=b
  with self.assertRaisesRegex(a.VerificationError,'out of range'):
   a.reconstruct_legacy_keymap(p,False)
 def test_key_assignment_zero_value_rejected(self):
  p=keyboard();va=a.OLD_VGUI_IMMEDIATE_FIRST
  for i in range(4):p.mem[va+6+i]=0
  with self.assertRaisesRegex(a.VerificationError,'invalid key'):
   a.reconstruct_legacy_keymap(p,False)
 def test_full_original_initializer_sha_enforced(self):
  with self.assertRaisesRegex(a.VerificationError,'SHA256'):
   a.reconstruct_legacy_keymap(keyboard(),True)
 def test_every_original_sound_validator_instruction(self):
  for va in a.ORIGINAL_SOUND_OPCODE_ANCHORS:
   with self.subTest(site=hex(va)):
    p=sound_old();p.corrupt(va)
    with self.assertRaises(a.VerificationError):
     a.verify_sound_namespace(p,sound_native(),False)
 def test_every_native_sound_validator_instruction(self):
  for va in a.NATIVE_SOUND_OPCODE_ANCHORS:
   with self.subTest(site=hex(va)):
    p=sound_native();p.corrupt(va)
    with self.assertRaises(a.VerificationError):
     a.verify_sound_namespace(sound_old(),p,False)
 def test_original_native_validator_sha_enforced(self):
  with self.assertRaisesRegex(a.VerificationError,'SHA256'):
   a.verify_sound_namespace(sound_old(),sound_native(),True)
 def test_native_sound_namespaces_are_explicitly_separate(self):
  r=a.verify_sound_namespace(sound_old(),sound_native(),False)
  self.assertEqual(r['old_name_to_token_slot'],3)
  self.assertEqual(r['native_candidate_name_to_token_slot'],7)
  self.assertFalse(r['native_candidates_abi_verified'])
 def test_all_six_input_binary_provenance_required(self):
  with self.assertRaises(a.VerificationError):
   a.phase98_audit(b'x',b'x',b'x',b'x',b'x',b'x')

if __name__=='__main__':unittest.main()
