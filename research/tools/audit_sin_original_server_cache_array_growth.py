#!/usr/bin/env python3
"""Phase201: original 2006 SiN server cache vector-grows and key initializers.

Original DLLs stay owner-private. Public evidence is digests, sparse opcode
witnesses and CALL targets, never full native x86 code or game assets.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
GROW={
 'scene':(0x221A3A10,'e4affe6087ab30ce324d99a085b63f7f6589c3532bfbf9969dc2a5dac389abd5'),
 'modelsounds':(0x2213AAC0,'c8cd4dd9c3ea9eaf97582206ea260481dde62a981a0518fede75a4fa42986f8f'),
}
INIT={
 'scene':(0x221A3A90,'f55c402b759c9af8cb82f01ca4ae3844018f8e2cf6e886e5e083b75d3b2ea53e'),
 'modelsounds':(0x2213AB40,'f55c402b759c9af8cb82f01ca4ae3844018f8e2cf6e886e5e083b75d3b2ea53e'),
}
GROW_LEN=119;INIT_LEN=36
GROW_NORM_SHA='609ec40a2754642eccef8c7d8715049b146d0f8990e0ead2bd5827c7e7d07192'
CALLS={0x51:0x2226E400,0x69:0x2226E390}
INCOMING={0x221A68AD:0x221A3A10,0x2214091D:0x2213AAC0}
# Sparse, disjoint native instruction witnesses. Not complete code.
GROW_WITNESSES={
 0x00:'568bf1',0x06:'83f9ff',0x0e:'8b542408',0x16:'7d25',
 0x1b:'85c0',0x23:'03c1',0x2a:'03c0',0x31:'c7460402000000',
 0x38:'395604',0x3b:'7cdb',0x3d:'8b0e',0x44:'7419',
 0x46:'8d1440',0x4f:'5251',0x56:'83c408',0x59:'8906',
 0x5c:'c20400',0x5f:'8d0440',0x68:'50',0x6e:'83c404',
 0x71:'8906',0x74:'c20400',
}
INIT_WITNESSES={0x00:'8b442404',0x06:'741b',0x15:'c74008feffffff',0x23:'c3'}
class VerificationError(ValueError):pass
class PE32:
 def __init__(self,b:bytes):
  if len(b)<0x100 or b[:2]!=b'MZ':raise VerificationError('invalid DOS signature')
  pe=struct.unpack_from('<I',b,0x3c)[0]
  if pe+24>len(b) or b[pe:pe+4]!=b'PE\x00\x00':raise VerificationError('bad PE header')
  machine,n=struct.unpack_from('<HH',b,pe+4);sz=struct.unpack_from('<H',b,pe+20)[0];opt=pe+24
  if machine!=0x14c or not 1<=n<=96 or opt+sz+40*n>len(b) or struct.unpack_from('<H',b,opt)[0]!=0x10b:
   raise VerificationError('expected bounded original i386 PE32')
  self.base=struct.unpack_from('<I',b,opt+28)[0];self.sections=[];self.b=b
  for i in range(n):
   p=opt+sz+i*40
   name=b[p:p+8].split(b'\x00')[0].decode('ascii','replace')
   vsz,rva,rawlen,raw=struct.unpack_from('<IIII',b,p+8)
   self.sections.append((name,rva,vsz,rawlen,raw))
 def at(self,va:int,n:int)->bytes:
  if n<0:raise VerificationError('negative length')
  rva=va-self.base
  for name,base,vsz,rawlen,off in self.sections:
   if base<=rva and rva-base+n<=rawlen and off+rva-base+n<=len(self.b):
    return self.b[off+rva-base:off+rva-base+n]
  raise VerificationError('VA not file backed %#x'%va)
 def section(self,va:int)->str:
  rva=va-self.base
  for name,base,vsz,rawlen,off in self.sections:
   if base<=rva<base+max(vsz,rawlen):return name
  return ''
def relcall(site:int,dest:int)->bytes:
 return b'\xe8'+struct.pack('<i',dest-site-5)
def verify_native(pe:PE32,*,check_hash=True)->dict:
 if pe.base!=0x22000000:raise VerificationError('server image base mismatch')
 norm=[];init_bodies=[]
 for label,(va,digest) in GROW.items():
  if pe.section(va)!='.text':raise VerificationError('growth not in original .text')
  body=bytearray(pe.at(va,GROW_LEN))
  if check_hash and hashlib.sha256(body).hexdigest()!=digest:
   raise VerificationError('whole growth body SHA mismatch '+label)
  for off,hexcode in GROW_WITNESSES.items():
   literal=bytes.fromhex(hexcode)
   if body[off:off+len(literal)]!=literal:
    raise VerificationError('growth sparse instruction witness mismatch at %#x'%(va+off))
  for off,target in CALLS.items():
   if body[off:off+5]!=relcall(va+off,target):
    raise VerificationError('growth native allocator CALL mismatch at %#x'%(va+off))
   body[off+1:off+5]=b'\x00'*4
  if pe.at(va+GROW_LEN,9)!=b'\xcc'*9:
   raise VerificationError('growth RET4/INT3 native boundary mismatch')
  norm.append(bytes(body))
 for label,(va,digest) in INIT.items():
  if pe.section(va)!='.text':raise VerificationError('init not in .text')
  body=pe.at(va,INIT_LEN)
  if check_hash and hashlib.sha256(body).hexdigest()!=digest:
   raise VerificationError('whole initializer body SHA mismatch '+label)
  for off,hexcode in INIT_WITNESSES.items():
   literal=bytes.fromhex(hexcode)
   if body[off:off+len(literal)]!=literal:
    raise VerificationError('initializer opcode mismatch at %#x'%(va+off))
  if pe.at(va+INIT_LEN,12)!=b'\xcc'*12:
   raise VerificationError('initializer RET/INT3 boundary mismatch')
  init_bodies.append(body)
 for site,target in INCOMING.items():
  if pe.at(site,5)!=relcall(site,target):
   raise VerificationError('Phase194 incoming native growth CALL mismatch %#x'%site)
 if norm[0]!=norm[1]:raise VerificationError('growth bodies differ beyond two rel32 fields')
 if init_bodies[0]!=init_bodies[1]:raise VerificationError('initializer native bodies differ')
 if check_hash and hashlib.sha256(norm[0]).hexdigest()!=GROW_NORM_SHA:
  raise VerificationError('normalized full growth body SHA mismatch')
 return {'new_sha_verified_functions':4 if check_hash else 0,
         'new_original_native_bytes':2*(GROW_LEN+INIT_LEN) if check_hash else 0,
         'sparse_opcode_witnesses':2*(len(GROW_WITNESSES)+len(INIT_WITNESSES)),
         'exact_outgoing_allocator_calls':2*len(CALLS),
         'exact_incoming_slot_allocator_calls':len(INCOMING),
         'normalized_growth_original_SHA256':GROW_NORM_SHA if check_hash else None,
         'init_native_body_identical':True,
         'array_this_relative_to_tree':4,
         'vector_fields':{'backing':0,'capacity':4,'policy':8},
         'growth_step_disabled':0xffffffff,'growth_step_double':0,
         'minimum_new_capacity':2,'node_stride':24,
         'original_heap_malloc_va':'0x2226e390','original_heap_realloc_va':'0x2226e400',
         'limits':['Runtime allocator and ownership not recovered','Native integer overflow/invalid pointer behavior not simulated','Original-engine game loading not demonstrated']}
def audit(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
  raise VerificationError('original complete DLL SHA256 mismatch')
 return verify_native(PE32(server))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path,required=True)
 p.add_argument('--client',type=Path,required=True)
 p.add_argument('--output',type=Path)
 args=p.parse_args()
 result=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+'\n'
 if args.output:args.output.write_text(result,encoding='utf-8')
 else:print(result,end='')
if __name__=='__main__':main()
