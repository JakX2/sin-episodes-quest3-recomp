#!/usr/bin/env python3
"""Phase98: exact original 2006 SoundEmitterSystem.dll/vgui2.dll vs
SourceVR 0.5.5 ARM64, plus chained original client.dll/server.dll.
Originals are privately checked by SHA256; JSON output contains metadata only.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_factories import PE32,VerificationError
from audit_sourcevr_native_sin_factories import ELF64
from audit_sourcevr_method_slot_compat import audit as prior_audit

ORIGINAL_SHA={
 'SoundEmitterSystem.dll':'8c92cdc58f366cde07a1e14b8d95bfc36abdb552242508993189479faa79dba5',
 'vgui2.dll':'b6a6a16f41e3dac8f5ffee1803b71d97bc4be376f0ee0e9d3fe68228001dd897'}
VTABLES={
 'old_sound':(0x2c019c88,36,'a774e77a808e744d441a4095b7c3bab8d0f3a2611aea834b7b99382268139c92'),
 'old_input':(0x100472ac,71,'03c7e3969272653459ad21d296aa4771643dcdfa62a01fd5bb8269549701b365'),
 'old_system':(0x1004b95c,33,'f49170def3240bbe74c11d9246d1d5c6f123a9cec49de9399c03e454cd93d30e')}
REGIONS={
 'old_sound':{
  'constructor':(0x2c0096d0,174,'9cd629ba754c09b14a5c84c9c15bead9c2914070bea12fb34a3c24c70e8edd68'),
  'name_to_token':(0x2c00a830,23,'2b50c0b51f219bd9d15b2063185dc60ef20db971dd82d117aa5ff17cfa42ff06'),
  'token_to_name':(0x2c008fc0,71,'5f65434fafff1a5e839afbce905aea344f95a5f5deab14bd9b4a4cd43a84845d')},
 'old_vgui':{
  'input_index_56':(0x10007d90,862,'d8c679d66933cf7be82e66975841655f4b42a5c487fb1955e19a6a7dcfc61086'),
  'system_index_24':(0x1001cb60,35,'d0b7c95e5541d68af2f9a1059f84d26bd440f6f94d6ff29bc43030eb8cd7f76f')},
 'native_sound':{
  'candidate_name_to_token_index_7':(0x133a0,336,'f2fd0908b3a15993aa23cda25464009aaedd3a250d2a156e73657e13a86aa07d'),
  'candidate_token_to_name_index_10':(0x13570,88,'4ab11e12272f034525b4f4ea1b4f5603d0700263c141385a8bc62cf02a8ed55e')},
 'native_vgui':{
  'input_index_56':(0x25ca0,184,'e0105f2de06f0f73749700acc13e5efb3d448437cf04cf88e4e9d02176774bff'),
  'system_index_24':(0x3de30,20,'bf0092b235f21ec08da8d4b35393986e0ea9a13c39651b4c3517b06496038b47')}}
X86_SITES={
 'old_sound':{
 0x2c0183d0:'682ca3012c',0x2c0183d5:'68f04b002c',
 0x2c0183da:'b95cdb012c',0x2c004bf0:'b858da012cc3',
 0x2c0096d4:'c706889c012c',0x2c00a830:'8b442404',
 0x2c00a835:'83c15c',0x2c00a83d:'83f8ff',
 0x2c00a844:'c20400',0x2c008fcb:'ff5010',
 0x2c008fd2:'b83c96012c',0x2c009004:'c20400'},
 'old_vgui':{
 0x100064e0:'b848370510c3',0x100064f0:'c706ac720410',
 0x10007dc3:'8b7c2414',0x10007dc7:'c6442f0401',
 0x10007dcc:'c6442f0e01',0x100080eb:'c20400',
 0x1001cb60:'e83b5c0000',0x1001cb65:'8b442404',
 0x1001cb6d:'3d00010000',0x1001cb74:'8b048570320510'}}
ARM_SITES={
 'native_sound':{
  0x133b4:0xb40003c1,0x133f4:0x39400288,
  0x13498:0x94000fe2,0x1349c:0x3100041f,
  0x134ac:0x79400513,0x134b4:0x12800013,0x134d8:0x2a1303e0,
  0x1357c:0xf9400008,0x13588:0xf9402108,
  0x135a4:0x12003e69,0x135ac:0xf8696908,
  0x135b4:0xf100011f,0x135b8:0x9a880000},
 'native_vgui':{
  0x25ca0:0xb40005a1,0x25cb0:0xb98e2008,0x25ce0:0xf9453a60,
  0x25d04:0xeb01019f,0x25d08:0x540000e0,
  0x25d3c:0x9400f2d9,0x25d44:0x51000508,
  0x3de30:0x90000168,0x3de34:0xf9411108,
  0x3de38:0xf9400108,0x3de3c:0xf9403d00,
  0x3de40:0xd61f0000}}
def u32(pe,va):return struct.unpack('<I',pe.at(va,4))[0]
def verify_group(pe,group,strict=True):
 for site,hexcode in X86_SITES.get(group,{}).items():
  op=bytes.fromhex(hexcode)
  if pe.at(site,len(op))!=op:raise VerificationError('Phase98 x86 opcode at %#x'%site)
 for site,op in ARM_SITES.get(group,{}).items():
  if pe.at(site,4)!=struct.pack('<I',op):
   raise VerificationError('Phase98 ARM64 opcode at %#x'%site)
 checks={}
 for name,(va,size,sha) in REGIONS[group].items():
  actual=hashlib.sha256(pe.at(va,size)).hexdigest()
  if strict and actual!=sha:raise VerificationError('Phase98 original-region SHA mismatch '+name)
  checks[name]={'original_va':hex(va),'size':size,'sha256':actual}
 return {'regions':checks,'opcode_sites':
    len(X86_SITES.get(group,{}))+len(ARM_SITES.get(group,{}))}

def vtable(pe,key,strict=True):
 va,count,sha=VTABLES[key];raw=pe.at(va,count*4)
 if strict and hashlib.sha256(raw).hexdigest()!=sha:
  raise VerificationError('Phase98 complete vtable SHA mismatch '+key)
 methods=struct.unpack('<%dI'%count,raw)
 return {'address':hex(va),'count':count,'sha256':hashlib.sha256(raw).hexdigest(),
         'methods':[hex(v) for v in methods]}
def old_sound(pe,strict=True):
 if pe.base!=0x2c000000 or pe.exports().get('CreateInterface')!=0x2c001230:
  raise VerificationError('original sound factory export mismatch')
 if pe.cstring(0x2c01a32c)!='VSoundEmitter001':
  raise VerificationError('original sound version missing')
 if pe.cstring(0x2c01963c)!='':raise VerificationError('original sound empty fallback')
 output=verify_group(pe,'old_sound',strict);table=vtable(pe,'old_sound',strict)
 if table['methods'][3]!='0x2c00a830' or table['methods'][6]!='0x2c008fc0':
  raise VerificationError('legacy emitter method position changed')
 output.update(vtable=table,version='VSoundEmitter001',
  factory_callback_va='0x2C004BF0',singleton_va='0x2C01DA58',
  original_index3_operation='name-to-token (32-bit result, -1 on missing)',
  original_index6_operation='full signed-32 index validation, name pointer or empty')
 return output
def old_vgui(pe,strict=True):
 if pe.base!=0x10000000 or pe.exports().get('CreateInterface')!=0x1000a520:
  raise VerificationError('original VGUI factory export mismatch')
 for va,name in [(0x10049404,'VGUI_InputInternal001'),(0x1004ba90,'VGUI_System010')]:
  if pe.cstring(va)!=name:raise VerificationError('original VGUI factory name changed')
 for at,expected in [(0x10051f08,0x100064e0),(0x10051f0c,0x10049404),
                     (0x100523b8,0x1001bdb0),(0x100523bc,0x1004ba90)]:
  if u32(pe,at)!=expected:raise VerificationError('original VGUI registration node mismatch')
 out=verify_group(pe,'old_vgui',strict)
 tables={'input':vtable(pe,'old_input',strict),'system':vtable(pe,'old_system',strict)}
 if tables['input']['methods'][56]!='0x10007d90' or tables['system']['methods'][24]!='0x1001cb60':
  raise VerificationError('original VGUI slot index changed')
 out.update(vtables=tables,original_input_index56='integer 0/1 to byte state flags',
  original_system_index24='integer key 0..255 translated through table 0x10053270')
 return out
def native(sound,vgui,strict=True):
 from audit_sourcevr_native_sin_factories import ELF64
 groups={'native_sound':ELF64(sound),'native_vgui':ELF64(vgui)}
 result={group:verify_group(pe,group,strict) for group,pe in groups.items()}
 result['sound_candidates']=[
    {'original_index':3,'native_candidate_index':7,'native_va':'0x133A0',
     'evidence':'string hash lookup, u16 result or -1','ABI_verified':False},
    {'original_index':6,'native_candidate_index':10,'native_va':'0x13570',
     'evidence':'validity via native slot8, mask index to u16, string or empty','ABI_verified':False}]
 result['vgui_proven_mismatches']=[
    {'index':56,'old_va':'0x10007D90','native_va':'0x25CA0',
     'evidence':'old uses integer 0/1 and writes byte flags; native removes pointer from list',
     'can_forward':False},
    {'index':24,'old_va':'0x1001CB60','native_va':'0x3DE30',
     'evidence':'old translates integer key from table; native tail-branches via global function pointer',
     'can_forward':False}]
 return result
def audit(client,server,original_sound,original_vgui,native_sound,native_vgui):
 prior_audit(client,server,native_sound,native_vgui) # all 4 prior exact SHA and vtables
 for label,data in [('SoundEmitterSystem.dll',original_sound),('vgui2.dll',original_vgui)]:
  if hashlib.sha256(data).hexdigest()!=ORIGINAL_SHA[label]:
   raise VerificationError('Phase98 original 2006 '+label+' SHA-256 mismatch')
 return {'original_sound':old_sound(PE32(original_sound)),
         'original_vgui':old_vgui(PE32(original_vgui)),
         'native':native(native_sound,native_vgui),
         'verdict':'Sound 7/10 are candidates, no live adapter. Input 56 and System 24 are proven mismatches.',
         'quest3_game_running':False}
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('client','server','old-sound','old-vgui','native-sound','native-vgui','output'):
  p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args()
 result=audit(a.client.read_bytes(),a.server.read_bytes(),a.old_sound.read_bytes(),
              a.old_vgui.read_bytes(),a.native_sound.read_bytes(),a.native_vgui.read_bytes())
 a.output.parent.mkdir(parents=True,exist_ok=True)
 a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print(result['verdict'])
if __name__=='__main__':main()
