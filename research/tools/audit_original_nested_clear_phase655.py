#!/usr/bin/env python3
"""Authenticate owner-original SiN 2006 nested clear, cleanup and name-view.

Original DLLs are needed only for --server/--client. Public CI --self-test uses
invented bytes and cannot silently accept a substituted retail file.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct

SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
IMAGE_BASE = 0x22000000
METHODS = (
  ('reset_nested_freelist',0x2228ec50,132,
   'be7e1d74e6cc2924a085a28c953618d1e8b21ac5da98245cc55cfa93634c21c6',b'\xc3'),
  ('release_nested_blocks',0x2228f400,49,
   'a2421243cd3fb6f5bb6a352ef57826558f6768eeb197757074f3e2202d28ec78',b'\xc3'),
  ('find_nested_name_view',0x2228f3c0,61,
   '3f52a0896437df27f02f59ad5b20abfeb4a53403cef8d38577bb274713b459b5',b'\xc2\x08\x00'),
)
CALLS = (
 (0x2228f404,0x2228ec50,'release_nested_calls_clear'),
 (0x2228f417,0x2226e3e0,'release_block_frees_via_recovered_allocator'),
 (0x2228f3ec,0x2228eb40,'name_view_calls_original_nested_search'),
 (0x2228f664,0x2228ec50,'another_original_nested_clear_caller'),
 (0x22122136,0x2228f400,'original_registry_release_caller_1'),
 (0x221b4de0,0x2228f400,'original_registry_release_caller_2'),
 (0x221b51dc,0x2228f400,'original_registry_release_caller_3'),
 (0x221b6a24,0x2228f400,'original_registry_release_caller_4'),
 (0x221f4e87,0x2228f400,'original_registry_release_caller_5'),
 (0x221a3f31,0x2228f3c0,'original_view_caller_1'),
 (0x221b5191,0x2228f3c0,'original_view_caller_2'),
 (0x221b569e,0x2228f3c0,'original_view_caller_3'),
 (0x221b5e3b,0x2228f3c0,'original_view_caller_4'),
 (0x221f45d0,0x2228f3c0,'original_view_caller_5'),
 (0x22285d42,0x2228f3c0,'original_view_caller_6'),
 (0x22287cc3,0x2228f3c0,'original_view_caller_7'),
)
SHORT_BRANCHES = (
 (0x2228ec65,0x2228ecb2,0x78),
 (0x2228ec6e,0x2228ec92,0x75),
 (0x2228ecaf,0x2228ec68,0x79),
 (0x2228f3c6,0x2228f3d4,0x75),
 (0x2228f40e,0x2228f427,0x7e),
 (0x2228f425,0x2228f410,0x7c),
)

def u16(data:bytes,pos:int)->int:
 return struct.unpack_from('<H',data,pos)[0]
def u32(data:bytes,pos:int)->int:
 return struct.unpack_from('<I',data,pos)[0]

def text_section(data:bytes)->tuple[int,bytes]:
 if data[:2]!=b'MZ':raise ValueError('not DOS/PE')
 pe=u32(data,0x3c)
 if data[pe:pe+4]!=b'PE\0\0' or u16(data,pe+4)!=0x14c:
  raise ValueError('not a PE32 i386 image')
 opt=pe+24
 if u16(data,opt)!=0x10b or u32(data,opt+28)!=IMAGE_BASE:
  raise ValueError('unexpected image base or PE type')
 offset=opt+u16(data,pe+20)
 for i in range(u16(data,pe+6)):
  off=offset+i*40
  if data[off:off+8].rstrip(b'\0')!=b'.text':continue
  rva=u32(data,off+12);size=u32(data,off+16);raw=u32(data,off+20)
  if not size or raw+size>len(data):raise ValueError('invalid .text bounds')
  return IMAGE_BASE+rva,data[raw:raw+size]
 raise ValueError('original .text not found')

def original(va:int,size:int,start:int,raw:bytes)->bytes:
 i=va-start
 if i<0 or i+size>len(raw):raise ValueError('address outside .text')
 return raw[i:i+size]

def call_target(va:int,start:int,raw:bytes)->int:
 op=original(va,5,start,raw)
 if op[0]!=0xe8:raise ValueError(f'wrong call opcode at {va:#x}')
 return (va+5+struct.unpack_from('<i',op,1)[0])&0xffffffff

def audit(server:bytes,client:bytes)->tuple[int,int,int]:
 if hashlib.sha256(server).hexdigest()!=SERVER:raise ValueError('wrong original server.dll')
 if hashlib.sha256(client).hexdigest()!=CLIENT:raise ValueError('wrong original client.dll')
 start,raw=text_section(server)
 for name,va,size,ret in METHODS:
  blob=original(va,size,start,raw)
  if hashlib.sha256(blob).hexdigest()!=METHODS[[m[0] for m in METHODS].index(name)][3]:
   raise ValueError(f'method hash does not match: {name}')
  if not blob.endswith(ret) or original(va+size,1,start,raw)!=b'\xcc':
   raise ValueError(f'method boundary does not match: {name}')
 for src,dst,label in CALLS:
  if call_target(src,start,raw)!=dst:raise ValueError(f'call mismatch {label}')
 for source,target,opcode in SHORT_BRANCHES:
  op=original(source,2,start,raw)
  if op[0]!=opcode or ((source+2+struct.unpack_from('<b',op,1)[0])&0xffffffff)!=target:
   raise ValueError(f'branch mismatch {source:#x}')
 return sum(m[2] for m in METHODS),len(CALLS),len(SHORT_BRANCHES)

def self_test()->None:
 data=bytearray(b'\x90'*128)
 data[8]=0xe8
 struct.pack_into('<i',data,9,40-13)
 assert call_target(8,0,bytes(data))==40
 data[9]^=1
 assert call_target(8,0,bytes(data))!=40
 fake=b'\x90'*100+b'\xc3'
 altered=bytearray(fake);altered[12]^=1
 assert hashlib.sha256(fake).digest()!=hashlib.sha256(altered).digest()
 try:text_section(b'bad PE')
 except ValueError:pass
 else:raise AssertionError('synthetic non-PE erroneously accepted')
 print('Phase655 public invented-byte self-test PASS')

def main()->None:
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--server',type=Path)
 ap.add_argument('--client',type=Path)
 ap.add_argument('--self-test',action='store_true')
 args=ap.parse_args()
 if args.self_test:
  self_test();return
 if not args.server or not args.client:ap.error('supply both original DLLs or --self-test')
 count,edges,branches=audit(args.server.read_bytes(),args.client.read_bytes())
 print(f'Phase655 original owner retail audit PASS: {count} complete bytes, '
       f'{edges} exact x86 direct calls, {branches} original branches, '
       'three method-end and next-byte INT3 boundaries')

if __name__=='__main__':main()
