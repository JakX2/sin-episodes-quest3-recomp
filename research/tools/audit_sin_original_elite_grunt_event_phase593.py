#!/usr/bin/env python3
"""Read-only whole-function authentication: original 2006 SiN Elite Grunt event override."""
import argparse,hashlib,struct,zipfile
from pathlib import Path
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
METHOD=(0x22355480,360,'67d8c8be564d99bde7f2bd5f2ae7956ae0b1213aee0faf5f33c7cbff3a42a639')
PARENT=(0x2235BFD0,3340,'b927291398dd19d73f43fb0e2fbb842926b6008b90f0292645936ab5fafceebe')
DIRECT={
0x223554A8:0x22355300,0x223554B3:0x22355370,0x223554BA:0x22315440,
0x223554C8:0x220325C0,0x22355503:0x22315320,0x22355519:0x2235BFD0,
0x22355550:0x22031A80,0x223555B2:0x220A35C0,0x223555C0:0x220325C0,
0x223555DD:0x2203D2D0,
}
WITNESSES={
0x22355480:'515657',0x22355483:'8b7c2410',0x22355489:'83f803',
0x22355494:'83f843',0x22355499:'83f847',0x2235549E:'8dbedc0f0000',
0x223554BF:'6a00',0x223554C1:'680000803f',
0x223554D5:'8b86dc0f0000',0x223554E8:'81e1ff0f0000',
0x223554F1:'8d4c1104',0x223554F5:'c1e80c',
0x2235550A:'8886d90f0000',0x22355510:'0f84cc000000',
0x22355516:'57',0x22355524:'d94704',0x22355530:'80cc0c',
0x2235553B:'db5c2408',0x2235553F:'837c240801',
0x22355544:'d96c2410',0x22355548:'0f8594000000',
0x22355594:'8b410c',0x2235559B:'89869c090000',
0x223555B7:'6a00',0x223555B9:'6800000040',
0x223555CB:'8bce',0x223555CD:'ff90dc040000',
0x223555D9:'6a00',0x223555E5:'c20400',
}
class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  pe=struct.unpack_from('<I',data,0x3c)[0];assert data[pe:pe+4]==b'PE\0\0'
  opt=pe+24;assert struct.unpack_from('<H',data,opt)[0]==0x10b
  self.base=struct.unpack_from('<I',data,opt+28)[0];self.data=data;self.sec=[]
  sh=opt+struct.unpack_from('<H',data,pe+20)[0]
  for i in range(struct.unpack_from('<H',data,pe+6)[0]):
   o=sh+40*i;vs,rv,rs,off=struct.unpack_from('<4I',data,o+8)
   self.sec.append((self.base+rv,self.base+rv+min(vs,rs),off))
 def read(self,va,n):
  for lo,hi,off in self.sec:
   if lo<=va and va+n<=hi:return self.data[off+va-lo:off+va-lo+n]
  raise AssertionError(f'Original VA unmapped {va:#x}')
 def u32(self,a):return struct.unpack('<I',self.read(a,4))[0]

def edge(pe,site,target):
 b=pe.read(site,5)
 assert b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'E8 mutation at {site:#x}'

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA
 p=PE(server);assert p.base==0x22000000
 for va,sz,sha in (METHOD,PARENT):
  assert hashlib.sha256(p.read(va,sz)).hexdigest()==sha,f'whole native method {va:#x}'
 assert p.read(0x223555E8,8)==b'\xcc'*8
 assert p.u32(0x2248DF04+0x444)==0x22355480
 assert p.u32(0x2248DF04+0x338)==0x2235A250
 assert p.u32(0x2248DF04+0x5A8)==0x2235AE30
 col=p.u32(0x2248DF04-4);typ=p.u32(col+12)
 n=b'.?AVCNPC_EliteGrunt@@\0';assert p.read(typ+8,len(n))==n
 # Parent handlers for other original Grunt classes vs Jetpack override.
 assert p.u32(0x224954A4+0x444)==0x2235BFD0
 assert p.u32(0x2249AAA4+0x444)==0x2235BFD0
 assert p.u32(0x224A416C+0x444)==0x2235BFD0
 assert p.u32(0x2248E83C+0x444)==0x22364280
 for site,target in DIRECT.items():edge(p,site,target)
 for va,hx in WITNESSES.items():
  b=bytes.fromhex(hx)
  assert p.read(va,len(b))==b,f'Native field witness changed {va:#x}'
 assert p.read(0x223d9ee8,8)==bytes.fromhex('0000000000000840') # original 3.0 double
 return dict(new_full_methods=1,new_native_bytes=360,whole_method_hash=METHOD[2],
             authenticated_outgoing_direct_calls=len(DIRECT),instruction_witnesses=len(WITNESSES),
             dedicated_elite_event_ids=(3,0x43,0x47),default_forward_event_0x54=True)

def selftest():
 class Fake:
  def __init__(self,a,b):self.a,self.b=a,b
  def read(self,a,n):return self.b[a-self.a:a-self.a+n]
 for site,target in DIRECT.items():
  base=b'\xe8'+struct.pack('<i',target-site-5)
  edge(Fake(site,base),site,target)
  for i in range(5):
   bad=bytearray(base);bad[i]^=1
   try:edge(Fake(site,bad),site,target)
   except AssertionError:pass
   else:raise AssertionError('Accepted mutated native direct call')
 try:audit(b'wrong',b'wrong')
 except AssertionError:pass
 else:raise AssertionError('Accepted invalid retail DLL')
 return 'PHASE593_ORIGINAL_FREE_ELITE_CALL_MUTATIONS_REFUSED'

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--self-test',action='store_true');p.add_argument('--bin-zip',type=Path)
 x=p.parse_args()
 if not x.self_test and not x.bin_zip:p.error('Need --self-test or --bin-zip')
 if x.self_test:print(selftest())
 if x.bin_zip:
  with zipfile.ZipFile(x.bin_zip) as z:print('SIN_ELITE_GRUNT_EVENT_PHASE593',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()