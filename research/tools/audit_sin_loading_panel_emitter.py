#!/usr/bin/env python3
"""Read-only original SiN 2006 nested HUD and sound emitter evidence verifier.

Chains all Phase94 exact original SHA / x86 / RTTI checks. Reports metadata,
never copies or publishes original client.dll or server.dll bytes.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_loadingdisc_modelsound_events import audit as previous_audit
from audit_sin_original_factories import PE32,VerificationError

CLIENT_REGIONS={
 'CLoadingDiscPanel_constructor':(0x241ee350,222,'2c5e8fea3c7332248b021c97705ce2aa2b69ef35a402047b4bca1735a5eaf283'),
 'vgui_Label_constructor_prefix':(0x2401bac0,80,'233bece8e700106b6f5c03f93112a040910d551396d65510ed6b1681254b6196'),
}
SERVER_REGIONS={
 'VSoundEmitter001_Init_factory_sequence':(0x221e7792,25,'d94db11b145522c33f27fecc5beef3d659f04ee5df72d9e459100040b18674d9'),
}
CLIENT_CALLS={
 0x241ee35e:0x2400cf70,0x241ee370:0x240054e0,
 0x241ee379:0x24008880,0x241ee385:0x24005940,
 0x241ee38e:0x24004dc0,0x241ee397:0x24006770,
 0x241ee3a1:0x2416fcf0,0x241ee3ba:0x2401bac0,
 0x241ee3c7:0x240049d0,0x241ee3e9:0x2400db60,
 0x241ee3fa:0x24004b10,0x241ee3ff:0x240ffad0,
 0x241ee40e:0x240ffae0,0x241ee41f:0x24004a80,
}
CLIENT_ANCHORS={
 0x241ee355:'6898c82a24',
 0x241ee36a:'c706c4c52a24',
 0x241ee37e:'68b4b62924',
 0x241ee39c:'68f0000000',
 0x241ee3ad:'680e332d24',
 0x241ee3b2:'6888c82a24',
 0x241ee3cc:'89bed8000000',
 0x241ee3d8:'ff9200010000',
 0x241ee3e2:'6868c82a24',
 0x241ee42b:'c20400',
 0x2401bad7:'c706644d2d24',
}
CLIENT_STRING_VAS={
 0x242ac898:'CLoadingDiscPanel',
 0x242ac888:'LoadingLabel',
 0x2429b6b4:'ClientScheme',
 0x242ac868:'resource/LoadingDiscPanel.res',
}
SERVER_ANCHORS={
 0x221e7792:'6a00',
 0x221e7794:'683c294422',
 0x221e7799:'ffd6',
 0x221e77a0:'a3d0215722',
 0x221e77a5:'0f84f6010000',
 0x221e77dd:'8b0dd0215722',
 0x221e77e6:'ff10',
 0x221456e1:'8b0dd0215722',
 0x221456ee:'ff500c',
 0x221a7393:'8b0dd0215722',
 0x221a739f:'ff5218',
 0x22143803:'8b0dd0215722',
 0x2214380f:'ff5218',
 0x221ab0f4:'ff520c',
 0x2214542e:'ff520c',
}
def u32(pe,va):return struct.unpack('<I',pe.at(va,4))[0]
def check(pe,va,raw,kind):
 if pe.at(va,len(raw))!=raw:
  raise VerificationError('Phase95 '+kind+' differs at %#x'%va)
def verify_regions(pe,regions,strict):
 out={}
 for name,(va,n,sha) in regions.items():
  actual=hashlib.sha256(pe.at(va,n)).hexdigest()
  if strict and actual!=sha:raise VerificationError('Phase95 SHA-256 mismatch: '+name)
  out[name]={'va':hex(va),'bytes':n,'sha256':actual}
 return out
def check_rtti(pe,vtable,name):
 loc=u32(pe,vtable-4)
 if pe.at(loc,12)!=bytes(12):
  raise VerificationError('Phase95 original RTTI locator mismatch')
 desc=u32(pe,loc+12)
 if pe.cstring(desc+8)!=name:
  raise VerificationError('Phase95 original RTTI type mismatch')
 return {'vtable':hex(vtable),'locator':hex(loc),
         'type_descriptor':hex(desc),'decorated_class':name}
def inspect_client(pe,strict=True):
 regions=verify_regions(pe,CLIENT_REGIONS,strict)
 for va,raw in CLIENT_ANCHORS.items():check(pe,va,bytes.fromhex(raw),'client opcode')
 for va,target in CLIENT_CALLS.items():
  check(pe,va,b'\xe8'+struct.pack('<i',target-va-5),'client CALL')
 for va,name in CLIENT_STRING_VAS.items():
  if pe.cstring(va)!=name:raise VerificationError('Phase95 panel literal differs')
 panel=check_rtti(pe,0x242ac5c4,'.?AVCLoadingDiscPanel@@')
 label=check_rtti(pe,0x242d4d64,'.?AVLabel@vgui@@')
 if u32(pe,0x241ee36c)!=0x242ac5c4 or u32(pe,0x2401bad9)!=0x242d4d64:
  raise VerificationError('Phase95 panel or Label vtable immediate mismatch')
 check(pe,0x241ee42e,b'\xcc','panel constructor boundary')
 return {'regions':regions,'panel_rtti':panel,'nested_label_rtti':label,
         'panel_allocation_bytes':220,'nested_label_allocation_bytes':240,
         'nested_label_member_offset':216,
         'strings':list(CLIENT_STRING_VAS.values()),
         'client_original_call_edges':len(CLIENT_CALLS),
         'client_opcode_anchors':len(CLIENT_ANCHORS),
         'scope':'Original x86 class identity and constructor calls; GUI rendering not implemented'}
def inspect_server(pe,strict=True):
 regions=verify_regions(pe,SERVER_REGIONS,strict)
 for va,raw in SERVER_ANCHORS.items():check(pe,va,bytes.fromhex(raw),'server opcode')
 if pe.cstring(0x2244293c)!='VSoundEmitter001':
  raise VerificationError('VSoundEmitter001 original request string mismatch')
 return {'regions':regions,'interface_version':'VSoundEmitter001',
         'factory_request_va':'0x221E7799','global_pointer_va':'0x225721D0',
         'init_virtual_slot_byte_offset':0,
         'init_gate_va':'0x221E77E6',
         'name_to_token_virtual_offset':12,
         'token_to_name_virtual_offset':24,
         'name_to_token_call_sites':['0x221456EE','0x221AB0F4','0x2214542E'],
         'token_to_name_call_sites':['0x221A739F','0x2214380F'],
         'server_opcode_anchors':len(SERVER_ANCHORS),
         'scope':'Original PE32 interface mapping only; Android/SourceVR signatures not proven'}
def audit(client,server):
 previous_audit(client,server)
 return {'client':inspect_client(PE32(client)),
         'server':inspect_server(PE32(server)),
         'limits':'x86 metadata + mock reference; not real ARM64 game modules'}
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--client',required=True,type=Path)
 ap.add_argument('--server',required=True,type=Path)
 ap.add_argument('--output',required=True,type=Path)
 args=ap.parse_args()
 result=audit(args.client.read_bytes(),args.server.read_bytes())
 args.output.parent.mkdir(parents=True,exist_ok=True)
 args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('Original client CLoadingDiscPanel/vgui::Label and server VSoundEmitter001 verified')
if __name__=='__main__':main()
