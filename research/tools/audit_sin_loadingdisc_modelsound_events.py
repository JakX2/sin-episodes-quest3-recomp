#!/usr/bin/env python3
"""Original SiN 2006 CLoadingDisc lazy resources and model-sound event scan.

Read-only original-file SHA256-chained PE32 audit. Reports metadata only.
Original proprietary executable code is never exported by this script.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_cache_slot2_hud_ctors import audit as previous_audit
from audit_sin_original_factories import PE32,VerificationError

REGIONS={
 'client.dll':{
   'loadingdisc_context_setter':(0x241ee470,10,'0a5b1a6d212e0caedbf2d98dea5a75fa8dba9f19919dae6658f37365af35e72f'),
   'loadingdisc_primary_toggle':(0x241ee4d0,80,'d2a35e0881a33f3447d073f20958f374636904b977db66133265bcc6ab3ec184'),
   'loadingdisc_secondary_toggle':(0x241ee530,96,'ccb0ba5f5b379d76389e23746ec9973fd70c045f7bf29a6a83da5a053c6fe9ca'),
 },
 'server.dll':{
   'modelsounds_event_scan':(0x22145520,447,'49ea37e723784624d6c81a3f45746e06f5e93aea1020b5c196e2699d886fcbb3'),
   'modelsounds_resolve_dedup':(0x221456e0,97,'51434bb637d61f0bffcef624958a06b1e95a83c67616cebcad24f97b42542f7a'),
 }}
OPCODES={
 'client.dll':{
 0x241ee46f:'c3',0x241ee474:'89410c',0x241ee4dc:'837e0400',
 0x241ee4e3:'68dc000000',0x241ee4f4:'8b4e0c',0x241ee50c:'897e04',
 0x241ee51a:'ff5278',0x241ee53c:'837f0800',
 0x241ee543:'68dc000000',0x241ee554:'8b4f0c',
 0x241ee56c:'897708',0x241ee571:'68c0c82a24',
 0x241ee578:'ff928c020000',0x241ee589:'ff5078',
 },
 'server.dll':{
  0x22145520:'81ec0c010000',0x22145577:'397518',
  0x22145584:'8b451c',0x22145591:'81f9f0030000',
  0x2214559f:'83f90e',0x221455a4:'81f9ec030000',
  0x221455b9:'8344241450',0x221455f4:'f6400810',
  0x22145627:'81f98c130000',0x22145633:'81f973170000',
  0x2214563f:'81f975170000',0x22145652:'803e00',
  0x22145657:'be8cb04022',0x2214565d:'6878b04022',
  0x22145666:'6800010000',0x22145679:'56',
  0x2214567a:'6864b04022',0x22145696:'56',
  0x22145697:'6854b04022',0x221456b6:'56',
  0x221456b7:'6840b04022',0x221456e1:'8b0dd0215722',
  0x221456ee:'ff500c',0x221456f1:'83f8ff',
  0x22145708:'8b31',0x22145710:'3906',
  0x22145714:'83c201',0x22145717:'83c604',
  0x2214571e:'8d442408',
 }}
CALLS={
 'client.dll':{
  0x241ee4e8:0x2416fcf0,0x241ee4fa:0x241ee350,0x241ee507:0x240049d0,
  0x241ee548:0x2416fcf0,0x241ee55a:0x241ee350,0x241ee567:0x240049d0,
 },
 'server.dll':{
  0x2214552f:0x220eb040,0x22145541:0x2211b7d0,
  0x2214556e:0x2211bd00,0x221455b1:0x221456e0,
  0x221455dc:0x2211b7d0,0x22145674:0x221456e0,
  0x22145691:0x221456e0,0x221456b1:0x221456e0,
  0x221456ce:0x221456e0,0x22145724:0x22095600,
  0x22145738:0x22095600,
 }}
STRING_SITES={
 0x2240b040:b'%s.FootstepRight\x00',
 0x2240b054:b'%s.FootstepLeft\x00',
 0x2240b064:b'%s.RunFootstepRight\x00',
 0x2240b078:b'%s.RunFootstepLeft\x00',
 0x2240b08c:b'NPC_grunt\x00',
}
ORIGINAL_LOADINGDISC_VTABLE=0x242ac8b0
ORIGINAL_LOADINGDISC_SINGLETON=0x2431ddd4

def verify_one(pe,module,strict=True):
 reg={}
 for name,(va,n,expected) in REGIONS[module].items():
  actual=hashlib.sha256(pe.at(va,n)).hexdigest()
  if strict and actual!=expected:
   raise VerificationError('original Phase94 code SHA mismatch: '+name)
  reg[name]={'original_va':hex(va),'bytes':n,'sha256':actual}
 for va,opcode in OPCODES[module].items():
  value=bytes.fromhex(opcode)
  if pe.at(va,len(value))!=value:
   raise VerificationError('original Phase94 instruction mismatch at %#x'%va)
 for va,target in CALLS[module].items():
  if pe.at(va,5)!=b'\xe8'+struct.pack('<i',target-va-5):
   raise VerificationError('original Phase94 direct-call target mismatch at %#x'%va)
 out={'module':module,'verified_functions':reg,'opcode_sites':len(OPCODES[module]),
      'direct_call_sites':len(CALLS[module])}
 if module=='client.dll':
  required_vtable=(0x241ee470,0x241ee480,0x241ee4d0,0x241ee530)
  actual=struct.unpack('<4I',pe.at(ORIGINAL_LOADINGDISC_VTABLE,16))
  if actual!=required_vtable:
   raise VerificationError('CLoadingDisc original virtual methods mismatch')
  raw=pe.at(ORIGINAL_LOADINGDISC_SINGLETON,16)
  if raw!=struct.pack('<4I',ORIGINAL_LOADINGDISC_VTABLE,0,0,0):
   raise VerificationError('CLoadingDisc original static singleton layout mismatch')
  out.update({'rtti_class':'CLoadingDisc',
   'original_singleton_va':hex(ORIGINAL_LOADINGDISC_SINGLETON),
   'original_vtable_va':hex(ORIGINAL_LOADINGDISC_VTABLE),
   'vtable_method_vas':[hex(x) for x in required_vtable],
   'primary_resource_offset':4,'secondary_resource_offset':8,
   'context_member_offset':12,'allocated_resource_bytes':220,
   'resource_toggle_virtual_offset':0x78,
   'secondary_configuration_virtual_offset':0x28c,
   'secondary_configuration_argument_original_va':'0x242AC8C0',
   'note':'two separate lazy creation paths; 16-byte constructor only writes +0 and +4; original static +8 is zero; both retain resources after disable'})
 else:
  for va,word in STRING_SITES.items():
   if pe.at(va,len(word))!=word:
    raise VerificationError('model sound original format text mismatch at %#x'%va)
  out.update({'rtti_class':'CModelSoundCache',
   'event_record_stride_bytes':0x50,
   'event_id_values':[14,1004,1008,5004,6004,6005],
   'event_14_requires_flag_bit':0x10,
   'footstep_event_ids':[6004,6005],
   'footstep_format_patterns':[w[:-1].decode('ascii') for addr,w in STRING_SITES.items() if addr!=0x2240b08c],
   'fallback_name':'NPC_grunt',
   'token_not_found':0xffffffff,
   'token_resolution_global_va':'0x225721D0',
   'token_lookup_virtual_byte_offset':12,
   'token_array_count_member_offset':12,
   'deduplicates_tokens':True,
   'warning':'Event record is a parsed in-memory structure, not a proven on-disk cache record'})
 out['limitations']='Original Windows x86 research, no original cache writer or Android SourceVR native game module.'
 return out

def audit(client,server):
 previous_audit(client,server)
 return {'client':verify_one(PE32(client),'client.dll'),
         'server':verify_one(PE32(server),'server.dll')}

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--client',type=Path,required=True)
 ap.add_argument('--server',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True)
 args=ap.parse_args()
 result=audit(args.client.read_bytes(),args.server.read_bytes())
 args.output.parent.mkdir(parents=True,exist_ok=True)
 args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('Original client CLoadingDisc paths and server CModelSoundCache event scanning verified')
if __name__=='__main__':main()
