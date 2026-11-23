#!/usr/bin/env python3
"""Authenticate original 2006 SiN nested-tree parent search, slot, and link.

Only owner-private exact retail DLLs can pass real checks. --self-test mutates
invented bytes and does not distribute retail original executable images.
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
 ('nested_parent_search',0x2228e470,261,'52909d48c6bde81a72b0e10e3f871e8d13de62ab03ca3a3aa2ffe6b145f3b871'),
 ('nested_node_acquisition',0x2228e620,86,'9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b'),
 ('nested_node_link_and_rebalance_dispatch',0x2228f460,109,'9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a'),
)
CALLS=(
 (0x2228e63c,0x2228e200),
 (0x2228e65d,0x2228e100),
 (0x2228f4a7,0x2228edf0),
 (0x2228f4b6,0x2228edf0),
 (0x2228f4c4,0x2228edf0),
 (0x2228f72b,0x2228e470),
 (0x2228f732,0x2228e620),
 (0x2228f746,0x2228f460),
)
BRANCHES=(
 (0x2228e48e,0x2228e56f),(0x2228e4bc,0x2228e515),
 (0x2228e4cc,0x2228e4f9),(0x2228e4fe,0x2228e50b),
 (0x2228e509,0x2228e563),(0x2228e513,0x2228e563),
 (0x2228e528,0x2228e54e),(0x2228e551,0x2228e55a),
 (0x2228e558,0x2228e55f),(0x2228e568,0x2228e495),
 (0x2228e62c,0x2228e658),(0x2228e635,0x2228e641),
 (0x2228f490,0x2228f4bf),(0x2228f4a0,0x2228f4b0),
)
PINS_TXT='''
2228e470:8b44240c 2228e47a:8be9 2228e486:66c703ffff
2228e495:8b542420 2228e4a2:8b4d04 2228e4ad:52 2228e4b4:ff5500
2228e4be:8b4c241c 2228e4ce:b9ffff0000 2228e4e5:66890d2e015922
2228e4fe:740b 2228e509:eb58 2228e515:8b54241c
2228e528:7524 2228e53b:66a32e015922 2228e551:7407
2228e55a:b82c015922 2228e56e:5e 2228e572:c21000
2228e620:56 2228e62b:57 2228e635:750a
2228e63c:e8bffbffff 2228e641:668b7e16 2228e64e:8d4f01
2228e657:c3 2228e65d:e89efaffff 2228e66c:668bc7 2228e675:c3
2228f460:8b5104 2228f46b:8d0440 2228f47b:66895004
2228f490:742d 2228f49d:8d0440 2228f4a7:e844f9ffff
2228f4ad:c20c00 2228f4b1:6689748202 2228f4b6:e835f9ffff
2228f4bc:c20c00 2228f4bf:56 2228f4c4:e827f9ffff
2228f4ca:c20c00
'''
PINS=tuple((int(t.split(':')[0],16),bytes.fromhex(t.split(':')[1])) for t in PINS_TXT.split())
class AuditError(ValueError):pass
class RetailPE:
 def __init__(self,data:bytes):
  self.data=data
  if len(data)<0x100 or data[:2]!=b'MZ':raise AuditError('not an MZ PE32 DLL')
  pe=struct.unpack_from('<I',data,0x3c)[0]
  if data[pe:pe+4]!=b'PE\x00\x00':raise AuditError('bad PE signature')
  co=pe+4;num=struct.unpack_from('<H',data,co+2)[0]
  opt=co+20;size=struct.unpack_from('<H',data,co+16)[0]
  if struct.unpack_from('<H',data,opt)[0]!=0x10b:raise AuditError('not native PE32')
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  self.sections=[]
  for i in range(num):
   off=opt+size+i*40
   name=data[off:off+8].rstrip(b'\x00').decode('ascii')
   virtual,rva,rawsize,raw=struct.unpack_from('<IIII',data,off+8)
   self.sections.append((name,self.base+rva,min(virtual,rawsize),raw))
 def section(self,va:int)->str:
  for name,base,n,off in self.sections:
   if base<=va<base+n:return name
  raise AuditError(f'VA outside PE32 sections {va:#x}')
 def read(self,va:int,n:int)->bytes:
  for name,base,length,raw in self.sections:
   if base<=va and va+n<=base+length:
    v=self.data[raw+va-base:raw+va-base+n]
    if len(v)!=n:raise AuditError('raw section truncated')
    return v
  raise AuditError(f'out-of-section VA {va:#x} +{n}')
def call_edge(read,src:int,dst:int):
 b=read(src,5)
 if b[0]!=0xe8 or src+5+struct.unpack_from('<i',b,1)[0]!=dst:
  raise AuditError(f'incorrect original rel32 call at {src:#x}')
def branch_edge(read,src:int,dst:int,expected:bytes=b''):
 b=read(src,6)
 if expected and not b.startswith(expected):raise AuditError(f'wrong original branch opcode at {src:#x}')
 if b[0] in (0x74,0x75,0xeb):
  offset=2;rel=struct.unpack_from('<b',b,1)[0]
 elif b[:2] in (b'\x0f\x84',b'\x0f\x85'):
  offset=6;rel=struct.unpack_from('<i',b,2)[0]
 else:raise AuditError(f'unexpected conditional or unconditional opcode {src:#x}')
 if src+offset+rel!=dst:raise AuditError(f'incorrect original jump target at {src:#x}')
def authenticate(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER or hashlib.sha256(client).hexdigest()!=CLIENT:
  raise AuditError('both original unchanged retail DLL SHA256s are required')
 pe=RetailPE(server)
 if pe.base!=0x22000000:raise AuditError('wrong original image base')
 for name,va,n,sha in METHODS:
  if pe.section(va)!='.text' or pe.section(va+n-1)!='.text':
   raise AuditError('not original executable .text')
  if hashlib.sha256(pe.read(va,n)).hexdigest()!=sha:
   raise AuditError(f'original complete-method SHA256 mismatch {name}')
  if pe.read(va+n,1)!=b'\xcc':raise AuditError(f'missing original INT3 boundary {name}')
 for src,dst in CALLS:
  if pe.section(src)!='.text' or pe.section(dst)!='.text':raise AuditError('call outside text')
  call_edge(pe.read,src,dst)
 opcodes=(b'\x0f\x84',b'\x74',b'\x75',b'\x74',b'\xeb',b'\xeb',b'\x75',b'\x74',b'\xeb',b'\x0f\x85',b'\x75',b'\x75',b'\x74',b'\x74')
 for (src,dst),op in zip(BRANCHES,opcodes):branch_edge(pe.read,src,dst,op)
 for va,op in PINS:
  if pe.read(va,len(op))!=op:raise AuditError(f'wrong original opcode witness at {va:#x}')
 for va,op in ((0x2228e4b4,b'\xff\x55\x00'),):
  if pe.read(va,len(op))!=op:raise AuditError('indirect comparator call missing')
 return {'status':'PHASE652_ORIGINAL_NESTED_TREE_CORE_AUTHENTICATED',
  'server_sha256':SERVER,'client_sha256':CLIENT,
  'methods':[{'name':name,'va':hex(va),'bytes':n,'full_sha256':sha} for name,va,n,sha in METHODS],
  'whole_original_function_bytes':sum(m[2] for m in METHODS),
  'verified_native_rel32_calls':len(CALLS),
  'verified_original_control_flow_branches':len(BRANCHES),
  'address_pinned_instruction_witnesses':len(PINS),
  'original_indirect_comparator_calls':1,
  'red_black_rebalance_2228edf0_recovered':False,
  'native_slot_growth_2228e200_recovered':False,
  'native_free_slot_resolver_2228e100_recovered':False,
  'gameplay_on_original_windows_dll_tested':False,
  'android_quest_gameplay_tested':False}
def self_test():
 for _,va,n,_ in METHODS:
  invented=bytes((i*29+n+13)&255 for i in range(n))
  digest=hashlib.sha256(invented).digest()
  for i in range(n):
   wrong=bytearray(invented);wrong[i]^=1
   if hashlib.sha256(wrong).digest()==digest:raise AssertionError('mutated invented method accepted')
 src,dst=0x71000000,0x71001000
 code=b'\xe8'+struct.pack('<i',dst-src-5)
 call_edge(lambda _va,n:code[:n],src,dst)
 for i in range(5):
  bad=bytearray(code);bad[i]^=1
  try:call_edge(lambda _va,n:bytes(bad[:n]),src,dst)
  except AuditError:pass
  else:raise AssertionError('mutated invented call accepted')
 for op in (b'\x74',b'\x75',b'\xeb',b'\x0f\x84',b'\x0f\x85'):
  raw=op+(struct.pack('<b',7) if len(op)==1 else struct.pack('<i',7))
  target=src+len(raw)+7
  branch_edge(lambda _va,n:(raw+b'\x00'*6)[:n],src,target,op)
  for i in range(len(raw)):
   wrong=bytearray(raw);wrong[i]^=1
   try:branch_edge(lambda _va,n:(bytes(wrong)+b'\x00'*6)[:n],src,target,op)
   except AuditError:pass
   else:raise AssertionError('altered invented branch accepted')
 try:authenticate(b'mock server',b'mock client')
 except AuditError:pass
 else:raise AssertionError('unrelated DLL accepted as retail')
 print('PASS Phase652 invented whole-body mutations, rel32, branches, retail DLL refusal')
def main():
 a=argparse.ArgumentParser(description=__doc__)
 a.add_argument('--server',type=Path);a.add_argument('--client',type=Path)
 a.add_argument('--output',type=Path);a.add_argument('--self-test',action='store_true')
 v=a.parse_args()
 if v.self_test:self_test();return
 if not v.server or not v.client:a.error('original --server and --client required, or --self-test')
 try:r=authenticate(v.server.read_bytes(),v.client.read_bytes())
 except (OSError,ValueError,struct.error) as e:a.exit(1,f'Phase652 retail audit REFUSED: {e}\n')
 data=json.dumps(r,indent=2)+'\n'
 if v.output:v.output.write_text(data,encoding='utf-8')
 else:print(data,end='')
if __name__=='__main__':main()
