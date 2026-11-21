#!/usr/bin/env python3
"""Owner-only 2006 original SiN client 12-byte string-token RB-tree audit.

Whole-file SHA gate; exact original method bytes and code/data witnesses.
Never prints or emits game DLL bytes, native disassembly or assets.
"""
from __future__ import annotations
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32,CLIENT_SHA,SERVER_SHA
METHODS=(
 (0x241D3750, 261,'5cc99b17ccadb013b2f921b34d7f237a68c8fdd988bab0e98226797010c12ed5','insertion_slot_find'),
 (0x241D3900,  86,'9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b','allocate_token_record'),
 (0x241D39F0, 235,'5b96eff8753dab4996f3b011a5e4bbd6d8f6ccbfec341d22c9833d94d3759b0a','rotate_left'),
 (0x241D3AE0, 238,'70f1138c52c9aa4ca3a4ce23218620b65935fced6026547bf222ce8a1365e572','rotate_right'),
 (0x241D3E20, 268,'f0b5fefed68fcc281f2d2105046372d02fcf58a064745897c6ce376dbcc5fdc8','lookup_token'),
 (0x241D40D0,1387,'3a2c9861ab0ec6917322c25a631f1a7641d1606d0fbfe19d6957c644508d88d1','red_black_fixup'),
 (0x241D4740, 109,'9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a','link_token_node'),
 (0x241D49F0,  94,'7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d','publish_new_token'),
)
PREVIOUS_TOKEN_INTERN=(0x241D4A80,261,'dc80847fa08da6c33e75659031da2d0bb955f5713e28319e50e1c5659e39c1cc')
CALLS=(
 (0x241D391C,0x241D34E0),(0x241D393D,0x241D33E0),
 (0x241D39FB,0x241D33E0),(0x241D3A07,0x241D33E0),(0x241D3A24,0x241D33E0),
 (0x241D3A33,0x241D33E0),(0x241D3A50,0x241D33E0),(0x241D3A70,0x241D33E0),
 (0x241D3A7C,0x241D33E0),(0x241D3A89,0x241D33E0),(0x241D3A9E,0x241D33E0),
 (0x241D3AEB,0x241D33E0),(0x241D3AF6,0x241D33E0),(0x241D3B13,0x241D33E0),
 (0x241D3B23,0x241D33E0),(0x241D3B41,0x241D33E0),(0x241D3B61,0x241D33E0),
 (0x241D3B6D,0x241D33E0),(0x241D3B7B,0x241D33E0),(0x241D3B91,0x241D33E0),
 (0x241D441C,0x241D39F0),(0x241D4424,0x241D33E0),
 (0x241D4430,0x241D33E0),(0x241D445A,0x241D3AE0),
 (0x241D45D3,0x241D3AE0),(0x241D45DB,0x241D33E0),
 (0x241D45E7,0x241D33E0),(0x241D4611,0x241D39F0),
 (0x241D4787,0x241D40D0),(0x241D4796,0x241D40D0),(0x241D47A4,0x241D40D0),
 (0x241D4A0B,0x241D3750),(0x241D4A12,0x241D3900),(0x241D4A26,0x241D4740),
 (0x241D4ABB,0x241D3E20),(0x241D4B6D,0x241D49F0),
)
INDIRECT=((0x241D3794,'ff5500'),(0x241D3E58,'ff17'),(0x241D3EBF,'ff17'))
ANCHORS=(
 (0x241D375D,'668b7d10','root token index word'),
 (0x241D3766,'66c703ffff','insertion parent -1 sentinel'),
 (0x241D376B,'c60000','insertion side initialized zero'),
 (0x241D377C,'8d3440','token tree index times three'),
 (0x241D3785,'03f603f6','index times twelve'),
 (0x241D379E,'8b4c241c','insertion orientation result'),
 (0x241D3903,'668b4614','free-list head ushort'),
 (0x241D3917,'6a01','grow by one element'),
 (0x241D392E,'8d4f01','next highwater token'),
 (0x241D3932,'66894e16','store 16-bit highwater counter'),
 (0x241D3E24,'668b6f10','lookup root index 16-bit'),
 (0x241D3E28,'6681fdffff','lookup missing root sentinel'),
 (0x241D3E4A,'8b4704','token node storage pointer'),
 (0x241D3E4D,'03f603f6','12-byte node stride'),
 (0x241D3E51,'8d443008','payload at node +8'),
 (0x241D3E9F,'668b2c30','follow smaller-child +0'),
 (0x241D3F0B,'668b6802','follow larger-child +2'),
 (0x241D4740,'8b5104','link tree node storage'),
 (0x241D475B,'66895004','new parent at node +4'),
 (0x241D475F,'66c74002ffff','larger child sentinel'),
 (0x241D4765,'66c700ffff','smaller child sentinel'),
 (0x241D476A,'66c740060000','fresh red color word zero'),
 (0x241D4783,'66893482','smaller child link'),
 (0x241D4791,'6689748202','larger child link'),
 (0x241D47A0,'66897110','empty tree root update'),
 (0x241D49F0,'8b442408','publication input stack argument'),
 (0x241D4A2B,'6683461201','increment active-token 16-bit count'),
 (0x241D4A39,'8d449008','token payload store at +8'),
 (0x241D4A4B,'c20800','publish return ABI'),
 (0x241D4632,'66895c8a06','root final color word one'),
)
class AuditError(ValueError):pass

def need(ok,why):
 if not ok:raise AuditError(why)

def call_ok(read,site,target):
 b=read(site,5)
 need(len(b)==5 and b[0]==0xE8 and site+5+struct.unpack_from('<i',b,1)[0]==target,
      'original CALL target changed at '+hex(site))

def verify(server,client):
 need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'original server sha mismatch')
 need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'original client sha mismatch')
 p=PE32(client)
 need(p.base==0x24000000,'unrecognized original PE base')
 for va,n,dig,name in METHODS:
  need(p.section(va)=='.text','not original client text method '+name)
  need(hashlib.sha256(p.at(va,n)).hexdigest()==dig,'whole method changed '+name)
  need(p.at(va+n,1)==b'\xcc','function boundary changed '+name)
 va,n,dig=PREVIOUS_TOKEN_INTERN
 need(hashlib.sha256(p.at(va,n)).hexdigest()==dig,'previous original token intern function changed')
 for site,target in CALLS:call_ok(p.at,site,target)
 for va,pattern in INDIRECT:
  b=bytes.fromhex(pattern)
  need(p.at(va,len(b))==b,'untrusted indirect token comparator '+hex(va))
 for va,pattern,role in ANCHORS:
  b=bytes.fromhex(pattern)
  need(p.at(va,len(b))==b,'original token tree witness changed: '+role)
 return {
 'status':'ORIGINAL_CLIENT_STRING_TOKEN_REDBLACK_AUTHENTICATED',
 'original_client_sha256':CLIENT_SHA,
 'complete_original_methods':len(METHODS),
 'new_complete_original_method_bytes':sum(x[1] for x in METHODS),
 'original_direct_calls_checked':len(CALLS),
 'indirect_comparator_calls_checked':len(INDIRECT),
 'instruction_witness_count':len(ANCHORS),
 'original_record_layout':{'stride':12,'smaller_child':0,'larger_child':2,
                            'parent':4,'color':6,'value_pointer':8},
 'original_tree_layout':{'root_token_word_offset':16,'active_count_word_offset':18,
                         'free_head_word_offset':20,'next_unallocated_word_offset':22},
 'missing_token':'0xFFFF','new_node_color':0,'root_color':1,
 'scope':'Research-only: no original C++ symbols, native DLL replacement or engine boot proven'
 }

def selftest():
 assert len(METHODS)==8 and sum(x[1] for x in METHODS)==2678
 assert len(CALLS)==36 and len(INDIRECT)==3
 a,b=0x24000080,0x24007510
 good=b'\xe8'+struct.pack('<i',b-a-5)
 call_ok(lambda at,n:good[:n],a,b)
 for i in range(5):
  broken=bytearray(good);broken[i]^=1
  try:call_ok(lambda at,n:bytes(broken[:n]),a,b)
  except AuditError:pass
  else:raise AssertionError('mutated call passed')
 try:verify(b'bad server',b'bad client')
 except AuditError:pass
 else:raise AssertionError('bad original sha accepted')
 print('PASS 8 token method metadata, 36 rel32 call witnesses, 3 indirect callbacks, mutations, SHA refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 a=p.parse_args()
 if not a.self_test and not a.bin_zip:p.error('pass --self-test or --bin-zip')
 if a.self_test:selftest()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   name={Path(x).name.lower():x for x in z.namelist()}
   need('client.dll' in name and 'server.dll' in name,'original DLL pair missing')
   out=verify(z.read(name['server.dll']),z.read(name['client.dll']))
  print(json.dumps(out,indent=2))
if __name__=='__main__':main()