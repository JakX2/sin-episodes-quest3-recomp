#!/usr/bin/env python3
"""Owner-only retail audit of two original SiN AutoGameSystem methods.

The original retail DLL files stay on the owner's computer. All public
regressions use invented bytes, never embedded proprietary executable code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_original_createinterface_phase657 import PE,u32

SERVER="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
FUNCTIONS=(
 ("auto_game_system_init_all",0x221f5700,277,
  "1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530"),
 ("auto_game_system_ctor",0x221f5820,70,
  "b3b95c759a1f9654324a0749f2a28593155a75e32270562c8ea8a557e316fe16"),
)
DIRECT_CALLS=(
 (0x221e7927,0x221f5700,"server_Init_calls_original_auto_game_init"),
 (0x221f575c,0x221f51a0,"auto_system_capacity_grow"),
 (0x221f5794,0x223a5d10,"original_overlapping_pointer_move"),
 (0x221f5848,0x221f54f0,"late_registration_original_array_insert"),
)

AUTO_CONSTRUCTOR_CALL_SITES=(
 0x2205f073, 0x2209a2f1, 0x220f9733, 0x22121c40, 0x221622c3, 0x221854e3,
 0x22188a03, 0x22193a53, 0x2219b443, 0x2219d1e3, 0x221ab184, 0x221bbc0f,
 0x221c00e3, 0x221c3413, 0x221f2fa3, 0x221f4c44, 0x22201614, 0x2220c053,
 0x2221ef64, 0x22255fe5, 0x22287f93, 0x22289163, 0x222a6243, 0x222abac3,
 0x2234a3e3, 0x22377fb3, 0x2238fe33, 0x223b72a5, 0x223b9a45, 0x223bb905,
 0x223bc8e5, 0x223bc9a5, 0x223bce25, 0x223bd085, 0x223bd1b5, 0x223bd8f5,
 0x223bda55, 0x223bdd55, 0x223bf7b6, 0x223c02e5, 0x223c09c6, 0x223c1ff5,
 0x223c2025, 0x223c28e5, 0x223c2aa5, 0x223c6325, 0x223c74a5, 0x223c7bd5,
)

HEAD=0x22572cc0
INITIALIZED_FLAG=0x22572cb8
ARRAY=0x22510a58
CAPACITY=0x22510a5c
COUNT=0x22510a64
CACHE=0x22510a68
VTABLE=0x22443e74
DUPLICATE_MESSAGE=0x22443e38
VTABLE_MANIFEST=Path(__file__).resolve().parents[1] / "data" / "original_server_autogamesystem_vtables_phase660.json"
DEFAULT_VIRTUAL_INIT=0x2205f0f0
OVERRIDE_INIT_TARGETS={
 0x2209b170,0x220fbed0,0x22121dc0,0x221ab1e0,
 0x221bb640,0x221bf980,0x221ff7d0,0x22223490,
}
DEFAULT_INIT_SHA="e18aacce29affcdddaa5f641ac1ec12552e0fe9d066e6408941b1792818b8619"

def rel32_target(pe:PE,va:int)->int:
 code=pe.code(va,5)
 if code[0]!=0xe8:raise ValueError(f"not rel32 E8 at {va:#x}")
 return (va+5+struct.unpack_from("<i",code,1)[0])&0xffffffff

def audit(original_server:bytes,original_client:bytes,manifest:dict)->dict:
 if hashlib.sha256(original_server).hexdigest()!=SERVER:
  raise ValueError("mismatched owner original server.dll SHA256")
 if hashlib.sha256(original_client).hexdigest()!=CLIENT:
  raise ValueError("mismatched owner original client.dll SHA256")
 pe=PE(original_server)
 if pe.base!=0x22000000:raise ValueError("wrong original PE32 image base")
 for name,start,length,sha in FUNCTIONS:
  body=pe.code(start,length)
  if hashlib.sha256(body).hexdigest()!=sha:
   raise ValueError(f"whole original {name} method hash mismatch")
  if body[-1:]!=b"\xc3" or pe.code(start+length,1)!=b"\xcc":
   raise ValueError(f"invalid original {name} RET/INT3 boundary")
 for source,destination,label in DIRECT_CALLS:
  if rel32_target(pe,source)!=destination:
   raise ValueError(f"original direct CALL mismatch: {label}")
 for site in AUTO_CONSTRUCTOR_CALL_SITES:
  if rel32_target(pe,site)!=0x221f5820:
   raise ValueError(f"game-specific AutoGameSystem constructor caller mismatch at {site:#x}")
 if pe.code(0x221f5700,5)!=b"\xa1\xc0\x2c\x57\x22":
  raise ValueError("pending-head pointer no longer read at init entry")
 if pe.code(0x221f57d5,11)!=(
    b"\xc7\x05\xc0\x2c\x57\x22\x00\x00\x00\x00\xc6"):
  raise ValueError("pending head zeroing and init-byte store not intact")
 if pe.code(0x221f57df,7)!=b"\xc6\x05\xb8\x2c\x57\x22\x01":
  raise ValueError("initialized flag not set after pending list drained")
 if pe.code(0x221f582b,6)!=b"\xc7\x06\x74\x3e\x44\x22":
  raise ValueError("registered system original vtable not installed")
 if pe.code(0x221f5852,6)!=b"\x8b\x15\xc0\x2c\x57\x22":
  raise ValueError("pending registration reads different list head")
 if pe.code(0x221f5858,9)!=b"\x89\x56\x04\x89\x35\xc0\x2c\x57\x22":
  raise ValueError("pending registration list-node layout mismatch")
 if pe.code(0x221f57fa,2)!=b"\xff\x12":
  raise ValueError("per-system virtual slot zero call missing")
 if pe.code(0x221f57fc,4)!=b"\x84\xc0\x74\x10":
  raise ValueError("per-system AL failure branch mismatch")
 if pe.asciiz(DUPLICATE_MESSAGE)!="AutoGameSystem already added to game system list!!!\n":
  raise ValueError("original duplicate registration diagnostic changed")
 for global_va in (HEAD,INITIALIZED_FLAG,ARRAY,CAPACITY,COUNT,CACHE):
  pe.belongs(global_va,".data")
 pe.belongs(VTABLE,".rdata")
 if len(manifest["entries"])!=27 or manifest["expected_constructors"]!=48:
  raise ValueError("unexpected original constructor/vtable census count")
 if manifest["expected_default_vtables"]!=19 or manifest["expected_nondefault_vtables"]!=8:
  raise ValueError("original default/override vtable census count mismatch")
 if manifest["default_init_va"]!=hex(DEFAULT_VIRTUAL_INIT):
  raise ValueError("original default system-init address changed")
 body=pe.code(DEFAULT_VIRTUAL_INIT,3)
 if hashlib.sha256(body).hexdigest()!=DEFAULT_INIT_SHA or body!=b"\xb0\x01\xc3":
  raise ValueError("original shared default true-return method hash mismatch")
 observed_sites=[];vtable_addrs=set();init_overrides=set();default_count=0
 for item in manifest["entries"]:
  derived=int(item["derived_vtable_va"],16)
  initializer=int(item["slot_zero_init_va"],16)
  if derived in vtable_addrs:raise ValueError("duplicate original derived vtable")
  vtable_addrs.add(derived)
  pe.belongs(derived,".rdata")
  if u32(pe.read(derived,4))!=initializer:
   raise ValueError(f"original derived vtable Init pointer mismatch at {derived:#x}")
  pe.code(initializer,1)
  default=initializer==DEFAULT_VIRTUAL_INIT
  if bool(item["default_inherited"])!=default:
   raise ValueError(f"wrong inherited/default Init classification at {derived:#x}")
  if default:default_count+=1
  else:init_overrides.add(initializer)
  for site_str in item["direct_constructor_call_sites"]:
   site=int(site_str,16)
   if rel32_target(pe,site)!=0x221f5820:
    raise ValueError(f"original derived AutoGameSystem caller mismatch at {site:#x}")
   observed_sites.append(site)
 if len(set(observed_sites))!=48 or set(observed_sites)!=set(AUTO_CONSTRUCTOR_CALL_SITES):
  raise ValueError("complete original 48 constructor-to-vtable callsite census mismatch")
 if default_count!=19 or init_overrides!=OVERRIDE_INIT_TARGETS:
  raise ValueError("original eight non-default virtual initialization targets changed")
 return {
  "complete_original_executable_bytes":sum(f[2] for f in FUNCTIONS),
  "original_method_count":len(FUNCTIONS),
  "direct_call_edges":len(DIRECT_CALLS)+len(AUTO_CONSTRUCTOR_CALL_SITES),
  "original_game_system_constructor_callers":len(AUTO_CONSTRUCTOR_CALL_SITES),
  "initialized_flag":hex(INITIALIZED_FLAG),
  "pending_head":hex(HEAD),
  "system_pointer_array":hex(ARRAY),
  "record_stride":4,
  "unique_derived_vtables":len(vtable_addrs),
  "inherited_trivial_true_init_vtables":default_count,
  "nondefault_virtual_init_methods":len(init_overrides),
  "provenance":"original static PE32; host runtime not executed"
 }

def self_test()->None:
 # Only synthetic method and call mutation witnesses.
 fake=bytearray(b"\x90"*80)
 at=12;target=65
 fake[at]=0xe8
 struct.pack_into("<i",fake,at+1,target-at-5)
 assert at+5+struct.unpack_from("<i",fake,at+1)[0]==target
 orig=hashlib.sha256(fake).digest()
 fake[33]^=1
 assert hashlib.sha256(fake).digest()!=orig
 assert fake[at]==0xe8
 fake[at]=0x90
 assert fake[at]!=0xe8
 try:PE(b"fake original server.dll")
 except ValueError:pass
 else:raise AssertionError("fake original DLL accepted")
 print("Phase660 public invented-byte auto-game-system audit self-test PASS")

def main()->None:
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--server",type=Path)
 parser.add_argument("--client",type=Path)
 parser.add_argument("--self-test",action="store_true")
 parser.add_argument("--manifest",type=Path,default=VTABLE_MANIFEST)
 args=parser.parse_args()
 if args.self_test:self_test();return
 if args.server is None or args.client is None:
  parser.error("provide original server/client DLLs, or --self-test")
 import json
 manifest=json.loads(args.manifest.read_text(encoding="utf-8"))
 print(json.dumps(audit(args.server.read_bytes(),args.client.read_bytes(),manifest),indent=2))

if __name__=="__main__":main()
