#!/usr/bin/env python3
"""Authenticate entire original 2006 SiN Jetpack Grunt event dispatcher and tables."""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHOD=(0x22364280,1146,'2e155aff9b5440527fa4eeed36fff5ae527957b5106a88c711e18724e951b9a6')
OLDER_589=(0x22365EE0,248,'ae554bb1fde7031138f9d918b998ad79095cfaba1cccbcaaf6f967b72faff636')
VTABLE_JETPACK={0x2248C514:'CNPC_DruglabJetpack',0x2248E83C:'CNPC_EliteJetpack',0x2249964C:'CNPC_JetpackGruntBase',0x224A4A4C:'CNPC_SintekJetpack'}
BRANCHES=(0x22364505,0x223642AF,0x22364438,0x22364470,0x223644A7,0x223644D0,0x22364403,0x22364311,0x223645FD)
SPECIAL={0x19:0,0x38:1,0x3C:1,0x9F:2,0xA0:3,0xA3:4,0xA4:5,0xA5:6,0xA6:7}
DIRECT={
 0x223642D5:0x22363D50,
 0x22364301:0x2203D2D0,0x2236432D:0x22363870,
 0x2236434E:0x220330C0,0x22364371:0x2202EB00,
 0x223643A1:0x223638F0,0x223643AD:0x2202EB00,
 0x223643D0:0x22073E00,0x223643D9:0x2203D2D0,
 0x2236441F:0x22363510,0x22364428:0x2203D2D0,
 0x22364460:0x2203D2D0,0x22364497:0x2203D2D0,
 0x223644F3:0x220325C0,0x22364520:0x22363870,
 0x2236453E:0x22039130,0x2236454F:0x22039130,
 0x22364556:0x220641F0,0x22364574:0x2204C660,
 0x22364593:0x22073E00,0x223645B9:0x223638F0,
 0x223645C3:0x2204C660,0x223645CA:0x220330C0,
 0x223645ED:0x22073E00,0x2236462C:0x2238B3C0,
 0x22364638:0x2235BFD0,
}
WITNESSES={
 0x22364280:'81ec90000000',
 0x22364291:'83c0e7',0x22364294:'3d8d000000',
 0x2236429B:'0f875c030000',
 0x223642A1:'0fb6806c463622',
 0x223642A8:'ff248548463622',
 0x223642CD:'8d8e24100000',
 0x22364456:'c786ec0f000000000000',
 0x2236448D:'c786ec0f000002000000',
 0x223644EA:'6a00',
 0x223644EC:'68cdcc4c3d',
 0x2236452B:'6a01',
 0x2236452D:'ff927c040000',
 0x22364579:'8b8ef8080000',
 0x2236458B:'8b8ef8080000',
 0x223645FD:'8b8624100000',
 0x22364610:'81e1ff0f0000',
 0x2236461D:'c1e80c',
 0x22364631:'84c0',
 0x22364635:'57',
}

class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  nt=struct.unpack_from('<I',data,0x3c)[0];assert data[nt:nt+4]==b'PE\0\0'
  opt=nt+24;assert struct.unpack_from('<H',data,opt)[0]==0x10b
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  section_table=opt+struct.unpack_from('<H',data,nt+20)[0]
  self.sections=[];self.data=data
  for i in range(struct.unpack_from('<H',data,nt+6)[0]):
   o=section_table+i*40;name=data[o:o+8].split(b'\0')[0]
   vsize,rva,rsize,off=struct.unpack_from('<4I',data,o+8)
   self.sections.append((name,self.base+rva,self.base+rva+min(vsize,rsize),off))
 def read(self,va,n):
  for name,start,end,off in self.sections:
   if start<=va and va+n<=end:return self.data[off+va-start:off+va-start+n]
  raise AssertionError(f'unmapped original VA {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]

def edge(pe,site,target):
 b=pe.read(site,5)
 assert b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'E8 drift {site:#x}'

def audit(client,server):
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA
 p=PE(server);assert p.base==0x22000000
 for va,size,hashhex in (METHOD,OLDER_589):
  assert hashlib.sha256(p.read(va,size)).hexdigest()==hashhex,f'original method drift {va:#x}'
  assert p.read(va+size,1)==b'\xcc'
 for site,dst in DIRECT.items():edge(p,site,dst)
 for va,hx in WITNESSES.items():
  raw=bytes.fromhex(hx)
  assert p.read(va,len(raw))==raw,f'original instruction drift at {va:#x}'
 assert tuple(p.u32(0x22364648+4*i) for i in range(9))==BRANCHES
 table=p.read(0x2236466C,142)
 assert len(table)==142 and all(x<=8 for x in table)
 assert {i+0x19:int(v) for i,v in enumerate(table) if v!=8}==SPECIAL
 assert sum(int(x==8) for x in table)==133
 assert p.read(0x223646FA,1)==b'\xcc'
 for vt,name in VTABLE_JETPACK.items():
  assert p.u32(vt+0x444)==0x22364280
  assert p.u32(vt+0x338)==0x22364700
  col=p.u32(vt-4);typ=p.u32(col+12)
  original_name=('.?AV'+name+'@@').encode()+b'\0'
  assert p.read(typ+8,len(original_name))==original_name
 assert p.read(0x223644EC,5)==b'\x68'+struct.pack('<I',struct.unpack('<I',struct.pack('<f',0.05))[0])
 return dict(new_full_original_methods=1,new_original_method_bytes=1146,table_bytes=36+142,original_direct_calls=len(DIRECT),opcode_witnesses=len(WITNESSES),jetpack_classes=len(VTABLE_JETPACK),special_event_ids=len(SPECIAL),default_in_range_events=133)

def self_test():
 class Fake:
  def __init__(self,va,body):self.va,self.body=va,body
  def read(self,va,n):return self.body[va-self.va:va-self.va+n]
 for addr,dest in DIRECT.items():
  original=b'\xe8'+struct.pack('<i',dest-addr-5)
  edge(Fake(addr,original),addr,dest)
  for pos in range(5):
   bad=bytearray(original);bad[pos]^=1
   try:edge(Fake(addr,bad),addr,dest)
   except AssertionError:pass
   else:raise AssertionError('mutated original CALL accepted')
 try:audit(b'bad',b'bad')
 except AssertionError:pass
 else:raise AssertionError('invalid original SHA pair accepted')
 return 'PHASE590_ORIGINAL_FREE_EVENT_TABLE_MUTATIONS_REFUSED'

def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--self-test',action='store_true');a.add_argument('--bin-zip',type=Path)
 x=a.parse_args()
 if not x.self_test and not x.bin_zip:a.error('--self-test or --bin-zip required')
 if x.self_test:print(self_test())
 if x.bin_zip:
  with zipfile.ZipFile(x.bin_zip) as z:
   print('SIN_ORIGINAL_JETPACK_EVENT_SWITCH_PHASE590',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()