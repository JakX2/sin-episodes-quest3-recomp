#!/usr/bin/env python3
"""SHA-verified read-only 2006 SiN client token comparator decompilation audit.

Only hashes and bounded metadata leave the owner-private original DLL archive.
Original code and proprietary bytes are not copied to the repository.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA

# These are independent whole-method digests (exclusive endpoint), not RVA guesses.
METHODS = (
 (0x241D3BF0, 157, '8285c595abf524eb7a9878b5d83e7712974bda42b8662529e22e44e330dacf76', 'bytewise_case_sensitive_less'),
 (0x241D3C90, 113, '91e93b8190e900c957b63eae158d119fefb8bbb56ca07ddfb2cb34231c011087', 'folded_less_wrapper'),
 (0x241D4640,  81, 'c6166577600eeb70647f1a4266e862e922e7e43f7469b36be2430a9df9568200', 'token_comparator_selector_constructor'),
 (0x2425F81B,  67, '0905847aa7a5fa843447c965b7ed17f401401ad60bf6be835038df5637559806', 'case_insensitive_locale_dispatch'),
 (0x24261C60,  78, '9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658', 'case_insensitive_ascii_fallback'),
 (0x2424C373, 213, '3fe51247dd4b30901765155d0d9057674a228f00e8384ef010a40040d6fa7838', 'char_casefold_provider'),
)
# All calls are original direct E8 rel32 instructions and independently resolve.
CALLS = (
 (0x241D3CEE, 0x2425F81B),
 (0x241D4653, 0x241D3D90),
 (0x2425F838, 0x2424C373),
 (0x2425F844, 0x2424C373),
)
JUMPS = ((0x2425F824, 0x24261C60),)
# Machine-code witnesses for the compare, zero handling, branch selection,
# byte-folding, fallback and full C-string behavior. These are minimal
# literal instruction encodings, not an original disassembly distribution.
ANCHORS = (
 (0x241D3BF4, '668b08', 'first token descriptor ID'),
 (0x241D3BF7, '6681f9ffff', 'descriptor zero-page sentinel'),
 (0x241D3C50, '8a10', 'load unsigned query byte'),
 (0x241D3C54, '3a16', 'bytewise compare without case folding'),
 (0x241D3C58, '84c9', 'test matching string terminator'),
 (0x241D3C61, '3a5601', 'unrolled second byte compare'),
 (0x241D3C70, '33c0', 'strings equal means not less'),
 (0x241D3C7D, '1bc083d8ff', 'unsigned difference sign normalized'),
 (0x241D3C86, '0f9cc1', 'signed-less after normalized difference'),
 (0x241D3C90, '8b4c2404', 'folded comparator start'),
 (0x241D3CF8, '85c0', 'folded comparator sign test'),
 (0x241D3CFA, '0f9cc1', 'folded result less than zero'),
 (0x241D4658, '385c2414', 'constructor flag byte test'),
 (0x241D4671, '740f', 'zero flag selects ordinal comparator'),
 (0x241D4673, '391e', 'nonzero existing callback guard'),
 (0x241D4677, 'c706903c1d24', 'nonzero flag chooses folded comparator'),
 (0x241D4682, '391e', 'preconfigured ordinal guard'),
 (0x241D4686, 'c706f03b1d24', 'zero flag chooses case sensitive comparator'),
 (0x2425F81B, '833dd095382400', 'conditional locale-state dispatch'),
 (0x2425F834, '0fb606', 'read unsigned first byte for locale folding'),
 (0x2425F850, '74043bd9', 'iterate until folded byte differs or null'),
 (0x2425F85A, '2bc1', 'return folded-byte difference'),
 (0x24261C82, '2c413c1a', 'ASCII uppercase classification first byte'),
 (0x24261C91, '2c413c1a', 'ASCII uppercase classification second byte'),
 (0x24261CA2, '1ac01cff', 'ASCII fallback returns sign of folded difference'),
 (0x2424C378, '833dd095382400', 'fold helper locale readiness condition'),
 (0x2424C434, '83fb41', 'no-locale ASCII uppercase lower bound'),
 (0x2424C439, '83fb5a', 'no-locale ASCII uppercase upper bound'),
 (0x2424C443, '5f5e5bc9c3', 'fold provider return'),
)
# 2006 methods that directly border the next function, not INT3:
BOUNDARY = {
 0x241D3BF0: b'\xcc',
 0x241D3C90: b'\xcc',
 0x241D4640: b'\xcc',
 0x2425F81B: b'\x83\x7c',
 0x24261C60: b'\x56\x8b',
 0x2424C373: b'\x55\x8b',
}
class EvidenceError(ValueError): pass

def require(ok, what):
 if not ok: raise EvidenceError(what)

def verify_rel32(reader, site, target, opcode):
 b=reader(site,5)
 require(len(b)==5 and b[0]==opcode and site+5+struct.unpack_from('<i',b,1)[0]==target,
         'rel32 edge changed at '+hex(site))

def verify(server:bytes, client:bytes):
 require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'unrecognized original server SHA')
 require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'unrecognized original client SHA')
 p=PE32(client)
 require(p.base==0x24000000,'unknown client PE32 base')
 for va,n,digest,label in METHODS:
  require(p.section(va)=='.text','method not in .text '+label)
  require(hashlib.sha256(p.at(va,n)).hexdigest()==digest,'whole method digest differs '+label)
  sig=BOUNDARY[va];require(p.at(va+n,len(sig))==sig,'method boundary changed '+label)
 for site,target in CALLS: verify_rel32(p.at,site,target,0xE8)
 for site,target in JUMPS: verify_rel32(p.at,site,target,0xE9)
 for va,hexbytes,name in ANCHORS:
  data=bytes.fromhex(hexbytes)
  require(p.at(va,len(data))==data,'native instruction witness changed '+name)
 # The *conditional* constructor sets the original token comparator only
 # if the virtual/field at +0x0 is zero. We do not assert actual runtime flag.
 return {
  'status':'ORIGINAL_CLIENT_TOKEN_COMPARATOR_VARIANTS_AUTHENTICATED',
  'original_client_sha256':CLIENT_SHA,
  'complete_native_methods':len(METHODS),
  'new_nonoverlapping_native_bytes':sum(n for _,n,_,_ in METHODS),
  'direct_call_edges':len(CALLS), 'direct_tail_jump_edges':len(JUMPS),
  'instruction_witnesses':len(ANCHORS),
  'native_callbacks':{'flag_zero':'0x241D3BF0 bytewise ordinal less',
                      'flag_nonzero':'0x241D3C90 folded string less',
                      'existing_callback_nonzero':'preserved'},
  'folded_provider':{'locale_ready':'native case-fold helper 0x2424C373',
                     'locale_unavailable':'native ASCII fallback 0x24261C60'},
  'scope':'Two original compare variants and constructor gating decompiled; dynamic locale settings and real active callback are unverified.'
 }

def selftest():
 assert len(METHODS)==6 and sum(n for _,n,_,_ in METHODS)==709
 assert len(CALLS)==4 and len(JUMPS)==1 and len(ANCHORS)>=25
 for opcode in (0xE8,0xE9):
  a,b=0x24001400,0x24006891
  original=bytes([opcode])+struct.pack('<i',b-a-5)
  verify_rel32(lambda va,n:original[:n],a,b,opcode)
  for i in range(5):
   t=bytearray(original);t[i]^=1
   try:verify_rel32(lambda va,n:bytes(t[:n]),a,b,opcode)
   except EvidenceError:pass
   else:raise AssertionError('corrupt relative jump accepted')
 try:verify(b'invalid server',b'invalid client')
 except EvidenceError:pass
 else:raise AssertionError('invalid original binary accepted')
 print('PASS 6 authenticated method specifications; corrupted CALL/JMP refusal; SHA gating')

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--self-test',action='store_true')
 ap.add_argument('--bin-zip',type=Path)
 args=ap.parse_args()
 if not args.self_test and args.bin_zip is None:ap.error('pass --self-test or --bin-zip')
 if args.self_test:selftest()
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   look={Path(x).name.lower():x for x in z.namelist()}
   require('server.dll' in look and 'client.dll' in look,'owner original DLL pair unavailable')
   result=verify(z.read(look['server.dll']),z.read(look['client.dll']))
  print(json.dumps(result,indent=2))
if __name__=='__main__': main()