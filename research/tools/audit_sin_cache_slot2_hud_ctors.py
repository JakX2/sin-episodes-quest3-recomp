#!/usr/bin/env python3
"""SHA-pinned original SiN 2006 client HUD constructor and server cache slot-2 audit.

Read-only. Requires user's original PE32 DLLs and chains Phase92 SHA + RTTI
evidence. Emits only method addresses, SHA digests and neutral metadata.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_cache_io_hud_lifecycle import audit as previous_audit
from audit_sin_original_factories import PE32, VerificationError
from audit_sin_rtti_cache_client_hud import CLIENT as PRIOR_HUD_RTTI

HUD_CTORS=[
 ('CDebugOverlayPanel',0x241eca50,0x242abf0c),
 ('CNetGraphPanelInterface',0x241ef9e0,0x242acc34),
 ('CFPS',0x241ed190,0x242ac1f0),
 ('CMessageChars',0x241eea20,0x242acb38),
 ('CLoadingDisc',0x241ee460,0x242ac8b0),
 ('CCenterPrint',0x241ec5e0,0x242abc4c),
 ('CTextMessage',0x241f2310,0x242ad484),
 ('CStatsPanelInterface',0x24207660,0x242af988),
]
SERVER_REGIONS={
 'scene_slot2':(0x221ad750,199,'bcc1b6624167e51ec29137ea7cff43041397674ca1ca76809b7861c7a1b0759e'),
 'scene_candidate_gate':(0x221ad820,97,'80451f83b0850ce692f181fdc8dda0ac0a7d7395c92050138f9b194a87626f91'),
 'modelsounds_slot2':(0x221454b0,101,'c3a10cccfee5943b097bf7af64d27f045436e9ff559670f1ca62f35f2faa5158'),
}
SERVER_ANCHORS={
 0x221ad750:'83ec08',
 0x221ad77d:'84c0',
 0x221ad7a5:'3bfe',
 0x221ad7b3:'3bde',
 0x221ad7cc:'3bf3',
 0x221ad802:'c20400',
 0x221ad814:'c20400',
 0x221ad828:'85f6',
 0x221ad833:'83f805',
 0x221ad83f:'85c0',
 0x221ad843:'6a40',
 0x221ad851:'84c0',
 0x221ad865:'83f8ff',
 0x221ad880:'c3',
 0x221454b0:'8b542404',
 0x221454b8:'c7470c00000000',
 0x221454c7:'6a01',
 0x221454cd:'83f8ff',
 0x221454e1:'85f6',
 0x221454f1:'83f803',
 0x22145502:'85c0',
 0x22145512:'c20400',
}
SERVER_CALLS={
 0x221ad778:0x221ad1b0,
 0x221ad79b:0x22119b20,
 0x221ad7ac:0x22112870,
 0x221ad7ba:0x22113bb0,
 0x221ad7c1:0x221ad820,
 0x221ad7d2:0x22113c10,
 0x221ad7e0:0x22116310,
 0x221ad7e6:0x2226e800,
 0x221ad7f4:0x2226e800,
 0x221ad806:0x221a3ef0,
 0x221ad82e:0x2210b290,
 0x221ad83a:0x2210bb30,
 0x221ad84c:0x2210c880,
 0x221ad877:0x22097e10,
 0x22145508:0x22145520,
}
SERVER_INDIRECT={
 0x221ad862:'ff520c',
 0x221454ca:'ff5010',
 0x221454dc:'ff5204',
 0x221454ee:'ff5024',
 0x221454ff:'ff5228',
}
def digest(data):return hashlib.sha256(data).hexdigest()
def audit_client(pe):
 start,text=pe.code()
 rows=[]
 for i,(name,entry,vtable) in enumerate(HUD_CTORS):
  original=PRIOR_HUD_RTTI[i]
  if original[4]!='.?AV'+name+'@@' or original[2]!=vtable:
   raise VerificationError('RTTI cross-phase mismatch: '+name)
  expected=(b'\x8b\xc1\xc7\x00'+struct.pack('<I',vtable)
            +b'\xc7\x40\x04\x00\x00\x00\x00\xc3')
  raw=pe.at(entry,16)
  if raw!=expected:raise VerificationError('original HUD constructor opcode mismatch: '+name)
  occurrences=text.count(struct.pack('<I',vtable))
  if occurrences!=1:raise VerificationError('original HUD vtable text reference count differs: '+name)
  rows.append(dict(class_name=name,original_constructor_va=hex(entry),
      original_vtable_va=hex(vtable),code_size=16,code_sha256=digest(raw),
      sets_vptr_0=True,zeros_member_4=True,does_not_write_member_8=True))
 return dict(original_hud_constructors=rows,count=len(rows),
   unverified='CLoadingDisc teardown uses +8, which is not initialized by this 16-byte constructor')
def audit_server(pe,verify_hash=True):
 regions={}
 for name,(address,size,sha) in SERVER_REGIONS.items():
  actual=digest(pe.at(address,size))
  if verify_hash and actual!=sha:
   raise VerificationError('original cache slot-2 SHA256 mismatch: '+name)
  regions[name]=dict(address=hex(address),size=size,sha256=actual)
 for address,raw in SERVER_ANCHORS.items():
  data=bytes.fromhex(raw)
  if pe.at(address,len(data))!=data:
   raise VerificationError('original cache x86 opcode mismatch: %#x'%address)
 for address,target in SERVER_CALLS.items():
  data=b'\xe8'+struct.pack('<i',target-address-5)
  if pe.at(address,5)!=data:
   raise VerificationError('original cache CALL mismatch: %#x'%address)
 for address,raw in SERVER_INDIRECT.items():
  data=bytes.fromhex(raw)
  if pe.at(address,len(data))!=data:
   raise VerificationError('original cache indirect vtable call mismatch: %#x'%address)
 if pe.at(0x221ad817,1)!=b'\xcc' or pe.at(0x22145515,1)!=b'\xcc':
  raise VerificationError('original slot-2 method boundary mismatch')
 return dict(regions=regions,original_opcode_anchors=len(SERVER_ANCHORS),
    direct_calls=len(SERVER_CALLS),indirect_calls=len(SERVER_INDIRECT),
    scene_requires_kind=5,scene_candidate_extra_predicate_must_be_zero=True,
    scene_string_capacity=64,scene_unresolved_token=0xffffffff,
    modelsounds_requires_kind=3,modelsounds_requires_associated_object=True,
    modelsounds_unresolved_token=0xffffffff,
    limitation='x86 dispatch evidence only, not complete cache serialization or game module')
def audit(client,server):
 previous_audit(client,server)
 return dict(client=audit_client(PE32(client)),server=audit_server(PE32(server)))
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--client',type=Path,required=True)
 ap.add_argument('--server',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args()
 report=audit(a.client.read_bytes(),a.server.read_bytes())
 a.output.parent.mkdir(parents=True,exist_ok=True)
 a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('Verified 8 original HUD constructors and 3 original cache code regions')
if __name__=='__main__':main()
