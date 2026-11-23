#!/usr/bin/env python3
"""Phase661 read-only authenticated 2006 SiN server system-Init audit.

With --server and --client, only exact owner-retail DLL hashes are accepted.
--self-test uses invented instruction bytes and does not need game binaries.
The reference C++ models below still rely on external engine/helper behavior.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import json
from audit_original_createinterface_phase657 import PE, u32
SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
METHODS=(
 ("nested_primary_slot_then_helper",0x2209ce50,30,
  "842f384f1327ad0aeaf17a656c7567d2a76d01e93613dd8f4bb14d02db4e6e09",
  0x223ec6e4,0xcc),
 ("five_named_entries",0x220fbed0,75,
  "d09c76d88825a2eda533193684cd7bd144a3ff54dfc445dcb0f8ead5f13db969",
  0x223ff514,0xcc),
 ("decal_script",0x22122540,16,
  "89c9fa58f846248977eb590a39b829b37023154d9958a8b9113b6a0c69a8c33a",
  0x22401c0c,0xf6), # valid following *different* function, not INT3
 ("spatial_partition",0x221ff7d0,39,
  "f640285aa3362c82b6fb889ed84b5d7299291900cb6839b1b9946057316912a7",
  0x22446a28,0xcc)
)
NAMES=(
 (0x220fbeda,0x2240016c,"phonemes"),
 (0x220fbee8,0x2240015c,"phonemes_weak"),
 (0x220fbef6,0x2240014c,"phonemes_strong"),
 (0x220fbf04,0x223d8ef8,"random"),
 (0x220fbf12,0x22400140,"randomAlert")
)
DIRECT_CALLS=(
 (0x2209ce65,0x2209cdd0),
 (0x220fbeda,0x220fb9b0),
 (0x220fbee8,0x220fb9b0),
 (0x220fbef6,0x220fb9b0),
 (0x220fbf04,0x220fb9b0),
 (0x220fbf12,0x220fb9b0),
 (0x22122548,0x22122280)
)
def call_target(pe:PE,site:int)->int:
 c=pe.code(site,5)
 if c[0]!=0xe8:raise ValueError(f"not E8 call at {site:#x}")
 return (site+5+struct.unpack_from("<i",c,1)[0])&0xffffffff
def check_name_push(payload:bytes,ptr:int,first:bool)->None:
 if (len(payload)!=9 or payload[0]!=0x68 or u32(payload,1)!=ptr or
     payload[5:7]!=b"\x6a\x00" or
     payload[7:9]!=(b"\x8b\xf1" if first else b"\x8b\xce")):
  raise ValueError("original named-init push-zero/this setup mismatch")
def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
  raise ValueError("server original SHA256 mismatch")
 if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise ValueError("client original SHA256 mismatch")
 pe=PE(server)
 if pe.base!=0x22000000:raise ValueError("unexpected retail PE32 server image base")
 for name,va,size,sha,vtable,next_byte in METHODS:
  code=pe.code(va,size)
  if hashlib.sha256(code).hexdigest()!=sha:
   raise ValueError(f"complete original {name} method SHA256 mismatch")
  if code[-1:]!=b"\xc3" or pe.code(va+size,1)!=bytes([next_byte]):
   raise ValueError(f"original {name} boundary mismatch")
  pe.belongs(vtable,".rdata")
  if u32(pe.read(vtable,4))!=va:raise ValueError(f"original {name} vtable[0] not matched")
 for site,target in DIRECT_CALLS:
  if call_target(pe,site)!=target:
   raise ValueError(f"original rel32 call at {site:#x} changed")
 # 0x2209CE50 works on ECX-0x630, invokes primary vtable[+0x18],
 # passes returned EAX to one helper, then MOV AL,1; RET.
 if pe.code(0x2209ce50,15)!=(
   b"\x8b\x81\xd0\xf9\xff\xff\x56\x8d\xb1\xd0\xf9\xff\xff\x8b\xce"):
  raise ValueError("original multiple-inheritance owner adjustment changed")
 if pe.code(0x2209ce5f,3)!=b"\xff\x50\x18" or (
   pe.code(0x2209ce6a,4)!=b"\xb0\x01\x5e\xc3"):
  raise ValueError("original primary vslot6 dispatch / true-return changed")
 # Original five named helper calls have the exact literal order and
 # zero argument; the first preserves ECX as ESI for later calls.
 for i,(call,literal,name) in enumerate(NAMES):
  pe.belongs(literal,".rdata")
  if pe.asciiz(literal)!=name:raise ValueError(f"original name {name} changed")
  check_name_push(pe.code(call-9,9),literal,i==0)
 if pe.code(0x220fbf17,4)!=b"\xb0\x01\x5e\xc3":
  raise ValueError("five registrations no longer end with true-return")
 # The original script stub has no INT3 separator: following bytes begin
 # an independent function at 0x22122550. Never scan beyond 16 bytes.
 if pe.asciiz(0x22401d70)!="scripts/decals_subrect.txt":
  raise ValueError("original decal script name mismatch")
 if pe.code(0x22122540,8)!=b"\x68\x70\x1d\x40\x22\x83\xc1\xfc":
  raise ValueError("original +4 enclosing-object delta/script pointer mismatch")
 if pe.code(0x2212254d,3)!=b"\xb0\x01\xc3":
  raise ValueError("original decal method return changed")
 # 0x221FF7D0 has two branches; both load interface from original
 # 0x225721B8, dispatch vtable byte offset +0x28, then return true.
 if pe.code(0x221ff7d0,4)!=b"\x85\xc9\x74\x12":
  raise ValueError("spatial partition null-this branch mismatch")
 if pe.code(0x221ff7d4,3)!=b"\x8d\x41\x08":
  raise ValueError("partition nonnull argument is no longer this+8")
 if pe.code(0x221ff7d7,6)!=b"\x8b\x0d\xb8\x21\x57\x22" or (
    pe.code(0x221ff7e6,6)!=b"\x8b\x0d\xb8\x21\x57\x22"):
  raise ValueError("original spatial partition engine global changed")
 pe.belongs(0x225721b8,".data")
 for site in (0x221ff7e0,0x221ff7f1):
  if pe.code(site,3)!=b"\xff\x52\x28":
   raise ValueError(f"partition vtable slot10 changed at {site:#x}")
 for site in (0x221ff7e3,0x221ff7f4):
  if pe.code(site,3)!=b"\xb0\x01\xc3":
   raise ValueError(f"partition true return changed at {site:#x}")
 return {
  "complete_original_methods":len(METHODS),
  "authenticated_original_executable_bytes":sum(m[2] for m in METHODS),
  "verified_original_rel32_calls":len(DIRECT_CALLS),
  "verified_original_named_literals":len(NAMES)+1,
  "verified_vtable_slot_zero_entries":len(METHODS),
  "remaining_substantive_system_init_va":["0x221bf980","0x22223490"],
  "proof_boundary":"retail static audit and derived C++ models, not original engine runtime"
 }
def self_test()->None:
 # Invented bytes. Mutate original-shape expectations, not original content.
 site=0x10001000;target=0x10002000
 invented=b"\xe8"+struct.pack("<i",target-(site+5))
 assert invented[0]==0xe8 and site+5+struct.unpack_from("<i",invented,1)[0]==target
 good=b"\x68"+struct.pack("<I",0x10003000)+b"\x6a\x00\x8b\xce"
 check_name_push(good,0x10003000,False)
 for at in (0,1,5,7):
  bad=bytearray(good);bad[at]^=1
  try:check_name_push(bytes(bad),0x10003000,False)
  except ValueError:pass
  else:raise AssertionError(f"synthetic push witness mutation +{at} accepted")
 try:check_name_push(good,0x10003000,True)
 except ValueError:pass
 else:raise AssertionError("first/later this register mismatch accepted")
 print("Phase661 invented-byte startup helper audit PASS")
def main()->None:
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--server",type=Path);p.add_argument("--client",type=Path)
 p.add_argument("--self-test",action="store_true")
 a=p.parse_args()
 if a.self_test:self_test();return
 if not a.server or not a.client:
  p.error("supply both original DLLs or --self-test")
 print(json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2))
if __name__=="__main__":main()
