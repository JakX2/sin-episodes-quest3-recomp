#!/usr/bin/env python3
"""Source-bounded original SiN 2006 Grunt activity conversion provenance.

Requires the owner's original DLL ZIP to inspect the actual native bytes.
Public synthetic tests need no proprietary binaries.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS={
 0x2235A250:(326,'aacea02b9e14ff6ba7a060bbbac75dd33bca28db374218029b1cb0f8e91d5351'),
 0x222A53E0:(143,'b80322d471115481e1b2b70b0e7e006930b103bd7af6c13d64dbdd7e107e9b21'),
 0x2203CFC0:(170,'a6898e05cf04d849b2c110e66f2878e5b4565d3b55d13b02f6452521562c5e56'),
 0x222AA840:(216,'305cffb912b1e78bdec5adc2d024fc4a49d15b4c8888d162ad5e176b13250956'),
}
CALLS={
 0x2235A2C4:0x222AA840,
 0x2235A38C:0x2203CFC0,
 0x2203D041:0x22039130,
 0x2203D054:0x220EEBD0,
 0x222AA88D:0x221387F0,
}
ACTIVITY_INIT=(0x220E7040,13780,'be2d64d38bfffb9d6285fbb36268bc308a5d3d03100caf75dd6081dcb4b82206')
ACT_SAMPLES={
 1:(0x220E7053,0x223D8DB8,'ACT_IDLE'),
 6:(0x220E708F,0x223D8D70,'ACT_WALK'),
 7:(0x220E709B,0x223D8D60,'ACT_WALK_AIM'),
 10:(0x220E70C2,0x223D8D34,'ACT_RUN'),
 11:(0x220E70CE,0x223D8D28,'ACT_RUN_AIM'),
 15:(0x220E70FE,0x223D8CD8,'ACT_SCRIPT_CUSTOM_MOVE'),
 16:(0x220E710D,0x223D8CC4,'ACT_RANGE_ATTACK1'),
 17:(0x220E7119,0x223D8CB0,'ACT_RANGE_ATTACK2'),
 18:(0x220E7125,0x223D8C98,'ACT_RANGE_ATTACK1_LOW'),
 45:(0x220E7272,0x223D8AFC,'ACT_CROUCHIDLE'),
 66:(0x220E7377,0x223D8998,'ACT_RELOAD'),
 67:(0x220E7383,0x223D8988,'ACT_RELOAD_LOW'),
 72:(0x220E73C2,0x223D8940,'ACT_IDLE_ANGRY'),
 196:(0x220E7A8E,0x223D7F04,'ACT_ON_KNEES'),
 199:(0x220E7ABB,0x223D7ED4,'ACT_SHOOT_ON_KNEES'),
 200:(0x220E7ACD,0x223D7EC0,'ACT_RELOAD_ON_KNEES'),
}
WITNESSES={
 0x2235A250:'5657',0x2235A256:'83ff10',0x2235A25D:'80bea00c000000',
 0x2235A266:'bfc7000000',0x2235A270:'80be640c000000',0x2235A27D:'bf12000000',
 0x2235A287:'83ff42',0x2235A295:'bfc8000000',0x2235A2AC:'bf43000000',
 0x2235A2B6:'83ff11',0x2235A2BD:'68400c4922',0x2235A2E1:'83ff0a',
 0x2235A2E6:'c686640c000001',0x2235A2EF:'83ff06',0x2235A2FD:'83ff01',
 0x2235A30B:'bfc4000000',0x2235A31B:'bf2d000000',0x2235A322:'8b8650080000',
 0x2235A332:'bf48000000',0x2235A337:'8b8ebc0d0000',0x2235A346:'ff90a8040000',
 0x2235A362:'bf0b000000',0x2235A369:'bf07000000',0x2235A370:'bf48000000',
 0x2235A375:'8b8e180c0000',0x2235A382:'ff527c',
 0x222A53E0:'8b918c070000',0x222A53F9:'81e6ff0f0000',
 0x222A5409:'395604',0x222A543F:'ff927c040000',0x222A5467:'c60100',
 0x2203CFC0:'56',0x2203CFC5:'ff90c0050000',0x2203CFCB:'a900000008',
 0x2203CFDA:'83f842',0x2203D00E:'ff9018070000',
 0x2203D029:'83f803',0x2203D033:'f6868809000002',
 0x2203D049:'ff9714070000',0x2203D05E:'bf01000000',
 0x222AA840:'53',0x222AA84C:'8db5cc060000',
 0x222AA861:'25ff0f0000',0x222AA885:'39580c',
 0x222AA8C2:'ff90e4020000',0x222AA8D4:'83ff30',
}
class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ','not a PE32 MZ file'
  p=struct.unpack_from('<I',data,0x3c)[0]
  assert data[p:p+4]==b'PE\0\0','missing PE header'
  n=struct.unpack_from('<H',data,p+6)[0];o=p+24
  assert struct.unpack_from('<H',data,o)[0]==0x10b,'not PE32'
  self.base=struct.unpack_from('<I',data,o+28)[0]
  sec=o+struct.unpack_from('<H',data,p+20)[0]
  self.sections=[]
  for i in range(n):
   t=sec+i*40;name=data[t:t+8].split(b'\0')[0]
   vsz,rva,raw,off=struct.unpack_from('<4I',data,t+8)
   self.sections.append((name,self.base+rva,self.base+rva+min(vsz,raw),off))
  self.data=data
 def read(self,va,length):
  for name,a,e,offset in self.sections:
   if a<=va and va+length<=e:return self.data[offset+va-a:offset+va-a+length]
  raise AssertionError(f'unmapped source VA {va:#x}')
 def section(self,va):
  for name,a,e,_ in self.sections:
   if a<=va<e:return name
  raise AssertionError('unmapped')
def check_direct(pe,site,target):
 b=pe.read(site,5)
 assert b[0]==0xE8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'wrong original E8 at {site:#x}'
def check_activities(pe):
 va,n,sha=ACTIVITY_INIT
 assert hashlib.sha256(pe.read(va,n)).hexdigest()==sha
 assert pe.read(va+n,1)==b'\xcc'
 for num,(site,ptr,name) in ACT_SAMPLES.items():
  check_direct(pe,site,0x220E6F60)
  assert pe.read(site-5,5)==b'\x68'+struct.pack('<I',ptr)
  prior=b'\x6a'+bytes((num,)) if num<128 else b'\x68'+struct.pack('<I',num)
  assert pe.read(site-5-len(prior),len(prior))==prior
  assert pe.read(ptr,len(name)+1)==name.encode('ascii')+b'\0'
def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client DLL hash not original owner binary'
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'server DLL hash not original owner binary'
 pe=PE(server)
 assert pe.base==0x22000000
 for va,(length,digest) in METHODS.items():
  assert pe.section(va)==b'.text'
  if digest: assert hashlib.sha256(pe.read(va,length)).hexdigest()==digest,f'method SHA mismatch {va:#x}'
  assert pe.read(va+length,1)==b'\xcc',f'original method extent incorrect {va:#x}'
 # The Grunt primary vtable proved independently by Grunt-specific functions.
 assert pe.read(0x2249AAA4+0x338,4)==struct.pack('<I',0x2235A250)
 assert pe.read(0x2249AAA4+0x33C,4)==struct.pack('<I',0x222A53E0)
 for s,t in CALLS.items():check_direct(pe,s,t)
 for va,h in WITNESSES.items():assert pe.read(va,len(bytes.fromhex(h)))==bytes.fromhex(h),f'opcode witness {va:#x}'
 assert pe.read(0x22490C40,len('weapon_grenadelauncher')+1)==b'weapon_grenadelauncher\0'
 check_activities(pe)
 return dict(new_methods=len(METHODS),authenticated_bytes=sum(length for length,_ in METHODS.values()),native_direct_calls=len(CALLS),witnesses=len(WITNESSES),named_activities=len(ACT_SAMPLES),primary_vtable_slots=2)
def self_test():
 class Fake:
  def __init__(self,site,raw):self.site,self.raw=site,raw
  def read(self,va,n):return self.raw[va-self.site:va-self.site+n]
 for site,target in {**CALLS,**{v[0]:0x220E6F60 for v in ACT_SAMPLES.values()}}.items():
  raw=b'\xE8'+struct.pack('<i',target-site-5)
  check_direct(Fake(site,raw),site,target)
  for pos in range(5):
   x=bytearray(raw);x[pos]^=1
   try:check_direct(Fake(site,x),site,target)
   except AssertionError:pass
   else:raise AssertionError('accepted changed E8 call')
 try:audit(b'bad',b'bad')
 except AssertionError:pass
 else:raise AssertionError('accepted fake original DLL pair')
 return 'PHASE587_SYNTHETIC_ORIGINAL_FREE_PASS'
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--self-test',action='store_true')
 ap.add_argument('--bin-zip',type=Path)
 args=ap.parse_args()
 if not args.self_test and not args.bin_zip:ap.error('use --self-test or --bin-zip')
 if args.self_test:print(self_test())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   report=audit(z.read('client.dll'),z.read('server.dll'))
  print('SIN_ORIGINAL_GRUNT_ACTIVITY_PHASE587',report)
if __name__=='__main__':main()