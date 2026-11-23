#!/usr/bin/env python3
"""Verify exact owner-original SiN 2006 nested-string lookup and tree search.

No original retail DLL bytes are committed. Public --self-test mutates invented
bytes; authentic verification requires BOTH original owner 2006 DLLs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_nested_map_phase647 import RetailPE, AuditFailure

SERVER_SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA256='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS=(
 ('original_nested_tree_bidirectional_search',0x2228eb40,268,
  '5e96923bb70b37ce9ee0c8028727f6c5487b9ad1b3026629dc1440e436fafe12'),
 ('original_string_block_fit',0x2228ea30,49,
  '4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25'),
 ('original_nested_lookup_or_insert',0x2228f7a0,261,
  'd84e5bbcbf05a7501ef5094690ae0827a991d8e63a33d33fe60d46a32d15268f'),
)
DIRECT_CALLS=(
 (0x2228f7db,0x2228eb40,'lookup_searches_nested_tree'),
 (0x2228f806,0x2228ea30,'lookup_finds_available_string_block'),
 (0x2228f823,0x2226e390,'lookup_allocates_new_block'),
 (0x2228f844,0x2228ed80,'lookup_appends_block_pointer'),
 (0x2228f88d,0x2228f710,'lookup_registers_new_string_ref'),
)
INDIRECT_COMPARISONS=((0x2228eb78,'ff17'),(0x2228ebdf,'ff17'))
BRANCHES=(
 (0x2228ea3a,0x2228ea58,'7e','fit_empty_signed_count'),
 (0x2228ea4c,0x2228ea5b,'7d','fit_signed_available'),
 (0x2228ea56,0x2228ea43,'7c','fit_continue'),
 (0x2228eb4d,0x2228ec44,'0f84','search_empty_tree'),
 (0x2228eb7f,0x2228ebd1,'74','key_not_less'),
 (0x2228eb88,0x2228ebb5,'75','sentinel_already_initialized_left'),
 (0x2228ebba,0x2228ebc7,'74','left_sentinel_slot'),
 (0x2228ebc5,0x2228ec2f,'eb','left_child_join'),
 (0x2228ebcf,0x2228ec2f,'eb','left_sentinel_join'),
 (0x2228ebe6,0x2228ec3a,'74','keys_equal_return_index'),
 (0x2228ebf4,0x2228ec1a,'75','sentinel_already_initialized_right'),
 (0x2228ec1d,0x2228ec26,'74','right_sentinel_slot'),
 (0x2228ec24,0x2228ec2b,'eb','right_child_join'),
 (0x2228ec34,0x2228eb60,'0f85','search_loop'),
 (0x2228f7ad,0x2228f7c0,'75','name_nonnull'),
 (0x2228f7e3,0x2228f893,'0f85','name_found'),
 (0x2228f7f7,0x2228f7f0,'75','scan_more_characters'),
 (0x2228f80e,0x2228f849,'75','existing_string_block'),
 (0x2228f818,0x2228f81f,'7f','new_block_len_gt_2048'),
)
# Sparse witnesses are *partial* address-pinned instruction bytes; entire
# methods are authenticated by full SHA256, not by recreating binary blobs.
PIN_TEXT='''
2228ea30:53 2228ea32:8b7128 2228ea37:85f6 2228ea3a:7e1c
2228ea3c:8b511c 2228ea43:8b0a 2228ea47:2b5904
2228ea4a:3bdf 2228ea4c:7d0d 2228ea51:83c204
2228ea54:3bc6 2228ea56:7ceb 2228ea58:83c8ff
2228ea5e:c20400
2228eb40:55 2228eb44:668b6f10 2228eb4d:0f84f1000000
2228eb54:8b5c2414 2228eb60:8b4c2414 2228eb67:8d3440
2228eb6f:03f6 2228eb71:8d443008 2228eb75:53
2228eb78:ff17 2228eb7d:84c0 2228eb81:f6053401592201
2228eb94:090534015922 2228eba1:66890d2e015922
2228ebaf:66a332015922 2228ebbf:668b2c30
2228ebd1:8b5704 2228ebd9:8d443208 2228ebdf:ff17
2228ebe4:84c0 2228ebef:b8ffff0000
2228ebfb:090d34015922 2228ec13:66890d32015922
2228ec1a:663be8 2228ec2b:668b6802
2228ec34:0f8526ffffff 2228ec3d:668bc5 2228ec41:c20800
2228f7a0:83ec08 2228f7a5:8b742418
2228f7ad:7511 2228f7b4:66c700ffff 2228f7bd:c20800
2228f7c6:bfffff0000 2228f7ce:897330
2228f7d1:66897c2424 2228f7db:e860f3ffff
2228f7e0:663bc7 2228f7e3:0f85aa000000
2228f7eb:8d5001 2228f7f0:8a08
2228f7f9:2bc2 2228f7fb:8d7801
2228f806:e825f2ffff 2228f810:81ff00080000
2228f81a:bd00080000 2228f81f:8d4d0b
2228f823:e868ebfdff 2228f82b:8928
2228f838:c7400400000000 2228f844:e837f5ffff
2228f849:8b4b1c 2228f84c:8b1481
2228f853:66894c2420 2228f866:f3a5
2228f86d:f3a4 2228f873:014a04
2228f880:53 2228f88d:e87efeffff
2228f899:668901 2228f89c:8bc1 2228f8a2:c20800
'''
PINS=tuple((int(item.split(':',1)[0],16),item.split(':',1)[1])
           for item in PIN_TEXT.split())

def check_call(read,src,dest):
 data=read(src,5)
 if len(data)!=5 or data[0]!=0xe8 or src+5+struct.unpack_from('<i',data,1)[0]!=dest:
  raise AuditFailure(f'wrong original rel32 call {src:#x}')
def check_branch(read,src,dest,opcode):
 code=bytes.fromhex(opcode)
 size=4 if code[0]==0x0f or code[0]==0xe9 else 1
 raw=read(src,len(code)+size)
 if not raw.startswith(code) or len(raw)!=len(code)+size:
  raise AuditFailure(f'wrong native branch opcode {src:#x}')
 relative=struct.unpack_from('<i' if size==4 else '<b',raw,len(code))[0]
 if src+len(raw)+relative!=dest:
  raise AuditFailure(f'wrong native branch destination {src:#x}')
def check_pin(read,site,code):
 expected=bytes.fromhex(code)
 if read(site,len(expected))!=expected:
  raise AuditFailure(f'wrong original instruction witness at {site:#x}')

def authenticate(server,client):
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
  raise AuditFailure('both owner-original full 2006 PE32 server/client DLL identities are required')
 pe=RetailPE(server)
 if pe.base!=0x22000000:raise AuditFailure('wrong original server image base')
 for name,va,count,digest in METHODS:
  if pe.section(va)!='.text' or pe.section(va+count-1)!='.text':
   raise AuditFailure(f'original method outside executable .text: {name}')
  if hashlib.sha256(pe.at(va,count)).hexdigest()!=digest:
   raise AuditFailure(f'complete original SHA256 mismatch for {name}')
  if pe.at(va+count,1)!=b'\xcc':
   raise AuditFailure(f'original method INT3 boundary mismatch for {name}')
 for src,dst,_ in DIRECT_CALLS:
  if pe.section(src)!='.text' or pe.section(dst)!='.text':
   raise AuditFailure('native call target outside executable text')
  check_call(pe.at,src,dst)
 for src,dst,op,_ in BRANCHES:check_branch(pe.at,src,dst,op)
 for src,code in INDIRECT_COMPARISONS:check_pin(pe.at,src,code)
 for src,code in PINS:check_pin(pe.at,src,code)
 return {
  'status':'PHASE650_ORIGINAL_NESTED_STRING_LOOKUP_AND_SEARCH_AUTHENTICATED',
  'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
  'methods':[{'name':name,'va':hex(va),'bytes':count,'sha256':digest}
             for name,va,count,digest in METHODS],
  'complete_original_executable_bytes':sum(x[2] for x in METHODS),
  'original_direct_rel32_calls':len(DIRECT_CALLS),
  'original_control_flow_edges':len(BRANCHES),
  'original_address_pinned_instruction_witnesses':len(PINS),
  'original_indirect_comparator_call_sites':len(INDIRECT_COMPARISONS),
  'original_nested_tree_stride_bytes':12,
  'block_allocation_min_capacity_bytes':2048,
  'block_header_allocation_extra_bytes':11,
  'original_block_array_append_0x2228ed80_recovered':False,
  'original_tree_register_0x2228f710_recovered':False,
  'original_retail_game_process_executed':False,
  'android_or_quest3_gameplay_executed':False,
 }

def self_test():
 for _,va,n,_ in METHODS:
  synthetic=bytes((i*19+n+17)&255 for i in range(n))
  digest=hashlib.sha256(synthetic).digest()
  for i in range(n):
   b=bytearray(synthetic);b[i]^=1
   if hashlib.sha256(b).digest()==digest:
    raise AssertionError('mutated whole synthetic function accepted')
 src,dst=0x71000000,0x71000400
 call=b'\xe8'+struct.pack('<i',dst-src-5)
 check_call(lambda _,n:call[:n],src,dst)
 for i in range(5):
  b=bytearray(call);b[i]^=1
  try:check_call(lambda _,n:bytes(b[:n]),src,dst)
  except AuditFailure:pass
  else:raise AssertionError('mutated rel32 accepted')
 for opcode in ('75','74','7e','7f','0f84','0f85'):
  prefix=bytes.fromhex(opcode);w=4 if prefix[0]==0x0f else 1
  code=prefix+(struct.pack('<i',7) if w==4 else struct.pack('<b',7))
  dest=src+len(code)+7
  check_branch(lambda _,n:code[:n],src,dest,opcode)
  for i in range(len(code)):
   bad=bytearray(code);bad[i]^=1
   try:check_branch(lambda _,n:bytes(bad[:n]),src,dest,opcode)
   except AuditFailure:pass
   else:raise AssertionError('altered synthetic branch accepted')
 for opcode in ('ff17','668901','83ec08','668b6f10'):
  check_pin(lambda _,n:bytes.fromhex(opcode)[:n],src,opcode)
 try:authenticate(b'not retail server',b'not retail client')
 except AuditFailure:pass
 else:raise AssertionError('unrelated binaries accepted')
 print('PASS Phase650 original-free invented full-method/branch/call/witness mutation tests')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path);p.add_argument('--client',type=Path)
 p.add_argument('--output',type=Path);p.add_argument('--self-test',action='store_true')
 args=p.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:p.error('provide original --server and --client or --self-test')
 try:report=authenticate(args.server.read_bytes(),args.client.read_bytes())
 except (OSError,ValueError,struct.error) as exc:
  p.exit(1,f'PHASE650 OWNER ORIGINAL RETAIL AUDIT REFUSED: {exc}\n')
 text=json.dumps(report,indent=2)+'\n'
 if args.output:args.output.write_text(text,encoding='utf-8')
 else:print(text,end='')
if __name__=='__main__':main()
