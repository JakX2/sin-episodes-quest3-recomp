#!/usr/bin/env python3
"""Original SiN Episodes server 0x2206EF20 two-trace clearance authenticator.

Read-only; no commercial machine-code dump or game binaries distributed.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

SERVER_HASH='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_HASH='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
START=0x2206EF20
SIZE=549
SHA='5f17c0fb7a03cf109174a5525df36a0efcaada39ad21fdaa4d9d19ecde7ccb43'
E8={0x2206EFE6:0x2206E730,0x2206F0BF:0x2206E730}
PREVIOUS_EDGES={0x22071ABD:0x2206EF20,0x2206F97A:0x2206EF20}
WITNESSES={
 0x2206EF20:'81ec9c000000',0x2206EF2C:'8b4704',0x2206EF37:'ff5004',
 0x2206EF77:'8bb424b0000000',0x2206EFE6:'e845f7ffff',
 0x2206EFED:'389c248f000000',0x2206EFF4:'0f84ca000000',
 0x2206F009:'83b90402000006',0x2206F014:'ff92f4000000',
 0x2206F01E:'8b4704',0x2206F02E:'d95c2434',
 0x2206F07E:'ff15c4b15122',0x2206F084:'dc1de88f3d22',
 0x2206F08D:'dfe0',0x2206F08F:'f6c405',
 0x2206F096:'d9056c074e22',0x2206F0BF:'e86cf6ffff',
 0x2206F0C4:'d905f8223d22',0x2206F0D1:'dae9',
 0x2206F0D5:'f6c444',0x2206F0D8:'7b42',
 0x2206F0DA:'80bc248e00000000',0x2206F0E4:'84db',
 0x2206F0E8:'80bc248f00000000',0x2206F0F2:'8b8424c0000000',
 0x2206F110:'b001',0x2206F119:'c21400',
 0x2206F11C:'8b16',0x2206F125:'8910',
 0x2206F132:'84db',0x2206F138:'0f95c0',
 0x2206F142:'c21400'
}

def map_pe(b):
 assert b[:2]==b'MZ'
 pos=struct.unpack_from('<I',b,0x3c)[0]
 assert b[pos:pos+4]==b'PE\0\0'
 assert struct.unpack_from('<H',b,pos+4)[0]==0x14c
 assert struct.unpack_from('<H',b,pos+24)[0]==0x10b
 base=struct.unpack_from('<I',b,pos+24+28)[0]
 assert base==0x22000000
 sections=[]
 off=pos+24+struct.unpack_from('<H',b,pos+20)[0]
 for i in range(struct.unpack_from('<H',b,pos+6)[0]):
  p=off+i*40
  name=b[p:p+8].split(b'\0')[0]
  _,rva,rsize,roff=struct.unpack_from('<IIII',b,p+8)
  sections.append((name,base+rva,rsize,roff))
 def read(va,n):
  for name,origin,size,roff in sections:
   if origin<=va and va+n<=origin+size:
    return b[roff+va-origin:roff+va-origin+n]
  raise AssertionError(f'out-of-range original section at {va:#x}')
 return read,sections

def edge(read,site,target):
 raw=read(site,5)
 assert raw[0]==0xE8
 assert site+5+struct.unpack('<i',raw[1:])[0]==target, f'wrong E8 {site:#x}'

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'wrong original client.dll'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'wrong original server.dll'
 read,sections=map_pe(server)
 assert any(name==b'.text' and lo<=START and START+SIZE<=lo+n for name,lo,n,_ in sections)
 assert hashlib.sha256(read(START,SIZE)).hexdigest()==SHA,'original body changed'
 assert read(START+SIZE,1)==b'\xcc','original function end changed'
 assert read(0x2206F119,3)==bytes.fromhex('c21400')
 assert read(0x2206F142,3)==bytes.fromhex('c21400')
 for site,target in {**E8,**PREVIOUS_EDGES}.items():edge(read,site,target)
 for site,h in WITNESSES.items():
  expected=bytes.fromhex(h)
  assert read(site,len(expected))==expected,f'original witness changed {site:#x}'
 for va,fmt,val in [(0x224E076C,'<f',0.0625),(0x223D8FE8,'<d',0.1),(0x223D22F8,'<f',1.0)]:
  expected=struct.pack(fmt,val)
  assert read(va,len(expected))==expected,f'native literal {va:#x} changed'
 return dict(complete_methods=1,authenticated_bytes=SIZE,direct_E8_calls=len(E8),prior_native_callers=len(PREVIOUS_EDGES),opcode_witnesses=len(WITNESSES),native_literals=3,branch_returns=2)

def selftest():
 class Stub:
  def __init__(self,addr,payload):self.addr=addr;self.payload=payload
  def __call__(self,addr,size):
   assert addr==self.addr and size==5
   return self.payload
 for site,target in {**E8,**PREVIOUS_EDGES}.items():
  raw=b'\xe8'+struct.pack('<i',target-site-5)
  edge(Stub(site,raw),site,target)
  for i in range(5):
   b=bytearray(raw);b[i]^=1
   try:edge(Stub(site,b),site,target)
   except AssertionError:pass
   else:raise AssertionError('mutated callsite accepted')
 try:audit(b'not original',b'not original')
 except AssertionError:pass
 else:raise AssertionError('unapproved original binary accepted')
 return 'PHASE575_SYNTHETIC_PASS'

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--self-test',action='store_true')
 ap.add_argument('--bin-zip',type=Path)
 args=ap.parse_args()
 if not(args.self_test or args.bin_zip):ap.error('specify --self-test and/or --bin-zip')
 if args.self_test:print(selftest())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   print('PHASE575_ORIGINAL_PASS',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()