#!/usr/bin/env python3
"""Read-only original SiN Jetpack Grunt activity override and class RTTI audit."""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHOD=(0x22364700,274,'b9d06b726a6036fba8a32cafc3bcb4b5c1d762483b7d26aae226a66a7a113173')
BASE_METHOD=(0x2235A250,326,'aacea02b9e14ff6ba7a060bbbac75dd33bca28db374218029b1cb0f8e91d5351')
JETPACK_VTABLES={
 0x2248C514:'CNPC_DruglabJetpack',
 0x2248E83C:'CNPC_EliteJetpack',
 0x2249964C:'CNPC_JetpackGruntBase',
 0x224A4A4C:'CNPC_SintekJetpack',
}
GRUNT_VTABLES={
 0x2248DF04:'CNPC_EliteGrunt',
 0x224954A4:'CNPC_GruntBase',
 0x2249AAA4:'CNPC_MobGrunt',
 0x224A416C:'CNPC_SintekGrunt',
}
ACT_SAMPLES={
 1:(0x220E7053,0x223D8DB8,'ACT_IDLE'),
 7:(0x220E709B,0x223D8D60,'ACT_WALK_AIM'),
 16:(0x220E710D,0x223D8CC4,'ACT_RANGE_ATTACK1'),
 66:(0x220E7377,0x223D8998,'ACT_RELOAD'),
 208:(0x220E7B48,0x223D7E00,'ACT_JETPACK_IDLE'),
 213:(0x220E7B93,0x223D7D9C,'ACT_JETPACK_MOVE'),
 218:(0x220E7BE1,0x223D7D38,'ACT_JETPACK_RELOAD'),
}
ACT_REGISTRY=(0x220E7040,13780,'be2d64d38bfffb9d6285fbb36268bc308a5d3d03100caf75dd6081dcb4b82206')
WITNESSES={
 0x22364700:'51568bf1', 0x22364704:'8b8624100000',
 0x2236470f:'8b1520564f22', 0x22364717:'81e1ff0f0000',
 0x22364732:'d980200c0000',0x22364739:'d9801c0c0000',
 0x2236473f:'d980180c0000',0x2236475e:'ff15c4b15122',
 0x22364772:'83f841',0x22364777:'0fb680d0473622',
 0x2236477e:'ff2485c4473622',0x22364785:'b8da000000',
 0x2236478f:'d9442404',0x22364793:'d81d30583d22',
 0x2236479b:'f6c441',0x223647a0:'b8d5000000',
 0x223647aa:'b8d0000000',0x223647b7:'e8945affff',
 0x223647c1:'8d4900',
}
class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  h=struct.unpack_from('<I',data,0x3c)[0]; assert data[h:h+4]==b'PE\0\0'
  n=struct.unpack_from('<H',data,h+6)[0];opt=h+24
  assert struct.unpack_from('<H',data,opt)[0]==0x10b
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  sec=opt+struct.unpack_from('<H',data,h+20)[0]
  self.data=data;self.sections=[]
  for i in range(n):
   o=sec+i*40;name=data[o:o+8].split(b'\0')[0]
   vsz,rva,raw,off=struct.unpack_from('<4I',data,o+8)
   self.sections.append((name,self.base+rva,self.base+rva+min(vsz,raw),off))
 def read(self,va,n):
  for name,start,end,offset in self.sections:
   if start<=va and va+n<=end:return self.data[offset+va-start:offset+va-start+n]
  raise AssertionError(f'unmapped PE VA: {va:#x} len {n}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def cstring(self,va,limit=128):return self.read(va,limit).split(b'\0',1)[0].decode('ascii')
def direct(pe,site,target):
 b=pe.read(site,5)
 assert b[:1]==b'\xe8' and site+5+struct.unpack('<i',b[1:])[0]==target,f'wrong original native E8 {site:#x}'
def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA
 pe=PE(server);assert pe.base==0x22000000
 va,n,sha=METHOD
 assert sha and hashlib.sha256(pe.read(va,n)).hexdigest()==sha
 assert pe.read(va+n,1)==b'\xcc','incorrect native method end'
 for ref in (METHOD,BASE_METHOD):
  s,length,expected=ref
  assert hashlib.sha256(pe.read(s,length)).hexdigest()==expected
 assert pe.read(BASE_METHOD[0]+BASE_METHOD[1],1)==b'\xcc'
 direct(pe,0x223647B7,0x2235A250)
 assert pe.u32(0x2251B1C4)==0x222680E0
 assert pe.read(0x223D5830,4)==struct.pack('<f',50.0)
 # 3 absolute code-destination words followed by 66 per-activity byte indices.
 branches=(0x2236478F,0x22364785,0x223647B4)
 assert tuple(pe.u32(0x223647C4+4*i) for i in range(3))==branches
 table=pe.read(0x223647D0,66)
 assert all(z in (0,1,2) for z in table)
 assert {i+1:z for i,z in enumerate(table) if z!=2}=={1:0,7:0,16:0,66:1}
 assert pe.read(0x223647D0+66,1)==b'\xcc'
 for vt,name in {**JETPACK_VTABLES,**GRUNT_VTABLES}.items():
  expected=0x22364700 if vt in JETPACK_VTABLES else 0x2235A250
  assert pe.u32(vt+0x338)==expected
  assert pe.u32(vt+0x33C)==0x222A53E0
  col=pe.u32(vt-4);typ=pe.u32(col+12)
  assert pe.read(typ+8,len('.?AV'+name+'@@')+1)==('.?AV'+name+'@@').encode('ascii')+b'\0',f'incorrect RTTI name {name}'
 for pos,hx in WITNESSES.items():assert pe.read(pos,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'opcode drift {pos:#x}'
 s,n,sha=ACT_REGISTRY
 assert hashlib.sha256(pe.read(s,n)).hexdigest()==sha
 for code,(site,string_va,name) in ACT_SAMPLES.items():
  direct(pe,site,0x220E6F60)
  assert pe.read(site-5,5)==b'\x68'+struct.pack('<I',string_va)
  before=(b'\x6a'+bytes([code]) if code<128 else b'\x68'+struct.pack('<I',code))
  assert pe.read(site-5-len(before),len(before))==before
  assert pe.read(string_va,len(name)+1)==name.encode('ascii')+b'\0'
 return dict(original_methods_new=1,bytes_new=METHOD[1],derived_jetpack_classes=len(JETPACK_VTABLES),grunt_base_classes=len(GRUNT_VTABLES),activity_names=len(ACT_SAMPLES),event_table_bytes=12+66,activity_dispatch_special_cases=4,opcode_witnesses=len(WITNESSES))
def self_test():
 class Fake:
  def __init__(self,site,body):self.site,self.body=site,body
  def read(self,site,n):return self.body[site-self.site:site-self.site+n]
 for site,target in [(0x223647B7,0x2235A250)]+[(x[0],0x220E6F60) for x in ACT_SAMPLES.values()]:
  data=b'\xe8'+struct.pack('<i',target-site-5)
  direct(Fake(site,data),site,target)
  for pos in range(5):
   mutated=bytearray(data);mutated[pos]^=1
   try:direct(Fake(site,mutated),site,target)
   except AssertionError:pass
   else:raise AssertionError('corrupt E8 accepted')
 try:audit(b'bad',b'bad')
 except AssertionError:pass
 else:raise AssertionError('fake original accepted')
 return 'PHASE588_SYNTHETIC_CORRUPTION_REFUSED'
def main():
 a=argparse.ArgumentParser(description=__doc__)
 a.add_argument('--self-test',action='store_true');a.add_argument('--bin-zip',type=Path)
 args=a.parse_args()
 if not args.self_test and not args.bin_zip:a.error('use --self-test or --bin-zip')
 if args.self_test:print(self_test())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   print('SIN_ORIGINAL_JETPACK_ACTIVITY_PHASE588',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()