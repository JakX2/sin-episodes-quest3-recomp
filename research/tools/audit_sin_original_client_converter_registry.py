#!/usr/bin/env python3
"""SHA-pinned read-only original SiN client type-converter registry audit.

Checks 2006 original client 16-bit token intern, ordered binary-tree
24-byte entry lookup via an authenticated token comparator, and registration. Emits only metadata, no DLL bytes.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, SERVER_SHA, CLIENT_SHA

METHODS=(
 (0x2400A3C0,19,'b0521eacdb9687665838d9a4df7374f3b2665f842939aea4d2d4c550dc97d413','unsigned_16bit_token_less_than'),
 (0x241D4A80,261,'dc80847fa08da6c33e75659031da2d0bb955f5713e28319e50e1c5659e39c1cc','string_token_intern'),
 (0x2400A4D0,230,'29e7ec79122a4b663661351234688ec42ada95894cad4006fbea930171f69f52','converter_list_find'),
 (0x24009F40,128,'c6a10d23962af7f91b8debedb98e7ae661cf044d54fe55d4dd812a58a945f41c','converter_record_insert'),
)
SOURCE_HANDLE_REGISTRY_VA=0x24009340
SOURCE_HANDLE_REGISTRY_SIZE=101
SOURCE_HANDLE_REGISTRY_SHA='a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e'
CALLS=(
 (0x2400935A,0x241D4A80),
 (0x24009371,0x2400A4D0),
 (0x2400939B,0x24009F40),
 (0x24009F57,0x241D4A80),
 (0x24009F74,0x2400ABA0),
 (0x24009F83,0x2400AB30),
 (0x241D4ABB,0x241D3E20),
 (0x241D4AE6,0x241D3D10),
 (0x241D4B03,0x2416F8F0),
 (0x241D4B24,0x241D4060),
 (0x241D4B6D,0x241D49F0),
)
class Refusal(ValueError): pass

def need(ok,why):
 if not ok:raise Refusal(why)

def check_call(read,site,target):
 b=read(site,5)
 need(len(b)==5 and b[0]==0xE8 and site+5+struct.unpack_from('<i',b,1)[0]==target,
      f'original direct call changed at {site:#x}')

def verify(server,client):
 need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'original server SHA mismatch')
 need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'original client SHA mismatch')
 pe=PE32(client)
 need(pe.base==0x24000000,'client PE base mismatch')
 for va,n,sha,role in METHODS:
  need(pe.section(va)=='.text','method not in .text: '+role)
  need(hashlib.sha256(pe.at(va,n)).hexdigest()==sha,'entire original method changed: '+role)
  if role=='converter_record_insert':
   # This original method is immediately followed by another entry at 0x24009FC0.
   need(pe.at(va+n,3)==b'\x83\xec\x0c','adjacent method entry changed')
  else:
   need(pe.at(va+n,1)==b'\xcc','original method boundary changed: '+role)
 need(hashlib.sha256(pe.at(SOURCE_HANDLE_REGISTRY_VA,SOURCE_HANDLE_REGISTRY_SIZE)).hexdigest()==SOURCE_HANDLE_REGISTRY_SHA,
      'previous Phase540 caller hash mismatch')
 for site,target in CALLS:check_call(pe.at,site,target)
 need(pe.at(0x242AC4F8,len('CHudTextureHandle')+1)==b'CHudTextureHandle\0',
      'converter type name changed')
 need(pe.at(0x242D3708,len('Already have converter for type %s, ignoring...\n')+1)==
      b'Already have converter for type %s, ignoring...\n\0',
      'original duplicate converter diagnostic changed')
 # This is a converter-name intern, not an ordinary HUD renderer or panel registration.
 need(pe.at(0x24009376,5)==b'\x83\xf8\xff\x74\x14',
      'duplicate converter -1 test and branch changed')
 need(pe.at(0x241D4A94,5)==b'\x66\xc7\x00\xff\xff',
      'null name no-token return changed')
 need(pe.at(0x241D4AAB,1)==b'\x50' and pe.at(0x241D4AEE,1)==b'\x75',
      'original intern token creation branch changed')
 need(pe.at(0x241D4AF0,6)==b'\x81\xff\x00\x08\x00\x00',
      'minimum 0x800-byte string pool allocation changed')
 need(pe.at(0x241D4B3F,3)==b'\xc1\xe9\x02',
      'original intern-copy DWORD stride changed')
 need(pe.u32(0x24322878)==0x2400A3C0,
      'converter descriptor does not own original 16-bit comparator')
 need(pe.at(0x2400A3C4,10)==b'\x66\x8b\x08\x8b\x54\x24\x08\x66\x3b\x0a',
      'original callback uint16 load and compare changed')
 need(pe.at(0x2400A3CE,5)==b'\x1b\xc0\xf7\xd8\xc3',
      'original unsigned-less-than Boolean conversion changed')
 need(pe.at(0x2400A4D7,4)==b'\x8b\x7d\x10\x83',
      'converter lookup head at record +0x10 changed')
 need(pe.at(0x2400A4ED,11)==b'\x8d\x34\x7f\x03\xf6\x03\xf6\x03\xf6\x8d\x44',
      'lookup record stride calculation differs from 24 bytes')
 need(pe.at(0x2400A4FC,3)==b'\xff\x55\x00',
      'lookup callback dispatch changed')
 need(pe.at(0x2400A553,5)==b'\x51\x50\xff\x55\x00',
      'lookup reverse-order callback invocation changed')
 need(pe.at(0x2400A5A5,9)==b'\x5e\x8b\xc7\x5f\x5d\x5b\xc2\x08\x00',
      'lookup result/ABI return changed')
 need(pe.at(0x24009F50,7)==b'\x66\xc7\x44\x24\x18\xff\xff',
      'registry token initialization sentinel changed')
 need(pe.at(0x24009F88,12)==b'\x8b\x57\x04\x8d\x0c\x40\x03\xc9\x03\xc9\x03\xc9',
      'registry 24-byte record index calculation changed')
 need(pe.at(0x24009F9C,5)==b'\x66\x8b\x74\x24\x10',
      'registry 16-bit token load changed')
 need(pe.at(0x24009FA1,3)==b'\x66\x89\x32',
      'registry token word store changed')
 need(pe.at(0x24009FB5,4)==b'\x89\x74\x11\x14',
      'registry converter value store changed')
 return {
  'status':'ORIGINAL_CLIENT_CONVERTER_REGISTRY_AUTHENTICATED',
  'new_complete_method_count':len(METHODS),
  'new_complete_method_bytes':sum(m[1] for m in METHODS),
  'original_comparison_callback':'0x2400a3c0',
  'comparison':'uint16_t(query) < uint16_t(candidate)',
  'registry_topology':'ordered binary search tree',
  'original_client_sha256':CLIENT_SHA,
  'prior_authenticated_caller':'0x24009340',
  'original_converter_label':'CHudTextureHandle',
  'original_converter_record_stride_bytes':24,
  'original_token_width_bits':16,
  'original_missing_token_sentinel':'0xFFFF',
  'original_missing_converter_sentinel':'0xFFFFFFFF',
  'direct_call_edges_checked':len(CALLS),
  'limitations':'No full container allocation/ownership, runtime client.dll or HUD rendering proved.'
 }

def self_test():
 origin,target=0x24002500,0x24009990
 good=b'\xe8'+struct.pack('<i',target-origin-5)
 check_call(lambda a,n:good[:n],origin,target)
 for i in range(5):
  b=bytearray(good);b[i]^=1
  try:check_call(lambda a,n:bytes(b[:n]),origin,target)
  except Refusal:pass
  else:raise AssertionError(f'mutated CALL passed at byte {i}')
 try:verify(b'wrong',b'wrong')
 except Refusal:pass
 else:raise AssertionError('bad original hashes passed')
 assert len(METHODS)==4 and sum(x[1] for x in METHODS)==638
 print('PASS original token comparator and ordered tree, synthetic call corruption checks and SHA refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--bin-zip',type=Path)
 p.add_argument('--self-test',action='store_true')
 a=p.parse_args()
 if not a.bin_zip and not a.self_test:p.error('pass --self-test or --bin-zip')
 if a.self_test:self_test()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   names={Path(x).name.lower():x for x in z.namelist()}
   need('server.dll' in names and 'client.dll' in names,'original private DLL pair missing')
   o=verify(z.read(names['server.dll']),z.read(names['client.dll']))
  print(json.dumps(o,indent=2))
if __name__=='__main__':main()