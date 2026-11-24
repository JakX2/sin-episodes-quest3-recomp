#!/usr/bin/env python3
"""Owner-retail CInfoChallenge factory, datamap and method verification.

The original 2006 PE32 binaries remain local. Public --self-test builds
invented records. Full method hashes do not imply all downstream helpers
have been reconstructed or executed.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_createinterface_phase657 import PE, SERVER, CLIENT

METHODS=(
 ("factory_registration",0x223c3ed0,22,"a35b89c8ef9d2dec073c56d37e966d5534017ac2a182780d59f2c61af3daa4e7",0xcc),
 ("capture_snapshot",0x222f4340,16,"8600786dcf2beff7bdb092a0c633a10e2a8255601849d0eb9b335da74c6ea58f",0xb8),
 ("initial_output",0x222f4390,29,"2cf2e08c177aae0c95460ce2ec004a81ada8b2c10ff9fb353c7be952a7373e61",0xcc),
 ("current_output",0x222f43b0,36,"0e2449c79d4cc79551042890ea55d31b48385ee7a31a2f67474d12fb8a6235f0",0xcc),
 ("difference_output",0x222f43e0,41,"8a8d8e01ae48eec6ddd9fe2fb3f7a90d131134425517abdd8ac2798a0b1d4677",0xcc),
 ("object_constructor",0x222f4610,80,"b78a81d2454910486c77ea51779579db64cc938c720478a7c3a42c47041ae50c",0x56),
 ("spawn_aggregate_helper",0x222d3630,241,"bd7d44df3f7234ac415bec2ffac2f370441b4741c38c9fbef012e3529ac61461",0xcc),
 ("current_aggregate_helper",0x222d3730,214,"f5b36281d4042be57b0446fbefd5e504189932ba4017a25bbef95351c63c3dd5",0xcc),
)
# Only these datamap records are file-backed initialized inputs.
# The next three output records are initialized by native code.
INPUTS=(
 (0x22527d2c,0x224683c4,"InputGetInitialChallenge",0x224683b0,"GetInitialChallenge",0x222f4390),
 (0x22527d60,0x22468394,"InputGetCurrentChallenge",0x22468380,"GetCurrentChallenge",0x222f43b0),
 (0x22527d94,0x22468364,"InputGetChallengeDifference",0x2246834c,"GetChallengeDifference",0x222f43e0),
)
CALLS=(
 (0x223c3ed0,0x2228b110),
 (0x222f4343,0x222d3630),
 (0x222f4393,0x222bc4a0),
 (0x222f43a4,0x2212dc30),
 (0x222f43b4,0x222d3730),
 (0x222f43ca,0x2212dc30),
 (0x222f43e3,0x222d3730),
 (0x222f4400,0x2212dc30),
)
def word(raw,offset=0):
 return struct.unpack_from("<I",raw,offset)[0]

def rel32(code,site):
 if len(code)!=5 or code[0]!=0xe8:raise ValueError("not E8 rel32")
 return (site+5+struct.unpack_from("<i",code,1)[0])&0xffffffff

def check_input(record,internal,external,fn):
 if len(record)!=52 or word(record,4)!=internal or word(record,16)!=0x80001 or word(record,20)!=external or word(record,28)!=fn:
  raise ValueError("wrong original input descriptor")

def audit(server,client):
 if hashlib.sha256(server).hexdigest()!=SERVER or hashlib.sha256(client).hexdigest()!=CLIENT:
  raise ValueError("retail SHA256 mismatch")
 pe=PE(server)
 for name,va,length,sha,next_byte in METHODS:
  code=pe.code(va,length)
  if hashlib.sha256(code).hexdigest()!=sha or pe.code(va+length,1)!=bytes([next_byte]):
   raise ValueError("original method changed: "+name)
 for site,target in CALLS:
  if rel32(pe.code(site,5),site)!=target:raise ValueError("native call mismatch "+hex(site))
 if pe.asciiz(0x224686c8)!="info_challenge" or pe.asciiz(0x22468308)!="CInfoChallenge":
  raise ValueError("original entity name mismatch")
 if pe.asciiz(0x22527cc8)!=".?AV?$CEntityFactory@VCInfoChallenge@@@@":
  raise ValueError("original factory RTTI mismatch")
 if word(pe.read(0x22527cbc,4))!=0x2246831c:
  raise ValueError("original factory vtable mismatch")
 if word(pe.read(0x2246843c+0x64,4))!=0x222f4340:
  raise ValueError("original snapshot virtual slot mismatch")
 for rec,iv,internal,ev,external,fn in INPUTS:
  if pe.asciiz(iv)!=internal or pe.asciiz(ev)!=external:
   raise ValueError("original input names changed")
  check_input(pe.read(rec,52),iv,ev,fn)
 if pe.code(0x222f4348,6)!=b"\xd9\x9e\xdc\x03\x00\x00":
  raise ValueError("snapshot float offset changed")
 if pe.code(0x222f4398,3)!=b"\x8b\x40\x08":
  raise ValueError("initial manager +8 read changed")
 for va,offset in [(0x222f439e,0x394),(0x222f43c4,0x3ac),(0x222f43f7,0x3c4)]:
  if pe.code(va,6)!=b"\x8d\x8e"+struct.pack("<I",offset):
   raise ValueError("original output receiver offset changed")
 if pe.code(0x222f43e8,6)!=b"\xd8\xa6\xdc\x03\x00\x00":
  raise ValueError("original difference snapshot subtraction changed")
 if pe.code(0x222f43f0,6)!=b"\xd8\x0d\x14\x63\x3d\x22":
  raise ValueError("original difference multiplier operand changed")
 if struct.unpack("<f",pe.read(0x223d6314,4))[0]!=100.0:
  raise ValueError("original scale constant changed")
 if pe.code(0x222d37fe,6)!=b"\xd8\x0d\x14\x63\x3d\x22":
  raise ValueError("current helper scale mismatch")
 if pe.code(0x222f45d1,5)!=b"\x68\xe0\x03\x00\x00":
  raise ValueError("original factory size changed")
 for va,name in [(0x22468324,"OnInitialChallenge"),(0x22468410,"OnCurrentChallenge"),(0x224683e0,"OnChallengeDifference")]:
  if pe.asciiz(va)!=name:raise ValueError("original output name changed")
 return {"methods_authenticated":len(METHODS),"complete_bytes_authenticated":sum(m[2] for m in METHODS),
         "native_call_edges":len(CALLS),"datamap_inputs":len(INPUTS),"entity_size":0x3e0,
         "modeled_entrypoints":4,"aggregate_helpers":"authenticated only, not reimplemented"}

def self_test():
 fixture=bytearray(52)
 internal,external,target=0x10001234,0x10005678,0x10009abc
 for off,val in [(4,internal),(16,0x80001),(20,external),(28,target)]:
  struct.pack_into("<I",fixture,off,val)
 check_input(fixture,internal,external,target)
 for at in (4,16,20,28):
  mutated=bytearray(fixture);mutated[at]^=1
  try:check_input(mutated,internal,external,target)
  except ValueError:pass
  else:raise AssertionError("bad descriptor accepted")
 site=0x10001000
 assert rel32(b"\xe8"+struct.pack("<i",target-site-5),site)==target
 print("Phase665 invented CInfoChallenge datamap/rel32 fixture PASS")

if __name__=="__main__":
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--server",type=Path)
 p.add_argument("--client",type=Path)
 p.add_argument("--self-test",action="store_true")
 a=p.parse_args()
 if a.self_test:self_test()
 elif a.server and a.client:print(json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2))
 else:p.error("provide --self-test or both original DLL paths")
