#!/usr/bin/env python3
"""Read-only, SHA-pinned original 2006 SiN Episodes client CVoiceStatus hash audit.

No DLL, disassembly dump or executable original bytes are redistributed.
Usage: python audit_sin_original_voice_block_hash.py --bin-zip /private/bin.zip
       python audit_sin_original_voice_block_hash.py --self-test
"""
import argparse
import hashlib
import struct
import zipfile
from dataclasses import dataclass

CLIENT_HASH='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_HASH='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

# Native methods are code intervals excluding all following CC alignment padding.
# Roles are descriptive, not original Ritual debug symbol names.
METHODS={
  0x241E5B20:(213,'af3cd7e9683f955e486e00b59b8a68b26e4d177719c0d583d36ffae2c48a5ac0','CVoiceStatus::SetPlayerBlockedState'),
  0x241E4F00:(19, '21c34399b2db440e778a7789b9c56ad818f816293ceb9443b681d1be78ff057a','Key presence bool adapter'),
  0x241E4F20:(83, '40cef3e9c91a2e90d37c45eb9f9a8fc91df0d277060d791a2106a6a3465203c9','Key membership insertion/removal'),
  0x241E4DD0:(95, 'e33a2b3f9595fc338753845bda1c5dbe2b52402f7394ad612be75a325e00846a','Hash and 32-byte key lookup'),
  0x241E4E30:(115,'fb2136d9b01e84421fdbfcc6c1bffb0d1ea82782b0a110cd98afac0fa918290e','Hash and append a key record'),
  0x241E4D90:(60, '7f2dfba82edd5099c5c57210b01203a3e1273e5e270158d87ff169c8c40c6b48','Initialize 256 sentinels'),
  0x241E4EC0:(60, 'e2950b9199e9d10df9c9511319baec6fff7d6e9903e146b28317a792034abba7','Free all key entries and reinitialize'),
  0x241E4EB0:(12, '1212a5e5a78c9175e875d348db77f508999f0f7858308c4c28dedb3d0fbf13c8','Constructor wrapper'),
}
# rel32 direct callsites, checked independently of the whole-function digest.
CALLS={
 0x241E5BA7:0x241E4F00,
 0x241E5BC9:0x241E4F00,
 0x241E5BDB:0x241E4F20,
 0x241E5BE4:0x241E5880, # Prior Phase550 complete method
 0x241E4F05:0x241E4DD0,
 0x241E4F30:0x241E4DD0,
 0x241E4F3C:0x241E4E30,
 0x241E4F4B:0x241E4DD0,
 0x241E4F67:0x2416FD60, # Original allocator/free helper
 0x241E4E36:0x2416FCF0, # Original allocator helper
 0x241E4EDB:0x2416FD60,
 0x241E4EB3:0x241E4D90,
}
JUMPS={0x241E4EF7:0x241E4D90}
# Literal native bytes at exact addresses for important branch/size witnesses.
# Instruction witnesses aren't the whole original code. The full SHA digests above
# are the integrity boundary for the complete original native method.
WITNESSES={
 0x241E5B20:'a1c0863724',     # logger global
 0x241E5B5B:'ff5220',       # engine virtual +0x20 player data getter
 0x241E5B7C:'8d47ff',       # player index - 1
 0x241E5B81:'83e11f',       # bit index and 31
 0x241E5B84:'ba01000000',   # mask 1
 0x241E5B8B:'c1f805',       # signed sar 5
 0x241E5B8E:'85548614',     # this+0x14 mask array
 0x241E5B9F:'8d4c2430',     # identity key slice of temporary player data
 0x241E5BAC:'f6d8',       # negate AL -> bool complement
 0x241E5BD0:'0f94c0',     # sete AL for !present
 0x241E5BE0:'6a00',       # forced=0 for native UpdateServerState
 0x241E5BF2:'c20800',     # thiscall ret 8 (two 4B stack arguments)
 0x241E4F05:'e8c6feffff', # exact wrapper call
 0x241E4F0A:'f7d8',       # neg eax; sbb eax,eax; neg eax (bool)
 0x241E4F10:'c20400',
 0x241E4F20:'807c240800', # read Boolean second parameter
 0x241E4F37:'7508',
 0x241E4F42:'5e',
 0x241E4F5A:'895124',     # prev/next list splice
 0x241E4F64:'895120',
 0x241E4F70:'c20800',
 0x241E4DD0:'53',
 0x241E4DDC:'8d4502',
 0x241E4DDF:'be08000000',  # exactly eight 32-bit chunks
 0x241E4DFA:'0fb6c2',     # unsigned byte hash
 0x241E4DFD:'8d1480',     # x5, then x8 -> 40-byte stride
 0x241E4E00:'8b44d724',   # bucket +0x24 first linked element
 0x241E4E10:'b908000000',  # compare 8 32-bit chunks
 0x241E4E1B:'f3a7',       # repe cmps dword
 0x241E4E2C:'c20400',
 0x241E4E32:'6a28',       # 40-byte allocation
 0x241E4E54:'bf08000000', # eight 4-byte chunks
 0x241E4E81:'b908000000',
 0x241E4E86:'f3a5',       # rep movs dword =32-byte key
 0x241E4E88:'894524',     # new.next = bucket
 0x241E4EA0:'c20400',
 0x241E4D93:'ba40000000', # 64 * 4 buckets
 0x241E4DC1:'05a0000000', # 160 bytes for 4 buckets
 0x241E4DCB:'c3',
 0x241E4EB8:'8bc6',
 0x241E4EBC:'cc',
 0x241E4EC8:'bb00010000', # 256 sentinels
 0x241E4EE9:'83c728',     # 40-byte stride
 0x241E4EF7:'e994feffff', # tail jump to reinitializer
}
STRINGS={
 0x242AA108:b'CVoiceStatus::SetPlayerBlockedState part 1\n',
 0x242AA0DC:b'CVoiceStatus::SetPlayerBlockedState part 2\n',
 0x242AA098:b'CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d\n',
}

@dataclass
class PE32:
 data:bytes
 def __post_init__(self):
  d=self.data
  if d[:2]!=b'MZ':raise ValueError('not MZ')
  offset=struct.unpack_from('<I',d,0x3c)[0]
  if d[offset:offset+4]!=b'PE\0\0':raise ValueError('not PE')
  if struct.unpack_from('<H',d,offset+4)[0]!=0x14c: raise ValueError('not PE32 x86')
  opt=offset+24
  if struct.unpack_from('<H',d,opt)[0]!=0x10b: raise ValueError('not PE32 optional')
  self.base=struct.unpack_from('<I',d,opt+28)[0]
  count=struct.unpack_from('<H',d,offset+6)[0]
  secoff=opt+struct.unpack_from('<H',d,offset+20)[0]
  self.sections=[]
  for i in range(count):
   p=secoff+40*i
   name=d[p:p+8].split(b'\0')[0]
   vsize,rva,rawsize,rawoff=struct.unpack_from('<IIII',d,p+8)
   self.sections.append((name,self.base+rva,vsize,rawsize,rawoff))
 def read(self,addr,size):
  for name,base,vsize,rawsize,offset in self.sections:
   if base<=addr and addr+size<=base+rawsize:
    return self.data[offset+(addr-base):offset+(addr-base)+size]
  raise ValueError(f'VA {addr:#x} size {size} not mapped in raw PE')
 def section(self,addr):
  for name,base,vsize,rawsize,offset in self.sections:
   if base<=addr<base+rawsize:return name
  return None


def check_edge(pe, site, target, opcode):
 b=pe.read(site,5)
 if b[0]!=opcode or site+5+struct.unpack('<i',b[1:])[0]!=target:
  raise AssertionError(f'wrong rel32 edge {site:#x} -> {target:#x} (opcode {opcode:#x})')

def audit(client,server):
 if hashlib.sha256(client).hexdigest()!=CLIENT_HASH or hashlib.sha256(server).hexdigest()!=SERVER_HASH:
  raise AssertionError('original pair SHA256 mismatch; refusing unsupported source')
 pe=PE32(client)
 if pe.base!=0x24000000:raise AssertionError('client image base mismatch')
 total=0
 intervals=[]
 for va,(n,sha,role) in METHODS.items():
  if pe.section(va)!=b'.text':raise AssertionError(f'non-text function {va:#x}')
  assert hashlib.sha256(pe.read(va,n)).hexdigest()==sha, f'{role} wrong method digest'
  assert pe.read(va+n,1)==b'\xcc',f'{role} missing next INT3 at {va+n:#x}'
  intervals.append((va,va+n))
  total+=n
 intervals.sort()
 assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:])), 'function bytes overlap'
 for site,target in CALLS.items():check_edge(pe,site,target,0xe8)
 for site,target in JUMPS.items():check_edge(pe,site,target,0xe9)
 for site,hx in WITNESSES.items():
  if pe.read(site,len(bytes.fromhex(hx)))!=bytes.fromhex(hx):
   raise AssertionError(f'native witness differs at {site:#x}')
 for site,literal in STRINGS.items():
  if pe.read(site,len(literal)+1)!=literal+b'\0':raise AssertionError(f'original literal missing at {site:#x}')
 assert total==657,(total,'unexpected total bytes')
 return {'methods':len(METHODS),'full_original_method_bytes':total,
         'direct_calls':len(CALLS),'direct_jumps':len(JUMPS),
         'instruction_witnesses':len(WITNESSES),'literal_strings':len(STRINGS)}

def selftest():
 import tempfile
 assert sum(n for n,_,_ in METHODS.values())==657
 assert len(set(CALLS))==len(CALLS)
 # Synthetic direct-rel32 edge test, each byte mutation is rejected.
 class Synthetic:
  def __init__(self,blob,base):self.blob,self.base=blob,base
  def read(self,addr,n):return self.blob[addr-self.base:addr-self.base+n]
 for opcode,site,target in [(0xe8,0x1000,0x2780),(0xe9,0x1000,0x0fff)]:
  good=bytes([opcode])+struct.pack('<i',target-(site+5))
  check_edge(Synthetic(good,site),site,target,opcode)
  for i in range(5):
   corrupted=bytearray(good);corrupted[i]^=0x1
   try:check_edge(Synthetic(corrupted,site),site,target,opcode)
   except AssertionError:pass
   else:raise AssertionError(f'bad mutation accepted at {i}')
 try:audit(b'MZfake',b'not-server')
 except AssertionError:pass
 else:raise AssertionError('bad whole-DLL SHA accepted')
 return 'SELF_TEST_OK'

def main():
 p=argparse.ArgumentParser();p.add_argument('--bin-zip');p.add_argument('--self-test',action='store_true');args=p.parse_args()
 if args.self_test:print(selftest())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   result=audit(z.read('client.dll'),z.read('server.dll'))
  print('ORIGINAL_CLIENT_VOICE_BLOCK_HASH_AUTHENTICATED', result)
 if not args.self_test and not args.bin_zip:p.error('specify --self-test and/or --bin-zip')
if __name__=='__main__':main()