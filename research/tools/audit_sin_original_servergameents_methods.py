#!/usr/bin/env python3
"""Original-image witness auditor for SiN ServerGameEnts001::MarkEntitiesAsTouching/CheckTransmit.

Owner-local binary only. Outputs metadata, not original executable bytes.
"""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
VTABLE=0x22443030
# Full verified method spans and the immediate helper chain; no double counting.
REGIONS=(
 (0x221e9380,299,'8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1','MarkEntitiesAsTouching'),
 (0x221e94d0,354,'e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176','CheckTransmit'),
 (0x22285a10,45,'35e66fcdbd33a37b6440a98ed9b363478da2322bc6fc388461ddb5e0edd2a28d','TraceInitializer'),
 (0x22221080,106,'7b7c2ab2d13044215c84ceac8a39306ea31cb1e17f142be6ce2e64771252b5d3','PairContactRegistration'),
 (0x2221f350,526,'bbdebe8fad62ff42eb88b6b282addbd23be90161aca0a7fd5aec4a814ced8dff','DirectedContactRegistration'),
 (0x2206e2c0,128,'383458ce02ffe087f4aa8fb8a3d4d68a761eeae74a77673b6e259949dad6f47d','GlobalTracePrefixCopy'),
)
# The witnesses are disjoint local instructions at behavioral branch points,
# not a pasted commercial disassembly. Full-method SHA checks also apply.
WITNESSES=(
 (0x221e9396,'ff5014','first edict networkable virtual +14'),
 (0x221e93b0,'ff5214','second edict networkable virtual +14'),
 (0x221e93d3,'8b8eb8000000','second entity dirty-origin state +b8'),
 (0x221e93eb,'8b97b8000000','first entity dirty-origin state +b8'),
 (0x221e9400,'d987bc020000','first entity X +2bc'),
 (0x221e9406,'d886bc020000','second entity X +2bc'),
 (0x221e944c,'d80d78973d22','X midpoint multiplier 0.5'),
 (0x221e9462,'d80d78973d22','Y midpoint multiplier 0.5'),
 (0x221e9481,'d80d78973d22','Z midpoint multiplier 0.5'),
 (0x22221089,'b9f84a5722','global trace prefix copy destination'),
 (0x2206e31c,'8b512c','global copy includes trace +2c'),
 (0x2206e33d,'c20400','global prefix copy return'),
 (0x22285a17,'b915000000','trace initializer 21 dwords'),
 (0x22285a1e,'f3ab','trace initializer rep stosd'),
 (0x22285a2e,'c7422c0000803f','trace +2c fraction equals 1'),
 (0x221e94d3,'8b0d98215722','global owner +4c dispatcher'),
 (0x221e94de,'ff504c','recipient lookup virtual +4c'),
 (0x221e9548,'0fb70c41','16-bit transmit candidate index'),
 (0x221e9553,'8d2c90','20-byte recipient entry stride'),
 (0x221e9559,'83e038','transmit flag mask 0x38'),
 (0x221e9564,'8bbb08200000','selected bitset at context+2008'),
 (0x221e9594,'8b830c200000','always bitset at context+200c'),
 (0x221e95ab,'ff5254','ShouldTransmit virtual +54'),
 (0x221e95e1,'ff525c','SetTransmit virtual +5c path 1'),
 (0x221e95f8,'ff525c','SetTransmit virtual +5c path 2'),
 (0x221e9610,'ff505c','SetTransmit virtual +5c path 3'),
)
DIRECT_CALLS=(
 (0x221e93ce,0x22285a10,'initialize trace before both origin refreshes'),
 (0x221e93e6,0x2213fa60,'refresh second origin'),
 (0x221e93fb,0x2213fa60,'refresh first origin'),
 (0x221e949e,0x22221080,'submit pair contact'),
 (0x2222108e,0x2206e2c0,'copy first 0x38 trace bytes to global state'),
 (0x222210d8,0x2221f350,'first directed contact registration'),
 (0x222210e0,0x2221f350,'reverse directed contact registration'),
 (0x221e9600,0x2227ee90,'CheckTransmit PVS test helper'),
)
class Refused(ValueError):pass
def require(ok,message):
 if not ok: raise Refused(message)
class PE32:
 def __init__(self,data):
  self.data=data
  require(len(data)>0x100 and data[:2]==b'MZ','DOS header')
  loc=struct.unpack_from('<I',data,0x3c)[0]
  require(loc+24<len(data) and data[loc:loc+4]==b'PE\x00\x00','NT header')
  coff=loc+4
  require(struct.unpack_from('<H',data,coff)[0]==0x14c,'not x86')
  n=struct.unpack_from('<H',data,coff+2)[0]
  opt=coff+20
  sz=struct.unpack_from('<H',data,coff+16)[0]
  require(n>0 and opt+sz+n*40<=len(data),'section headers invalid')
  require(struct.unpack_from('<H',data,opt)[0]==0x10b,'not PE32')
  self.base=struct.unpack_from('<I',data,opt+28)[0]
  self.sections=[]
  for i in range(n):
   off=opt+sz+i*40
   name=data[off:off+8].split(b'\x00')[0].decode('ascii','replace')
   vs,rva,rs,raw=struct.unpack_from('<IIII',data,off+8)
   require(raw+rs<=len(data),'invalid section extent')
   self.sections.append((name,self.base+rva,min(vs,rs),raw))
 def at(self,addr,n):
  for _,start,size,off in self.sections:
   if start<=addr and n>=0 and addr+n<=start+size:
    return self.data[off+addr-start:off+addr-start+n]
  raise Refused('unmapped original address '+hex(addr))
 def u32(self,addr):return struct.unpack('<I',self.at(addr,4))[0]
 def section(self,addr):
  for name,start,size,_ in self.sections:
   if start<=addr<start+size:return name
  return None

def check_call(pe,site,target):
 raw=pe.at(site,5)
 require(raw[0]==0xe8 and site+5+struct.unpack_from('<i',raw,1)[0]==target,
         'relative call target mismatch '+hex(site))

def check_spans(pe,spans):
 for addr,n,digest,name in spans:
  require(pe.section(addr)=='.text','not code: '+name)
  require(hashlib.sha256(pe.at(addr,n)).hexdigest()==digest,
          'original complete function SHA mismatch: '+name)
  require(pe.at(addr+n,1)==b'\xcc','non-INT3 end boundary: '+name)

def verify(data,check_whole=True):
 if check_whole:require(hashlib.sha256(data).hexdigest()==SERVER_SHA,'server.dll SHA mismatch')
 pe=PE32(data)
 require(pe.base==0x22000000,'wrong base')
 require(pe.u32(VTABLE+0x08)==REGIONS[0][0] and
         pe.u32(VTABLE+0x18)==REGIONS[1][0], 'ServerGameEnts method vtable mapping changed')
 check_spans(pe,REGIONS)
 for va,witness,note in WITNESSES:
  require(pe.at(va,len(witness)//2)==bytes.fromhex(witness),note)
 for site,target,note in DIRECT_CALLS:check_call(pe,site,target)
 require(pe.u32(0x223d9778)==0x3f000000,'midpoint constant changed')
 require(pe.at(0x221e94a8,3)==b'\xc2\x08\x00','touch method ret 8 changed')
 require(pe.at(0x221e962f,3)==b'\xc2\x0c\x00','transmit method ret 12 changed')
 return dict(status='ORIGINAL_GAMEENTS_TOUCH_AND_TRANSMIT_FLOW_AUTHENTICATED',
             owner_sha256=SERVER_SHA,
             methods=[dict(name=name,va=hex(va),length=n,sha256=sha)
                      for va,n,sha,name in REGIONS],
             direct_calls=[dict(site=hex(site),target=hex(target),reason=note)
                           for site,target,note in DIRECT_CALLS],
             witness_count=len(WITNESSES),
             limits='Source helper effects and runtime receiver ABI are not yet owned; no retail boot proof')

def self_test():
 class Fake:
  def __init__(self):self.words={0x1000:b'\xe8\x7b\x00\x00\x00',0x2000:b'\x8b\x40\x14\xcc'}
  def at(self,va,n):
   for start,data in self.words.items():
    if start<=va and va+n<=start+len(data):return data[va-start:va-start+n]
   raise Refused('synthetic unmapped')
  def section(self,va):return '.text'
 f=Fake()
 check_call(f,0x1000,0x1080)
 try:check_call(f,0x1000,0x1081)
 except Refused:pass
 else:raise AssertionError('wrong call accepted')
 class V:
  def at(self,va,n):return b'\x90\xc3\xcc'[va-0x1000:va-0x1000+n]
  def section(self,va):return '.text'
 check_spans(V(),[(0x1000,2,hashlib.sha256(b'\x90\xc3').hexdigest(),'synthetic')])
 try:check_spans(V(),[(0x1000,2,hashlib.sha256(b'\x90\x90').hexdigest(),'bad')])
 except Refused:pass
 else:raise AssertionError('tampered synthetic function accepted')

def corruption_test(raw):
 original=PE32(raw)
 for target in (REGIONS[0][0],REGIONS[1][0],REGIONS[2][0],REGIONS[3][0],
                REGIONS[4][0],REGIONS[5][0],VTABLE+0x08,VTABLE+0x18,0x223d9778):
  for _,start,size,off in original.sections:
   if start<=target<start+size:
    changed=bytearray(raw);changed[off+target-start]^=1
    try:verify(bytes(changed),False)
    except Refused:pass
    else:raise AssertionError('original corruption admitted: '+hex(target))
    break

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--server',type=Path,help='owner-local original server.dll')
 parser.add_argument('--self-test',action='store_true')
 args=parser.parse_args()
 if args.self_test:self_test()
 if args.server:
  raw=args.server.read_bytes();result=verify(raw)
  if args.self_test:corruption_test(raw);result['corruption_test']='passed'
  print(json.dumps(result,indent=2,sort_keys=True))
 else:
  require(args.self_test,'provide --server or --self-test')
  print(json.dumps({'status':'SYNTHETIC_GAMEENTS_METHOD_AUDIT_PASSED'}))
if __name__=='__main__':
 try:main()
 except (Refused,OSError,struct.error) as error:raise SystemExit('REFUSED: '+str(error))
