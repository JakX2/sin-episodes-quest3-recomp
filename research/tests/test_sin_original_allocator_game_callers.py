#!/usr/bin/env python3
"""Phase209 mutation tests: synthetic PE bytes only, never original SiN code."""
from __future__ import annotations
import struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_allocator_game_callers as original
from audit_sin_original_allocator_game_callers import (
 SERVER_VA,SERVER_LEN,CLIENT_VA,CLIENT_LEN,SERVER_WITNESSES,CLIENT_WITNESSES,
 WITNESSES,CALL_EDGES,RDATA_REFERENCES,
 rel_call,verify_native,audit,rdata_refs,VerificationError
)

class SyntheticPE:
 def __init__(self,role:str):
  self.role=role;self.base=0x22000000 if role=="server" else 0x24000000
  self.memory={};self.data=b"";self.sections=[]
  va=SERVER_VA if role=="server" else CLIENT_VA
  length=SERVER_LEN if role=="server" else CLIENT_LEN
  for off,hexcode in WITNESSES[role].items():
   self.put(va+off,bytes.fromhex(hexcode))
  self.put(va+length,b"\xcc"*(12 if role=="server" else 4))
  for name,(side,site,dest) in CALL_EDGES.items():
   if side==role:self.put(site,rel_call(site,dest))
 def put(self,va,code):
  for i,b in enumerate(code):
   if va+i in self.memory and self.memory[va+i]!=b:
    raise AssertionError("conflicting synthetic fixture opcode")
   self.memory[va+i]=b
 def at(self,va,n):
  return bytes(self.memory.get(va+i,0) for i in range(n))
 def section_of(self,va):
  return ".text"
 def flip(self,va):
  self.memory[va]=self.memory.get(va,0)^1
 def put_references(self):
  # Fake rdata range: all expected references plus tiny padding.
  first=RDATA_REFERENCES[0]-0x10
  size=RDATA_REFERENCES[-1]-first+0x10
  rdata=bytearray(size)
  for va in RDATA_REFERENCES:
   struct.pack_into("<I",rdata,va-first,SERVER_VA)
  self.sections=[(".rdata",first-self.base,size,0,size)]
  self.data=bytes(rdata)

class Tests(unittest.TestCase):
 @staticmethod
 def sample():return SyntheticPE("server"),SyntheticPE("client")
 @staticmethod
 def verify(s,c,*,refs=False,imports=False):
  return verify_native(s,c,check_hash=False,
                       check_imports=imports,check_rdata=refs)
 def test_synthetic_fixtures_never_count_as_native_originals(self):
  s,c=self.sample();report=self.verify(s,c)
  self.assertEqual(report["new_sha_verified_original_functions"],0)
  self.assertEqual(report["new_sha_verified_original_native_bytes"],0)
  self.assertEqual(report["sparse_original_opcode_witnesses"],41)
  self.assertEqual(report["exact_original_direct_call_edges"],5)
  self.assertEqual(report["original_server_rdata_references"],0)
 def test_mutate_every_server_and_client_sparse_opcode_witness(self):
  for role,va in (("server",SERVER_VA),("client",CLIENT_VA)):
   for offset in WITNESSES[role]:
    with self.subTest(role=role,offset=offset):
     s,c=self.sample()
     (s if role=="server" else c).flip(va+offset)
     with self.assertRaises(VerificationError):self.verify(s,c)
 def test_mutate_every_original_direct_call_destination(self):
  for name,(role,site,dest) in CALL_EDGES.items():
   with self.subTest(name=name):
    s,c=self.sample()
    (s if role=="server" else c).flip(site+1)
    with self.assertRaises(VerificationError):self.verify(s,c)
 def test_mutate_both_method_boundaries(self):
  for role,va,size in (("server",SERVER_VA,SERVER_LEN),
                       ("client",CLIENT_VA,CLIENT_LEN)):
   with self.subTest(role=role):
    s,c=self.sample()
    (s if role=="server" else c).flip(va+size)
    with self.assertRaises(VerificationError):self.verify(s,c)
 def test_wrong_image_base_and_code_section(self):
  s,c=self.sample();s.base=0x24000000
  with self.assertRaises(VerificationError):self.verify(s,c)
  s,c=self.sample();c.section_of=lambda _:".data"
  with self.assertRaises(VerificationError):self.verify(s,c)
 def test_real_method_sha_gate_rejects_synthetic_code(self):
  s,c=self.sample()
  with self.assertRaises(VerificationError):
   verify_native(s,c,check_hash=True,check_imports=False,check_rdata=False)
  with self.assertRaises(VerificationError):audit(b"MZ",b"MZ")
 def test_original_server_rdata_references_exact_count_and_positions(self):
  s,c=self.sample();s.put_references()
  self.assertEqual(rdata_refs(s),RDATA_REFERENCES)
  self.assertEqual(self.verify(s,c,refs=True)["original_server_rdata_references"],7)
  mutated=bytearray(s.data)
  first=RDATA_REFERENCES[0]-0x10
  mutated[RDATA_REFERENCES[3]-first]^=1
  s.data=bytes(mutated)
  with self.assertRaises(VerificationError):self.verify(s,c,refs=True)
 def test_unlisted_extra_rdata_reference_rejected(self):
  s,c=self.sample();s.put_references()
  mutated=bytearray(s.data)
  struct.pack_into("<I",mutated,0,SERVER_VA)
  s.data=bytes(mutated)
  with self.assertRaises(VerificationError):self.verify(s,c,refs=True)
 def test_randomint_import_must_have_exact_original_name_and_slot(self):
  s,c=self.sample()
  def native_iat(pe):
   va=0x223d2124 if pe.role=="server" else 0x242761cc
   return {"g_pMemAlloc":("tier0.dll",va)}
  with patch.object(original,"imports_with_iat",side_effect=native_iat):
   with patch.object(original,"named_iat",return_value=[0x223d21ac]):
    self.assertEqual(self.verify(s,c,imports=True)["original_named_imports_checked"],3)
   with patch.object(original,"named_iat",return_value=[0x223d21b0]):
    with self.assertRaises(VerificationError):self.verify(s,c,imports=True)
 def test_wrong_original_allocator_import_library_fails(self):
  s,c=self.sample()
  with patch.object(original,"imports_with_iat",
                    return_value={"g_pMemAlloc":("fake.dll",0x223d2124)}):
   with self.assertRaises(VerificationError):self.verify(s,c,imports=True)
 def test_empty_or_truncated_rdata_references_fail(self):
  s,c=self.sample()
  with self.assertRaises(VerificationError):self.verify(s,c,refs=True)
  s.put_references()
  s.sections=[(".data",0,0,0,0)]
  with self.assertRaises(VerificationError):self.verify(s,c,refs=True)

if __name__=="__main__":
 unittest.main()
