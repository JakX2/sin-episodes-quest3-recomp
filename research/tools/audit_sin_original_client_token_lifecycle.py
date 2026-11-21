#!/usr/bin/env python3
"""Authenticate the owner-original SiN 2006 client token/string lifecycle.

Checks exact whole-DLL identities, complete native method SHA digests,
original CALL edges and selected original instruction witnesses. Emits no
proprietary binary or full disassembly content.
"""
from __future__ import annotations
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32,CLIENT_SHA,SERVER_SHA

METHODS=(
 (0x241D3D50, 51, '779f7fdb02b3897d68b0e5656ebf7807e7c0d94a4bc3b38d633e1b84469d66cc', 'token_to_cstring'),
 (0x241D3F30,132, 'ec8181c826014919010e06ace20c83b2388f0311a3dbf0ab66507226078ee304', 'reset_existing_token_tree_as_free_list'),
 (0x241D4000, 83, '3d425e64b29a356d539f8fdbc88a6e857784a5aae72deb4893878881a5efbd2e', 'release_page_pointer_array'),
 (0x241D47B0,151, '662c71b540086a90b25970a1881f89261864234a765f2dd04bab34681de34bb6', 'lazy_global_token_table_initializer'),
 (0x241D4940, 93, '2a5f32bd203b6987c8582d1ced523f799368065a79d27bac3a256f802cde6964', 'release_token_pages_and_tree_nodes'),
 (0x241D4B90, 43, '4f487899621b4710f1488b4037d200ca521ebe6a3fe09a615aea768af35384a6', 'global_symbol_from_string_wrapper'),
)
# Phase545's original 261-byte intern method is rechecked, not recounted.
PREVIOUS_INTERN=(0x241D4A80,261,'dc80847fa08da6c33e75659031da2d0bb955f5713e28319e50e1c5659e39c1cc')
CALLS=(
 (0x241D4017,0x2416F940),(0x241D403C,0x2416F940),
 (0x241D47C2,0x2416FCF0),(0x241D47E4,0x2416F8F0),
 (0x241D4944,0x241D3F30),(0x241D4957,0x2416F940),
 (0x241D4971,0x241D4000),(0x241D4984,0x2416F940),
 (0x241D4B94,0x241D47B0),(0x241D4BA9,0x241D4A80),
 # Pre-existing original owner method calls, rechecked for source linkage.
 (0x241D49D3,0x241D4940),(0x241D4ABB,0x241D3E20),
 (0x241D4AE6,0x241D3D10),(0x241D4B24,0x241D4060),
 (0x241D4B6D,0x241D49F0),
)
ANCHORS=(
 (0x241D3D54,'663dffff','missing token descriptor guard'),
 (0x241D3D5A,'b80e332d24','static empty string address'),
 (0x241D3D62,'8b5104','native node-array source'),
 (0x241D3D65,'8b491c','native string page-array source'),
 (0x241D3D6B,'8d0440','descriptor node offset times three'),
 (0x241D3D6E,'8d448208','descriptor address node+8'),
 (0x241D3D72,'0fb710','page index uint16'),
 (0x241D3D75,'0fb74002','page byte offset uint16'),
 (0x241D3D7C,'8d440108','page header plus eight'),
 (0x241D3F30,'0fb75116','free-list reset reads 16-bit highwater'),
 (0x241D3F43,'8bf7','missing sentinel initial free-next'),
 (0x241D3F7F,'6689742802','next free index at larger-child +2'),
 (0x241D3F87,'66891430','current index written into smaller child'),
 (0x241D3F92,'668b4116','read highwater for free-head calculation'),
 (0x241D3F99,'66897910','set root missing'),
 (0x241D3F9D,'66c741120000','clear active token count'),
 (0x241D3FAE,'66894114','publish free-list head'),
 (0x241D4003,'837e08ff','page array mode guard -1'),
 (0x241D4007,'c7460c00000000','clear page pointer-list count'),
 (0x241D4032,'894610','refresh native cached base pointer'),
 (0x241D47B3,'381d58703724','global table initialization bit test'),
 (0x241D47C0,'6a34','0x34-byte singleton allocation'),
 (0x241D47D2,'6880010000','initial 0x180-byte 32-node reservation'),
 (0x241D47DA,'c7460820000000','global node capacity 32'),
 (0x241D47F1,'66894610','missing root index 0xffff'),
 (0x241D4810,'c7462408000000','global page pointer grow setting'),
 (0x241D4821,'c706f03b1d24','global token comparator bytewise callback'),
 (0x241D4827,'893544703724','publish singleton pointer'),
 (0x241D482E,'c6055870372401','mark singleton attempted initialization'),
 (0x241D494B,'397e28','page-count loop test'),
 (0x241D4950,'8b461c','page array source'),
 (0x241D496A,'c7410c00000000','clear page array logical count'),
 (0x241D4976,'837e0cff','token node storage mode guard'),
 (0x241D498C,'c7460400000000','clear native token node data pointer'),
 (0x241D4BA3,'8b0d44703724','read global table at singleton address'),
 (0x241D4BB1,'668916','store returned 16-bit string token'),
)
class AuditError(ValueError):pass

def need(ok,msg):
 if not ok:raise AuditError(msg)

def check_call(read,at,target):
 raw=read(at,5)
 need(len(raw)==5 and raw[0]==0xe8 and
      at+5+struct.unpack_from('<i',raw,1)[0]==target,
      'changed original rel32 CALL at '+hex(at))

def verify(server,client):
 need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'non-original server SHA')
 need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'non-original client SHA')
 pe=PE32(client)
 need(pe.base==0x24000000,'unrecognized original client PE32 image base')
 for va,n,sha,name in METHODS:
  need(pe.section(va)=='.text','original method not in .text: '+name)
  need(hashlib.sha256(pe.at(va,n)).hexdigest()==sha,
       'complete original method changed: '+name)
  need(pe.at(va+n,1)==b'\xcc','original method INT3 boundary changed: '+name)
 v,n,sha=PREVIOUS_INTERN
 need(hashlib.sha256(pe.at(v,n)).hexdigest()==sha,
      'prior original token intern not authenticated')
 need(pe.at(0x242D330E,1)==b'\0','missing-token string not empty')
 for at,target in CALLS:check_call(pe.at,at,target)
 for at,pattern,meaning in ANCHORS:
  raw=bytes.fromhex(pattern)
  need(pe.at(at,len(raw))==raw,'original lifecycle witness changed: '+meaning)
 return {
  'status':'ORIGINAL_CLIENT_TOKEN_LIFECYCLE_AUTHENTICATED',
  'original_client_sha256':CLIENT_SHA,
  'new_complete_native_methods':len(METHODS),
  'new_non_overlapping_native_bytes':sum(x[1] for x in METHODS),
  'verified_original_direct_calls':len(CALLS),
  'verified_native_instruction_witnesses':len(ANCHORS),
  'global_token_table_initializer':hex(0x241D47B0),
  'global_default_token_comparator':'case-sensitive',
  'separate_HUD_converter_token_table_default':'case-folded (Phase546)',
  'global_initial_token_nodes':32,
  'global_node_buffer_alloc_bytes':384,
  'global_table_object_bytes':52,
  'previous_261_byte_intern_function':'rechecked but not double-counted',
  'limits':'Binary and bounded source flow, not full native game lifecycle, port or runtime locale.'
 }

def self_test():
 need(len(METHODS)==6 and sum(x[1] for x in METHODS)==553,'method totals drifted')
 a,b=0x24008400,0x241DC050
 good=b'\xe8'+struct.pack('<i',b-a-5)
 check_call(lambda _a,n:good[:n],a,b)
 for i in range(5):
  bad=bytearray(good);bad[i]^=1
  try:check_call(lambda _a,n:bytes(bad[:n]),a,b)
  except AuditError:pass
  else:raise AssertionError('bad CALL byte accepted '+str(i))
 try:verify(b'fake',b'fake')
 except AuditError:pass
 else:raise AssertionError('invalid game DLL SHA accepted')
 print('PASS 6 complete original lifecycle methods; 553 bytes; original-free call mutations and SHA refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 a=p.parse_args()
 if not a.self_test and not a.bin_zip:p.error('select --self-test or --bin-zip')
 if a.self_test:self_test()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   files={Path(x).name.lower():x for x in z.namelist()}
   need('client.dll' in files and 'server.dll' in files,'owner original DLLs missing')
   res=verify(z.read(files['server.dll']),z.read(files['client.dll']))
  print(json.dumps(res,indent=2))
if __name__=='__main__':main()