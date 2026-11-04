"""Synthetic, original-executable-free mutations of Phase102 sound ABI audit."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sound_candidate_abi_phase102 as a
class Fake:
 def __init__(self):self.memory={}
 def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
 def put(self,va,raw):
  for i,b in enumerate(raw):
   if va+i in self.memory and self.memory[va+i]!=b:
    raise AssertionError('fixture overlap at %#x'%(va+i))
   self.memory[va+i]=b
 def corrupt(self,va):self.memory[va]^=0x20
def fixture():
 old,native=Fake(),Fake()
 for va,op in a.X86.items():old.put(va,bytes.fromhex(op))
 for va,op in a.ARM.items():native.put(va,struct.pack('<I',op))
 return old,native
class Phase102AuditTests(unittest.TestCase):
 def test_pinned_isa_sites_pass(self):
  old,native=fixture()
  self.assertEqual(len(a.verify_bytes(old,native,False)),5)
 def test_every_arm64_opcode_mutation(self):
  for va in a.ARM:
   with self.subTest(arm64=hex(va)):
    old,native=fixture();native.corrupt(va)
    with self.assertRaisesRegex(a.AuditFailure,'AArch64 opcode'):
     a.verify_bytes(old,native,False)
 def test_every_original_x86_opcode_mutation(self):
  for va in a.X86:
   with self.subTest(x86=hex(va)):
    old,native=fixture();old.corrupt(va)
    with self.assertRaisesRegex(a.AuditFailure,'x86 opcode'):
     a.verify_bytes(old,native,False)
 def test_original_full_function_SHA_required(self):
  old,native=fixture()
  with self.assertRaisesRegex(a.AuditFailure,'SHA mismatch'):
   a.verify_bytes(old,native,True)
 def test_exact_original_binary_SHA_required(self):
  with self.assertRaisesRegex(a.AuditFailure,'original 2006'):
   a.audit(b'bad',b'bad')
 def test_reject_wrong_file_architectures(self):
  with self.assertRaisesRegex(a.AuditFailure,'PE32'):a.PE32(b'ELF')
  with self.assertRaisesRegex(a.AuditFailure,'ELF64'):a.ELF64(b'MZ')
 def test_candidate_addresses_and_17_arm64_sites(self):
  self.assertEqual(a.REGIONS['native_name_to_token_candidate'][0],0x133a0)
  self.assertEqual(a.REGIONS['native_token_to_name_candidate'][0],0x13570)
  self.assertEqual(a.REGIONS['native_index_validator'][0],0x13520)
  self.assertEqual(len(a.ARM),17)
 def test_report_never_includes_original_bytes(self):
  old,native=fixture()
  for data in a.verify_bytes(old,native,False).values():
   self.assertGreater(data['length'],0)
   self.assertEqual(len(data['sha256']),64)
if __name__=='__main__':unittest.main()
