#!/usr/bin/env python3
"""Original SiN PE32 vs real SourceVR 0.5.5 ARM64 vtable-slot verifier.

Verifies complete method pointer tables, original x86 callsite opcodes,
native constructor/code fingerprints, factory registration and original DLL
SHA-256 through prior phases. Outputs metadata, never original binaries.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sourcevr_native_sin_factories import (
    ELF64,verify_native_registration,audit_original_sin)
from audit_sin_original_factories import PE32,VerificationError

TABLES={
 'sound':('libsoundemittersystem.so',0x41f98,47,'4aacb2497a53c6e96462fa27a4107344924d42b3cc2a9da95f99ff692cf65c88'),
 'input':('libvgui2.so',0x670c8,89,'f80d17a906a17c5cd70da0ac42f2a8dc9ff44e5b6df98c4ff3db72eb3b5160b6'),
 'system':('libvgui2.so',0x67cb0,40,'1a6186a109e05a47b21e700847683df27f0ff8a8ca4e8afbec4989b0aea4d58c')}
AARCH64={
 'libsoundemittersystem.so':{
  0x127b0:0xf0000168,0x127b4:0x913e6108,0x127c0:0xf9000008,
  0x16334:0x101905a0,0x16338:0xd65f03c0,
  0x12970:0x52800020,0x12974:0xd65f03c0,
  0x13380:0xb940d008,0x13384:0x71000508,0x13388:0xb900d008,
  0x1338c:0x5400004d,0x13390:0xd65f03c0,
  0x13570:0xa9be7bfd,0x13588:0xf9402108,
  0x13e50:0xb9405009,0x13e68:0xaa1f03e0,0x13e98:0x91002100},
 'libvgui2.so':{
  0x222f4:0x10261f60,0x222f8:0xd65f03c0,
  0x22314:0xb0000228,0x22318:0x91032108,0x2231c:0xf9000008,
  0x3d4d4:0x1019ea20,0x3d4d8:0xd65f03c0,
  0x3d4ec:0xd0000148,0x3d4f0:0x9132c108,0x3d508:0xf9000008}}
X86={
 'server.dll':{
   0x221456ee:'ff500c',0x2214542e:'ff520c',
   0x221a739f:'ff5218',0x2214380f:'ff5218'},
 'client.dll':{
   0x240fa345:'ff90dc000000',
   0x240f97c9:'ff90e0000000',0x240f97e2:'ff92e0000000',
   0x240f97fb:'ff90e8000000',0x240f9814:'ff92e8000000',
   0x240f987a:'ff97f4000000',0x240f98c5:'ff92f8000000',
   0x240f9870:'ff5260'}}
VGUI_SLOTS={'input':{55:0xdc,56:0xe0,58:0xe8,61:0xf4,62:0xf8},
             'system':{24:0x60}}
def validate_table(elf,definition,strict=True):
    module,va,count,digest=definition
    raw=elf.at(va,count*8)
    sha=hashlib.sha256(raw).hexdigest()
    if strict and sha!=digest:
        raise VerificationError('Phase97 native vtable SHA-256 mismatch: '+module)
    ptrs=struct.unpack('<%dQ'%count,raw)
    text=elf.sections['.text']
    for i,ptr in enumerate(ptrs):
        if not text[3]<=ptr<text[3]+text[5]:
            raise VerificationError('Phase97 non-executable vtable method #%d'%i)
    if struct.unpack('<Q',elf.at(va+count*8,8))[0]:
        raise VerificationError('Phase97 native vtable boundary changed')
    return {'va':hex(va),'method_count':count,'sha256':sha,
            'native_pointer_width':8,'method_vas':[hex(p) for p in ptrs]}
def inspect_native(sound,vgui,strict=True,registrations=True):
    modules={'libsoundemittersystem.so':ELF64(sound),'libvgui2.so':ELF64(vgui)}
    for name,elf in modules.items():
        if registrations:verify_native_registration(elf,name,strict)
        for va,op in AARCH64[name].items():
            if elf.at(va,4)!=struct.pack('<I',op):
                raise VerificationError('Phase97 AArch64 opcode mismatch at %#x'%va)
    vtables={key:validate_table(modules[d[0]],d,strict) for key,d in TABLES.items()}
    sound_slots=[]
    for slot,operation,behavior,expected in (
            (3,'name_to_token','constant_true',0x12970),
            (6,'token_to_name','decrement_reference_count',0x13380)):
        actual=int(vtables['sound']['method_vas'][slot],16)
        if actual!=expected:
            raise VerificationError('Phase97 sound emitter native slot index changed')
        sound_slots.append({'index':slot,'original_required':operation,
            'native_observed_behavior':behavior,'native_method_va':hex(actual),
            'original_x86_byte_offset':hex(slot*4),
            'same_index_arm64_byte_offset':hex(slot*8),
            'compatibility':'proven_incompatible'})
    vgui_methods=[]
    for key,slots in VGUI_SLOTS.items():
        for slot,old in slots.items():
            vgui_methods.append({'interface':'VGUI_InputInternal001' if key=='input' else 'VGUI_System010',
                'index':slot,'original_x86_byte_offset':hex(old),
                'arm64_byte_offset':hex(slot*8),
                'actual_native_method_va':vtables[key]['method_vas'][slot],
                'method_signatures_verified':False})
    return {'verified_complete_native_vtables':vtables,
      'native_opcode_anchors':sum(map(len,AARCH64.values())),
      'sound_emitter_method_mismatches':sound_slots,
      'original_client_used_vgui_slots':vgui_methods,
      'safe_to_invoke_original_methods_on_native_objects':False}
def inspect_original(client,server):
    audit_original_sin(client,server)
    for name,data in (('client.dll',client),('server.dll',server)):
        pe=PE32(data)
        for va,raw in X86[name].items():
            if pe.at(va,len(bytes.fromhex(raw)))!=bytes.fromhex(raw):
                raise VerificationError('Phase97 original x86 method call mismatch at %#x'%va)
    return {'client_sha256':hashlib.sha256(client).hexdigest(),
            'server_sha256':hashlib.sha256(server).hexdigest(),
            'original_client_virtual_calls':len(X86['client.dll']),
            'original_server_virtual_calls':len(X86['server.dll'])}
def audit(client,server,sound,vgui):
    return {'original':inspect_original(client,server),
            'sourcevr_055':inspect_native(sound,vgui),
            'verdict':'Sound emitter 001/002 same-index alias disproven; VGUI method ABI unverified',
            'playable_quest3_port':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('client','server','native-sound','native-vgui','output'):
        p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args()
    r=audit(a.client.read_bytes(),a.server.read_bytes(),
      a.native_sound.read_bytes(),a.native_vgui.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print(r['verdict'])
if __name__=='__main__':main()
