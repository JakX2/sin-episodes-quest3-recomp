#!/usr/bin/env python3
"""Phase220 SHA-gated original SiN client type-5 DWORD comparison/preview audit.

Owner-original full DLL SHA256 is mandatory. Only original method hashes,
short sparse instructions, control-flow targets and strings are shared.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA, BASE
from audit_sin_original_client_snapshot_record_router import CALL_EDGES
from audit_sin_original_allocator_game_callers import named_iat

# (original entry, full bytes, full SHA256, INT3 padding, original caller, RET pop, sparse x86)
METHODS={
 "type5_dword_compare_and_first_mismatch_log":(
  0x24197DB0,125,
  "256748201d4c29fb46076705316ea2345bbeb24a885a978e21d7b6bb9824cdbf",
  3,0x241999BC,12,{
   0x00:"51",0x01:"80791400",0x10:"8b4118",
   0x13:"f6401304",0x1D:"8b742420",
   0x33:"2bdf",0x35:"8b0c13",0x38:"3b0a",
   0x5A:"8bca",0x5C:"2bc8",0x6B:"e8f0faffff",
   0x78:"5b",0x7A:"c20c00"
  }),
 "type5_integer_symbol_aware_preview":(
  0x241983B0,144,
  "88eb84ee0d78cae0b3754d272b8b462e89e662e52f2735a8c547018b48fad874",
  0,0x241999E5,16,{
   0x00:"81ec00020000",0x09:"8b4718",
   0x0C:"394734",0x11:"f6401308",
   0x1F:"8b06",0x21:"8b0dacf63324",
   0x2A:"ff5204",0x31:"6800020000",
   0x46:"8b11",0x48:"50",
   0x49:"ff520c",0x4B:"50",
   0x4C:"ff1554622724",
   0x5B:"6820402a24",
   0x61:"e8cafbffff",
   0x77:"6810402a24",
   0x7D:"e8aefbffff",
   0x8D:"c21000"
  }),
}
NESTED_CALLS=(
 (0x24197E1B,0x24197910),
 (0x24198411,0x24197FE0),
 (0x2419842D,0x24197FE0),
)
FORMATS={
 0x242A3ED0:"int differs (net %i pred %i) diff(%i)\n",
 0x242A4010:"integer (%i)",
 0x242A4020:"integer (%i->%s)",
}
Q_FILEBASE_IAT=0x24276254
SYMBOL_RESOLVER_GLOBAL=0x2433F6AC
NEXT_VA=0x24198440
NEXT_BYTES=bytes.fromhex("8b41343b4118")
PHASE213_COPY=(0x241999CB,0x24198A10)
PHASE217_DIAGNOSTIC=(0x241999D6,0x24197B60)

def rel_call(site:int,target:int)->bytes:
 return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,check_imports:bool=True)->dict:
 if client.base!=BASE["client"]:
  raise VerificationError("Phase220 original client PE32 base mismatch")
 checked={}
 anchors=0
 for name,(va,size,digest,pad,site,pop,witnesses) in METHODS.items():
  if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
   raise VerificationError("Phase220 native helper not entirely .text: "+name)
  body=client.at(va,size)
  if check_hash and hashlib.sha256(body).hexdigest()!=digest:
   raise VerificationError("Phase220 full original x86 SHA256 mismatch: "+name)
  if body[-3:]!=b"\xc2"+struct.pack("<H",pop):
   raise VerificationError("Phase220 native RET cleanup mismatch: "+name)
  if client.at(va+size,pad)!=b"\xcc"*pad:
   raise VerificationError("Phase220 code boundary INT3 mismatch: "+name)
  for offset,hx in witnesses.items():
   sig=bytes.fromhex(hx)
   if body[offset:offset+len(sig)]!=sig:
    raise VerificationError(f"Phase220 sparse original opcode {name}+{offset:#x}")
   anchors+=1
  if (site,va) not in CALL_EDGES or client.at(site,5)!=rel_call(site,va):
   raise VerificationError("Phase220 original Phase212 CALL mismatch: "+name)
  checked[name]={"entry_va":f"0x{va:08X}","complete_original_x86_bytes":size,
    "complete_original_sha256":digest if check_hash else None,
    "router_call":f"0x{site:08X}","ret_stack_bytes":pop}
 for site,target in NESTED_CALLS+(PHASE213_COPY,PHASE217_DIAGNOSTIC):
  if client.section_of(site)!=".text" or client.section_of(target)!=".text" or (
      client.at(site,5)!=rel_call(site,target)):
   raise VerificationError("Phase220 nested or existing type5 CALL drift")
 for va,value in FORMATS.items():
  if client.section_of(va)!=".rdata" or (
      client.at(va,len(value)+1)!=value.encode("ascii")+b"\0"):
   raise VerificationError("Phase220 native formatter literal drift")
 if client.at(NEXT_VA,len(NEXT_BYTES))!=NEXT_BYTES or client.section_of(NEXT_VA)!=".text":
  raise VerificationError("Phase220 adjacent type6 preview method boundary drift")
 if METHODS["type5_integer_symbol_aware_preview"][0]+144!=NEXT_VA:
  raise VerificationError("Phase220 original no-padding adjacency changed")
 if client.section_of(SYMBOL_RESOLVER_GLOBAL)!=".data":
  raise VerificationError("Phase220 resolver global not original .data")
 if check_imports and named_iat(client,"vstdlib.dll","Q_FileBase")!=[Q_FILEBASE_IAT]:
  raise VerificationError("Phase220 original Q_FileBase import/IAT mismatch")
 return {
  "new_sha_verified_original_client_functions":2 if check_hash else 0,
  "new_complete_original_x86_bytes":269 if check_hash else 0,
  "method_evidence":checked,
  "sparse_opcode_witnesses":anchors,
  "exact_original_type5_router_calls":4,
  "exact_original_nested_calls":len(NESTED_CALLS),
  "native_formats":list(FORMATS.values()),
  "original_preview_adjacent_without_int3":True,
  "limits":[
   "Native type5 compare logs the first mismatch before the caller performs the copy.",
   "Q_FileBase symbol resolver object, vtable internals and formatter implementations remain opaque.",
   "Original pointer faults/CPU flags are not emulated by host-only bounds checks.",
   "Windows retail engine has not run reconstructed gameplay DLLs."],
 }

def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or (
     hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]):
  raise VerificationError("Phase220 needs both complete SHA-matched original SiN DLLs")
 return verify_native(PEImage(client))

def main()->None:
 p=argparse.ArgumentParser(description=__doc__)
 group=p.add_mutually_exclusive_group(required=True)
 group.add_argument("--archive",type=Path)
 group.add_argument("--server",type=Path)
 p.add_argument("--client",type=Path)
 a=p.parse_args()
 if a.archive:
  if a.client:p.error("--archive and --client cannot be combined")
  with zipfile.ZipFile(a.archive) as z:
   if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
    raise VerificationError("archive must contain exactly one root game DLL pair")
   data=audit(z.read("server.dll"),z.read("client.dll"))
 else:
  if not a.client:p.error("--server requires --client")
  data=audit(a.server.read_bytes(),a.client.read_bytes())
 print(json.dumps(data,indent=2))
if __name__=="__main__":main()
