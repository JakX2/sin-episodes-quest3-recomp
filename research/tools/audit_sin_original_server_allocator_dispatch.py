#!/usr/bin/env python3
"""Phase202: strict owner-original 2006 SiN server.dll heap-dispatch audit.

Only SHA256 digests and sparse, noncontiguous native witnesses are embedded.
No original game binaries, asset data or complete machine code redistributed.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_server_cache_array_growth import PE32,VerificationError
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGIONS={
 'context':(0x2226E290,90,'84049bbc203aa4d0f0fa107d0294be44dd7075dcd827a13f7c2fda933f9f0659'),
 'malloc':(0x2226E390,79,'8c8192a3d478efa799525609e38affe36e9548e5ee449e57aadc914237e13d1a'),
 'free':(0x2226E3E0,18,'4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc'),
 'realloc':(0x2226E400,90,'fccf3ba0ea064576d8806a024df924f2281e8829d202ae99c023010f2283d110'),
}
WITNESSES={
 'context':{0x00:'a124213d22',0x0c:'ff525c',0x11:'7441',0x14:'6804010000',0x19:'6a00',0x2a:'6a1c',0x31:'68d8855822',0x40:'6804010000',0x53:'c3',0x54:'33c0',0x59:'c3'},
 'malloc':{0x00:'b801000000',0x0b:'7512',0x1d:'eb05',0x24:'85c0',0x2f:'8b442404',0x36:'ff5204',0x39:'c3',0x44:'6a00',0x4b:'50',0x4c:'ff12',0x4e:'c3'},
 'free':{0x00:'a124213d22',0x07:'8b442404',0x0e:'ff5214',0x11:'c3'},
 'realloc':{0x00:'b801000000',0x0b:'7512',0x1d:'eb05',0x24:'85c0',0x2f:'8b442408',0x35:'50',0x3b:'ff520c',0x3e:'c3',0x49:'6a00',0x50:'50',0x56:'ff5208',0x59:'c3'},
}
# Embedded absolute x86 address operands, including original segment.
SLOTS={
 'context':[(0x01,0x223D2124,'.rdata'),(0x1d,0x223D2024,'.rdata'),(0x24,0x223D2020,'.rdata'),(0x32,0x225885D8,'.data'),(0x38,0x223D201C,'.rdata'),(0x49,0x223D2018,'.rdata')],
 'malloc':[(0x07,0x225885E0,'.data'),(0x0f,0x225885E0,'.data'),(0x19,0x225885DC,'.data'),(0x20,0x225885DC,'.data'),(0x29,0x223D2124,'.rdata'),(0x3c,0x223D2124,'.rdata')],
 'free':[(0x01,0x223D2124,'.rdata')],
 'realloc':[(0x07,0x225885E8,'.data'),(0x0f,0x225885E8,'.data'),(0x19,0x225885E4,'.data'),(0x20,0x225885E4,'.data'),(0x29,0x223D2124,'.rdata'),(0x41,0x223D2124,'.rdata')],
}
DIRECT_CALLS={
 0x2226E3A3:0x2226E290,
 0x2226E413:0x2226E290,
 0x221A3A79:0x2226E390,
 0x2213AB29:0x2226E390,
 0x221A3A61:0x2226E400,
 0x2213AB11:0x2226E400,
}
# Context initializer IAT methods stay opaque; no guessed Windows API names.
BOUNDARIES={'context':6,'malloc':1,'free':14,'realloc':6}
def rel_call(site:int,destination:int)->bytes:
 return b'\xe8'+struct.pack('<i',destination-site-5)
def verify_native(pe:PE32,*,check_hash=True)->dict:
 if pe.base!=0x22000000:raise VerificationError('original 2006 server image base mismatch')
 for name,(va,n,digest) in REGIONS.items():
  if pe.section(va)!='.text':raise VerificationError('original function not .text: '+name)
  code=pe.at(va,n)
  if check_hash and hashlib.sha256(code).hexdigest()!=digest:
   raise VerificationError('original full native function SHA mismatch: '+name)
  for off,hexcode in WITNESSES[name].items():
   expected=bytes.fromhex(hexcode)
   if code[off:off+len(expected)]!=expected:
    raise VerificationError('sparse original opcode witness mismatch: %s +%#x'%(name,off))
  for off,expected,segment in SLOTS[name]:
   if struct.unpack_from('<I',code,off)[0]!=expected or pe.section(expected)!=segment:
    raise VerificationError('native original static or IAT pointer mismatch: %s +%#x'%(name,off))
  if pe.at(va+n,BOUNDARIES[name])!=b'\xcc'*BOUNDARIES[name]:
   raise VerificationError('native function end/INT3 boundary mismatch: '+name)
 for site,target in DIRECT_CALLS.items():
  if pe.at(site,5)!=rel_call(site,target):
   raise VerificationError('exact original direct call edge mismatch: %#x'%site)
 if len({x[1] for name in ('malloc','realloc') for x in SLOTS[name] if x[2]=='.data'})!=4:
  raise VerificationError('wrong number of unique allocator cache statics')
 return {
  'new_sha_verified_original_functions':len(REGIONS) if check_hash else 0,
  'new_native_function_bytes':sum(x[1] for x in REGIONS.values()) if check_hash else 0,
  'original_full_method_sha256':{name:row[2] for name,row in REGIONS.items()} if check_hash else None,
  'sparse_opcode_witnesses':sum(len(x) for x in WITNESSES.values()),
  'exact_absolute_pointers':sum(len(x) for x in SLOTS.values()),
  'exact_direct_calls':len(DIRECT_CALLS),
  'original_allocator_vtable_offsets':{'alloc_basic':4,'alloc_contextual':0,'free':20,'realloc_basic':12,'realloc_contextual':8,'instrumentation_test':92},
  'two_independent_cache_flags':[hex(0x225885E0),hex(0x225885E8)],
  'limits':['IAT helper identities not claimed','Allocator interface implementation and ownership are not recovered','No original Windows retail-engine game session demonstrated'],
 }
def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise VerificationError('whole owner-original server/client DLL SHA mismatch')
 return verify_native(PE32(server))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',required=True,type=Path)
 p.add_argument('--client',required=True,type=Path)
 p.add_argument('--output',type=Path)
 args=p.parse_args()
 data=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+'\n'
 if args.output:args.output.write_text(data,encoding='utf8')
 else:print(data,end='')
if __name__=='__main__':main()
