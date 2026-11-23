#!/usr/bin/env python3
"""Read-only owner-retail PE32 factory/InterfaceReg ABI authentication.

Private --server and --client binaries never leave their owner's machine.
Public --self-test uses invented code bytes; this does NOT load any DLL.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
# Side, image base, DLL hash, InterfaceReg __thiscall ctor, ctor whole SHA,
# CreateInterface export, export whole SHA, linked-list head global.
MODULES = {
 'server':(0x22000000,SERVER,0x221fb5d0,
   '5e4a38bbe846cdfddece56859b823676d516af3f171181d7415ebbdfc1376654',
   0x221fb5f0,'39ed7fe9be839faa563964d7f5c2ad810dcd1992511131659da7fe823194b98a',
   0x22573c5c),
 'client':(0x24000000,CLIENT,0x24164160,
   '48699a1f29f3e63113789813739fcb588bfdc154be3047b2a5e712ea873c1a80',
   0x24164180,'f74837ce59ab917becd4c214e627ed91762085cb52883617278a206118267eec',
   0x24369378)
}
# Side, name, 21B static initializer, name VA, 6B factory,
# returned singleton, 12B InterfaceReg storage, initialized pointer to thunk,
# exact initializer SHA, exact factory SHA, optional statically backed vtable
# and four observed vtable entry addresses (NO method signatures inferred).
REG = (
 ('client','VClient011',0x24268300,0x24296e28,0x240fd840,0x242fa4b8,0x2433f7f0,
  0x242f3988,'ba18f2a12188966d90f536c7cf9c84364f598aa0cc4e80913616ccd96ef1aa3e',
  '72b920d06b2ea5b37dc9fe0b0a8821128ab5d7604d4d4bae35a9f6e01a86aa6c',
  0x24296c6c,(0x240fe0e0,0x240ff3d0,0x240fdb40,0x240fdc00)),
 ('client','VClientEntityList003',0x24268390,0x242973ec,0x241056b0,0x2434fddc,0x24357e48,
  0x242f399c,'8cd44efa85ebeeefe539f1ddb36625aacf1eaabd18c1da56435788eb5e204e6d',
  'af71d0de06e3cbfaf91c55fd1a68e973d49c6998f6dea1b6e834d33aaa465905',
  None,()),
 ('client','VClientPrediction001',0x24271450,0x242c6824,0x2422ea30,0x2437fa00,0x2437fa34,
  0x242f4810,'d39365be71be631383edf67ea843adfb38c469f268f1609fc715a24cadb4330c',
  '72a64b5a986505d8284d4487eb23c37528a0d79f7ba3a000a2c484790b33d1d8',
  None,()),
 ('server','ServerGameDLL003',0x223bf140,0x224432ac,0x221e7630,0x22510650,0x225722ec,
  0x224d756c,'1d704ab1bab3ed7cdbbac122cf11d73b1c78a1269eb8f6651c03c7072a15a074',
  'd878272797e03ad3c82d0a18ffaceef5be1f25fb3d33dbaf1493c240fefc369b',
  0x22442efc,(0x221e7640,0x221e7b00,0x221ea060,0x221e8f80)),
 ('server','ServerGameEnts001',0x223bf210,0x22443314,0x221e82c0,0x22510654,0x225723e4,
  0x224d7584,'6c46b07d21b9fa3c4a216b491ec0719e94c4b1dd6d2a72bbf9b13bb9df79209a',
  '81f3c5629ce13db9b52fddf5328c9d8301d28f7d02a510d2108ede5d9d3ea532',
  0x22443030,(0x221e9360,0x221e82d0,0x221e9380,0x221e82e0)),
 ('server','ServerGameClients003',0x223bf230,0x22443328,0x221e8320,0x22510658,0x225723f0,
  0x224d7588,'9336d0906b45de2f0a62d0a3ff1b21369dffeb18353b33b3490e54e277a675d6',
  'c811e01f62d7f1a8ec30e417c67c5e99dbf5f6dd9fa6a7112f92790e4bdece9c',
  0x22443054,(0x221e63f0,0x221e8330,0x221ea330,0x221ea3a0)),
)
def u16(b,o=0):return struct.unpack_from('<H',b,o)[0]
def u32(b,o=0):return struct.unpack_from('<I',b,o)[0]
def digest(b):return hashlib.sha256(b).hexdigest()

class PE:
 def __init__(self,data):
  self.data=data
  if len(data)<512 or data[:2]!=b'MZ':raise ValueError('invalid PE DOS header')
  pe=u32(data,0x3c)
  if pe+248>len(data) or data[pe:pe+4]!=b'PE\0\0' or u16(data,pe+4)!=0x14c:
   raise ValueError('not i386 PE')
  opt=pe+24
  if u16(data,opt)!=0x10b:raise ValueError('not PE32')
  self.base=u32(data,opt+28)
  self.export_rva=u32(data,opt+96)
  self.export_size=u32(data,opt+100)
  sec=opt+u16(data,pe+20)
  count=u16(data,pe+6)
  if count>96 or sec+count*40>len(data):raise ValueError('bad sections')
  self.sections=[]
  for i in range(count):
   pos=sec+i*40
   name=data[pos:pos+8].rstrip(b'\0').decode('ascii')
   virt,rva,rawlen,raw=struct.unpack_from('<IIII',data,pos+8)
   if rawlen and raw+rawlen>len(data):raise ValueError('bad raw bounds')
   self.sections.append((name,rva,max(virt,rawlen),rawlen,raw,u32(data,pos+36)))
 def sec(self,va,size=1):
  rva=va-self.base
  for x in self.sections:
   if x[1]<=rva and rva+size<=x[1]+x[2]:return x
  raise ValueError(f'VA {va:#x} outside sections')
 def read(self,va,size):
  name,rva,_,rawlen,raw,_=self.sec(va,size)
  rel=va-self.base-rva
  if rel+size>rawlen:raise ValueError(f'{va:#x}: not backed by disk bytes')
  return self.data[raw+rel:raw+rel+size]
 def asciiz(self,va):
  buf=bytearray()
  for off in range(128):
   ch=self.read(va+off,1)[0]
   if not ch:return buf.decode('ascii')
   buf.append(ch)
  raise ValueError('string not terminated')
 def exported(self,name):
  if not self.export_rva or not self.export_size:raise ValueError('no exports')
  t=self.read(self.base+self.export_rva,40)
  funcs,names,fn_rva,n_rva,ord_rva=(u32(t,x) for x in (20,24,28,32,36))
  if max(funcs,names)>5000:raise ValueError('bad export count')
  for i in range(names):
   nr=u32(self.read(self.base+n_rva+i*4,4))
   if self.asciiz(self.base+nr)!=name:continue
   ordinal=u16(self.read(self.base+ord_rva+i*2,2))
   if ordinal>=funcs:raise ValueError('bad export ordinal')
   rva=u32(self.read(self.base+fn_rva+ordinal*4,4))
   if self.export_rva<=rva<self.export_rva+self.export_size:raise ValueError('forwarded export')
   return self.base+rva
  raise ValueError(f'export {name} absent')
 def code(self,va,size):
  sec=self.sec(va,size)
  if sec[0]!='.text' or not (sec[5]&0x20000000):
   raise ValueError(f'{va:#x} outside executable .text')
  return self.read(va,size)
 def belongs(self,va,name):
  if self.sec(va)[0]!=name:raise ValueError(f'{va:#x} not in {name}')

def check_init_shape(code,name_va,factory,reg,ctor_va,init_va):
 if len(code)!=21 or code[0]!=0x68 or u32(code,1)!=name_va or \
    code[5]!=0x68 or u32(code,6)!=factory or code[10]!=0xb9 or \
    u32(code,11)!=reg or code[15]!=0xe8 or code[20]!=0xc3:
  raise ValueError('bad InterfaceReg initializer opcode/arguments')
 if (init_va+20+struct.unpack_from('<i',code,16)[0])&0xffffffff!=ctor_va:
  raise ValueError('bad InterfaceReg call target')

def check(side,pe):
 base,fullhash,ctor,ctorhash,exp,exphash,head=MODULES[side]
 if pe.base!=base or digest(pe.data)!=fullhash:raise ValueError(f'wrong {side} original binary')
 if pe.exported('CreateInterface')!=exp:raise ValueError('CreateInterface export RVA mismatch')
 c=pe.code(ctor,32);e=pe.code(exp,114)
 if digest(c)!=ctorhash or digest(e)!=exphash:raise ValueError('complete ctor/export hash mismatch')
 if c[:4]!=b'\x8b\x54\x24\x04' or c[6:10]!=b'\x8b\x4c\x24\x08' or \
  c[10:13]!=b'\x89\x48\x04' or c[13:15]!=b'\x89\x10' or \
  c[15:17]!=b'\x8b\x0d' or u32(c,17)!=head or c[21:24]!=b'\x89\x48\x08' or \
  c[24]!=0xa3 or u32(c,25)!=head or c[-3:]!=b'\xc2\x08\x00':
  raise ValueError('not the exact __thiscall 12-byte InterfaceReg linked-list ctor')
 if e[:3]!=b'\x55\x56\x57' or e[3:5]!=b'\x8b\x3d' or u32(e,5)!=head or \
    e[0x43:0x46]!=b'\x8b\x7f\x08' or e[0x6c:0x6e]!=b'\xff\x17' or \
    e[-1:]!=b'\xc3' or pe.read(exp+114,1)!=b'\xcc':
  raise ValueError('CreateInterface linked-list traversal/factory invocation mismatch')
 pe.belongs(head,'.data')
 out=[]
 for (kind,name,init,np,factory,obj,reg,init_slot,ih,fh,vtable,prefix) in REG:
  if kind!=side:continue
  i=pe.code(init,21);f=pe.code(factory,6)
  if digest(i)!=ih or digest(f)!=fh:raise ValueError(f'{name} full method hash mismatch')
  if pe.asciiz(np)!=name:raise ValueError(f'{name} bad original name string')
  check_init_shape(i,np,factory,reg,ctor,init)
  if f[0]!=0xb8 or u32(f,1)!=obj or f[5]!=0xc3:
   raise ValueError(f'{name} factory returns different object')
  if u32(pe.read(init_slot,4))!=init:raise ValueError(f'{name} missing static initializer pointer')
  for va,section in ((np,'.rdata'),(obj,'.data'),(reg,'.data'),(init_slot,'.data')):
   pe.belongs(va,section)
  if vtable is None:
   try:pe.read(obj,4)
   except ValueError:vt='runtime-initialized singleton, not file-backed'
   else:raise ValueError(f'{name} no longer expected BSS singleton')
  else:
   if u32(pe.read(obj,4))!=vtable:raise ValueError(f'{name} initialized vtable mismatch')
   pe.belongs(vtable,'.rdata')
   for index,fn in enumerate(prefix):
    if u32(pe.read(vtable+index*4,4))!=fn:raise ValueError(f'{name} vtable prefix changed')
    pe.code(fn,1)
   vt=f'{vtable:#x}; {len(prefix)} static entries authenticated (signatures unknown)'
  out.append({'interface':name,'initializer':hex(init),'factory':hex(factory),
              'singleton':hex(obj),'registration_node':hex(reg),
              'initializer_pointer':hex(init_slot),'vtable':vt})
 return {'CreateInterface':hex(exp),'InterfaceReg_ctor':hex(ctor),
         'registration_head':hex(head),'node_stride_bytes':12,'registrations':out}

def audit(server,client):
 result={};files={'server':server,'client':client}
 for side,b in files.items():result[side]=check(side,PE(b))
 result['fully_authenticated_executable_bytes']=2*(32+114)+6*(21+6)
 result['factory_registrations']=6
 result['proof_boundary']='original static PE evidence, not retail Windows process execution'
 return result

def self_test():
 # Invented source-model opcode fixture and mutation checks: never retail bytes.
 init=0x10001000;ctor=0x10002000;name=0x10003000;factory=0x10004000
 reg=0x10005000
 fake=b'\x68'+struct.pack('<I',name)+b'\x68'+struct.pack('<I',factory)+\
     b'\xb9'+struct.pack('<I',reg)+b'\xe8'+struct.pack('<i',ctor-init-20)+b'\xc3'
 check_init_shape(fake,name,factory,reg,ctor,init)
 for where in (1,6,11,16,20):
  bad=bytearray(fake);bad[where]^=1
  try:check_init_shape(bytes(bad),name,factory,reg,ctor,init)
  except ValueError:pass
  else:raise AssertionError(f'synthetic mutation {where} erroneously accepted')
 try:PE(b'not PE')
 except ValueError:pass
 else:raise AssertionError('fake PE accepted')
 assert digest(fake)!=digest(fake+b'\0')
 print('Phase657 invented-byte factory/registration self-test PASS')

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--server',type=Path)
 parser.add_argument('--client',type=Path)
 parser.add_argument('--self-test',action='store_true')
 args=parser.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:parser.error('pass both original DLL paths, or --self-test')
 print(json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2))

if __name__=='__main__':main()
