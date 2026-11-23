#!/usr/bin/env python3
"""Authenticate full original SiN 2006 slot resolver/growth methods in retail.

--self-test uses invented bytes only. Do not commit or publish retail DLLs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS=(
 ('nested_free_slot_pointer_resolver',0x2228e100,84,
  '108da89a3abdf321d8eb9e8c542c01db13ea09a046d54a7affe15ce82ed908b1'),
 ('nested_12byte_slot_array_growth',0x2228e200,115,
  'bf973beee94fe5f7c2bd5e649e41eb973f99bb49dee68e28b9861757796adb2b'),
)
CALLS=(
 (0x2228e24f,0x2226e400,'grow_reallocate'),
 (0x2228e265,0x2226e390,'grow_allocate'),
 (0x2228e63c,0x2228e200,'phase652_fresh_slot_calls_grow'),
 (0x2228e65d,0x2228e100,'phase652_recycled_slot_calls_resolver'),
)
BRANCHES=(
 (0x2228e110,0x2228e133,'75','sentinel_cached'),
 (0x2228e13b,0x2228e14c,'74','sentinel_index'),
 (0x2228e209,0x2228e26f,'74','growth_disabled'),
 (0x2228e216,0x2228e23d,'7d','capacity_enough_signed'),
 (0x2228e21d,0x2228e231,'74','zero_capacity_initial_three'),
 (0x2228e221,0x2228e22a,'74','doubling_not_fixed_step'),
 (0x2228e228,0x2228e238,'eb','additive_join'),
 (0x2228e22f,0x2228e238,'eb','doubling_join'),
 (0x2228e23b,0x2228e218,'7c','repeat_while_below_signed_target'),
 (0x2228e244,0x2228e25d,'74','allocation_not_reallocation'),
)
PIN_TEXT='''
2228e100:b801000000 2228e105:840534015922 2228e10b:baffff0000
2228e110:7521 2228e112:090534015922 2228e118:6689152c015922
2228e11f:6689152e015922 2228e126:66891530015922
2228e12d:66a332015922 2228e133:668b442404 2228e138:663bc2
2228e13b:740f 2228e13d:8b4904 2228e140:0fb7c0
2228e143:8d0440 2228e146:8d0481 2228e149:c20400
2228e14c:b82c015922 2228e151:c20400
2228e200:56 2228e201:8bf1 2228e203:8b4e08
2228e206:83f9ff 2228e209:7464 2228e20b:8b4604
2228e20e:8b542408 2228e212:03d0 2228e214:3bc2
2228e216:7d25 2228e218:8b4604 2228e21b:85c0
2228e21d:7412 2228e21f:85c9 2228e221:7407
2228e223:03c1 2228e225:894604 2228e228:eb0e
2228e22a:03c0 2228e22c:894604 2228e22f:eb07
2228e231:c7460403000000 2228e238:395604 2228e23b:7cdb
2228e23d:8b0e 2228e23f:85c9 2228e241:8b4604
2228e244:7417 2228e246:8d1440 2228e249:03d2
2228e24b:03d2 2228e24d:52 2228e24e:51
2228e24f:e8ac01feff 2228e254:83c408 2228e257:8906
2228e259:5e 2228e25a:c20400 2228e25d:8d0440
2228e260:03c0 2228e262:03c0 2228e264:50
2228e265:e82601feff 2228e26a:83c404 2228e26d:8906
2228e26f:5e 2228e270:c20400
'''
PINS=tuple((int(t.split(':',1)[0],16),bytes.fromhex(t.split(':',1)[1]))
           for t in PIN_TEXT.split())
class VerificationError(ValueError):pass
class RetailPE:
 def __init__(self,data:bytes):
  self.data=data
  if len(data)<0x100 or data[:2]!=b'MZ':raise VerificationError('not a PE32 DLL')
  pe=struct.unpack_from('<I',data,0x3c)[0]
  if data[pe:pe+4]!=b'PE\x00\x00':raise VerificationError('wrong PE signature')
  coff=pe+4;num=struct.unpack_from('<H',data,coff+2)[0]
  opt=coff+20;optional_len=struct.unpack_from('<H',data,coff+16)[0]
  if struct.unpack_from('<H',data,opt)[0]!=0x10b:raise VerificationError('not original PE32')
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  self.sections=[]
  for i in range(num):
   at=opt+optional_len+40*i
   name=data[at:at+8].rstrip(b'\x00').decode('ascii')
   virtual,rva,raw_length,offset=struct.unpack_from('<IIII',data,at+8)
   self.sections.append((name,self.base+rva,min(virtual,raw_length),offset))
 def section(self,va:int)->str:
  for name,base,n,_ in self.sections:
   if base<=va<base+n:return name
  raise VerificationError(f'original VA outside PE32 sections at {va:#x}')
 def read(self,va:int,n:int)->bytes:
  for name,base,size,offset in self.sections:
   if base<=va and va+n<=base+size:
    v=self.data[offset+va-base:offset+va-base+n]
    if len(v)!=n:raise VerificationError('truncated PE32 raw section')
    return v
  raise VerificationError(f'out-of-section original VA {va:#x} +{n}')
def check_call(read,site,target):
 code=read(site,5)
 if code[0]!=0xe8 or site+5+struct.unpack_from('<i',code,1)[0]!=target:
  raise VerificationError(f'original rel32 call changed at {site:#x}')
def check_branch(read,site,target,opcode):
 code=read(site,2)
 if code[0]!=int(opcode,16) or site+2+struct.unpack_from('<b',code,1)[0]!=target:
  raise VerificationError(f'original branch target changed at {site:#x}')
def check_pin(read,site,expected):
 if read(site,len(expected))!=expected:
  raise VerificationError(f'original opcode witness changed at {site:#x}')
def authenticate(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER or hashlib.sha256(client).hexdigest()!=CLIENT:
  raise VerificationError('owner original retail server/client DLL SHA256 mismatch')
 pe=RetailPE(server)
 if pe.base!=0x22000000:raise VerificationError('unexpected original image base')
 for name,va,n,sha in METHODS:
  if pe.section(va)!='.text' or pe.section(va+n-1)!='.text':
   raise VerificationError(f'{name} not original .text')
  if hashlib.sha256(pe.read(va,n)).hexdigest()!=sha:
   raise VerificationError(f'complete original SHA256 mismatch in {name}')
  if pe.read(va+n,1)!=b'\xcc':raise VerificationError(f'missing original INT3 boundary in {name}')
 for src,target,_ in CALLS:
  if pe.section(src)!='.text' or pe.section(target)!='.text':
   raise VerificationError('original call outside PE32 .text')
  check_call(pe.read,src,target)
 for src,target,op,_ in BRANCHES:check_branch(pe.read,src,target,op)
 for va,expected in PINS:check_pin(pe.read,va,expected)
 return {'status':'PHASE653_RETAIL_NESTED_SLOT_RESOLVER_AND_GROWTH_AUTHENTICATED',
  'server_sha256':SERVER,'client_sha256':CLIENT,
  'methods':[{'name':name,'va':hex(va),'bytes':n,'whole_sha256':sha}
             for name,va,n,sha in METHODS],
  'complete_original_executable_bytes':sum(m[2] for m in METHODS),
  'native_direct_rel32_calls':len(CALLS),
  'native_conditional_and_unconditional_branches':len(BRANCHES),
  'address_pinned_instruction_witnesses':len(PINS),
  'original_slot_stride_bytes':12,'original_initial_slot_capacity':3,
  'original_sentinel_va':'0x2259012c',
  'original_2228edf0_rebalance_implemented':False,
  'original_retail_gameplay_executed':False,'android_quest_gameplay_executed':False}
def self_test():
 for _,va,n,_ in METHODS:
  invented=bytes((i*23+n+61)&255 for i in range(n))
  digest=hashlib.sha256(invented).digest()
  for i in range(n):
   wrong=bytearray(invented);wrong[i]^=1
   if hashlib.sha256(wrong).digest()==digest:
    raise AssertionError('mutated invented function accepted')
 site,target=0x71000000,0x71000400
 code=b'\xe8'+struct.pack('<i',target-site-5)
 check_call(lambda _va,n:code[:n],site,target)
 for i in range(5):
  changed=bytearray(code);changed[i]^=1
  try:check_call(lambda _va,n:bytes(changed[:n]),site,target)
  except VerificationError:pass
  else:raise AssertionError('mutated invented rel32 accepted')
 for op in ('74','75','7c','7d','eb'):
  code=bytes.fromhex(op)+struct.pack('<b',7)
  check_branch(lambda _va,n:code[:n],site,site+9,op)
  for i in range(2):
   changed=bytearray(code);changed[i]^=1
   try:check_branch(lambda _va,n:bytes(changed[:n]),site,site+9,op)
   except VerificationError:pass
   else:raise AssertionError('mutated invented branch accepted')
 check_pin(lambda _va,n:bytes.fromhex('c7460403000000')[:n],site,
           bytes.fromhex('c7460403000000'))
 try:authenticate(b'incorrect',b'incorrect')
 except VerificationError:pass
 else:raise AssertionError('accepted unrelated DLLs')
 print('PASS Phase653 invented SHA mutation, rel32, branch, original identity refusal')
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path);p.add_argument('--client',type=Path)
 p.add_argument('--output',type=Path);p.add_argument('--self-test',action='store_true')
 a=p.parse_args()
 if a.self_test:self_test();return
 if not a.server or not a.client:p.error('supply both original --server/--client or --self-test')
 try:r=authenticate(a.server.read_bytes(),a.client.read_bytes())
 except (OSError,VerificationError,ValueError,struct.error) as exc:
  p.exit(1,'PHASE653 ORIGINAL AUDIT REFUSED: '+str(exc)+'\n')
 out=json.dumps(r,indent=2)+'\n'
 if a.output:a.output.write_text(out,encoding='utf-8')
 else:print(out,end='')
if __name__=='__main__':main()
