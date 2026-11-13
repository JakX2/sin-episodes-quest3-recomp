#!/usr/bin/env python3
"""Phase210 original-free PE32 mutation tests; synthetic bytes only."""
from __future__ import annotations
import sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_prediction_savedata as native
from audit_sin_original_client_prediction_savedata import (
 INIT_VA,INIT_LEN,SAVE_VA,SAVE_LEN,WITNESSES,DIRECT_CALLS,
 ORIGINAL_LITERALS,CLIENT_NAMED_IAT,rel_call,verify_native,
 audit,VerificationError
)

class SyntheticPE:
 def __init__(self):
  self.base=0x24000000
  self.mem={}
  for name,addr,n,padding in (
    ("init",INIT_VA,INIT_LEN,6),("save",SAVE_VA,SAVE_LEN,15)):
   for off,hx in WITNESSES[name].items():
    self.put(addr+off,bytes.fromhex(hx))
   self.put(addr+n,b"\xcc"*padding)
  for site,target in DIRECT_CALLS.values():
   self.put(site,rel_call(site,target))
  for addr,string in ORIGINAL_LITERALS.items():
   self.put(addr,string.encode("ascii")+b"\0")
 def put(self,addr,code):
  for i,b in enumerate(code):
   if addr+i in self.mem and self.mem[addr+i]!=b:
    raise AssertionError("synthetic native fixture bytes conflict at %#x"%(addr+i))
   self.mem[addr+i]=b
 def at(self,addr,n):return bytes(self.mem.get(addr+i,0) for i in range(n))
 def section_of(self,addr):
  if addr==0x242f7f78:return ".data"
  if any(begin<=addr<begin+len(s)+1 for begin,s in ORIGINAL_LITERALS.items()):
   return ".rdata"
  return ".text"
 def flip(self,addr):self.mem[addr]=self.mem.get(addr,0)^1

class Phase210SyntheticTests(unittest.TestCase):
 @staticmethod
 def fake():return SyntheticPE()
 @staticmethod
 def checked(pe,*,imports=False,literals=False):
  return verify_native(pe,check_hash=False,
                       check_imports=imports,check_literals=literals)
 def test_fixture_no_unsupported_claim_of_original_recovery(self):
  r=self.checked(self.fake(),literals=True)
  self.assertEqual(r["new_sha_verified_client_functions"],0)
  self.assertEqual(r["new_original_x86_bytes"],0)
  self.assertEqual(r["exact_direct_native_call_edges"],7)
  self.assertEqual(r["sparse_x86_opcode_witnesses"],39)
  self.assertEqual(r["original_rdata_strings_checked"],5)
  self.assertEqual(r["native_slot_iterations"],90)
 def test_mutate_every_sparse_instruction_witness(self):
  for name,base in (("init",INIT_VA),("save",SAVE_VA)):
   for offset in WITNESSES[name]:
    with self.subTest(name=name,off=offset):
     pe=self.fake();pe.flip(base+offset)
     with self.assertRaises(VerificationError):self.checked(pe)
 def test_mutate_every_direct_original_call_edge(self):
  for name,(site,target) in DIRECT_CALLS.items():
   with self.subTest(name=name):
    pe=self.fake();pe.flip(site+1)
    with self.assertRaises(VerificationError):self.checked(pe)
 def test_exact_native_ret_and_int3_alignment(self):
  for va,size in ((INIT_VA,INIT_LEN),(SAVE_VA,SAVE_LEN)):
   pe=self.fake();pe.flip(va+size)
   with self.assertRaises(VerificationError):self.checked(pe)
 def test_all_original_rdata_string_literals_are_pinned(self):
  for addr in ORIGINAL_LITERALS:
   with self.subTest(addr=hex(addr)):
    pe=self.fake();pe.flip(addr)
    with self.assertRaises(VerificationError):
     self.checked(pe,literals=True)
 def test_native_global_object_must_be_data(self):
  pe=self.fake()
  pe.section_of=lambda addr:".text"
  with self.assertRaises(VerificationError):
   self.checked(pe,literals=True)
 def test_original_named_iat_slot_and_provider_must_match(self):
  pe=self.fake()
  def correct(_pe,lib,name):return [CLIENT_NAMED_IAT[(lib,name)]]
  with patch.object(native,"named_iat",side_effect=correct):
   r=self.checked(pe,imports=True)
   self.assertEqual(r["original_named_imports_checked"],4)
  for item in CLIENT_NAMED_IAT:
   with self.subTest(import_name=item):
    def wrong(_pe,lib,name):
     return [CLIENT_NAMED_IAT[(lib,name)]+4] if (lib,name)==item else [CLIENT_NAMED_IAT[(lib,name)]]
    with patch.object(native,"named_iat",side_effect=wrong):
     with self.assertRaises(VerificationError):self.checked(pe,imports=True)
 def test_wrong_original_binary_image_base_and_segment(self):
  pe=self.fake();pe.base=0x23000000
  with self.assertRaises(VerificationError):self.checked(pe)
  pe=self.fake();pe.section_of=lambda _:".rdata"
  with self.assertRaises(VerificationError):self.checked(pe)
 def test_owner_only_sha_gate_always_rejects_synthetic_evidence(self):
  pe=self.fake()
  with self.assertRaises(VerificationError):
   verify_native(pe,check_hash=True,check_imports=False,check_literals=False)
  with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
 def test_true_native_special_branch_signed_idiv_and_90_loop(self):
  self.assertEqual(bytes.fromhex(WITNESSES["save"][0x2a]),b"\x83\xf9\xff")
  self.assertEqual(bytes.fromhex(WITNESSES["save"][0x47]),b"\xbf\x5a\x00\x00\x00")
  self.assertEqual(bytes.fromhex(WITNESSES["save"][0x4c]),b"\xf7\xff")
  self.assertEqual(bytes.fromhex(WITNESSES["init"][0x52]),b"\x83\xff\x5a")
  self.assertEqual(bytes.fromhex(WITNESSES["save"][0xee]),b"\xc2\x0c\x00")

if __name__=="__main__":
 unittest.main()
