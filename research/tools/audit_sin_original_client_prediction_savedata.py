#!/usr/bin/env python3
"""Phase210: original SiN client prediction SaveData call-graph audit.

Source-gated to the owner's 2006 PE32 DLLs, pinned by complete SHA256.
The public auditor stores NO whole proprietary method bodies/disassembly;
only the independently checked full-body digests and short opcode evidence.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage,VerificationError,parse_imports
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA,BASE
from audit_sin_original_allocator_game_callers import named_iat

INIT_VA=0x240caf40
INIT_LEN=90
INIT_SHA="78c3167df98218dd8298d9e7f7c194fac9bc224a0d3c773d65e023e22ff2ed2e"
SAVE_VA=0x240c5960
SAVE_LEN=241
SAVE_SHA="2c11e9007379162694493491f4f794c975019f3abd2c7119b228bcce84d7a58f"
WITNESSES={
 "init":{
  0x00:"568bf1",0x03:"8b06",0x06:"c686e802000001",0x0d:"ff5044",
  0x1f:"51",0x20:"8b8eb4000000",0x2a:"b9787f2f24",0x34:"6a00",
  0x3d:"33ff90",0x40:"6a0057",0x43:"68dc932824",0x4f:"83c701",
  0x52:"83ff5a",0x55:"7ce9",0x57:"5f5ec3"
 },
 "save":{
  0x00:"83ec7c5657",0x09:"683c392d24",0x12:"8b0df4612724",
  0x18:"68748f2824",0x1d:"ff15f8612724",0x2a:"83f9ff",
  0x2f:"8bbe54040000",0x37:"83be5404000000",
  0x46:"99",0x47:"bf5a000000",0x4c:"f7ff",
  0x4e:"8bbc96ec020000",0x62:"685c8f2824",
  0x6e:"ff1574622724",0x82:"68448f2824",0x8e:"ff1574622724",
  0x9e:"6a006a006a01",0xbf:"ff5240",0xc7:"ff5024",
  0xd9:"8b0df4612724",0xe1:"ff15f0612724",
  0xeb:"83c47c",0xee:"c20c00"
 }
}
DIRECT_CALLS={
 "init_virtual_return_setup":(0x240caf53,0x240c8f00),
 "init_91_buffers":(0x240caf5a,0x240c8720),
 "init_global_b4":(0x240caf6f,0x240cac00),
 "init_local_zero":(0x240caf78,0x240c6900),
 "init_90_save_calls":(0x240caf8a,SAVE_VA),
 "save_snapshot_builder":(0x240c5a13,0x241978a0),
 "save_snapshot_serializer":(0x240c5a34,0x24199ba0),
}
CLIENT_NAMED_IAT={
 ("tier0.dll","?EnterScope@CVProfile@@QAEXPBDH0_NH@Z"):0x242761f8,
 ("tier0.dll","?ExitScope@CVProfile@@QAEXXZ"):0x242761f0,
 ("tier0.dll","g_VProfCurrentProfile"):0x242761f4,
 ("vstdlib.dll","Q_snprintf"):0x24276274,
}
ORIGINAL_LITERALS={
 0x242893dc:"InitPredictable",
 0x24288f74:"C_BaseEntity::SaveData",
 0x24288f44:"%s SaveData(slot %02i)",
 0x24288f5c:"%s SaveData(original)",
 0x242d393c:"Unaccounted",
}
def rel_call(site:int,target:int)->bytes:
 return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  check_imports:bool=True,check_literals:bool=True)->dict:
 if client.base!=BASE["client"]:
  raise VerificationError("Phase210 unexpected original client.dll PE32 base")
 for name,addr,size,whole_digest,padding in (
  ("init",INIT_VA,INIT_LEN,INIT_SHA,6),
  ("save",SAVE_VA,SAVE_LEN,SAVE_SHA,15)):
  if client.section_of(addr)!=".text":
   raise VerificationError("Phase210 original native code outside .text "+name)
  code=client.at(addr,size)
  if check_hash and hashlib.sha256(code).hexdigest()!=whole_digest:
   raise VerificationError("Phase210 original full native SHA mismatch "+name)
  for offset,hx in WITNESSES[name].items():
   data=bytes.fromhex(hx)
   if code[offset:offset+len(data)]!=data:
    raise VerificationError("Phase210 native opcode witness mismatch "+name+" %#x"%offset)
  if client.at(addr+size,padding)!=b"\xcc"*padding:
   raise VerificationError("Phase210 original x86 method boundary mismatch "+name)
 for name,(site,target) in DIRECT_CALLS.items():
  if client.at(site,5)!=rel_call(site,target):
   raise VerificationError("Phase210 exact original native CALL changed "+name)
 if check_literals:
  for va,string in ORIGINAL_LITERALS.items():
   if client.section_of(va)!=".rdata" or client.at(va,len(string)+1)!=string.encode()+b"\0":
    raise VerificationError("Phase210 original SaveData string evidence mismatch "+hex(va))
  if client.section_of(0x242f7f78)!=".data":
   raise VerificationError("Phase210 original global SaveData object section changed")
 if check_imports:
  for (library,name),iat in CLIENT_NAMED_IAT.items():
   if named_iat(client,library,name)!=[iat]:
    raise VerificationError("Phase210 original imported named API/IAT mismatch "+library+" "+name)
 return {
  "new_sha_verified_client_functions":2 if check_hash else 0,
  "new_original_x86_bytes":331 if check_hash else 0,
  "full_native_method_sha256":{"init_predictable":INIT_SHA,"save_data":SAVE_SHA} if check_hash else None,
  "sparse_x86_opcode_witnesses":sum(len(v) for v in WITNESSES.values()),
  "exact_direct_native_call_edges":len(DIRECT_CALLS),
  "original_named_imports_checked":len(CLIENT_NAMED_IAT) if check_imports else 0,
  "original_rdata_strings_checked":len(ORIGINAL_LITERALS) if check_literals else 0,
  "native_slot_iterations":90,
  "native_slot_index_special_case":-1,
  "native_slot_modulo":90,
  "original_slot_pointer_base":"object+0x2EC",
  "original_slot_guard_pointer":"object+0x454",
  "limits":[
   "Source class names and externally implemented SaveData serializer remain unproven",
   "Observed SaveData(slot) string and CVProfile symbols do not imply original engine serialization parity",
   "Reference C++ intentionally rejects unsafe negative slot indexes and OOM",
   "No retail Windows executable-hosted game startup demonstrated"
  ]
 }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
 if hashlib.sha256(server_bytes).hexdigest()!=FULL_SHA["server"] or \
    hashlib.sha256(client_bytes).hexdigest()!=FULL_SHA["client"]:
  raise VerificationError("Phase210 requires BOTH full SHA256-matched owner-original 2006 DLLs")
 return verify_native(PEImage(client_bytes))

def main()->None:
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--server",type=Path,required=True)
 p.add_argument("--client",type=Path,required=True)
 p.add_argument("--output",type=Path)
 a=p.parse_args()
 report=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+"\n"
 if a.output:a.output.write_text(report,encoding="utf-8")
 else:print(report,end="")
if __name__=="__main__":main()
