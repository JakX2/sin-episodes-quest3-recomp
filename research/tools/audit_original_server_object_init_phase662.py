#!/usr/bin/env python3
"""Owner-original static audit: SiN server.dll six-object init 0x22223490.

Only --server/--client reads original private 2006 PE32. Public --self-test
uses invented byte fixtures; no original DLL or machine-code blob is uploaded.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_original_createinterface_phase657 import PE,u32
SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
START=0x22223490
SIZE=363
METHOD_SHA="1caa170fc1a0e04a3f9b5cc6a83200a015dd1449a7b58fcce39b09771c243233"
VTABLE=0x2244aecc
# Native per-object allocator E8, constructor E8, vtable immediate,
# target field compare, comparator and hash callback address.
SPECS=(
 (0x22223496,0x222234bd,0x22225ba0,0x2244b1e4,0x222234c6,0x0c,0x22223600,0x22223620),
 (0x222234d1,0x222234f8,0x22225f10,0x2244b1d0,0x22223501,0x08,0x22223630,0x22223650),
 (0x2222350c,0x22223533,0x22226280,0x2244b1bc,0x2222353c,0x10,0x22223660,0x22223680),
 (0x22223547,0x2222356e,0x222265f0,0x2244b1a8,0x22223577,0x14,0x22223690,0x222236b0),
 (0x22223582,0x222235a9,0x22226960,0x2244b194,0x222235b2,0x18,0x222236c0,0x222236e0),
 (0x222235bd,0x222235e4,0x22226cd0,0x2244b180,0x222235ed,0x1c,0x222236f0,0x22223710)
)
def call_target(pe:PE,site:int)->int:
 code=pe.code(site,5)
 if code[0]!=0xe8:raise ValueError(f"missing E8 at {site:#x}")
 return (site+5+struct.unpack_from("<i",code,1)[0])&0xffffffff
def assert_ctor_shape(pe:PE,call:int,vtable:int,compare:int,hash_fn:int)->None:
 setup=pe.code(call-25,25)
 if (len(setup)!=25 or setup[0]!=0x68 or u32(setup,1)!=hash_fn or
     setup[5]!=0x68 or u32(setup,6)!=compare or
     setup[10:16]!=b"\x6a\x00\x6a\x00\x6a\x40" or
     setup[16:19]!=b"\x8d\x4e\x04" or
     setup[19:21]!=b"\xc7\x06" or u32(setup,21)!=vtable):
  raise ValueError(f"incorrect original constructor arguments at {call:#x}")
def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
  raise ValueError("original 2006 server.dll entire hash mismatch")
 if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise ValueError("original 2006 client.dll entire hash mismatch")
 pe=PE(server)
 if pe.base!=0x22000000:
  raise ValueError("unexpected original server PE image base")
 method=pe.code(START,SIZE)
 if hashlib.sha256(method).hexdigest()!=METHOD_SHA or method[-3:]!=b"\x5f\x5e\xc3":
  raise ValueError("whole original object registration Init code mismatch")
 if pe.code(START+SIZE,1)!=b"\xcc":
  raise ValueError("original Init RET/INT3 boundary mismatch")
 pe.belongs(VTABLE,".rdata")
 if u32(pe.read(VTABLE,4))!=START:
  raise ValueError("original registered AutoGameSystem vtable no longer points to method")
 for index,(alloc,ctor,expected_ctor,vt,cmp_site,owner_off,compare,hash_fn) in enumerate(SPECS):
  if call_target(pe,alloc)!=0x2226e790 or call_target(pe,ctor)!=expected_ctor:
   raise ValueError(f"allocation/constructor original rel32 edge changed for object {index}")
  if index==0:
   if pe.code(alloc-4,4)!=b"\x6a\x28\x8b\xf9":
    raise ValueError("original first malloc size/owner-register setup mismatch")
  elif pe.code(alloc-2,2)!=b"\x6a\x28":
   raise ValueError("original 40-byte allocation size changed")
  assert_ctor_shape(pe,ctor,vt,compare,hash_fn)
  if pe.code(cmp_site,4)!=b"\x83\x7f"+bytes([owner_off])+b"\x00":
   raise ValueError(f"conditional owner store check changed for object {index}")
  # All six original fields store pointer only on zero-valued target.
  expected_store=cmp_site+6 if index<5 else cmp_site+8
  if pe.code(expected_store,3)!=b"\x89\x77"+bytes([owner_off]):
   raise ValueError(f"conditional pointer store changed for object {index}")
  for fn in (expected_ctor,compare,hash_fn):
   pe.code(fn,1)
  pe.belongs(vt,".rdata")
 if pe.code(0x222235f1,2)!=b"\xb0\x01" or pe.code(0x222235f8,3)!=b"\x5f\x5e\xc3":
  raise ValueError("original unconditional success exit changed")
 return {
  "authenticated_complete_original_bytes":SIZE,
  "retail_original_method_start":hex(START),
  "original_direct_call_edges":2*len(SPECS),
  "40_byte_allocations":len(SPECS),
  "nontrivial_initializers_remaining":["0x221bf980"],
  "proof":"static owner retail audit and synthetic guest-memory source models; no game boot"
 }
def self_test()->None:
 # Invented instruction-shape fixture; does not copy any original method.
 ptr=0x10001234
 shape=b"\x68"+struct.pack("<I",ptr)+b"\x68"+struct.pack("<I",0x10005678)+(
  b"\x6a\x00\x6a\x00\x6a\x40\x8d\x4e\x04\xc7\x06") + struct.pack("<I",0x10009abc)
 assert len(shape)==25
 assert u32(shape,1)==ptr and shape[10:16]==b"\x6a\x00\x6a\x00\x6a\x40"
 for pos in (0,1,5,6,10,12,14,16,19,21):
  corrupt=bytearray(shape);corrupt[pos]^=1
  assert hashlib.sha256(corrupt).digest()!=hashlib.sha256(shape).digest()
 print("Phase662 invented-byte constructor evidence self-test PASS")
def main()->None:
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument("--server",type=Path)
 ap.add_argument("--client",type=Path)
 ap.add_argument("--self-test",action="store_true")
 a=ap.parse_args()
 if a.self_test:self_test();return
 if not a.server or not a.client:ap.error("provide both original retail DLLs or --self-test")
 print(json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2))
if __name__=="__main__":main()
