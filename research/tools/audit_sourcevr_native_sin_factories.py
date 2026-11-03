#!/usr/bin/env python3
"""Read-only, SHA-pinned SourceVR 0.5.5 native ARM64 factory registration audit.

Chains SHA-verified original SiN 2006 client.dll/server.dll evidence. Accepts
the final-volume SourceVR archive with the two small ELF blobs accessible.
Never redistributes the original PE32 DLLs or ARM64 libraries.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_loading_panel_emitter import audit as audit_original_sin
from audit_sin_original_factories import PE32, VerificationError

PROFILES=("hl2","ep1","ep2","lostcoast","portal","portal2","entropyzero")
ZIP_PREFIX="source vr/assets/"
SOUND_SHA="484e5bb2c1457ee6e62f39d47c83bc68b8cee491c2f1479886a8e0d93740c454"
VGUI_SHA="7b7b7ec9f8c3994f9bd72a6a9820dd0f1057e569e86f01a8cb40462d94e3fa12"
NATIVE={"libsoundemittersystem.so":SOUND_SHA,"libvgui2.so":VGUI_SHA}

# Exact AArch64 opcodes: name, static registration-node, callback, registrar,
# exported CreateInterface's strcmp and result-code/error branches.
SOUND_INST={
 0x18DA0:0x90000180,0x18DA4:0x91146000, # registry node 0x48518
 0x18DA8:0xD0FFFFE1,0x18DAC:0x910CC021, # callback 0x16330
 0x18DB0:0xD0FFFF42,0x18DB4:0x9121E442, # version name 0x2879
 0x18DC0:0x14000528,                    # registrar 0x1A260
 0x16334:0x101905A0,0x16338:0xD65F03C0, # singleton callback 0x483E8
 0x1A264:0xA9000801,0x1A26C:0xF9037D00,0x1A270:0xF9000809,
 0x1A324:0xF94006A0,0x1A32C:0x94008E0D, # strcmp
 0x1A344:0xB9000268,0x1A348:0xAA1F03E0, # missing status 1, null
 0x1A360:0xB900027F,0x1A364:0xF94002A0, # found status 0, callback
}
VGUI_INST={
 0x26A3C:0xB0000240,0x26A40:0x91156000, # InputInternal node 0x6F558
 0x26A44:0x90FFFFE1,0x26A48:0x910BC021, # callback 0x222F0
 0x26A4C:0xF0FFFEE2,0x26A50:0x91135042, # name 0x54D4
 0x26A5C:0x140068E9,                   # registrar 0x40E00
 0x3E04C:0xF0000180,0x3E050:0x9115A000, # System node 0x71568
 0x3E054:0xF0FFFFE1,0x3E058:0x91134021, # callback 0x3D4D0
 0x3E060:0x50E42762,0x3E06C:0x14000B65, # name 0x654E; registrar 0x40E00
 0x40E04:0xA9000801,0x40E0C:0xF903A100,0x40E10:0xF9000809,
 0x40EC4:0xF94006A0,0x40ECC:0x94008671, # strcmp
 0x40EE4:0xB9000268,0x40EE8:0xAA1F03E0, # missing
 0x40F00:0xB900027F,0x40F04:0xF94002A0, # found
}

class ELF64:
    """Minimal stdlib ELF64 section and dynamic-symbol parser."""
    def __init__(self,data:bytes):
        self.data=data
        if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01':
            raise VerificationError('expected ELF64 little-endian file')
        typ,machine=struct.unpack_from('<HH',data,16)
        if (typ,machine)!=(3,183):
            raise VerificationError('expected AArch64 ET_DYN, not Windows x86')
        offset=struct.unpack_from('<Q',data,40)[0]
        entsize,count,strings_index=struct.unpack_from('<HHH',data,58)
        if entsize<64 or not 0<count<=300 or strings_index>=count or offset+entsize*count>len(data):
            raise VerificationError('invalid ELF64 section headers')
        headers=[struct.unpack_from('<IIQQQQIIQQ',data,offset+i*entsize) for i in range(count)]
        st=headers[strings_index]
        names=self._slice(st[4],st[5])
        self.sections={}
        for h in headers:
            if h[0]>=len(names):raise VerificationError('invalid section name index')
            name=names[h[0]:].split(b'\0',1)[0].decode('ascii','replace')
            if name and name not in self.sections:self.sections[name]=h
    def _slice(self,off,n):
        if off<0 or n<0 or off+n>len(self.data):
            raise VerificationError('ELF section beyond file')
        return self.data[off:off+n]
    def section(self,name):
        h=self.sections[name]
        if h[1]==8:raise VerificationError('cannot read NOBITS section')
        return self._slice(h[4],h[5])
    def at(self,va,n):
        for h in self.sections.values():
            if h[1]!=8 and h[3]<=va and va+n<=h[3]+h[5]:
                return self._slice(h[4]+va-h[3],n)
        raise VerificationError('unmapped ELF VA %#x'%va)
    def symbol(self,name):
        h=self.sections['.dynsym']
        if (h[9] or 24)!=24:raise VerificationError('invalid ARM64 dynamic symbol size')
        raw=self.section('.dynsym')
        string_data=self.section('.dynstr')
        for i in range(0,len(raw)-23,24):
            ix,info,_,_,value,size=struct.unpack_from('<IBBHQQ',raw,i)
            if ix>=len(string_data):continue
            if string_data[ix:].split(b'\0',1)[0].decode('utf-8','replace')==name:
                return {'va':value,'size':size,'type':info&15}
        raise VerificationError('missing native export '+name)
    def init_array(self):
        raw=self.section('.init_array')
        if len(raw)%8:raise VerificationError('malformed init_array')
        return struct.unpack('<%dQ'%(len(raw)//8),raw)

def verify_native_registration(elf,module,check_sha=True):
    actual=hashlib.sha256(elf.data).hexdigest()
    if check_sha and actual!=NATIVE[module]:
        raise VerificationError('SourceVR selected native SHA-256 mismatch: '+module)
    if module=='libsoundemittersystem.so':
        words=SOUND_INST;start=0x18D50;export=0x1A300;registrar=0x1A260
        entries={'VSoundEmitter002':{'name_va':0x2879,'node_va':0x48518,
            'callback_va':0x16330,'singleton_va':0x483E8}}
        if b'VSoundEmitter001\0' in elf.data:
            raise VerificationError('unexpected legacy version string in selected native emitter')
    elif module=='libvgui2.so':
        words=VGUI_INST;start=0x269D0;export=0x40EA0;registrar=0x40E00
        entries={
            'VGUI_InputInternal001':{'name_va':0x54D4,'node_va':0x6F558,'callback_va':0x222F0},
            'VGUI_System010':{'name_va':0x654E,'node_va':0x71568,'callback_va':0x3D4D0}}
        if 0x3DFB0 not in elf.init_array():
            raise VerificationError('VGUI System initializer missing from init_array')
    else:raise VerificationError('unknown audited native module '+module)
    sym=elf.symbol('CreateInterface')
    if sym!={'va':export,'size':120,'type':2}:
        raise VerificationError('native CreateInterface exported location/signature mismatch')
    if start not in elf.init_array():
        raise VerificationError('native registration initializer missing from init_array')
    for va,op in words.items():
        if elf.at(va,4)!=struct.pack('<I',op):
            raise VerificationError('native factory opcode mismatch at %#x'%va)
    for name,item in entries.items():
        if elf.at(item['name_va'],len(name)+1)!=name.encode()+b'\0':
            raise VerificationError('native registered name mismatch: '+name)
    return {'module':module,'selected_sha256':actual,'create_interface_va':hex(export),
        'interface_registrar_va':hex(registrar),
        'registration_node_layout_64bit':{'factory_callback_offset':0,
            'interface_name_offset':8,'next_offset':16},
        'exact_case_sensitive_name_match':True,
        'missing_return_status':1,'found_return_status':0,
        'init_array_contains_registration':True,
        'native_registration_entries':entries,
        'inspected_original_opcode_sites':len(words),
        'abi_methods_verified':False}

def manifest_rows(z,profile):
    path=ZIP_PREFIX+'sourcevr/v1/native/'+profile+'.manifest'
    lines=z.read(path).decode('utf-8').splitlines()
    if not lines or lines[0]!=f'SOURCEVR_NATIVE_V1\t{profile}':
        raise VerificationError('invalid SourceVR native manifest: '+profile)
    rows={}
    for raw in lines[1:]:
        if not raw:continue
        cols=raw.split('\t')
        if len(cols)<5:raise VerificationError('invalid native manifest row')
        name=cols[2]
        if name in rows:raise VerificationError('duplicate native library in manifest: '+name)
        rows[name]={'sha256':cols[1],'asset':cols[4]}
    return rows

def audit_distribution(z,strict_hash=True):
    profiles={};entries={}
    for profile in PROFILES:
        rows=manifest_rows(z,profile)
        profiles[profile]={}
        for lib,digest in NATIVE.items():
            if lib not in rows:raise VerificationError(profile+' missing '+lib)
            row=rows[lib]
            if strict_hash and row['sha256']!=digest:
                raise VerificationError(profile+' selects unexpected '+lib+' SHA-256')
            profiles[profile][lib]=row['sha256']
            entries[lib]=ZIP_PREFIX+row['asset']
    if ZIP_PREFIX+'sourcevr/v1/native/sin.manifest' in z.namelist():
        raise VerificationError('SiN native profile now exists: re-audit required')
    modules={}
    for lib,path in entries.items():
        modules[lib]=verify_native_registration(ELF64(z.read(path)),lib,strict_hash)
    return {'profiles':profiles,'selected_native':modules,
        'sin_specific_native_profile_found':False,
        'distribution_constraint':'Some client/server/engine blobs need earlier multipart archive volumes'}

def audit(original_client,original_server,distribution):
    prev=audit_original_sin(original_client,original_server)
    client=PE32(original_client)
    if (client.cstring(0x242AC50C),client.cstring(0x242D32C0))!=(
        'VGUI_InputInternal001','VGUI_System010'):
        raise VerificationError('original SiN client VGUI requests differ')
    if prev['server']['original_interface']!='VSoundEmitter001':
        raise VerificationError('original SiN sound emitter interface differs')
    native=audit_distribution(distribution)
    return {'original_sin':{
        'client_required_vgui':['VGUI_InputInternal001','VGUI_System010'],
        'server_required_sound_emitter':'VSoundEmitter001',
        'original_client_sha256':hashlib.sha256(original_client).hexdigest(),
        'original_server_sha256':hashlib.sha256(original_server).hexdigest()},
        'sourcevr':native,'compatibility':[
        {'requirement':'VSoundEmitter001','sourcevr_registered':'VSoundEmitter002',
         'result':'version_mismatch_requires_verified_adapter','can_invoke_native_vtable':False},
        {'requirement':'VGUI_InputInternal001','sourcevr_registered':'VGUI_InputInternal001',
         'result':'factory_name_match_methods_unverified','can_invoke_native_vtable':False},
        {'requirement':'VGUI_System010','sourcevr_registered':'VGUI_System010',
         'result':'factory_name_match_methods_unverified','can_invoke_native_vtable':False}],
        'not_claimed':['playable Quest 3 port','old/new game module ABI compatibility',
                       'complete legacy sound emitter method signatures']}
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--client',type=Path,required=True)
    ap.add_argument('--server',type=Path,required=True)
    ap.add_argument('--sourcevr-zip',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    with zipfile.ZipFile(args.sourcevr_zip) as z:
        result=audit(args.client.read_bytes(),args.server.read_bytes(),z)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('SourceVR ARM64 VSoundEmitter002 mismatch; VGUI names match, ABI unverified')
if __name__=='__main__':main()
