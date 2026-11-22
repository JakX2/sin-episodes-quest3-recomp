#!/usr/bin/env python3
"""Read-only SiN 2006 Grunt animation parent/optional component filter authentication.

The commercial binaries remain private. Only independent hashes, call witnesses,
small original field/instruction patterns, and synthetic regressions are exported.
"""
import argparse
import hashlib
import re
import struct
import zipfile
from pathlib import Path

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
METHODS = {
  0x2235BFD0: (3340,'b927291398dd19d73f43fb0e2fbb842926b6008b90f0292645936ab5fafceebe'),
  0x2202D930: (72,'969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2'),
}
GRUNT_TABLE = 0x2235CC40
GRUNT_PTRS = 0x2235CBDC
CASES = (
  0x2235C14B,0x2235C278,0x2235C41D,0x2235C58A,0x2235C465,
  0x2235C287,0x2235C294,0x2235C03A,0x2235C39D,0x2235C025,
  0x2235C610,0x2235C522,0x2235C6F1,0x2235C734,0x2235C767,
  0x2235C82B,0x2235C8BC,0x2235CA87,0x2235CB37,0x2235CB67,
  0x2235C331,0x2235CB90,0x2235C012,0x2235CBBB,0x2235C27F,
)
# Only genuine non-default entries in original 156-byte table.
SPECIAL = {
  0x03:0,0x2B:1,0x2C:1,0x38:2,0x3C:2,0x43:3,0x62:4,
  0x8B:5,0x8C:6,0x8D:7,0x8E:8,0x8F:9,0x90:10,
  0x92:11,0x93:12,0x94:13,0x95:14,0x96:15,0x97:16,
  0x98:17,0x99:18,0x9A:19,0x9B:20,0x9C:21,0x9D:22,0x9E:23,
}
# Direct x86 E8 callsites in the original parent method and 1 in intermediary.
DIRECT = {  # all 100 verified 5-byte original E8 encodings
  0x2235BFE7:0x22072100,
  0x2235C028:0x22359720,
  0x2235C04A:0x220A9480,
  0x2235C058:0x2202E310,
  0x2235C06B:0x22054D40,
  0x2235C0B9:0x220092D0,
  0x2235C0C6:0x221B8230,
  0x2235C0CD:0x22027DB0,
  0x2235C125:0x22049C20,
  0x2235C139:0x22049C20,
  0x2235C177:0x22031A80,
  0x2235C1E9:0x220A35C0,
  0x2235C1F7:0x220325C0,
  0x2235C24A:0x220A35C0,
  0x2235C264:0x220325C0,
  0x2235C282:0x2202D930,
  0x2235C2A0:0x220A9480,
  0x2235C2E3:0x22027DB0,
  0x2235C2EA:0x22027D60,
  0x2235C316:0x2203D190,
  0x2235C31F:0x2203D2D0,
  0x2235C342:0x220A9300,
  0x2235C35F:0x223A5BAB,
  0x2235C37E:0x220A93C0,
  0x2235C38B:0x2203D2D0,
  0x2235C3AE:0x220A9300,
  0x2235C3CF:0x223A5BAB,
  0x2235C3DD:0x22357FC0,
  0x2235C3FE:0x220A93C0,
  0x2235C40B:0x2203D2D0,
  0x2235C426:0x220397C0,
  0x2235C436:0x2202D930,
  0x2235C453:0x22049C20,
  0x2235C4B6:0x220A9720,
  0x2235C4D0:0x220A35C0,
  0x2235C4E5:0x22139F60,
  0x2235C4F4:0x22139F60,
  0x2235C510:0x2203D2D0,
  0x2235C524:0x222AA640,
  0x2235C52F:0x222AA640,
  0x2235C546:0x222AA640,
  0x2235C57E:0x2203D190,
  0x2235C58C:0x222AA640,
  0x2235C597:0x222AA640,
  0x2235C5AE:0x222AA640,
  0x2235C5E6:0x2203D190,
  0x2235C5EF:0x2204E590,
  0x2235C61B:0x221F8610,
  0x2235C62C:0x220295F0,
  0x2235C637:0x22028F50,
  0x2235C64D:0x220303F0,
  0x2235C666:0x220337A0,
  0x2235C670:0x22033740,
  0x2235C6B0:0x2202E6E0,
  0x2235C6DF:0x2203D2D0,
  0x2235C6FC:0x220398A0,
  0x2235C705:0x220398A0,
  0x2235C718:0x22358570,
  0x2235C742:0x2235A0F0,
  0x2235C755:0x2203D2D0,
  0x2235C769:0x220330C0,
  0x2235C77A:0x220330C0,
  0x2235C7A2:0x220330C0,
  0x2235C7A9:0x220092D0,
  0x2235C7B3:0x2204C660,
  0x2235C7C1:0x220330C0,
  0x2235C850:0x220092D0,
  0x2235C864:0x220092D0,
  0x2235C86B:0x22027DB0,
  0x2235C89B:0x2228B220,
  0x2235C8AA:0x2203D2D0,
  0x2235C8D3:0x220A9300,
  0x2235C8EF:0x223A5BAB,
  0x2235C914:0x220A93C0,
  0x2235C92D:0x220A8FC0,
  0x2235C94B:0x223A5BAB,
  0x2235C95F:0x220092D0,
  0x2235C967:0x220092D0,
  0x2235C96E:0x220ADAD0,
  0x2235C9BD:0x220A9300,
  0x2235C9DF:0x223A5BAB,
  0x2235CA0E:0x220092D0,
  0x2235CA16:0x220092D0,
  0x2235CA1D:0x220ADAD0,
  0x2235CA3C:0x220A93C0,
  0x2235CA4A:0x2203D2D0,
  0x2235CA6C:0x22358540,
  0x2235CA75:0x2203D2D0,
  0x2235CAC2:0x220092D0,
  0x2235CACA:0x220AB4E0,
  0x2235CAFA:0x2202EB00,
  0x2235CB25:0x2203D2D0,
  0x2235CB4C:0x220A35C0,
  0x2235CB55:0x2203D2D0,
  0x2235CB7E:0x2204E590,
  0x2235CB98:0x22028FB0,
  0x2235CBA0:0x2203E740,
  0x2235CBA9:0x2203D2D0,
  0x2235CBC9:0x2203D2D0,
  0x2202D95D:0x22050150,
}
INCOMING = {0x22364638:0x2235BFD0,0x22355519:0x2235BFD0}
WITNESSES = {
  0x2235BFD0:'81ec98000000',0x2235BFDC:'680000403f',
  0x2235BFF3:'8b4d00',0x2235BFF6:'8d41fd',
  0x2235BFF9:'3d9b000000',0x2235BFFE:'0f877b020000',
  0x2235C004:'0fb68040cc3522',0x2235C00B:'ff2485dccb3522',
  0x2235C278:'83a688090000fd',0x2235C27F:'8bce55',
  0x2235CBD8:'c20400',0x2235CBDB:'90',
  0x2202D930:'538b5c2408',0x2202D935:'833b43',
  0x2202D93E:'8bb7180c0000',0x2202D94D:'c6460801',
  0x2202D951:'ff5060',0x2202D954:'807e0800',
  0x2202D95B:'8bcf',0x2202D968:'8b17',
  0x2202D96D:'ff9280070000',0x2202D975:'c20400',
  0x22050150:'81ec40070000',0x22050163:'8d43ff',
  0x22050166:'3d89000000',0x22050173:'ff2485142d0522',
  0x2205259D:'d94704',0x220525AD:'80cc0c',
  0x220525B8:'db5c2414',0x220525C5:'ff92a8050000',
  0x2235AE3C:'83e801',0x2235AE47:'8b4824',
  0x2235AE4F:'d81dfc8e3f22',0x2235AE57:'f6c441',
  0x2235AE60:'ff92fc070000',
}
class PE:
 def __init__(self,data):
  assert data[:2]==b'MZ'
  pe=struct.unpack_from('<I',data,0x3c)[0];assert data[pe:pe+4]==b'PE\0\0'
  opt=pe+24;assert struct.unpack_from('<H',data,opt)[0]==0x10b
  self.base=struct.unpack_from('<I',data,opt+28)[0];self.data=data;self.sections=[]
  sec=opt+struct.unpack_from('<H',data,pe+20)[0]
  for i in range(struct.unpack_from('<H',data,pe+6)[0]):
   o=sec+40*i;name=data[o:o+8].split(b'\0')[0]
   vs,rv,rs,off=struct.unpack_from('<4I',data,o+8)
   self.sections.append((name,self.base+rv,self.base+rv+min(vs,rs),off))
 def read(self,va,n):
  for name,start,end,off in self.sections:
   if start<=va and va+n<=end:return self.data[off+va-start:off+va-start+n]
  raise AssertionError(f'unmapped original PE {va:#x} ({n} bytes)')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]

def check_edge(pe,site,target):
 b=pe.read(site,5)
 assert len(b)==5 and b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'E8 edge changed {site:#x}'

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'wrong original client.dll'
 assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'wrong original server.dll'
 pe=PE(server);assert pe.base==0x22000000
 for va,(count,sha) in METHODS.items():
  assert hashlib.sha256(pe.read(va,count)).hexdigest()==sha,f'whole method SHA mismatch {va:#x}'
 assert pe.read(0x2235CCDC,4)==b'\xcc'*4
 assert pe.read(0x2235CCE0,6)==bytes.fromhex('83ec6056578b')
 assert pe.read(0x2202D978,8)==b'\xcc'*8
 assert tuple(pe.u32(GRUNT_PTRS+4*i) for i in range(25))==CASES
 table=pe.read(GRUNT_TABLE,156)
 assert len(table)==156 and all(0<=x<=24 for x in table)
 assert {i+3:int(v) for i,v in enumerate(table) if v!=24}==SPECIAL
 assert sum(x==24 for x in table)==130
 assert table[0x54-3]==24 and pe.u32(GRUNT_PTRS+4*24)==0x2235C27F
 for site,target in {**DIRECT,**INCOMING}.items():check_edge(pe,site,target)
 for va,hexbytes in WITNESSES.items():
  original=bytes.fromhex(hexbytes)
  assert pe.read(va,len(original))==original,f'instruction witness mismatch {va:#x}'
 # Authentic chain: shared NPC 0x54 event -> Grunt virtual+0x5A8 -> route distance >240.
 assert pe.u32(0x22052D14 + 4*(0x54-1))==0x2205259D
 assert pe.u32(0x2249AAA4+0x444)==0x2235BFD0
 assert pe.u32(0x2249AAA4+0x5A8)==0x2235AE30
 assert pe.read(0x223F8EFC,4)==struct.pack('<f',240.0)
 return {'new_complete_original_methods':2,'new_distinct_native_bytes':3412,'embedded_parent_tables_bytes':256,
         'original_direct_calls':len(DIRECT),'incoming_parent_calls':len(INCOMING),
         'original_opcode_field_witnesses':len(WITNESSES),'specific_event_ids':len(SPECIAL),
         'fallback_in_range_event_ids':130,'event_0x54_uses_intermediary':True}

def selftest():
 class Fake:
  def __init__(self,va,data):self.va,self.data=va,data
  def read(self,va,n):return self.data[va-self.va:va-self.va+n]
 for site,target in {**DIRECT,**INCOMING}.items():
  src=b'\xe8'+struct.pack('<i',target-site-5)
  check_edge(Fake(site,src),site,target)
  for i in range(5):
   bad=bytearray(src);bad[i]^=1
   try:check_edge(Fake(site,bad),site,target)
   except AssertionError:pass
   else:raise AssertionError('E8 mutation incorrectly accepted')
 try:audit(b'fake',b'fake')
 except AssertionError:pass
 else:raise AssertionError('fake original DLL pair accepted')
 return 'PHASE592_ORIGINAL_FREE_100_E8_CALL_MUTATIONS_REFUSED'

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--self-test',action='store_true');ap.add_argument('--bin-zip',type=Path)
 args=ap.parse_args()
 if not args.self_test and not args.bin_zip:ap.error('specify --self-test or --bin-zip')
 if args.self_test:print(selftest())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as f:
   print('SIN_ORIGINAL_GRUNT_PARENT_PHASE592',audit(f.read('client.dll'),f.read('server.dll')))
if __name__=='__main__':main()