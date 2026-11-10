#!/usr/bin/env python3
"""Original-only phase171 entity create/destroy registry x86 evidence.

Checks original 2006 owner-held client/server PE32 hashes, four complete
server function regions, sparse native opcode witnesses, original direct calls,
registry vtable slots and original warning strings. No original executable
region is embedded in this public module.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

HASHES = {
 'client':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
 'server':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
}
REGIONS={
 'registry_classname_lookup':(0x2228B010,94,'ba7b2d918b3b72235c9457731f50ebcfb3b6f001c699d981c788d7fed8c9abf5'),
 'registry_create_wrapper':(0x22285550,46,'58f62479791e357d5b220d60803a1a72e2cfc003944000c50f667f30730854f1'),
 'registry_destroy_wrapper':(0x22285580,50,'83eb972b13abd551e76ba781d13eed5ab614cd013c0d7f8c133020f7a2b2a5db'),
 'registry_record_accessor':(0x2228AEA0,18,'28b0a8d65b4efb61b7c2be4b21a107fa30757f2208c14612ceb98396691cde47'),
}
# Independent, non-overlapping partial instruction witnesses (not a dump).
OPCODES={
 0x2228B010:'83ec08',0x2228B017:'56',0x2228B019:'8bf9',
 0x2228B020:'8d7704',0x2228B027:'66c7442410ffff',
 0x2228B033:'668b10',0x2228B036:'6a00',0x2228B03D:'8bce',
 0x2228B049:'663dffff',0x2228B04D:'750a',0x2228B050:'33c0',
 0x2228B056:'c20400',0x2228B059:'8b5708',0x2228B05C:'0fb7c8',
 0x2228B05F:'c1e104',0x2228B062:'8b44110c',0x2228B06B:'c20400',
 0x22285550:'8b01',0x22285552:'56',0x22285557:'56',
 0x22285558:'ff500c',0x2228555B:'85c0',0x2228555D:'56',
 0x22285560:'68505f4522',0x22285565:'ff1548213d22',
 0x2228556B:'83c408',0x2228556E:'33c0',0x22285571:'c20400',
 0x22285574:'8b10',0x22285578:'ff12',0x2228557B:'c20400',
 0x22285580:'8b01',0x22285582:'56',0x22285587:'56',
 0x22285588:'ff500c',0x2228558B:'85c0',0x2228558D:'7513',
 0x2228558F:'56',0x22285590:'68805f4522',
 0x22285595:'ff1548213d22',0x2228559B:'83c408',
 0x2228559F:'c20800',0x222855A2:'8b4c240c',0x222855A6:'8b10',
 0x222855AB:'ff5204',0x222855AF:'c20800',
 0x2228AEA0:'0fb7442404',0x2228AEA5:'8b4904',
 0x2228AEA8:'c1e004',0x2228AEAB:'8d44080c',0x2228AEAF:'c20400',
}
CALLS={0x2228B02E:0x2228F7A0,0x2228B044:0x2228A760}
# Registry class vtable emitted in original .rdata at VA 0x22456324.
VTABLE={0x22456328:0x22285550,0x2245632C:0x22285580,
        0x22456330:0x2228B010}
WARNINGS={
 0x22455F50:b'Attempted to create unknown entity type %s!\n\0',
 0x22455F80:b'Attempted to destroy unknown entity type %s!\n\0',
}
class OriginalError(ValueError): pass
class OriginalPE:
 def __init__(self,b):
  self.data=b
  if b[:2]!=b'MZ':raise OriginalError('not MZ')
  pe=struct.unpack_from('<I',b,0x3c)[0]
  if b[pe:pe+4]!=b'PE\0\0':raise OriginalError('not PE32')
  if struct.unpack_from('<H',b,pe+4)[0]!=0x14c:raise OriginalError('not x86')
  optional=pe+24
  if struct.unpack_from('<H',b,optional)[0]!=0x10b:raise OriginalError('not PE32 optional header')
  self.base=struct.unpack_from('<I',b,optional+28)[0]
  count=struct.unpack_from('<H',b,pe+6)[0]
  entry=optional+struct.unpack_from('<H',b,pe+20)[0]
  self.sections=[]
  for i in range(count):
   x=entry+40*i
   self.sections.append((struct.unpack_from('<I',b,x+12)[0],
     max(struct.unpack_from('<I',b,x+8)[0],struct.unpack_from('<I',b,x+16)[0]),
     struct.unpack_from('<I',b,x+20)[0]))
 def at(self,va,length):
  rva=va-self.base
  for start,size,fileoff in self.sections:
   if start<=rva and rva+length<=start+size:
    off=fileoff+rva-start
    if off+length>len(self.data):break
    return self.data[off:off+length]
  raise OriginalError('VA outside backed PE section %#x'%va)

def verify_native(pe,*,check_hashes=True):
 if pe.base!=0x22000000:raise OriginalError('incorrect original server base')
 for va,rawhex in OPCODES.items():
  raw=bytes.fromhex(rawhex)
  if pe.at(va,len(raw))!=raw:raise OriginalError('opcode mismatch %#x'%va)
 for site,target in CALLS.items():
  if pe.at(site,5)!=bytes((0xe8,))+struct.pack('<i',target-site-5):
   raise OriginalError('CALL rel32 mismatch %#x'%site)
 for va,target in VTABLE.items():
  if pe.at(va,4)!=struct.pack('<I',target):raise OriginalError('vtable mismatch %#x'%va)
 for va,raw in WARNINGS.items():
  if pe.at(va,len(raw))!=raw:raise OriginalError('diagnostic literal mismatch %#x'%va)
 if check_hashes:
  for name,(va,size,expect) in REGIONS.items():
   if hashlib.sha256(pe.at(va,size)).hexdigest()!=expect:
    raise OriginalError('whole-function SHA mismatch '+name)
 return {'whole_original_function_regions':len(REGIONS) if check_hashes else 0,
         'native_opcode_witnesses':len(OPCODES),'native_direct_calls':len(CALLS),
         'native_vtable_slots':len(VTABLE),'native_diagnostic_literals':len(WARNINGS)}

def audit(client,server):
 for name,b in [('client',client),('server',server)]:
  if hashlib.sha256(b).hexdigest()!=HASHES[name]:
   raise OriginalError('original %s.dll file SHA256 mismatch'%name)
 return verify_native(OriginalPE(server))

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--client',type=Path,required=True)
 ap.add_argument('--server',type=Path,required=True)
 ap.add_argument('--output',type=Path)
 a=ap.parse_args()
 result=json.dumps(audit(a.client.read_bytes(),a.server.read_bytes()),indent=2)+'\n'
 if a.output:
  a.output.parent.mkdir(parents=True,exist_ok=True)
  a.output.write_text(result)
 else:print(result,end='')
if __name__=='__main__':main()