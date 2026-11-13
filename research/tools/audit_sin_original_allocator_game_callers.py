#!/usr/bin/env python3
"""Phase209: SHA-gated original SiN 2006 server/client allocator CALLERS.

Owner-original DLLs are read privately. Only hashes, short opcode witnesses,
entry VAs, import bindings and direct-call edges are embedded here. No game
DLL bytes, assets or full native code bodies are distributed.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage,VerificationError,parse_imports
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA,BASE,IAT,imports_with_iat

SERVER_VA=0x22358870
SERVER_LEN=52
SERVER_SHA="b44c5112406cb01608fcd932d088a9106822257a12c2dcb67cab11968275cd8d"
RANDOM_IAT=0x223d21ac
CLIENT_VA=0x240c8720
CLIENT_LEN=156
CLIENT_SHA="5af411946aa3f82304cf82bfcd2bae6b509549b6cd9a5b2347d995396da75787"
RDATA_REFERENCES=(0x2248cacc,0x2248edf4,0x22495a5c,0x22499c04,
                  0x2249b05c,0x224a4724,0x224a5004)
WITNESSES={
 "server":{
  0x00:"56576a03",0x04:"8bf9",0x0b:"8bf0",0x0d:"6a036a00",
  0x11:"c6065f",0x14:"ff15ac213d22",0x1a:"0441",0x1c:"83c40c",
  0x1f:"884601",0x22:"c6460200",0x26:"8b07568bcf",
  0x2b:"ff90b0050000",0x31:"5f5ec3"
 },
 "client":{
  0x00:"538bd9",0x03:"83bb5404000000",0x0a:"0f858a000000",
  0x10:"8b03ff5040",0x19:"80781100",0x20:"8bcb",
  0x2c:"8bcbff5240",0x31:"8b7014",0x34:"83fe047f05",
  0x39:"be04000000",0x3e:"56",0x48:"c1e902",
  0x4d:"89bb54040000",0x53:"33c0f3ab",0x59:"83e103",
  0x5c:"83c404",0x5f:"f3aa",0x61:"81c3ec020000",
  0x67:"bd5a000000",0x70:"56",0x7a:"c1e902",
  0x7f:"893b",0x81:"33c0f3ab",0x8a:"83c404",
  0x8d:"83c304",0x90:"83ed01",0x93:"f3aa75d9",
  0x97:"5f5e5d5bc3"
 }
}
CALL_EDGES={
 "server_alloc":("server",0x22358876,0x2226e820),
 "client_incoming":("client",0x240caf5a,CLIENT_VA),
 "client_preinit":("client",0x240c8742,0x240c57b0),
 "client_first_alloc":("client",0x240c875f,0x2416fd80),
 "client_loop_alloc":("client",0x240c8791,0x2416fd80),
}

def rel_call(site:int,dest:int)->bytes:
 return b"\xe8"+struct.pack("<i",dest-site-5)

def named_iat(pe:PEImage,dll:str,symbol:str)->list[int]:
 hdr=struct.unpack_from("<I",pe.data,0x3c)[0]
 directory=struct.unpack_from("<I",pe.data,hdr+24+104)[0]
 found=[]
 for i,library in enumerate(parse_imports(pe)):
  _,_,_,_,iat_rva=struct.unpack("<IIIII",pe.at(pe.base+directory+20*i,20))
  for j,imp in enumerate(library["symbols"]):
   if library["dll"].lower()==dll.lower() and imp.get("name")==symbol:
    found.append(pe.base+iat_rva+4*j)
 return found

def rdata_refs(pe:PEImage)->tuple[int,...]:
 needle=struct.pack("<I",SERVER_VA)
 for name,rva,_,offset,rawsize in pe.sections:
  if name!=".rdata":continue
  section=pe.data[offset:offset+rawsize]
  addresses=[];start=0
  while True:
   position=section.find(needle,start)
   if position<0:return tuple(addresses)
   addresses.append(pe.base+rva+position)
   start=position+1
 raise VerificationError("original server missing raw .rdata section")

def verify_native(server:PEImage,client:PEImage,*,check_hash=True,
                  check_imports=True,check_rdata=True)->dict:
 if server.base!=BASE["server"] or client.base!=BASE["client"]:
  raise VerificationError("Phase209 original PE32 image bases changed")
 for role,pe,addr,length,digest,boundary in (
  ("server",server,SERVER_VA,SERVER_LEN,SERVER_SHA,12),
  ("client",client,CLIENT_VA,CLIENT_LEN,CLIENT_SHA,4)):
  if pe.section_of(addr)!=".text":
   raise VerificationError("Phase209 original function outside .text "+role)
  code=pe.at(addr,length)
  if check_hash and hashlib.sha256(code).hexdigest()!=digest:
   raise VerificationError("Phase209 complete original function hash mismatch "+role)
  for offset,expected in WITNESSES[role].items():
   data=bytes.fromhex(expected)
   if code[offset:offset+len(data)]!=data:
    raise VerificationError("Phase209 sparse original opcode mismatch "+role+" +%#x"%offset)
  if pe.at(addr+length,boundary)!=b"\xcc"*boundary:
   raise VerificationError("Phase209 original RET/INT3 boundary mismatch "+role)
 for name,(role,site,target) in CALL_EDGES.items():
  pe=server if role=="server" else client
  if pe.at(site,5)!=rel_call(site,target):
   raise VerificationError("Phase209 original direct CALL edge mismatch "+name)
 if check_imports:
  for role,pe in (("server",server),("client",client)):
   if imports_with_iat(pe).get("g_pMemAlloc")!=("tier0.dll",IAT[role]["g_pMemAlloc"]):
    raise VerificationError("Phase209 original imported tier0 allocator binding mismatch "+role)
  if named_iat(server,"vstdlib.dll","RandomInt")!=[RANDOM_IAT]:
   raise VerificationError("Phase209 original imported RandomInt slot mismatch")
 if check_rdata and rdata_refs(server)!=RDATA_REFERENCES:
  raise VerificationError("Phase209 original server method pointer cross-references mismatch")
 return {
  "new_sha_verified_original_functions":2 if check_hash else 0,
  "new_sha_verified_original_native_bytes":208 if check_hash else 0,
  "whole_original_function_SHA256":{"server":SERVER_SHA,"client":CLIENT_SHA} if check_hash else None,
  "sparse_original_opcode_witnesses":sum(len(x) for x in WITNESSES.values()),
  "exact_original_direct_call_edges":len(CALL_EDGES),
  "original_server_rdata_references":len(RDATA_REFERENCES) if check_rdata else 0,
  "original_named_imports_checked":3 if check_imports else 0,
  "client_original_buffer_count":91,
  "client_minimum_buffer_bytes":4,
  "limits":[
   "Method pointers do not identify original server classes",
   "Original server +0x5B0 object method may transfer allocated string ownership",
   "Original client lacks null/overflow guards on memory allocation",
   "Host-neutral C++ models do not prove actual Windows game-engine execution"
  ],
 }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
 if hashlib.sha256(server_bytes).hexdigest()!=FULL_SHA["server"] or \
    hashlib.sha256(client_bytes).hexdigest()!=FULL_SHA["client"]:
  raise VerificationError("Phase209 requires BOTH SHA256-matched full original DLLs")
 return verify_native(PEImage(server_bytes),PEImage(client_bytes))

def main()->None:
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument("--server",required=True,type=Path)
 ap.add_argument("--client",required=True,type=Path)
 ap.add_argument("--output",type=Path)
 args=ap.parse_args()
 report=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+"\n"
 if args.output:args.output.write_text(report,encoding="utf-8")
 else:print(report,end="")
if __name__=="__main__":main()
