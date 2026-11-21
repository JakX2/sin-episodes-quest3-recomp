#!/usr/bin/env python3
"""Read-only authentication of SiN 2006 linked-group bitmap cache and bit access.

Original PE32 files are private opt-in inputs and are NOT redistributed.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32,CLIENT_HASH,SERVER_HASH,edge

METHODS={
 0x220AA920:(733,'dee90dc641af8bb1a7e92c8a42118dd2eb8238d79e8f17bdd2c8a8c25a28d7ed','group bitmap lookup and 30-second refresh'),
 0x220AAC00:(38,'5e08de620d899200b2a2495bceebe81b68716af275e81f308adc638d7375fb1c','group per-entity bit set'),
 0x220AAC30:(40,'26a74fbb563d364c4467942e6ebfb3b932cfe3af225cd0edfa2e313eabebd670','group per-entity bit clear'),
 0x220AAE80:(44,'f101866ac1dd4c33ab4adbb637f7eb21fd611a15e759ddf42b729434428dbdc0','group per-entity bit membership'),
}
CALLS={
 0x220AA9CB:0x220A9A00,0x220AA9E0:0x220AA8C0,
 0x220AAA62:0x220A9A00,0x220AAAA2:0x2226E3E0,
 0x220AAB5D:0x220A8460,0x220AAB87:0x223A5D10,
 0x220AAC05:0x220AA920,0x220AAC35:0x220AA920,
 0x220AAE85:0x220AA920,
}
# Phase560's range reservation repeatedly calls the same original 733-byte
# cache/bitset accessor; recheck caller edges without recounting caller code.
CROSS_CALLS={
 0x220AAEE3:0x220AA920,0x220AAF1F:0x220AA920,
 0x220AAF54:0x220AA920,0x220AAF75:0x220AA920,
}
WITNESSES={
 0x220AA920:'a190215722', # native engine global
 0x220AA925:'d9400c',     # original time float at engine+0x0c
 0x220AA931:'d85f74',     # fcomp original deadline +0x74
 0x220AA934:'dfe0',       # status word
 0x220AA936:'f6c441',     # x87 unordered/equal/less skip condition bits
 0x220AA939:'0f857d010000', # branch skipping refresh work
 0x220AA941:'39776c',     # record count +0x6c
 0x220AA952:'89775c',     # cached selected ptr reset at +0x5c
 0x220AA983:'8d5f08',     # original group handles +0x08
 0x220AA98D:'8b1520564f22',# original serial entity table
 0x220AA9B2:'ff9054010000',# entity virtual +0x154
 0x220AAA00:'8b4760',    # record vector at +0x60
 0x220AAA03:'8d14f0',    # 8-byte record stride
 0x220AAA6C:'8b4f6c',    # cached record count
 0x220AAA85:'83476cff',  # decrement compacted vector count
 0x220AAAB0:'d9420c',    # time float for updated deadline
 0x220AAAB3:'d805e08f3d22',# add fixed time delta
 0x220AAAB9:'d95f74',    # owner+0x74 deadline store
 0x220AAABC:'8b475c',    # cached record pointer check
 0x220AAB3F:'c7475c00000000', # reset cached index pointer
 0x220AABB8:'742b',      # selected matching owner branch
 0x220AABC8:'89475c',    # update cached record ptr
 0x220AAC10:'c1fa05',    # word index = arithmetic >> 5
 0x220AAC17:'83e11f',    # bit index lower 5
 0x220AAC21:'0910',      # set bit
 0x220AAC43:'8d449004', # word offset +4
 0x220AAC47:'83e11f',    # bit index lower 5
 0x220AAC51:'f7d2',      # inverse mask
 0x220AAC53:'2110',      # clear bit
 0x220AAE90:'c1fa05',    # bit word
 0x220AAE97:'83e11f',    # bit offset
 0x220AAEA1:'2302',      # test bit
}
FLOAT_CONST=0x223D8FE0

def audit(client,server):
  assert hashlib.sha256(client).hexdigest()==CLIENT_HASH
  assert hashlib.sha256(server).hexdigest()==SERVER_HASH
  pe=PE32(server)
  assert pe.base==0x22000000
  regions=[]
  for va,(n,sha,label) in METHODS.items():
    assert pe.section(va)==b'.text',label
    assert hashlib.sha256(pe.read(va,n)).hexdigest()==sha,label
    assert pe.read(va+n,1)==b'\xcc',f'method boundary {va:#x}'
    regions.append((va,va+n))
  regions.sort()
  assert all(a[1]<=b[0] for a,b in zip(regions,regions[1:]))
  for a,b in {**CALLS,**CROSS_CALLS}.items(): edge(pe,a,b)
  for a,h in WITNESSES.items():
    assert pe.read(a,len(bytes.fromhex(h)))==bytes.fromhex(h),f'instruction {a:#x}'
  assert struct.unpack('<f',pe.read(FLOAT_CONST,4))[0]==30.0
  assert sum(n for _,(n,_,_) in METHODS.items())==855
  return dict(new_methods=4,original_server_bytes=855,
              native_calls=len(CALLS),previous_phase_xrefs=len(CROSS_CALLS),
              instruction_witnesses=len(WITNESSES),native_refresh_seconds=30.0)

def self_test():
  class Stub:
    def __init__(self,va,data):self.va,self.data=va,data
    def read(self,addr,n):return self.data[addr-self.va:addr-self.va+n]
  for site,target in {**CALLS,**CROSS_CALLS}.items():
    data=b'\xe8'+struct.pack('<i',target-site-5)
    edge(Stub(site,data),site,target)
    for i in range(5):
      corruption=bytearray(data);corruption[i]^=0x01
      try:edge(Stub(site,corruption),site,target)
      except AssertionError:pass
      else:raise AssertionError('undetected rel32 mutation')
  try:audit(b'MZ bogus client',b'MZ bogus server')
  except AssertionError:pass
  else:raise AssertionError('wrong DLL accepted')
  return 'SELF_TEST_OK'

def main():
  p=argparse.ArgumentParser(description=__doc__)
  p.add_argument('--self-test',action='store_true')
  p.add_argument('--bin-zip',type=Path)
  a=p.parse_args()
  if not(a.self_test or a.bin_zip):p.error('choose --self-test and/or --bin-zip')
  if a.self_test:print(self_test())
  if a.bin_zip:
    with zipfile.ZipFile(a.bin_zip) as z:
      print('SIN_ORIGINAL_GRUNT_GROUP_BITMAP_PHASE561_AUTHENTICATED',
            audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()