#!/usr/bin/env python3
"""Owner-local exact-2006 SiN client token storage/control-flow audit.

Does not emit original bytes, disassembly, imported game files or pseudocode
copied from binary tools. Hosted self-test needs no original assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA

# Original client VA, complete method byte length, whole-function SHA-256,
# original-free descriptive role. Sizes are exclusive; NO overlapping slices.
METHODS = (
 (0x241D33E0, 84, '53ae0afeb7402568cb9a6b9b1feeba349e7a16e4070e7a1f2205c963d4f366ff', 'resolve_16bit_node_or_sentinel'),
 (0x241D3440, 39, 'd74a6d305dd4c5f752fbc8125f8e5ecf7b597c9e68710715b91b78c9eff00f39', 'release_owned_array_when_allowed'),
 (0x241D3470, 53, '4ddcc57be0e926c518af6de9dc1c9e9becb495545a855d573a6a0ce2e81a59ed', 'shift_four_byte_elements_left'),
 (0x241D34E0,115, 'b01c92221c5fdb33aa5135ef4c01968b9b93c0e6fa0add63e93489475cfe34a3', 'grow_twelve_byte_node_buffer'),
 (0x241D3560,112, 'ec6c3aa11039b1dfe047e7956b901b39027e56ea76efe1b2e954d8b655d61a8e', 'grow_four_byte_pointer_buffer'),
 (0x241D35D0, 17, 'a3769bba1d056aa8563ed86f52ef4fcb70fbd830eb1520a99cc079c785565142', 'get_parent_node_index'),
 (0x241D3960, 46, '4723bd92f6896d1f8887b2652295669e8280e5d4373b6cf2947b7947ed0927a4', 'extend_four_byte_pointer_buffer'),
 (0x241D3D10, 49, '4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25', 'select_string_page_with_space'),
 (0x241D4060,100, '51a642927a366b450ae5ffc17e16160325e17f12af49f89c7fd914bcf6487712', 'insert_string_page_pointer'),
)
# Native direct-call edges inside all nine methods and verified callers in
# Phase545's token allocator/intern. These are relative x86 CALL witnesses.
CALLS = (
 (0x241D3450,0x2416F940), (0x241D3499,0x24248B50),
 (0x241D352F,0x2416F960), (0x241D3545,0x2416F8F0),
 (0x241D35AC,0x2416F960), (0x241D35C2,0x2416F8F0),
 (0x241D35D5,0x241D33E0), (0x241D397C,0x241D3560),
 (0x241D4079,0x241D3560), (0x241D40A4,0x24248B50),
 # Recovered full original Phase545/Phase542 callers, checked again here.
 (0x241D391C,0x241D34E0), (0x241D393D,0x241D33E0),
 (0x241D39FB,0x241D33E0), (0x241D4AE6,0x241D3D10),
 (0x241D4B24,0x241D4060),
)
# Tiny independent instruction witnesses. Never print these bytes.
ANCHORS = (
 (0x241D33E5, '840554703724', 'global sentinel flag tested'),
 (0x241D33F2, '090554703724', 'global sentinel bit set'),
 (0x241D33F8, '6689154c703724', 'sentinel left child'),
 (0x241D33FF, '6689154e703724', 'sentinel right child'),
 (0x241D340D, '66a352703724', 'sentinel final word set 1'),
 (0x241D3413, '668b442404', 'index argument is 16 bit'),
 (0x241D341D, '8b4904', 'node array pointer at owner+4'),
 (0x241D3423, '8d0440', 'index times 3'),
 (0x241D3426, '8d0481', 'index times 12'),
 (0x241D342C, 'b84c703724', 'missing sentinel native address'),
 (0x241D3443, '837e08ff', 'do not free fixed/borrowed storage'),
 (0x241D3458, 'c70600000000', 'free sets array pointer null'),
 (0x241D345E, 'c7460400000000', 'free resets element capacity'),
 (0x241D3470, '8b410c', 'shift helper reads active count'),
 (0x241D348A, '03c003c0', 'shift count times four'),
 (0x241D34E3, '8b4e08', '12-byte reserve mode at owner+8'),
 (0x241D34E6, '83f9ff', 'no-grow guard -1'),
 (0x241D3503, '03c1', 'add growth increment'),
 (0x241D350A, '03c0', 'double when increment zero'),
 (0x241D3511, 'c7460403000000', '12-byte reserve initial capacity 3'),
 (0x241D3526, '8d1440', '12-byte byte-count first multiply'),
 (0x241D3544, '50', 'fresh allocation receives bytes'),
 (0x241D3563, '8b4e08', '4-byte reserve mode at owner+8'),
 (0x241D3583, '03c1', '4-byte reserve add increment'),
 (0x241D358A, '03c0', '4-byte reserve double'),
 (0x241D3591, 'c7460408000000', '4-byte reserve initial capacity 8'),
 (0x241D35D0, '8b44240450', 'parent accessor pushes 16-bit index'),
 (0x241D35DA, '668b4004', 'parent stored at node+4'),
 (0x241D3963, '8b460c', 'extend reads active count'),
 (0x241D3981, '017e0c', 'extend updates active count'),
 (0x241D3987, '894610', 'extend refreshes cached array pointer'),
 (0x241D3D12, '8b7128', 'string page-count field'),
 (0x241D3D1C, '8b511c', 'page pointer-array source'),
 (0x241D3D25, '8b19', 'page header capacity'),
 (0x241D3D27, '2b5904', 'page header used-byte subtraction'),
 (0x241D3D38, '83c8ff', 'no page found -1'),
 (0x241D4063, '8b460c', 'insert reads pointer-list count'),
 (0x241D4071, '2bc1', 'reserve deficit from capacity'),
 (0x241D407E, '83460c01', 'increment inserted-list count'),
 (0x241D4082, '8b460c', 'read new list count'),
 (0x241D4097, '03c003c0', 'tail shift bytes count times four'),
 (0x241D40AC, '8b0e', 'postgrowth base pointer read'),
 (0x241D40BD, '8bc7', 'insert returns requested index'),
)

class AuditError(ValueError): pass

def need(ok,why):
 if not ok: raise AuditError(why)

def check_call(reader,at,dest):
 raw=reader(at,5)
 need(len(raw)==5 and raw[0]==0xE8 and
      at+5+struct.unpack_from('<i',raw,1)[0]==dest,
      'unexpected original CALL '+hex(at))

def verify(server,client):
 need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'server.dll owner hash mismatch')
 need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client.dll owner hash mismatch')
 pe=PE32(client)
 need(pe.base==0x24000000,'unexpected original client PE32 base')
 for va,size,sha,role in METHODS:
  need(pe.section(va)=='.text','method not in .text: '+role)
  need(hashlib.sha256(pe.at(va,size)).hexdigest()==sha,
       'exact native function SHA changed: '+role)
  if va==0x241D3560:
   # No INT3. Its RET4 ends at 0x241D35D0 and a distinct helper starts.
   need(pe.at(va+size,5)==bytes.fromhex('8b44240450'),
        'adjacent parent-index accessor entry changed')
  else:
   need(pe.at(va+size,1)==b'\xcc','function boundary changed: '+role)
 for site,target in CALLS:check_call(pe.at,site,target)
 for addr,hexstr,why in ANCHORS:
  b=bytes.fromhex(hexstr)
  need(pe.at(addr,len(b))==b,'original instruction witness mismatch: '+why)
 return {
  'status':'ORIGINAL_CLIENT_TOKEN_STORAGE_ORIGINAL_AUTHENTICATED',
  'original_client_sha256':CLIENT_SHA,
  'complete_original_methods':len(METHODS),
  'new_non_overlapping_original_method_bytes':sum(x[1] for x in METHODS),
  'original_call_rel32_edges_checked':len(CALLS),
  'instruction_witnesses_checked':len(ANCHORS),
  'node_index_bits':16,
  'node_stride_bytes':12,
  'missing_node_index':'0xFFFF',
  'node_array_initial_capacity':3,
  'string_page_pointer_initial_capacity':8,
  'original_allocation_null_handling':'Not established as safe',
  'proof_boundary':'Binary-authenticated methods and bounded control flow; not full SDK source or game startup.'
 }

def self_test():
 need(len(METHODS)==9 and sum(x[1] for x in METHODS)==615,
      'method byte totals have drifted')
 need(len({x[0] for x in METHODS})==len(METHODS),'duplicate original method')
 va,dest=0x24006500,0x241D4000
 original=b'\xe8'+struct.pack('<i',dest-va-5)
 check_call(lambda _a,n:original[:n],va,dest)
 for i in range(5):
  corrupted=bytearray(original);corrupted[i]^=1
  try:check_call(lambda _a,n:bytes(corrupted[:n]),va,dest)
  except AuditError:pass
  else:raise AssertionError('bad original CALL accepted at byte '+str(i))
 for sa,cl in [(b'invalid server',b'invalid client'),(b'bad',b'')]:
  try:verify(sa,cl)
  except AuditError:pass
  else:raise AssertionError('invalid original SHA passed')
 print('PASS 9 method metadata; 615 bytes; CALL mutation refusal; invalid original SHA refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 args=p.parse_args()
 if not args.self_test and not args.bin_zip:p.error('select --self-test or --bin-zip')
 if args.self_test:self_test()
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   names={Path(x).name.lower():x for x in z.namelist()}
   need('server.dll' in names and 'client.dll' in names,'missing owner game DLL pair')
   result=verify(z.read(names['server.dll']),z.read(names['client.dll']))
  print(json.dumps(result,indent=2))
if __name__=='__main__':main()