#!/usr/bin/env python3
"""Original 2006 SiN waypoint native allocation, splice and pool-release SHA audit.
Reads owner-private bin.zip only; never emits game bytes.
"""
import argparse, hashlib, struct, zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge
METHODS={
0x220B2B10:(184,'572d5112f9b4963c0ff9c6869ecb770f3e47ac4e32befd11f3f17a6ea31f2c94','splice/replacement and terminal flag propagation'),
0x220B2BD0:(27,'135f14e970afe12191b785d331ada22a7dac96d449d1d22ee34f13bd0371a93c','splice-wrapper and set holder.first'),
0x220B2BF0:(137,'fcb01b8df91ea477cf9ff0fec1b47ab00c0c94c2f7b42f277ff7c488570266d4','allocate 0x30-byte waypoint then splice into route'),
0x220B2C80:(65,'dcef9c71e6c6da6acab06761b58518ee4a274554c1644619a416a33c9b8fbd4b','native forward chain detach and per-node pool release'),
0x220B2CD0:(24,'f7ddd3665e1027521969b2b09508780c27535cea6bb4b7095b40038e64c60a05','pointer holder native chain cleanup and clear'),
}
CALLS={
0x220B2B60:0x2226EF40,
0x220B2BDC:0x220B2B10,
0x220B2BFB:0x2226EE80,
0x220B2C55:0x220B2B10,
0x220B2C6A:0x220B2B10,
0x220B2CB5:0x2226EF40,
0x220B2CD8:0x220B2C80,
}
CROSSCALLS={0x220B2CF6:0x220B2C80}
WITNESSES={
0x220B2B10:'53',0x220B2B16:'8d4628',0x220B2B2A:'8b4610',
0x220B2B2D:'836620f7',0x220B2B31:'83f8ff',0x220B2B3B:'3b4110',
0x220B2B40:'8b7928',0x220B2B4A:'89472c',
0x220B2B5B:'b9f4af5422',0x220B2B72:'3bfb',
0x220B2B89:'8b5628',0x220B2B9C:'8b492c',
0x220B2BBA:'83492008',0x220B2BC1:'834e2008',
0x220B2BD0:'56',0x220B2BD3:'8b06',0x220B2BE4:'893e',
0x220B2BF0:'53',0x220B2BF4:'6a30',0x220B2C02:'85d2',
0x220B2C19:'f3ab',0x220B2C37:'897210',
0x220B2C41:'894a24',0x220B2C44:'894220',
0x220B2C47:'c74214000080bf',0x220B2C5D:'8933',
0x220B2C72:'8933',
0x220B2C80:'56',0x220B2C85:'85f6',
0x220B2C90:'8d4628',0x220B2C95:'8b30',
0x220B2C9D:'8b792c',0x220B2CA0:'897a2c',
0x220B2CAA:'8b00',0x220B2CAC:'894228',
0x220B2CB0:'b9f4af5422',0x220B2CBA:'85f6',
0x220B2CD0:'56',0x220B2CD5:'8b06',
0x220B2CE0:'c70600000000',
}
def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'not owner-original client'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'not owner-original server'
 pe=PE32(server);assert pe.base==0x22000000
 spans=[]
 for va,(size,digest,label) in METHODS.items():
  assert pe.section(va)==b'.text',label
  b=pe.read(va,size)
  assert hashlib.sha256(b).hexdigest()==digest,f'whole native function mismatch: {label}'
  assert b[-1]==0xc3 or b[-3:] in (b'\xc2\x04\x00',b'\xc2\x0c\x00'),label
  assert pe.read(va+size,1)==b'\xcc',f'wrong original method boundary {va:#x}'
  spans.append((va,va+size))
 spans.sort();assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
 for site,target in {**CALLS,**CROSSCALLS}.items():edge(pe,site,target)
 for va,hx in WITNESSES.items():
  raw=bytes.fromhex(hx)
  assert pe.read(va,len(raw))==raw,f'instruction witness failure {va:#x}'
 return {'complete_original_functions':len(METHODS),'authenticated_code_bytes':sum(v[0] for v in METHODS.values()),
         'direct_E8_calls':len(CALLS),'incoming_original_E8_calls':len(CROSSCALLS),
         'opcode_witnesses':len(WITNESSES)}
def self_test():
 class PE:
  def __init__(self,va,data):self.va=va;self.data=data
  def read(self,va,n):return self.data[va-self.va:va-self.va+n]
 for site,target in {**CALLS,**CROSSCALLS}.items():
  native=b'\xe8'+struct.pack('<i',target-site-5)
  edge(PE(site,native),site,target)
  for i in range(5):
   mutant=bytearray(native);mutant[i]^=1
   try:edge(PE(site,mutant),site,target)
   except AssertionError:pass
   else:raise AssertionError('corrupted direct call accepted')
 try:audit(b'bad',b'bad')
 except AssertionError:pass
 else:raise AssertionError('wrong original binaries accepted')
 return 'PHASE570_SELF_TEST_OK'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--self-test',action='store_true');p.add_argument('--bin-zip',type=Path);a=p.parse_args()
 if not (a.self_test or a.bin_zip):p.error('use --self-test and/or --bin-zip')
 if a.self_test:print(self_test())
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z: print('PHASE570_ORIGINAL_VERIFIED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()