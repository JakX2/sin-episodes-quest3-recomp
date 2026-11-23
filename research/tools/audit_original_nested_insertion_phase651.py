#!/usr/bin/env python3
"""Owner-private authentication of original SiN Episodes 2006 insertion helpers.

Public --self-test uses invented bytes; no retail code or DLL is distributed.
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
 ('original_block_array_insertion',0x2228ed80,100,
  'b5212200ca591fab856c37cf4f4179a5585b9c7729d29158ef167ac45f9a9a52'),
 ('original_nested_tree_reference_insertion',0x2228f710,94,
  '7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d'),
 ('original_block_pointer_capacity_growth',0x2228e280,112,
  '1b91b29a1820e36300655fa1f9440dfc0f2653be30b61ca5a6f1ea44d6e8565b'),
)
DIRECT_CALLS=(
 (0x2228ed99,0x2228e280,'array_insert_calls_grow'),
 (0x2228edc4,0x223a5d10,'array_insert_calls_memmove'),
 (0x2228f72b,0x2228e470,'tree_insert_finds_parent_branch'),
 (0x2228f732,0x2228e620,'tree_insert_acquires_12byte_node'),
 (0x2228f746,0x2228f460,'tree_insert_links_and_rebalances'),
 (0x2228e2cc,0x2226e400,'grow_calls_original_reallocate'),
 (0x2228e2e2,0x2226e390,'grow_calls_original_allocate'),
 (0x2228f88d,0x2228f710,'prior_Phase650_lookup_calls_tree_insert'),
 (0x2228f844,0x2228ed80,'prior_Phase650_lookup_calls_block_insert'),
)
BRANCHES=(
 (0x2228ed8f,0x2228ed9e,'7e','enough_capacity'),
 (0x2228edb5,0x2228edcc,'7e','no_shift_when_signed_nonpositive'),
 (0x2228edd3,0x2228eddd,'74','skip_write_to_null_destination'),
 (0x2228f75f,0x2228f765,'74','skip_copy_to_null_record_address'),
 (0x2228e289,0x2228e2ec,'74','disabled_growth'),
 (0x2228e296,0x2228e2bd,'7d','signed_capacity_already_sufficient'),
 (0x2228e29d,0x2228e2b1,'74','initial_capacity_zero'),
 (0x2228e2a1,0x2228e2aa,'74','step_zero_double'),
 (0x2228e2a8,0x2228e2b8,'eb','additive_join'),
 (0x2228e2af,0x2228e2b8,'eb','doubling_join'),
 (0x2228e2bb,0x2228e298,'7c','retry_signed_growth'),
 (0x2228e2c1,0x2228e2da,'74','initial_allocate_when_null'),
)
PINS_TEXT='''
2228ed80:56 2228ed81:8bf1 2228ed83:8b460c
2228ed86:8b4e04 2228ed89:8d5001 2228ed8c:3bd1
2228ed8f:7e0d 2228ed91:2bc1 2228ed93:83c001
2228ed96:50 2228ed97:8bce 2228ed99:e8e2f4ffff
2228ed9e:83460c01 2228eda2:8b460c 2228eda5:8b7c240c
2228eda9:8b0e 2228edab:2bc7 2228edad:83e801
2228edb0:85c0 2228edb2:894e10 2228edb5:7e15
2228edb7:03c0 2228edb9:03c0 2228edbb:8d0cb9
2228edbe:50 2228edbf:51 2228edc0:83c104
2228edc3:51 2228edc4:e8476f1100 2228edc9:83c40c
2228edcc:8b0e 2228edce:8d04b9 2228edd1:85c0
2228edd3:7408 2228edd5:8b542410 2228edd9:8b0a
2228eddb:8908 2228eddd:8bc7 2228eddf:5f
2228ede0:5e 2228ede1:c20800
2228f710:8b442408 2228f714:53 2228f715:8b5c2408
2228f719:56 2228f71a:57 2228f71b:8bf1
2228f71d:50 2228f71e:8d4c2418 2228f722:51
2228f723:8d542418 2228f727:52 2228f728:53
2228f729:8bce 2228f72b:e840edffff
2228f730:8bce 2228f732:e8e9eeffff
2228f737:8b4c2410 2228f73b:8bf8 2228f73d:8b442414
2228f741:50 2228f742:51 2228f743:57
2228f744:8bce 2228f746:e815fdffff
2228f74b:6683461201 2228f750:0fb7c7
2228f753:8d1440 2228f756:8b4604
2228f759:8d449008 2228f75d:85c0
2228f75f:7404 2228f761:8b0b 2228f763:8908
2228f765:668bc7 2228f768:5f 2228f769:5e
2228f76a:5b 2228f76b:c20800
2228e280:56 2228e281:8bf1 2228e283:8b4e08
2228e286:83f9ff 2228e289:7461 2228e28b:8b4604
2228e28e:8b542408 2228e292:03d0 2228e294:3bc2
2228e296:7d25 2228e298:8b4604 2228e29b:85c0
2228e29d:7412 2228e29f:85c9 2228e2a1:7407
2228e2a3:03c1 2228e2a5:894604 2228e2a8:eb0e
2228e2aa:03c0 2228e2ac:894604 2228e2af:eb07
2228e2b1:c7460408000000 2228e2b8:395604
2228e2bb:7cdb 2228e2bd:8b06 2228e2bf:85c0
2228e2c1:7417 2228e2c3:8b4e04 2228e2c6:03c9
2228e2c8:03c9 2228e2ca:51 2228e2cb:50
2228e2cc:e82f01feff 2228e2d1:83c408
2228e2d4:8906 2228e2d6:5e 2228e2d7:c20400
2228e2da:8b5604 2228e2dd:03d2 2228e2df:03d2
2228e2e1:52 2228e2e2:e8a900feff
2228e2e7:83c404 2228e2ea:8906 2228e2ec:5e
2228e2ed:c20400
'''
PINS=tuple((int(piece.split(':',1)[0],16),piece.split(':',1)[1])
            for piece in PINS_TEXT.split())

def check_call(read,site,target):
 data=read(site,5)
 if len(data)!=5 or data[0]!=0xe8 or site+5+struct.unpack_from('<i',data,1)[0]!=target:
  raise AuditFailure(f'original call incorrect at {site:#x}')

def check_branch(read,site,target,opcode):
 op=bytes.fromhex(opcode)
 raw=read(site,len(op)+1)
 if len(raw)!=len(op)+1 or not raw.startswith(op):
  raise AuditFailure(f'original branch opcode incorrect at {site:#x}')
 relative=struct.unpack_from('<b',raw,len(op))[0]
 if site+len(raw)+relative!=target:
  raise AuditFailure(f'original branch destination incorrect at {site:#x}')

def check_pin(read,site,value):
 op=bytes.fromhex(value)
 if read(site,len(op))!=op:raise AuditFailure(f'original native instruction mismatch at {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
 if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
  raise AuditFailure('Both complete original 2006 server/client SHA256s required')
 pe=RetailPE(server)
 if pe.base!=0x22000000:raise AuditFailure('Incorrect original PE32 image base')
 for name,va,length,digest in METHODS:
  if pe.section(va)!='.text' or pe.section(va+length-1)!='.text':
   raise AuditFailure(f'Original {name} outside executable .text')
  if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
   raise AuditFailure(f'Original {name} complete method SHA256 mismatch')
  expected=b'\x8b\x44\x24\x04' if name=='original_block_pointer_capacity_growth' else b'\xcc'
  if pe.at(va+length,len(expected))!=expected:
   raise AuditFailure(f'Original {name} next instruction boundary mismatch')
 for site,dest,role in DIRECT_CALLS:
  if pe.section(site)!='.text' or pe.section(dest)!='.text':
   raise AuditFailure(f'Original {role} target not in executable .text')
  check_call(pe.at,site,dest)
 for site,dest,opcode,_ in BRANCHES:check_branch(pe.at,site,dest,opcode)
 for site,op in PINS:check_pin(pe.at,site,op)
 return {
  'status':'PHASE651_OWNER_ORIGINAL_NESTED_INSERTION_AUTHENTICATED',
  'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
  'methods':[{'name':name,'va':hex(va),'bytes':size,'full_sha256':digest}
             for name,va,size,digest in METHODS],
  'complete_original_method_bytes':sum(m[2] for m in METHODS),
  'direct_rel32_calls':len(DIRECT_CALLS),
  'control_flow_branches':len(BRANCHES),
  'pinned_opcode_witnesses':len(PINS),
  'block_pointer_initial_capacity':8,
  'original_block_pointer_stride_bytes':4,
  'original_nested_tree_node_stride_bytes':12,
  'original_parent_search_2228e470_recovered':False,
  'original_node_acquire_2228e620_recovered':False,
  'original_link_rebalance_2228f460_recovered':False,
  'original_memory_move_223a5d10_recovered':False,
  'retail_game_process_executed':False,
  'android_quest_gameplay_executed':False,
 }

def self_test():
 for _,va,n,_ in METHODS:
  invented=bytes((i*23+n+19)&255 for i in range(n))
  digest=hashlib.sha256(invented).digest()
  for index in range(n):
   changed=bytearray(invented);changed[index]^=1
   if hashlib.sha256(changed).digest()==digest:
    raise AssertionError('Invented whole-body mutation accepted')
 site,target=0x71000000,0x71000200
 call=b'\xe8'+struct.pack('<i',target-site-5)
 check_call(lambda _v,n:call[:n],site,target)
 for i in range(5):
  bad=bytearray(call);bad[i]^=1
  try:check_call(lambda _v,n:bytes(bad[:n]),site,target)
  except AuditFailure:pass
  else:raise AssertionError('Altered rel32 accepted')
 for opcode in ('7e','7d','74','7c','eb'):
  code=bytes.fromhex(opcode)+struct.pack('<b',9)
  check_branch(lambda _v,n:code[:n],site,site+len(code)+9,opcode)
  for i in range(len(code)):
   bad=bytearray(code);bad[i]^=1
   try:check_branch(lambda _v,n:bytes(bad[:n]),site,site+len(code)+9,opcode)
   except AuditFailure:pass
   else:raise AssertionError('Altered branch accepted')
 for opcode in ('83460c01','6683461201','c7460408000000','c20800'):
  check_pin(lambda _v,n:bytes.fromhex(opcode)[:n],site,opcode)
 try:authenticate(b'unrelated',b'dll')
 except AuditFailure:pass
 else:raise AssertionError('Incorrect DLL authentication accepted')
 print('PASS Phase651 original-free synthetic complete-body/call/branch/opcode mutations')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--server',type=Path)
 p.add_argument('--client',type=Path)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--output',type=Path)
 args=p.parse_args()
 if args.self_test:self_test();return
 if not args.server or not args.client:
  p.error('Supply original --server --client, or --self-test')
 try:r=authenticate(args.server.read_bytes(),args.client.read_bytes())
 except (AuditFailure,OSError,ValueError,struct.error) as exc:
  p.exit(1,'PHASE651 ORIGINAL RETAIL AUDIT REFUSED: '+str(exc)+'\n')
 out=json.dumps(r,indent=2)+'\n'
 if args.output:args.output.write_text(out,encoding='utf-8')
 else:print(out,end='')
if __name__=='__main__':main()
