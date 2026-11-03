"""Phase98 original-free synthetic mutation tests; no proprietary engine DLL needed."""
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_original_engine_sound_vgui_abi as a
class FakePE:
 def __init__(self,base):
  self.base=base;self.memory={};self.names={};self.data=b'synthetic'
 def put(self,va,raw):
  for i,b in enumerate(raw):
   at=va+i
   if at in self.memory and self.memory[at]!=b:
    raise AssertionError('fixture overlap at %#x'%at)
   self.memory[at]=b
 def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def word(self,va,value):self.put(va,struct.pack('<I',value))
 def corrupt(self,va):self.memory[va]^=0x40
 def cstring(self,va):return self.names[va]
 def exports(self):return {'CreateInterface':0x2c001230 if self.base==0x2c000000 else 0x1000a520}
def old_sound():
 p=FakePE(0x2c000000)
 for va,raw in a.X86_SITES['old_sound'].items():p.put(va,bytes.fromhex(raw))
 va,n,sha=a.VTABLES['old_sound']
 methods=[0x2c004730]*n;methods[3]=0x2c00a830;methods[6]=0x2c008fc0
 p.put(va,struct.pack('<%dI'%n,*methods))
 p.names.update({0x2c01a32c:'VSoundEmitter001',0x2c01963c:''})
 return p
def old_vgui():
 p=FakePE(0x10000000)
 for va,raw in a.X86_SITES['old_vgui'].items():p.put(va,bytes.fromhex(raw))
 for va,value in [(0x10051f08,0x100064e0),(0x10051f0c,0x10049404),
                  (0x100523b8,0x1001bdb0),(0x100523bc,0x1004ba90)]:
  p.word(va,value)
 p.names.update({0x10049404:'VGUI_InputInternal001',0x1004ba90:'VGUI_System010'})
 for key,slot,target in [('old_input',56,0x10007d90),('old_system',24,0x1001cb60)]:
  va,n,sha=a.VTABLES[key]
  methods=[0x10006490]*n;methods[slot]=target
  p.put(va,struct.pack('<%dI'%n,*methods))
 return p
class FakeELF:
 def __init__(self,group):
  self.memory={};self.group=group
  for va,opcode in a.ARM_SITES[group].items():self.put(va,struct.pack('<I',opcode))
 def put(self,va,raw):
  for i,b in enumerate(raw):
   if va+i in self.memory and self.memory[va+i]!=b:
    raise AssertionError('ELF fixture overlap at %#x'%(va+i))
   self.memory[va+i]=b
 def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def corrupt(self,va):self.memory[va]^=0x40
def synthetic_native(strict=False):
 from audit_sourcevr_native_sin_factories import ELF64
 with patch('audit_sourcevr_native_sin_factories.ELF64',
            side_effect=[FakeELF('native_sound'),FakeELF('native_vgui')]):
  return a.native(b'synthetic sound',b'synthetic vgui',strict)
class Phase98Tests(unittest.TestCase):
 def test_both_original_factories_and_vtables(self):
  self.assertEqual(a.old_sound(old_sound(),False)['vtable']['count'],36)
  v=a.old_vgui(old_vgui(),False)['vtables']
  self.assertEqual((v['input']['count'],v['system']['count']),(71,33))
 def test_all_original_x86_opcode_mutations(self):
  for group,fixture,method in [('old_sound',old_sound,a.old_sound),
                               ('old_vgui',old_vgui,a.old_vgui)]:
   for va in a.X86_SITES[group]:
    with self.subTest(group=group,site=hex(va)):
     p=fixture();p.corrupt(va)
     with self.assertRaises(a.VerificationError):method(p,False)
 def test_all_three_old_vtable_hashes_enforced(self):
  with self.assertRaisesRegex(a.VerificationError,'vtable SHA'):
   a.old_sound(old_sound(),True)
  with self.assertRaisesRegex(a.VerificationError,'vtable SHA'):
   a.old_vgui(old_vgui(),True)
 def test_all_four_original_critical_slot_mutations(self):
  for factory,table,index,method in [
      (old_sound,a.VTABLES['old_sound'],3,a.old_sound),
      (old_sound,a.VTABLES['old_sound'],6,a.old_sound),
      (old_vgui,a.VTABLES['old_input'],56,a.old_vgui),
      (old_vgui,a.VTABLES['old_system'],24,a.old_vgui)]:
   p=factory();p.corrupt(table[0]+4*index)
   with self.assertRaises(a.VerificationError):method(p,False)
 def test_old_sound_exact_factory_name(self):
  p=old_sound();p.names[0x2c01a32c]='VSoundEmitter002'
  with self.assertRaisesRegex(a.VerificationError,'version'):a.old_sound(p,False)
 def test_old_vgui_node_tamper(self):
  p=old_vgui();p.corrupt(0x100523b8)
  with self.assertRaisesRegex(a.VerificationError,'registration'):a.old_vgui(p,False)
 def test_original_region_sha_mutations_detected(self):
  for group,factory in [('old_sound',old_sound),('old_vgui',old_vgui)]:
   with self.assertRaisesRegex(a.VerificationError,'original-region SHA'):
    a.verify_group(factory(),group,True)
 def test_native_synthetic_matrix(self):
  r=synthetic_native(False)
  self.assertEqual([(x['original_index'],x['native_candidate_index'])
    for x in r['sound_candidates']],[(3,7),(6,10)])
  self.assertEqual([x['index'] for x in r['vgui_proven_mismatches']],[56,24])
 def test_all_25_native_opcode_mutations(self):
  self.assertEqual(sum(map(len,a.ARM_SITES.values())),25)
  for group in a.ARM_SITES:
   for va in a.ARM_SITES[group]:
    with self.subTest(group=group,site=hex(va)):
     sound,vgui=FakeELF('native_sound'),FakeELF('native_vgui')
     (sound if group=='native_sound' else vgui).corrupt(va)
     with patch('audit_sourcevr_native_sin_factories.ELF64',
                side_effect=[sound,vgui]):
      with self.assertRaisesRegex(a.VerificationError,'ARM64 opcode'):
       a.native(b'synthetic sound',b'synthetic vgui',False)
 def test_all_nine_entire_region_hashes_enforced(self):
  self.assertEqual(sum(map(len,a.REGIONS.values())),9)
  for group in ('native_sound','native_vgui'):
   f=FakeELF(group)
   with self.assertRaisesRegex(a.VerificationError,'original-region SHA'):
    a.verify_group(f,group,True)
 def test_full_original_SiN_SHA_chain_required(self):
  with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
   a.prior_audit(b'fake client',b'fake server',b'fake sound',b'fake vgui')
 def test_no_adapter_calls_enabled(self):
  r=synthetic_native(False)
  self.assertTrue(all(not x['ABI_verified'] for x in r['sound_candidates']))
  self.assertTrue(all(not x['can_forward'] for x in r['vgui_proven_mismatches']))
if __name__=='__main__':unittest.main()
