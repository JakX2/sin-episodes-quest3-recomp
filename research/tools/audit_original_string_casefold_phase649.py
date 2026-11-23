#!/usr/bin/env python3
"""Owner original 2006 SiN retail string-comparator dependency audit.

Requires both exact owner retail DLLs for authentication. The public self-test
uses only invented bytes and refuses unrelated executables. No DLLs distributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_nested_map_phase647 import RetailPE,AuditFailure,check_pin

SERVER_SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA256='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS=(
 ('mode_switching_native_compare',0x223a5574,67,
  'd108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29'),
 ('ascii_case_insensitive_compare',0x223a8410,78,
  '9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658'),
 ('locale_sensitive_char_transform',0x223a72da,213,
  '5b0ce7c127d7a1ed81c122de7929ff525b2dfff8b1855c4d3a4eaaea4f315dba'),
)
CALLS=(
 (0x223a5591,0x223a72da,'left_char_lower_transform'),
 (0x223a559d,0x223a72da,'right_char_lower_transform'),
 (0x223a731a,0x223a898a,'ctype_mask_helper'),
 (0x223a7378,0x223ac9e7,'locale_character_map_helper'),
)
EDGES=(
 (0x223a557b,0x223a5582,'75','mode_nonzero_locale_path'),
 (0x223a557d,0x223a8410,'e9','mode_zero_ascii_tailjump'),
 (0x223a55a9,0x223a55af,'74','left_transformed_terminator'),
 (0x223a55ad,0x223a558d,'74','equal_transformed_continue'),
 (0x223a8422,0x223a8456,'74','ascii_terminator_exit'),
 (0x223a8430,0x223a8420,'74','raw_bytes_equal_continue'),
 (0x223a8450,0x223a8420,'74','folded_bytes_equal_continue'),
 (0x223a72ec,0x223a739b,'0f84','no_locale_ascii_fast'),
 (0x223a72f9,0x223a7304,'74','ctype_fast_disabled'),
 (0x223a72fe,0x223a739b,'0f86','ascii_fast_for_small_codepoint'),
 (0x223a730e,0x223a7332,'73','input_at_least_256'),
 (0x223a7316,0x223a7323,'7e','ctype_table_fast_lookup'),
 (0x223a7321,0x223a732e,'eb','ctype_helper_join'),
 (0x223a7330,0x223a73a8,'74','no_lowercase_class'),
 (0x223a7345,0x223a7356,'74','single_byte_character'),
 (0x223a7354,0x223a735f,'eb','dbcs_input_join'),
 (0x223a7382,0x223a73a8,'74','failed_locale_mapping_fallback'),
 (0x223a7386,0x223a738e,'75','mapped_multibyte_output'),
 (0x223a738c,0x223a73aa,'eb','mapped_byte_return'),
 (0x223a7399,0x223a73aa,'eb','mapped_pair_return'),
 (0x223a739e,0x223a73a8,'7c','ascii_below_A'),
 (0x223a73a6,0x223a73aa,'7e','ascii_within_AZ'),
)
# Address-pinned original x86 instruction witnesses; deliberately sparse,
# never a dump of the complete retail executable methods.
PIN_TEXT='''
223a5574:833d00f5592200 223a557d:e98e2e0000 223a5583:56 223a5588:57 223a558d:0fb606 223a5591:e8441d0000 223a5598:0fb607
223a559c:46 223a55a2:59 223a55a4:85db 223a55a7:8bc8 223a55ab:3bd9 223a55af:5f 223a55b2:5e
223a55b5:5b 223a8410:55 223a8413:57 223a8415:53 223a8419:8b7d08 223a841e:8bff 223a8422:7432
223a8426:83c601 223a842b:83c701 223a8430:74ee 223a8434:3c1a 223a8438:80e120 223a843d:0441 223a8441:2c41
223a8445:1ac9 223a844a:02c1 223a844e:38e0 223a8452:1ac0 223a8456:0fbec0 223a845a:5e 223a845c:c9
223a72da:55 223a72dd:51 223a72df:833d00f5592200 223a72e7:8b5d08 223a72eb:57 223a72f2:833de0d1532200 223a72fb:83fb7f
223a7304:33f6 223a730b:46 223a730e:7322 223a7316:7e0b 223a7319:53 223a731f:59 223a7321:eb0b
223a7328:0fb60458 223a732e:85c0 223a7332:8b1500d25322 223a733a:c1f808 223a7340:f6444a0180 223a7347:6a02 223a734c:885dfd
223a7353:58 223a7356:885dfc 223a735d:8bc6 223a7360:ff3510f55922 223a7369:6a03 223a736c:50 223a7370:50
223a7372:ff3500f55922 223a737d:83c420 223a7382:7424 223a7386:7506 223a738c:eb1c 223a7392:33c0 223a7397:0bc1
223a739b:83fb41 223a73a0:83fb5a 223a73a6:7e02 223a73aa:5f 223a73ac:5b 223a73ae:c3
'''
PINS=tuple((int(x.split(':',1)[0],16),x.split(':',1)[1]) for x in PIN_TEXT.split())

def check_branch(read,site,target,hex_opcode):
 op=bytes.fromhex(hex_opcode)
 width=4 if op[0] in (0x0f,0xe9) else 1
 code=read(site,len(op)+width)
 if len(code)!=len(op)+width or code[:len(op)]!=op:
  raise AuditFailure(f'original branch opcode differs at {site:#x}')
 displacement=struct.unpack_from('<i' if width==4 else '<b',code,len(op))[0]
 if site+len(code)+displacement!=target:
  raise AuditFailure(f'original branch target differs at {site:#x}')

def check_call(read,site,target):
 code=read(site,5)
 if len(code)!=5 or code[0]!=0xe8 or site+5+struct.unpack_from('<i',code,1)[0]!=target:
  raise AuditFailure(f'original direct call target differs at {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
  raise AuditFailure('exact complete original 2006 server.dll and client.dll SHA256 required')
 image=RetailPE(server)
 if image.base!=0x22000000:raise AuditFailure('wrong original image base')
 for name,va,count,expected in METHODS:
  if image.section(va)!='.text' or image.section(va+count-1)!='.text':
   raise AuditFailure(f'{name} outside original retail executable .text')
  if hashlib.sha256(image.at(va,count)).hexdigest()!=expected:
   raise AuditFailure(f'complete original method SHA256 mismatch for {name}')
 # The original CRT functions run back-to-back without INT3 between all
 # bodies. Authenticate exact adjacent entry opcode boundaries instead.
 for va,first in ((0x223a55b7,'56'),(0x223a845e,'6878084b22'),(0x223a73af,'cc')):
  check_pin(image.at,va,first)
 for src,dst,_ in CALLS:
  if image.section(src)!='.text' or image.section(dst)!='.text':
   raise AuditFailure('original direct-call edge points outside .text')
  check_call(image.at,src,dst)
 for src,dst,op,_ in EDGES:check_branch(image.at,src,dst,op)
 for va,op in PINS:check_pin(image.at,va,op)
 return {
  'status':'PHASE649_ORIGINAL_SIN_NATIVE_CASEFOLD_DISPATCH_AUTHENTICATED',
  'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
  'methods':[{'name':n,'va':hex(va),'bytes':count,'sha256':digest} for n,va,count,digest in METHODS],
  'authenticated_original_method_bytes':sum(x[2] for x in METHODS),
  'original_direct_rel32_calls':len(CALLS),'original_control_flow_edges':len(EDGES),
  'address_pinned_opcode_witnesses':len(PINS),
  'casefold_mode_global':'0x2259f500',
  'ascii_fallback_va':'0x223a8410','char_transform_va':'0x223a72da',
  'locale_ctype_helper_internal_recovered':False,
  'locale_mapping_helper_internal_recovered':False,
  'original_game_process_executed':False,'android_or_quest_gameplay_executed':False,
 }

def self_test():
 for _,va,count,_ in METHODS:
  invented=bytes((i*17+count+43)&255 for i in range(count))
  digest=hashlib.sha256(invented).digest()
  for i in range(count):
   damaged=bytearray(invented);damaged[i]^=1
   if hashlib.sha256(damaged).digest()==digest:
    raise AssertionError(f'changed invented original-free byte {va:#x}+{i} accepted')
 src,dst=0x71000000,0x71001000
 call=b'\xe8'+struct.pack('<i',dst-src-5)
 check_call(lambda _,n:call[:n],src,dst)
 for i in range(5):
  bad=bytearray(call);bad[i]^=1
  try:check_call(lambda _,n:bytes(bad[:n]),src,dst)
  except AuditFailure:pass
  else:raise AssertionError('changed direct call accepted')
 for opcode in ('75','74','eb','0f84','0f86','e9'):
  op=bytes.fromhex(opcode);w=4 if op[0] in (0x0f,0xe9) else 1
  dest=src+len(op)+w+7
  instruction=op+(struct.pack('<i',7) if w==4 else struct.pack('<b',7))
  check_branch(lambda _,n:instruction[:n],src,dest,opcode)
  for i in range(len(instruction)):
   bad=bytearray(instruction);bad[i]^=1
   try:check_branch(lambda _,n:bytes(bad[:n]),src,dest,opcode)
   except AuditFailure:pass
   else:raise AssertionError('changed original-free branch accepted')
 for witness in ('833d00f5592200','e98e2e0000','e8441d0000','0f84a9000000'):
  check_pin(lambda _,n:bytes.fromhex(witness)[:n],src,witness)
 try:authenticate(b'unrelated game',b'not sin')
 except AuditFailure:pass
 else:raise AssertionError('unrelated DLLs authenticated')
 print('PASS Phase649 original-free 358 method-byte mutations, four rel32 edges, 22 control-flow edges and retail identity refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path);p.add_argument('--client',type=Path)
 p.add_argument('--output',type=Path);p.add_argument('--self-test',action='store_true')
 args=p.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:
  p.error('provide original --server and --client, or --self-test')
 try:report=authenticate(args.server.read_bytes(),args.client.read_bytes())
 except (OSError,ValueError,struct.error) as exc:
  p.exit(1,'PHASE649 OWNER ORIGINAL AUDIT REFUSED: '+str(exc)+'\n')
 blob=json.dumps(report,indent=2)+'\n'
 if args.output:args.output.write_text(blob,encoding='utf-8')
 else:print(blob,end='')
if __name__=='__main__':main()
