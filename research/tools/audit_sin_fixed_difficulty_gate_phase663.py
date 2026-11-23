#!/usr/bin/env python3
"""Read-only audit: original SiN 2006 dd_enabled ConVar and adaptive call gate.

Requires both exact SHA256-matched original retail DLLs with --server --client.
Public --self-test constructs invented-data instruction witnesses.
This proves one explicit native call is gated; it does not prove every
possible difficulty-manager update has the same gate or is removable.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_original_createinterface_phase657 import PE

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
CONVAR_NAME_VA=0x2245cf0c
DEFAULT_LITERAL_VA=0x223dc2c0
CONVAR_OBJECT_VA=0x22592390
CONVAR_PARENT_VA=CONVAR_OBJECT_VA+0x18
CONVAR_CTOR_VA=0x223c2f50
CONVAR_CTOR_SIZE=37
CONVAR_CTOR_SHA="24701d080911d0d676f8243b6b4006b69a5cece74c47ebdc5ffb3f14d38b7963"
CONVAR_CTOR_TARGET=0x2225bd20
ORIGINAL_GATE_VA=0x222bc47f
ORIGINAL_GATE_SIZE=20
ORIGINAL_GATE_SHA="4aa756cc66c476fb23302fad1e2baeb1c6acc6e8946d96bf919cc21842490cf8"
GATED_CALLSITE=0x222bc48e
GATED_CALL_TARGET=0x222bc0e0
ORIGINAL_FALLTHROUGH_VA=0x222bc493

def rel32_target(code:bytes,site:int)->int:
 if len(code)!=5 or code[:1]!=b"\xe8":
  raise ValueError("expected direct native CALL rel32")
 return (site+5+struct.unpack_from("<i",code,1)[0])&0xffffffff

def check_gate(code:bytes,base:int,parent:int,target:int)->None:
 if len(code)!=20:
  raise ValueError("incorrect gate length")
 if (code[:2]!=b"\x8b\x15" or
     struct.unpack_from("<I",code,2)[0]!=parent or
     code[6:15]!=b"\x83\x7a\x2c\x00\x74\x08\x55\x8b\xce" or
     rel32_target(code[15:20],base+15)!=target):
  raise ValueError("original dd_enabled gate / skip branch / manager call changed")

def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
  raise ValueError("not owner's matching original 2006 server.dll")
 if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise ValueError("not owner's matching original 2006 client.dll")
 pe=PE(server)
 if pe.base!=0x22000000:
  raise ValueError("original server image base changed")
 ctor=pe.code(CONVAR_CTOR_VA,CONVAR_CTOR_SIZE)
 if hashlib.sha256(ctor).hexdigest()!=CONVAR_CTOR_SHA:
  raise ValueError("whole original dd_enabled ConVar constructor mismatch")
 if pe.code(CONVAR_CTOR_VA+CONVAR_CTOR_SIZE,1)!=b"\xcc":
  raise ValueError("original dd_enabled constructor boundary not INT3")
 if pe.asciiz(CONVAR_NAME_VA)!="dd_enabled":
  raise ValueError("ConVar registered name differs")
 if pe.asciiz(DEFAULT_LITERAL_VA)!="1":
  raise ValueError("original dd_enabled registered default is not 1")
 # Static constructor pushes flags 0x4000, default string, name;
 # stores this=0x22592390 into ECX, then calls ConVar initializer.
 if (ctor[:5]!=b"\x68\x00\x40\x00\x00" or
     ctor[5:6]!=b"\x68" or
     struct.unpack_from("<I",ctor,6)[0]!=DEFAULT_LITERAL_VA or
     ctor[10:11]!=b"\x68" or
     struct.unpack_from("<I",ctor,11)[0]!=CONVAR_NAME_VA or
     ctor[15:16]!=b"\xb9" or
     struct.unpack_from("<I",ctor,16)[0]!=CONVAR_OBJECT_VA or
     rel32_target(ctor[20:25],CONVAR_CTOR_VA+20)!=CONVAR_CTOR_TARGET):
  raise ValueError("native ConVar 0x4000 flag/name/default/object/call mismatch")
 pe.belongs(CONVAR_OBJECT_VA,".data")
 pe.belongs(CONVAR_PARENT_VA,".data")
 gate=pe.code(ORIGINAL_GATE_VA,ORIGINAL_GATE_SIZE)
 if hashlib.sha256(gate).hexdigest()!=ORIGINAL_GATE_SHA:
  raise ValueError("whole original dd_enabled conditional call block mismatch")
 check_gate(gate,ORIGINAL_GATE_VA,CONVAR_PARENT_VA,GATED_CALL_TARGET)
 if GATED_CALLSITE!=ORIGINAL_GATE_VA+15:
  raise ValueError("guarded native CALL address mismatch")
 # Exact original control flow: if stored int +0x2C equals zero,
 # JE +8 skips PUSH EBP, MOV ECX,ESI, CALL; both paths reconverge
 # at 0x222BC493, then the host manager routine returns.
 if ORIGINAL_GATE_VA+12+8 != ORIGINAL_FALLTHROUGH_VA:
  raise ValueError("native conditional branch target mismatch")
 if pe.code(ORIGINAL_FALLTHROUGH_VA,9)!=b"\x5f\x5e\x5d\x83\xc4\x0c\xc2\x0c\x00":
  raise ValueError("gated update did not converge at original epilogue")
 pe.code(GATED_CALL_TARGET,1)
 return {
  "registered_convar":"dd_enabled",
  "registered_default":"1",
  "registered_flags":0x4000,
  "convar_parent_global":hex(CONVAR_PARENT_VA),
  "enabled_check":"int32 [parent+0x2c] != 0",
  "original_gated_callsite":hex(GATED_CALLSITE),
  "gated_subroutine":hex(GATED_CALL_TARGET),
  "on_zero":"skip this update call, continue same manager epilogue",
  "whole_original_checked_executable_bytes":CONVAR_CTOR_SIZE+ORIGINAL_GATE_SIZE,
  "scope":"one original native gated call verified; does not prove all adaptive paths disabled"
 }

def self_test()->None:
 start=0x10001000
 parent=0x10005000
 target=0x10002000
 synthetic=(b"\x8b\x15"+struct.pack("<I",parent)+
            b"\x83\x7a\x2c\x00\x74\x08\x55\x8b\xce"+
            b"\xe8"+struct.pack("<i",target-(start+20)))
 check_gate(synthetic,start,parent,target)
 for at in (0,2,6,8,10,11,12,13,14,15,16,18,19):
  copy=bytearray(synthetic);copy[at]^=1
  try:check_gate(bytes(copy),start,parent,target)
  except ValueError:pass
  else:raise AssertionError(f"original-free gate mutation at +{at} not rejected")
 try:check_gate(synthetic,start,parent+4,target)
 except ValueError:pass
 else:raise AssertionError("wrong ConVar parent unexpectedly accepted")
 print("Phase663 original-free dd_enabled native gate mutation self-test PASS")

def main()->None:
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--server",type=Path)
 parser.add_argument("--client",type=Path)
 parser.add_argument("--self-test",action="store_true")
 args=parser.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:
  parser.error("provide original retail DLL paths or --self-test")
 print(json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2))
if __name__=="__main__":main()
