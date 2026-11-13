#!/usr/bin/env python3
"""Phase195: SHA-gated original SiN 2006 server.dll cache node resolver pair.

Only digest and sparse witness metadata included, never original commercial
DLLs, game assets or complete original instruction bodies.
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
 'scene':(0x221A4D90,84,'84be32439e28d397699f9afaadf46ac70d857f88bd26ab6d19bd8eb2d7b7c855',0x2256E1F0),
 'modelsounds':(0x2213E990,84,'fd045383426de31ea6e97afd788d3959bcf057a2290be6cff4b870e5a2a82a64',0x22558D50),
}
# Seven absolute address operands, compared to each function's original
# sentinel base (four WORDs) and the separate initialization DWORD at +8.
ADDRESS_OFFSETS={7:8,20:8,27:0,34:2,41:4,47:6,77:0}
# Intentionally sparse; do NOT reconstruct the complete original x86 body.
WITNESSES={
 0:'b801000000',5:'8405',11:'baffff0000',16:'7521',18:'0905',
 24:'668915',31:'668915',38:'668915',45:'66a3',
 51:'668b442404',56:'663bc2',59:'740f',61:'8b4904',
 73:'c20400',76:'b8',81:'c20400',
}
INCOMING_CALLS={0x221A68C2:0x221A4D90,0x22140932:0x2213E990}
NORMALIZED_SHA='2eab8413dc78ddc215c2c60be186262755adfa638f84e5de6c73be2725a69a1c'
class VerificationError(ValueError):pass

class PE32:
 def __init__(self,b:bytes):
  if len(b)<0x100 or b[:2]!=b'MZ':raise VerificationError('invalid original DOS header')
  pe=struct.unpack_from('<I',b,0x3c)[0]
  if pe+24>len(b) or b[pe:pe+4]!=b'PE\x00\x00':raise VerificationError('invalid original PE signature')
  machine,n=struct.unpack_from('<HH',b,pe+4)
  optsize=struct.unpack_from('<H',b,pe+20)[0]
  opt=pe+24
  if machine!=0x14c or not 1<=n<=96 or opt+optsize+40*n>len(b) or struct.unpack_from('<H',b,opt)[0]!=0x10b:
   raise VerificationError('not bounded original i386 PE32')
  self.base=struct.unpack_from('<I',b,opt+28)[0]
  self.b=b;self.sections=[]
  for i in range(n):
   p=opt+optsize+i*40
   name=b[p:p+8].split(b'\x00')[0].decode('ascii','replace')
   virtual,rva,rawlen,raw=struct.unpack_from('<IIII',b,p+8)
   self.sections.append((name,rva,virtual,rawlen,raw))
 def at(self,va:int,n:int)->bytes:
  if n<0:raise VerificationError('negative length')
  rv=va-self.base
  for name,rva,virtual,rawlen,raw in self.sections:
   if rva<=rv and rv-rva+n<=rawlen and raw+rv-rva+n<=len(self.b):
    return self.b[raw+rv-rva:raw+rv-rva+n]
  raise VerificationError('native VA not original file-backed: %#x'%va)
 def section(self,va:int)->str:
  rv=va-self.base
  for name,rva,virtual,rawlen,_ in self.sections:
   if rva<=rv<rva+max(virtual,rawlen):return name
  return ''

def rel_call(site:int,target:int)->bytes:
 return b'\xe8'+struct.pack('<i',target-site-5)

def verify_native(pe:PE32,*,check_hash=True)->dict:
 if pe.base!=0x22000000:raise VerificationError('original SiN server image base mismatch')
 normalized=[]
 for label,(va,length,digest,sentinel) in REGIONS.items():
  if pe.section(va)!='.text':raise VerificationError('native resolver outside original .text')
  body=bytearray(pe.at(va,length))
  if check_hash and hashlib.sha256(body).hexdigest()!=digest:
   raise VerificationError('whole native helper SHA256 mismatch: '+label)
  for off,hexcode in WITNESSES.items():
   witness=bytes.fromhex(hexcode)
   if body[off:off+len(witness)]!=witness:
    raise VerificationError('native resolver witness mismatch at %#x'%(va+off))
  for off,delta in ADDRESS_OFFSETS.items():
   absolute=struct.unpack_from('<I',body,off)[0]
   if absolute!=sentinel+delta or pe.section(absolute)!='.data':
    raise VerificationError('native resolver sentinel address mismatch at %#x'%(va+off))
   body[off:off+4]=bytes(4)
  if pe.at(va+length,12)!=b'\xcc'*12:
   raise VerificationError('original resolver RET4/INT3 boundary mismatch')
  normalized.append(bytes(body))
 for site,target in INCOMING_CALLS.items():
  if pe.at(site,5)!=rel_call(site,target):
   raise VerificationError('Phase194 caller edge mismatch at %#x'%site)
 if normalized[0]!=normalized[1]:
  raise VerificationError('native resolver pair differs outside seven absolute addresses')
 if check_hash and hashlib.sha256(normalized[0]).hexdigest()!=NORMALIZED_SHA:
  raise VerificationError('normalized complete native resolver SHA mismatch')
 return {
   'new_original_functions':2 if check_hash else 0,
   'new_original_native_bytes':168 if check_hash else 0,
   'original_opcode_witnesses':2*len(WITNESSES),
   'absolute_dword_references':2*len(ADDRESS_OFFSETS),
   'exact_incoming_calls':len(INCOMING_CALLS),
   'byte_identical_after_seven_pointer_operands':True,
   'normalized_sha256':NORMALIZED_SHA if check_hash else None,
   'node_stride_bytes':24,'missing_index':0xffff,
   'static_sentinel_words':[0xffff,0xffff,0xffff,1],
   'sentinel_flag_bit':0,'callee_stack_cleanup_bytes':4,
   'limits':['Host must not treat the 8-byte sentinel as a 24-byte node',
             'Original unchecked pointer arithmetic is replaced by host bounds checking',
             'No real Windows engine-hosted gameplay is proven'],
 }

def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise VerificationError('original full SiN DLL SHA256 mismatch')
 return verify_native(PE32(server))

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path,required=True)
 p.add_argument('--client',type=Path,required=True)
 p.add_argument('--output',type=Path)
 a=p.parse_args()
 result=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+'\n'
 if a.output:a.output.write_text(result,encoding='utf-8')
 else:print(result,end='')
if __name__=='__main__':main()
