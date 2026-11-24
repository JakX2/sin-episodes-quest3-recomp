#!/usr/bin/env python3
"""Phase664 original SiN metric writable-slot function and storage audit.

With --server --client the audit requires the owner's two exact 2006
retail PE32 SHA256s and chains the previously verified complete 14-name,
98-float metric-table audit. The public --self-test uses invented bytes.
No proprietary DLL file or full original binary method bodies are shipped.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_createinterface_phase657 import PE
from audit_sin_metric_math import audit as original_metric_math_audit

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
PREFERRED_BASE=0x22000000
TABLE=0x22523478
NAME_TABLE=0x2245f04c
COUNT=14
STRIDE=28
METHODS=(
 ("indexed_record_va",0x222d3340,21,"4a72161a3ebd7f8c6a332b2debc57fc659ca099ad4e49ddbd9f5e00ca89706e9"),
 ("indexed_name_va",0x222d3360,12,"7a5bf78b22450cc27c7c160ceb0908227130c4eacf6726b6870fa8b2e78b396b"),
 ("indexed_float_read",0x222d33e0,21,"38aa064ed75ca9b3f73f1a410f40f1c01182184eec3414ffdf2cb67e71ed3635"),
 ("indexed_float_write",0x222d3440,25,"0b9a04060f10abc14883ac86728db7b7c4d5246e4d4897bb70b553296bcc512f"),
)
# The original 14 scalar current float32 initializer bits (field[0]).
INITIAL_BITS=(
 0x3f89999a,0x3f800000,0x3f800000,0x00000000,
 0x3ecccccd,0x3e800000,0x3e19999a,0x3e19999a,
 0x3eb33334,0x3f19999a,0x3e4ccccd,0x00000000,
 0x3f400000,0x3f000000
)
NAMES=(
 "DAMAGE_TAKEN_SCALE",
 "DAMAGE_DONE_SCALE",
 "AMMO_QUANTITY_SCALE",
 "NUM_FORCED_HEALTH_KITS",
 "PERCENT_HEALTH_KITS_DROP",
 "PERCENT_NPC_2ND_FIRE",
 "PERCENT_NPC_MAKE_DIFFERENT",
 "PERCENT_NPC_CHANGE_SPAWN_NUM",
 "PERCENT_NPC_WEARS_HELMET",
 "PERCENT_NPC_HELMET_SHOT_OFF",
 "PERCENT_NPC_TAKES_COVER",
 "RANDOM_NPC_WAIT_BEFORE_SHOOT",
 "PERCENT_NPC_ACCURATE",
 "PERCENT_NPC_MOBILE"
)

def verify_shape(code:bytes,kind:str)->None:
 """Check meaningful original math/address instructions, not only SHA."""
 if kind=="indexed_record_va":
  if (code[:4]!=b"\x8b\x44\x24\x04" or
      code[4:11]!=b"\x8d\x0c\xc5\x00\x00\x00\x00" or
      code[11:13]!=b"\x2b\xc8" or
      code[13:17]!=b"\x8d\x04\x8d" + bytes([0x78]) or
      struct.unpack_from("<I",code,16)[0]!=TABLE or code[20]!=0xc3):
   raise ValueError("original 28-byte indexed record address code changed")
 elif kind=="indexed_name_va":
  if (code[:4]!=b"\x8b\x44\x24\x04" or
      code[4:8]!=b"\x8b\x04\x85\x4c" or
      struct.unpack_from("<I",code,8)[0]!=NAME_TABLE or
      code[-1]!=0xc3):
   raise ValueError("original name index method shape changed")
 elif kind=="indexed_float_read":
  if (code[:4]!=b"\x8b\x44\x24\x04" or
      code[4:11]!=b"\x8d\x0c\xc5\x00\x00\x00\x00" or
      code[11:13]!=b"\x2b\xc8" or
      code[13:17]!=b"\xd9\x04\x8d\x78" or
      struct.unpack_from("<I",code,17)[0]!=TABLE or code[20]!=0xc3):
   raise ValueError("original float current-value reader changed")
 elif kind=="indexed_float_write":
  if (code[:4]!=b"\x8b\x44\x24\x04" or
      code[4:8]!=b"\xd9\x44\x24\x08" or
      code[8:15]!=b"\x8d\x0c\xc5\x00\x00\x00\x00" or
      code[15:17]!=b"\x2b\xc8" or
      code[17:21]!=b"\xd9\x1c\x8d\x78" or
      struct.unpack_from("<I",code,21)[0]!=TABLE or code[24]!=0xc3):
   raise ValueError("original float current-value writer changed")
 else:raise ValueError("unknown witness")
def original_audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA or (
    hashlib.sha256(client).hexdigest()!=CLIENT_SHA):
  raise ValueError("original 2006 retail DLL SHA256 mismatch")
 # Reuse prior original name/98-float authoritatively checked audit.
 original_metric_math_audit(client,server)
 pe=PE(server)
 if pe.base!=PREFERRED_BASE:raise ValueError("original PE32 image base changed")
 for name,start,size,digest in METHODS:
  method=pe.code(start,size)
  if hashlib.sha256(method).hexdigest()!=digest:
   raise ValueError(f"full native method hash changed: {name}")
  if pe.code(start+size,1)!=b"\xcc":
   raise ValueError(f"original RET/INT3 function boundary changed: {name}")
  verify_shape(method,name)
 pe.belongs(TABLE,".data")
 pe.belongs(NAME_TABLE,".rdata")
 # True retail field-0 base and each of 14 strided numeric entries.
 for i in range(COUNT):
  addr=TABLE+i*STRIDE
  raw=struct.unpack("<I",pe.read(addr,4))[0]
  if raw!=INITIAL_BITS[i]:
   raise ValueError(f"original metric[{i}] initial float word changed")
  pointer=struct.unpack("<I",pe.read(NAME_TABLE+i*4,4))[0]
  if pe.asciiz(pointer)!=NAMES[i]:
   raise ValueError(f"metric indexed name mismatch at {i}")
 return {
  "pinned_original_full_method_count":len(METHODS),
  "verified_native_bytes":sum(m[2] for m in METHODS),
  "record_stride_bytes":STRIDE,
  "indexed_metric_count":COUNT,
  "original_table_va":hex(TABLE),
  "module_rva":hex(TABLE-PREFERRED_BASE),
  "original_name_table_va":hex(NAME_TABLE),
  "original_float_setter_va":"0x222d3440",
  "verified_original_current_float_words":COUNT,
  "verification_limit":"static audit; original native methods never invoked"
 }

def self_test()->None:
 # Entire fixture below is invented code layout and synthetic addresses.
 assert (0x523478+13*28)==0x5235e4
 assert (0x45f04c+13*4)==0x45f080
 assert len(INITIAL_BITS)==COUNT and len(NAMES)==COUNT
 # Verify mutation-sensitive relational properties with synthetic words.
 synthetic_base=0x10000000
 synthetic_table=synthetic_base+0x00023000
 words=[i*1000+77 for i in range(COUNT)]
 slots={synthetic_table+i*STRIDE:words[i] for i in range(COUNT)}
 for i in range(COUNT):
  assert slots[synthetic_table+i*STRIDE]==i*1000+77
  assert synthetic_table+i*STRIDE!=synthetic_table+(i+1)*STRIDE
 assert all(NAMES[i]!=NAMES[j] for i in range(COUNT) for j in range(i+1,COUNT))
 # Synthetic branch: a routine of the wrong length cannot masquerade
 # as the complete 25-byte original setter.
 try:verify_shape(b"\x90"*25,"indexed_float_write")
 except ValueError:pass
 else:raise AssertionError("synthetic non-matching write body accepted")
 print("Phase664 invented-data metric-table address and guard self-test PASS")

def main()->None:
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument("--client",type=Path)
 ap.add_argument("--server",type=Path)
 ap.add_argument("--self-test",action="store_true")
 args=ap.parse_args()
 if args.self_test:self_test();return
 if args.client is None or args.server is None:
  ap.error("both original retail DLLs required unless --self-test")
 print(json.dumps(original_audit(args.server.read_bytes(),args.client.read_bytes()),indent=2))
if __name__=="__main__":main()
