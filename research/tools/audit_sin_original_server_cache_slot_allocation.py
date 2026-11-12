#!/usr/bin/env python3
"""Phase194: owner-original 2006 SiN cache tree slot-allocator verification.

No commercial DLLs, assets, or complete original function bodies distributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGIONS={
 'scene':(0x221A6890,119,'1e267f1ece8a0520553fbdcececa0c3efaca4d652a2d1d8b30c3906dbea29923'),
 'modelsounds':(0x22140900,119,'3733292d698d8ac51d8b814834070a68375630d65d14655b422f5e7d9877395c'),
}
CALLS={
 0x221A6890:{0x1d:0x221A3A10,0x32:0x221A4D90},
 0x22140900:{0x1d:0x2213AAC0,0x32:0x2213E990},
}
INCOMING={0x221ACD22:0x221A6890,0x22146F12:0x22140900}
NORMALIZED_SHA='66d48e1aaa823eeb953c3c70acf9c8095bb601326fe1bad696dd517266d15a51'
# Twenty sparse original x86 instructions per function. No full body.
WITNESSES={
 0x00:'568bf1',0x04:'668b7e14',0x08:'6681ffffff',0x0f:'0fb74616',
 0x13:'394608',0x18:'6a01',0x22:'668b7e16',0x29:'66894e16',
 0x37:'668b5002',0x3b:'66895614',0x3f:'8b4e04',0x42:'0fb7c7',
 0x48:'8d44c108',0x4e:'741b',0x50:'c70000000000',
 0x56:'c7400400000000',0x5d:'c74008feffffff',0x64:'c7400cffffffff',
 0x72:'895618',0x76:'c3',
}
class VerificationError(ValueError):pass

class PE32:
 def __init__(self,b:bytes):
  if len(b)<0x100 or b[:2]!=b'MZ':raise VerificationError('invalid DOS header')
  pe=struct.unpack_from('<I',b,0x3c)[0]
  if pe+24>len(b) or b[pe:pe+4]!=b'PE\x00\x00':raise VerificationError('invalid PE header')
  n=struct.unpack_from('<H',b,pe+6)[0]
  machine=struct.unpack_from('<H',b,pe+4)[0]
  optsz=struct.unpack_from('<H',b,pe+20)[0]
  opt=pe+24
  if machine!=0x14c or not 1<=n<=96 or opt+optsz+n*40>len(b) or struct.unpack_from('<H',b,opt)[0]!=0x10b:
   raise VerificationError('not a bounded i386 PE32 file')
  self.base=struct.unpack_from('<I',b,opt+28)[0];self.b=b;self.sections=[]
  for i in range(n):
   p=opt+optsz+40*i
   name=b[p:p+8].split(b'\x00')[0].decode('ascii','replace')
   vsz,rva,rawlen,raw=struct.unpack_from('<IIII',b,p+8)
   self.sections.append((name,rva,vsz,rawlen,raw))
 def at(self,va:int,n:int)->bytes:
  if n<0:raise VerificationError('negative read length')
  rva=va-self.base
  for name,s,vsz,rawlen,off in self.sections:
   if s<=rva and rva-s+n<=rawlen and off+rva-s+n<=len(self.b):
    return self.b[off+rva-s:off+rva-s+n]
  raise VerificationError('unmapped original RVA %#x'%va)
 def section(self,va:int)->str:
  rva=va-self.base
  for name,s,vsz,rawlen,off in self.sections:
   if s<=rva<s+max(vsz,rawlen):return name
  return ''

def rel32(site:int,destination:int)->bytes:
 return b'\xe8'+struct.pack('<i',destination-site-5)

def verify_native(pe:PE32,*,check_hash:bool=True)->dict:
 if pe.base!=0x22000000:raise VerificationError('incorrect original server image base')
 normalized=[]
 for label,(va,n,digest) in REGIONS.items():
  if pe.section(va)!='.text':raise VerificationError('expected original .text')
  body=bytearray(pe.at(va,n))
  if check_hash and hashlib.sha256(body).hexdigest()!=digest:
   raise VerificationError('original complete x86 function hash mismatch: '+label)
  for off,word in WITNESSES.items():
   literal=bytes.fromhex(word)
   if body[off:off+len(literal)]!=literal:
    raise VerificationError('original x86 opcode witness mismatch at %#x'%(va+off))
  for off,dest in CALLS[va].items():
   if body[off:off+5]!=rel32(va+off,dest):
    raise VerificationError('native outgoing CALL mismatch at %#x'%(va+off))
   body[off+1:off+5]=bytes(4)
  if pe.at(va+n,9)!=b'\xcc'*9:
   raise VerificationError('original function end/INT3 boundary mismatch')
  normalized.append(bytes(body))
 for site,target in INCOMING.items():
  if pe.at(site,5)!=rel32(site,target):
   raise VerificationError('original Phase193 incoming CALL mismatch at %#x'%site)
 if normalized[0]!=normalized[1]:
  raise VerificationError('native allocation methods differ outside rel32 CALLs')
 if check_hash and hashlib.sha256(normalized[0]).hexdigest()!=NORMALIZED_SHA:
  raise VerificationError('complete normalized native function SHA mismatch')
 return {
  'new_original_sha_verified_functions':2 if check_hash else 0,
  'new_original_sha_verified_bytes':238 if check_hash else 0,
  'original_witnesses':2*len(WITNESSES),
  'exact_direct_outgoing_calls':sum(map(len,CALLS.values())),
  'exact_direct_incoming_calls':len(INCOMING),
  'normalized_original_function_sha256':NORMALIZED_SHA if check_hash else None,
  'native_word_free_head_offset':0x14,
  'native_word_next_unused_offset':0x16,
  'native_dword_capacity_offset':0x08,
  'native_dword_base_snapshot_offset':0x18,
  'native_slot_stride':24,
  'fresh_slot_key_initial_values':['0','0','0xfffffffe','0xffffffff'],
  'limitations':['Growth/free-node helpers opaque',
                 'Native unchecked pointer arithmetic replaced by host bounds guard',
                 'No actual Windows executable-hosted replacement game DLL'],
 }

def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise VerificationError('original full-file SHA256 mismatch')
 return verify_native(PE32(server))

def main()->None:
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path,required=True)
 p.add_argument('--client',type=Path,required=True)
 p.add_argument('--output',type=Path)
 a=p.parse_args()
 result=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+'\n'
 if a.output:a.output.write_text(result,encoding='utf8')
 else:print(result,end='')
if __name__=='__main__':main()
