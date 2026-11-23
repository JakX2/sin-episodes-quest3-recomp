#!/usr/bin/env python3
"""Owner-only retail audit of two original SiN AutoGameSystem methods.

The original retail DLL files stay on the owner's computer. All public
regressions use invented bytes, never embedded proprietary executable code.
"""
from __future__ import annotations
import argparse
import hashlib
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
HEAD=0x22572cc0
INITIALIZED_FLAG=0x22572cb8
ARRAY=0x22510a58
CAPACITY=0x22510a5c
COUNT=0x22510a64
CACHE=0x22510a68
VTABLE=0x22443e74
DUPLICATE_MESSAGE=0x22443e38

def rel32_target(pe:PE,va:int)->int:
 code=pe.code(va,5)
 if code[0]!=0xe8:raise ValueError(f"not rel32 E8 at {va:#x}")
 return (va+5+struct.unpack_from("<i",code,1)[0])&0xffffffff

def audit(original_server:bytes,original_client:bytes)->dict:
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
 return {
  "complete_original_executable_bytes":sum(f[2] for f in FUNCTIONS),
  "original_method_count":len(FUNCTIONS),
  "direct_call_edges":len(DIRECT_CALLS),
  "initialized_flag":hex(INITIALIZED_FLAG),
  "pending_head":hex(HEAD),
  "system_pointer_array":hex(ARRAY),
  "record_stride":4,
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
 args=parser.parse_args()
 if args.self_test:self_test();return
 if args.server is None or args.client is None:
  parser.error("provide original server/client DLLs, or --self-test")
 import json
 print(json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2))

if __name__=="__main__":main()
