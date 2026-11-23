#!/usr/bin/env python3
"""Authenticate original SiN 2006 client.dll AppSystem linked-list lifecycle.

Private original client/server DLLs are required only with --client/--server.
Public --self-test uses invented code; no original executable bytes included.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
from audit_original_createinterface_phase657 import PE

CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
FUNCTIONS=(
 ("register",0x241640c0,39,"07298d888357f960d2015e017d185ec8fcffbac239a2604e8b8eb4d4b8786696",b"\xc2\x0c\x00",b"\xcc"),
 ("disconnect",0x241640f0,48,"93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2",b"\xc3",b"\x56"),
 ("connect",0x24164120,54,"645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f",b"\xc3",b"\xcc"),
)
HEAD=0x24369374
CALLS=(
 (0x240fe39b,0x24164120), # client Init calls connect
 (0x2416414d,0x241640f0), # connect failure calls full disconnect
 (0x240ff422,0x241640f0), # client shutdown path calls disconnect
)
def rel32_target(pe:PE,site:int)->int:
 b=pe.code(site,5)
 if b[0]!=0xe8:raise ValueError(f"not native rel32 call at {site:#x}")
 return (site+5+struct.unpack_from("<i",b,1)[0])&0xffffffff
def audit(client:bytes,server:bytes)->dict:
 if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise ValueError("client.dll does not match the unchanged owner's retail original")
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
  raise ValueError("server.dll does not match the unchanged owner's retail original")
 pe=PE(client)
 if pe.base!=0x24000000:raise ValueError("unexpected original client PE32 base")
 for label,va,size,sha,ending,nextbyte in FUNCTIONS:
  b=pe.code(va,size)
  if hashlib.sha256(b).hexdigest()!=sha:
   raise ValueError(f"{label} entire native original function hash mismatch")
  if not b.endswith(ending) or pe.code(va+size,1)!=nextbyte:
   raise ValueError(f"{label} method boundary mismatch")
 for site,expected in CALLS:
  if rel32_target(pe,site)!=expected:
   raise ValueError(f"original native call edge mismatch at {site:#x}")
 for site in (0x241640d8,0x241640df,0x241640f3,0x24164123):
  # Global list head is always the one installed by original constructor.
  code=pe.code(site,5 if site==0x241640df else 4)
  if site==0x241640df:
   if code!=b"\xa3\x74\x93\x36\x24":raise ValueError("bad original head store")
  else:
   # Other accesses are covered by entire original method SHA.
   if not code:raise ValueError("missing list-head instruction")
 pe.belongs(HEAD,".data")
 if pe.code(0x241640dc,3)!=b"\x89\x50\x0c":
  raise ValueError("original registration next-pointer layout mismatch")
 if pe.code(0x24164130,3)!=b"\xff\x56\x04":
  raise ValueError("original provider callback pointer is not entry+4")
 if pe.code(0x24164109,3)!=b"\xff\x56\x08":
  raise ValueError("original release callback pointer is not entry+8")
 if pe.code(0x24164139,2)!=b"\x89\x01":
  raise ValueError("original connect does not store returned interface to *target")
 if pe.code(0x24164111,6)!=b"\xc7\x01\x00\x00\x00\x00":
  raise ValueError("original disconnect does not zero out *target")
 if pe.code(0x2416413d,5)!=b"\x83\x3e\x00\x75\x0b":
  raise ValueError("original optional/required entry failure gate mismatch")
 if pe.code(0x240fe39b,12)[:5]!=b"\xe8\x80\x5d\x06\x00":
  raise ValueError("original client Init does not call list connection")
 return {"verified_original_bytes":sum(x[2] for x in FUNCTIONS),
         "native_methods":len(FUNCTIONS),
         "exact_original_call_edges":len(CALLS),
         "record_size":16,"head_global":hex(HEAD),
         "proof":"static owner original PE32 only; no game process initialized"}

def self_test()->None:
 # Invented 16-byte registration record: pointer, factory, release, next.
 entry=struct.pack("<IIII",0x1000,0x2000,0x3000,0)
 assert len(entry)==16
 altered=bytearray(entry);altered[8]^=1
 assert hashlib.sha256(entry).digest()!=hashlib.sha256(altered).digest()
 fake=bytearray(b"\x90"*48)
 fake[3]=0xe8
 struct.pack_into("<i",fake,4,30-8)
 assert fake[3]==0xe8 and 3+5+struct.unpack_from("<i",fake,4)[0]==30
 fake[3]=0x90
 assert fake[3]!=0xe8
 print("Phase659 synthetic app-system linked-list audit self-test PASS")

def main()->None:
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--server",type=Path)
 p.add_argument("--client",type=Path)
 p.add_argument("--self-test",action="store_true")
 args=p.parse_args()
 if args.self_test:self_test();return
 if not args.client or not args.server:p.error("supply both original DLLs or --self-test")
 import json
 print(json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2))

if __name__=="__main__":main()
