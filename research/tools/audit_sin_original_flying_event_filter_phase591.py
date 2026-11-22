#!/usr/bin/env python3
"""Read-only 2006 SiN flying-attachment event filter and Grunt fallback proof."""
import argparse,hashlib,struct,zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHOD=(0x2238B3C0,560,'66002e4d22399e1af4767a15152e3f00142a2940009a48c29528c482bf61677f')
PHASE590=(0x22364280,1146,'2e155aff9b5440527fa4eeed36fff5ae527957b5106a88c711e18724e951b9a6')
CASEPTRS=(0x2238B48B,0x2238B531,0x2238B46C,0x2238B3E8,0x2238B461)
SPECIAL={0x09:0,0x0A:0,0x28:1,0x2B:2,0x38:3,0x3C:3}
DIRECT={
 0x2238B42A:0x22060D30,0x2238B45C:0x2203D2D0,
 0x2238B476:0x22060D30,0x2238B481:0x22073E00,
 0x2238B493:0x22060D30,0x2238B49A:0x220330C0,
 0x2238B4A5:0x22060D30,0x2238B4CD:0x2204BC20,
 0x2238B4D4:0x22060D30,0x2238B4F3:0x22060D30,
 0x2238B506:0x22060D30,0x2238B511:0x22077E60,
 0x2238B51A:0x22060D30,0x2238B521:0x2203D2D0,
 0x2238B539:0x22060D30,0x2238B551:0x22075AB0,
 0x2238B55E:0x22060D30,0x2238B569:0x22073E00,
 0x2238B572:0x22060D30,0x2238B579:0x2203D2D0,
 0x2238B589:0x22060D30,
 0x2236462C:0x2238B3C0, # previous authentic Jetpack override incoming call
 0x22364638:0x2235BFD0, # parent Grunt fallback on false
}
WITNESSES={
 0x2238B3C0:'83ec3c',0x2238B3CB:'83c0f7',
 0x2238B3CE:'83f833',0x2238B3D4:'0f8787000000',
 0x2238B3DA:'0fb680bcb53822',0x2238B3E1:'ff2485a8b53822',
 0x2238B3E8:'8b86500c0000',0x2238B3F1:'8dbe500c0000',
 0x2238B401:'81e1ff0f0000',0x2238B41E:'ff9054010000',
 0x2238B424:'85c0',0x2238B44B:'680000803f',
 0x2238B452:'ff9770070000',0x2238B458:'6a00',
 0x2238B461:'5f5e32c05b',0x2238B469:'c20400',
 0x2238B46C:'81c6500c0000',0x2238B472:'6a07',
 0x2238B486:'e98b000000',0x2238B48B:'81c6500c0000',
 0x2238B516:'6a00',0x2238B528:'b001',
 0x2238B52E:'c20400',0x2238B531:'8dbe500c0000',
 0x2238B59C:'b001',0x2238B5A2:'c20400',
}

class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  nt=struct.unpack_from('<I',data,0x3C)[0];assert data[nt:nt+4]==b'PE\0\0'
  opt=nt+24;assert struct.unpack_from('<H',data,opt)[0]==0x10B
  self.base=struct.unpack_from('<I',data,opt+28)[0];self.data=data
  sec=opt+struct.unpack_from('<H',data,nt+20)[0];self.sections=[]
  for i in range(struct.unpack_from('<H',data,nt+6)[0]):
   o=sec+40*i;name=data[o:o+8].split(b'\0')[0];vsize,rva,rsize,off=struct.unpack_from('<4I',data,o+8)
   self.sections.append((name,self.base+rva,self.base+rva+min(vsize,rsize),off))
 def read(self,va,n):
  for name,start,end,off in self.sections:
   if start<=va and va+n<=end:return self.data[off+va-start:off+va-start+n]
  raise AssertionError(f'unmapped PE address {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]

def direct(pe,site,target):
 b=pe.read(site,5)
 assert b[0]==0xE8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'call {site:#x} drift'

def audit(client,server):
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA
 p=PE(server);assert p.base==0x22000000
 for va,length,sha in [METHOD,PHASE590]:
  assert hashlib.sha256(p.read(va,length)).hexdigest()==sha,f'original method changed {va:#x}'
 # Unlike phase590, this original method is IMMEDIATELY ADJACENT to next
 # method (0x2238B5F0), with no INT3 padding at its boundary.
 assert p.read(0x2238B5F0,6)==bytes.fromhex('83ec208b4424')
 assert tuple(p.u32(0x2238B5A8+4*i) for i in range(5))==CASEPTRS
 table=p.read(0x2238B5BC,52)
 assert len(table)==52 and all(v<=4 for v in table)
 assert {i+9:int(v) for i,v in enumerate(table) if v!=4}==SPECIAL
 assert sum(v==4 for v in table)==46
 assert 0x54>0x3C # original mode bound: this event returns false
 for site,dst in DIRECT.items():direct(p,site,dst)
 for va,hx in WITNESSES.items():
  raw=bytes.fromhex(hx)
  assert p.read(va,len(raw))==raw,f'native witness drift {va:#x}'
 assert p.read(0x2238B461,5)==bytes.fromhex('5f5e32c05b') # AL=0
 assert p.read(0x2238B528,2)==bytes.fromhex('b001') # AL=1
 return dict(new_full_original_methods=1,new_native_bytes=560,embedded_table_bytes=20+52,verified_call_edges=len(DIRECT),opcode_witnesses=len(WITNESSES),special_events=len(SPECIAL),default_in_range_events=46,event_0x54_filter_return=False)

def selftest():
 class Fake:
  def __init__(self,a,b):self.a,self.b=a,b
  def read(self,a,n):return self.b[a-self.a:a-self.a+n]
 for site,target in DIRECT.items():
  orig=b'\xe8'+struct.pack('<i',target-site-5)
  direct(Fake(site,orig),site,target)
  for i in range(5):
   mutated=bytearray(orig);mutated[i]^=1
   try:direct(Fake(site,mutated),site,target)
   except AssertionError:pass
   else:raise AssertionError('mutated E8 accepted')
 try:audit(b'fake',b'fake')
 except AssertionError:pass
 else:raise AssertionError('fake DLL pair accepted')
 return 'PHASE591_ORIGINAL_FREE_FILTER_MUTATIONS_REFUSED'

def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--self-test',action='store_true');a.add_argument('--bin-zip',type=Path)
 opts=a.parse_args()
 if not opts.self_test and not opts.bin_zip:a.error('need --self-test or --bin-zip')
 if opts.self_test:print(selftest())
 if opts.bin_zip:
  with zipfile.ZipFile(opts.bin_zip) as z:print('SIN_ORIGINAL_FLYING_EVENT_FILTER_PHASE591',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()