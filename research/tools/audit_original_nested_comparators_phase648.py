#!/usr/bin/env python3
"""Owner-original 2006 SiN nested-map comparator whole-body authentication.

Private retail DLLs are required to prove method bytes. --self-test uses only
invented byte arrays and must never be taken as retail validation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_nested_map_phase647 import (RetailPE,AuditFailure,
    SERVER_SHA256,CLIENT_SHA256,check_call,check_branch,check_pin)

METHODS=(
 ('bytewise_null_terminated_comparator',0x2228e910,157,
  '8285c595abf524eb7a9878b5d83e7712974bda42b8662529e22e44e330dacf76'),
 ('delegating_string_comparator',0x2228e9b0,113,
  '44b4473790bc069cc4184ffff3deba0220146b714df135f3cac25e63b5e4738e'),
)
CALLS=((0x2228ea0e,0x223a5574,'actual_retail_case_transforming_compare'),)
BRANCHES=(
 (0x2228e91d,0x2228e92e,'75','left_not_ffff'),
 (0x2228e923,0x2228e92e,'75','left_only_first_ffff'),
 (0x2228e92c,0x2228e943,'eb','left_fallback_join'),
 (0x2228e94f,0x2228e95c,'75','right_not_ffff'),
 (0x2228e955,0x2228e95c,'75','right_only_first_ffff'),
 (0x2228e95a,0x2228e970,'eb','right_fallback_join'),
 (0x2228e976,0x2228e99d,'75','first_byte_differs'),
 (0x2228e97a,0x2228e990,'74','first_byte_terminator'),
 (0x2228e984,0x2228e99d,'75','second_byte_differs'),
 (0x2228e98e,0x2228e970,'75','two_byte_continue'),
 (0x2228e9bc,0x2228e9cd,'75','delegated_left_not_ffff'),
 (0x2228e9c2,0x2228e9cd,'75','delegated_left_single_ffff'),
 (0x2228e9cb,0x2228e9e2,'eb','delegated_left_join'),
 (0x2228e9ee,0x2228e9fb,'75','delegated_right_not_ffff'),
 (0x2228e9f4,0x2228e9fb,'75','delegated_right_single_ffff'),
 (0x2228e9f9,0x2228ea0c,'eb','delegated_right_join'),
)
PIN_TEXT='''
2228e910:8b442404 2228e914:668b08 2228e917:6681f9ffff
2228e91c:56 2228e91d:750f 2228e91f:66394802
2228e923:7509 2228e925:8b4c2410 2228e929:8b4130
2228e92c:eb15 2228e92e:0fb74002 2228e932:0fb7d1
2228e935:8b4c2410 2228e939:8b711c 2228e93c:8b1496
2228e93f:8d440208 2228e943:8b54240c 2228e947:668b32
2228e94a:6681feffff 2228e94f:750b 2228e951:66397202
2228e955:7505 2228e957:8b7130 2228e95a:eb14
2228e95c:8b491c 2228e95f:0fb75202 2228e963:0fb7f6
2228e966:8b0cb1 2228e969:8d741108 2228e970:8a10
2228e972:8aca 2228e974:3a16 2228e976:7525
2228e978:84c9 2228e97a:7414 2228e97c:8a5001
2228e97f:8aca 2228e981:3a5601 2228e984:7517
2228e986:83c002 2228e989:83c602 2228e98c:84c9
2228e98e:75e0 2228e990:33c0 2228e992:33c9
2228e994:85c0 2228e996:0f9cc1 2228e999:8ac1
2228e99b:5e 2228e99c:c3 2228e99d:1bc0
2228e99f:83d8ff 2228e9a2:33c9 2228e9a4:85c0
2228e9a6:0f9cc1 2228e9a9:8ac1 2228e9ab:5e
2228e9ac:c3
2228e9b0:8b4c2404 2228e9b4:668b01 2228e9b7:663dffff
2228e9bb:56 2228e9bc:750f 2228e9be:66394102
2228e9c2:7509 2228e9c4:8b442410 2228e9c8:8b7030
2228e9cb:eb15 2228e9cd:0fb74902 2228e9d1:0fb7d0
2228e9d4:8b442410 2228e9d8:8b701c 2228e9db:8b1496
2228e9de:8d740a08 2228e9e2:8b4c240c 2228e9e6:668b11
2228e9e9:6681faff 2228e9ee:750b 2228e9f0:66395102
2228e9f4:7505 2228e9f6:8b4030 2228e9f9:eb11
2228e9fb:8b401c 2228e9fe:0fb74902 2228ea02:0fb7d2
2228ea05:8b1490 2228ea08:8d440a08 2228ea0c:50
2228ea0d:56 2228ea0e:e8616b1100 2228ea13:83c408
2228ea16:33c9 2228ea18:85c0 2228ea1a:0f9cc1
2228ea1d:8ac1 2228ea1f:5e 2228ea20:c3
'''
PINS=tuple((int(p.split(':',1)[0],16),p.split(':',1)[1]) for p in PIN_TEXT.split())

def authenticate(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
  raise AuditFailure('original owner 2006 whole server/client DLL SHA256 mismatch')
 pe=RetailPE(server)
 if pe.base!=0x22000000:raise AuditFailure('incorrect original retail image base')
 for name,va,n,digest in METHODS:
  if pe.section(va)!='.text' or pe.section(va+n-1)!='.text':raise AuditFailure('method outside retail .text: '+name)
  if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:raise AuditFailure('full 2006 method SHA256 mismatch: '+name)
  if pe.at(va+n,1)!=b'\xcc':raise AuditFailure('original INT3 method boundary missing: '+name)
 for src,dst,_ in CALLS:
  if pe.section(src)!='.text' or pe.section(dst)!='.text':raise AuditFailure('direct call not executable')
  check_call(pe.at,src,dst)
 for src,dst,opcode,_ in BRANCHES:check_branch(pe.at,src,dst,opcode)
 for va,opcode in PINS:check_pin(pe.at,va,opcode)
 return {'status':'PHASE648_ORIGINAL_NESTED_STRING_COMPARATORS_VERIFIED',
  'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
  'methods':[{'name':name,'va':hex(va),'bytes':size,'sha256':digest} for name,va,size,digest in METHODS],
  'total_complete_original_bytes':sum(x[2] for x in METHODS),
  'original_rel32_calls':len(CALLS),'original_branch_edges':len(BRANCHES),
  'original_address_pinned_instruction_witnesses':len(PINS),
  'nested_string_fallback_map_offset':'0x30','string_block_table_map_offset':'0x1c',
  'record_block_index_and_offset_are_unsigned_16bit':True,
  'original_223a5574_internals_reconstructed':False,
  'actual_original_game_process_executed':False,'android_vr_gameplay_executed':False}

def self_test():
 for _,va,n,_ in METHODS:
  invented=bytes((i*17+n+91)&255 for i in range(n))
  digest=hashlib.sha256(invented).digest()
  assert len(invented)==n
  for i in range(n):
   altered=bytearray(invented);altered[i]^=1
   if hashlib.sha256(altered).digest()==digest:raise AssertionError('mutated invented whole-method bytes accepted')
 site,dest=0x71000000,0x71000200
 call=b'\xe8'+struct.pack('<i',dest-site-5)
 check_call(lambda _va,k:call[:k],site,dest)
 for i in range(5):
  bad=bytearray(call);bad[i]^=1
  try:check_call(lambda _va,k:bytes(bad[:k]),site,dest)
  except AuditFailure:pass
  else:raise AssertionError('changed native call accepted')
 for opcode in ('74','75','eb'):
  original=bytes.fromhex(opcode)+struct.pack('<b',7)
  check_branch(lambda _va,k:original[:k],site,site+9,opcode)
  for i in range(2):
   bad=bytearray(original);bad[i]^=1
   try:check_branch(lambda _va,k:bytes(bad[:k]),site,site+9,opcode)
   except AuditFailure:pass
   else:raise AssertionError('mutated branch accepted')
 for pin in ('6681f9ffff','0f9cc1','83c408','e8616b1100'):
  check_pin(lambda _va,k:bytes.fromhex(pin)[:k],site,pin)
 try:authenticate(b'unrelated',b'not sin')
 except AuditFailure:pass
 else:raise AssertionError('non-retail DLLs accepted')
 print('PASS Phase648 original-free invented 270-byte complete-method SHA, rel32, branches, instruction witness, retail identity refusal')

def main():
 a=argparse.ArgumentParser(description=__doc__)
 a.add_argument('--server',type=Path);a.add_argument('--client',type=Path)
 a.add_argument('--output',type=Path);a.add_argument('--self-test',action='store_true')
 args=a.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:a.error('supply original --server and --client DLLs, or --self-test')
 try:result=authenticate(args.server.read_bytes(),args.client.read_bytes())
 except (OSError,ValueError,struct.error) as error:a.exit(1,f'PHASE648 AUDIT REFUSED: {error}\n')
 output=json.dumps(result,indent=2)+'\n'
 if args.output:args.output.write_text(output,encoding='utf-8')
 else:print(output,end='')
if __name__=='__main__':main()
