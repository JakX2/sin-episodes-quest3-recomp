#!/usr/bin/env python3
"""Owner-original SiN Episodes Jetpack +0x1024 attachment lifetime audit (read-only)."""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS={
 0x22363D20:(38,'4ef9bb85842856c477d97383f502a68b6439ae49c86d636f6ac0e56b79a9da52'),
 0x22363D50:(42,'3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6'),
 0x22363DD0:(107,'addceba4a412ccf8b2fdf0d265667f1aa6193b362182f09f55d7e3f9032e34cd'),
 0x22365EE0:(248,'ae554bb1fde7031138f9d918b998ad79095cfaba1cccbcaaf6f967b72faff636'),
 0x22368F60:(161,'621c741c1b3c8d00a0e62ade0fc040b7a9bd078e1360e6502cc2e61fca8b6fbc'),
 0x223C7B10:(22,'542fc31bda25e807bd2320ff4c85d32db21ea320fe7f5f93e5aafe6ff5095200'),
}
CROSS_PHASE_588=(0x22364700,274,'b9d06b726a6036fba8a32cafc3bcb4b5c1d762483b7d26aae226a66a7a113173')
# All distinct original CALL E8 bytes, including two genuine prior-phase incoming edges.
DIRECT={
 0x22363E2F:0x22269B30,
 0x22365F01:0x22144DA0,
 0x22365F5C:0x2214D6F0,
 0x22365F90:0x22363DD0,
 0x22368F63:0x22360D80,
 0x22368F82:0x22360BC0,
 0x22368F94:0x223687F0,
 0x22368FE3:0x221F8610,
 0x22368FEE:0x2239A100,
 0x223C7B10:0x2228B110,
 0x22367F3F:0x22365EE0,
 0x223642D5:0x22363D50,
}
WITNESSES={
 0x22363D20:'568bf1', 0x22363D23:'8b4c2408', 0x22363D30:'8b08',
 0x22363D32:'890e', 0x22363D3A:'c706ffffffff',
 0x22363D50:'8b01', 0x22363D52:'83f8ff', 0x22363D5F:'81e1ff0f0000',
 0x22363D6F:'39410475', 0x22363D74:'8b01', 0x22363D77:'33c0',
 0x22363DE2:'898e500c0000',0x22363DEA:'c786500c0000ffffffff',
 0x22363DF4:'8b86500c0000',0x22363E22:'81c6240c0000',
 0x22363E28:'056c030000',
 0x22365EE0:'83ec0c', 0x22365EE9:'68c4855822',
 0x22365EF5:'ff90c4010000',0x22365EFC:'68e0844922',
 0x22365F09:'85c0',0x22365F16:'898624100000',
 0x22365F1E:'c78624100000ffffffff',0x22365F28:'8b8624100000',
 0x22365F3B:'81e1ff0f0000',0x22365F4B:'39410475',
 0x22365F56:'6a00',0x22365F58:'6a00',0x22365F5A:'6a00',
 0x22365FBE:'8b108bc8',0x22365FC2:'ff5264',
 0x22368F68:'c7064c964922',0x22368F6E:'c7869007000020964922',
 0x22368F78:'c786140c0000e4954922',
 0x22368FAF:'c786e00f000000000000',
 0x22368FB9:'8986e40f0000',0x22368FC5:'8986f40f0000',
 0x22368FCB:'898600100000',0x22368FD1:'898624100000',
 0x22368FDD:'898628100000',0x22368FF3:'c7860810000000000000',
 0x22368FFD:'8bc6',
 0x223C7B17:'68e0844922',0x223C7B1C:'68809a5322',0x223C7B23:'ff12',
}

class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  h=struct.unpack_from('<I',data,0x3C)[0];assert data[h:h+4]==b'PE\0\0'
  opt=h+24;assert struct.unpack_from('<H',data,opt)[0]==0x10b
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  sections=opt+struct.unpack_from('<H',data,h+20)[0]
  self.data=data;self.sections=[]
  for i in range(struct.unpack_from('<H',data,h+6)[0]):
   o=sections+40*i
   name=data[o:o+8].split(b'\0',1)[0];vsize,rva,rsize,roff=struct.unpack_from('<4I',data,o+8)
   self.sections.append((name,self.base+rva,self.base+rva+min(vsize,rsize),roff))
 def read(self,va,n):
  for name,start,end,offset in self.sections:
   if start<=va and va+n<=end:return self.data[offset+va-start:offset+va-start+n]
  raise AssertionError(f'Unmapped original PE address {va:#x} ({n} bytes)')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]

def check_call(pe,site,target):
 raw=pe.read(site,5)
 assert raw[0]==0xe8 and (site+5+struct.unpack('<i',raw[1:])[0])==target,f'original call edge drift at {site:#x}'

def audit(client,server):
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'wrong original server.dll SHA256'
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'wrong original client.dll SHA256'
 pe=PE(server);assert pe.base==0x22000000
 for address,(length,expected) in METHODS.items():
  assert hashlib.sha256(pe.read(address,length)).hexdigest()==expected,f'whole method drift {address:#x}'
  assert pe.read(address+length,1)==b'\xcc',f'original end not INT3 at {address+length:#x}'
 va,n,expected=CROSS_PHASE_588
 assert hashlib.sha256(pe.read(va,n)).hexdigest()==expected,'prior Jetpack ACT method changed'
 for site,target in DIRECT.items():check_call(pe,site,target)
 for va,hexbytes in WITNESSES.items():
  expected=bytes.fromhex(hexbytes)
  assert pe.read(va,len(expected))==expected,f'instruction witness drift {va:#x}'
 assert pe.read(0x224984E0,len('ai_base_physics_flying')+1)==b'ai_base_physics_flying\0'
 assert pe.u32(0x2249964C+0x338)==0x22364700 # original Jetpack primary vtable
 assert pe.read(0x22368F68,6)==b'\xc7\x06'+struct.pack('<I',0x2249964C)
 assert pe.read(0x223C7B17,5)==b'\x68'+struct.pack('<I',0x224984E0)
 assert pe.read(0x223C7B1C,5)==b'\x68'+struct.pack('<I',0x22539A80)
 assert pe.read(0x22365F1E,10)==b'\xc7\x86\x24\x10\0\0'+struct.pack('<I',0xffffffff)
 assert pe.read(0x22368FD1,6)==b'\x89\x86\x24\x10\0\0'
 return dict(original_methods_new=len(METHODS),distinct_original_bytes=sum(x[0] for x in METHODS.values()),direct_calls=len(DIRECT),opcode_witnesses=len(WITNESSES),attachment_entity_key='ai_base_physics_flying',jetpack_handle_offset='0x1024')

def selftest():
 class Fake:
  def __init__(self,va,body):self.va,self.body=va,body
  def read(self,va,n):return self.body[va-self.va:va-self.va+n]
 for site,target in DIRECT.items():
  raw=b'\xe8'+struct.pack('<i',target-site-5)
  check_call(Fake(site,raw),site,target)
  for offset in range(5):
   altered=bytearray(raw);altered[offset]^=1
   try:check_call(Fake(site,altered),site,target)
   except AssertionError:pass
   else:raise AssertionError('altered E8 call accepted')
 try:audit(b'fake',b'fake')
 except AssertionError:pass
 else:raise AssertionError('unsupported DLL identity accepted')
 return 'PHASE589_ORIGINAL_FREE_MUTATIONS_REJECTED'

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--self-test',action='store_true');parser.add_argument('--bin-zip',type=Path)
 a=parser.parse_args()
 if not a.self_test and a.bin_zip is None:parser.error('--self-test or --bin-zip required')
 if a.self_test:print(selftest())
 if a.bin_zip is not None:
  with zipfile.ZipFile(a.bin_zip) as z:
   print('SIN_ORIGINAL_JETPACK_ATTACHMENT_PHASE589',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()