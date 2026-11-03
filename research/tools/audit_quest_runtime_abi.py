#!/usr/bin/env python3
"""Read-only HL2VR 0.989 standalone Android runtime inventory.

Accepts an APK or ZIP holding exactly one APK. Examines ELF headers,
dependencies, exports and advertised interface text without executing code.
Game/library bytes are not written to the report or committed to source.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import zipfile
import shutil

INSTALLED='lib/arm64-v8a/'
TURBO='assets/hl2quest_turbo/arm64-v8a/'
REQUIRED=('libengine.so','libclient.so','libserver.so','libsourcevr.so',
          'libopenxr_loader.so','libSDL2.so')
FACTORY=('libengine.so','libclient.so','libserver.so','libsourcevr.so')
MARKERS=('VClient017','ServerGameDLL010','VEngineClient014',
         'VEngineServer023','VClientEntityList003','VClientPrediction001',
         'VEngineCvar004','VFileSystem022','VMaterialSystem081')

class AuditError(ValueError): pass

def cstr(data:bytes,pos:int)->str:
    if not 0<=pos<len(data):return ''
    end=data.find(b'\x00',pos)
    return data[pos:end if end>=0 else len(data)].decode('utf-8','replace')

def inspect_elf(data:bytes)->dict:
    if len(data)<64 or data[:4]!=b'\x7fELF' or data[4:6]!=b'\x02\x01':
        raise AuditError('expected little-endian ELF64')
    if struct.unpack_from('<H',data,18)[0]!=183:
        raise AuditError('expected AArch64 (e_machine 183)')
    if struct.unpack_from('<H',data,16)[0]!=3:
        raise AuditError('expected ELF shared object ET_DYN')
    shoff=struct.unpack_from('<Q',data,40)[0]
    entsize,num,namesidx=struct.unpack_from('<HHH',data,58)
    if num==0 or entsize<64 or namesidx>=num or shoff+num*entsize>len(data):
        raise AuditError('invalid ELF section table')
    headers=[]
    for i in range(num):
        v=struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*entsize)
        record=dict(nameidx=v[0],kind=v[1],offset=v[4],size=v[5],entry=v[9])
        if record['kind']!=8 and record['offset']+record['size']>len(data):
            raise AuditError('section points outside ELF')
        headers.append(record)
    names_sec=headers[namesidx]
    names=data[names_sec['offset']:names_sec['offset']+names_sec['size']]
    sections={cstr(names,h['nameidx']):h for h in headers}
    syms,strings=sections.get('.dynsym'),sections.get('.dynstr')
    if not syms or not strings or syms['entry']<24:
        raise AuditError('missing ELF dynamic symbols')
    stringdata=data[strings['offset']:strings['offset']+strings['size']]
    exports=[]
    for pos in range(syms['offset'],syms['offset']+syms['size'],syms['entry']):
        if pos+24>len(data):break
        n,info,other,sec,val,size=struct.unpack_from('<IBBHQQ',data,pos)
        if sec and (info>>4) in (1,2):
            exports.append(cstr(stringdata,n))
    dependency=[];soname=None
    dynamic=sections.get('.dynamic')
    if dynamic and dynamic['entry']>=16:
        for pos in range(dynamic['offset'],dynamic['offset']+dynamic['size'],dynamic['entry']):
            if pos+16>len(data):break
            tag,offset=struct.unpack_from('<qQ',data,pos)
            if tag==0:break
            if tag==1:dependency.append(cstr(stringdata,offset))
            if tag==14:soname=cstr(stringdata,offset)
    note=sections.get('.note.gnu.build-id')
    buildid=None
    if note and note['size']>=12:
        raw=data[note['offset']:note['offset']+note['size']]
        namelen,desclen,kind=struct.unpack_from('<III',raw,0)
        start=12+((namelen+3)//4)*4
        if kind==3 and raw[12:12+namelen].rstrip(b'\x00')==b'GNU' and start+desclen<=len(raw):
            buildid=raw[start:start+desclen].hex()
    markers=[m for m in MARKERS if re.search(
        rb'(?<![a-zA-Z0-9_])'+re.escape(m.encode())+rb'\x00',data)]
    return dict(architecture='AArch64 ELF64',soname=soname,needed=dependency,
                build_id=buildid,create_interface_exported='CreateInterface' in exports,
                interface_string_candidates=markers,dynamic_export_count=len(exports))

def inspect_apk(archive:zipfile.ZipFile)->dict:
    names=archive.namelist()
    def group(prefix:str):
        result={}
        for name in names:
            if not name.startswith(prefix) or not name.endswith('.so'):continue
            libname=name[len(prefix):]
            if '/' in libname:continue
            data=archive.read(name)
            result[libname]=dict(size_bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),
                                 **inspect_elf(data))
        return result
    regular=group(INSTALLED);turbo=group(TURBO)
    missing=sorted(set(REQUIRED)-set(regular))
    if missing:raise AuditError('missing required installed libs: '+', '.join(missing))
    no_factory=sorted(n for n in FACTORY if not regular[n]['create_interface_exported'])
    if no_factory:raise AuditError('CreateInterface export absent: '+', '.join(no_factory))
    common=set(regular)&set(turbo)
    return dict(installed_count=len(regular),turbo_count=len(turbo),
        installed_only=sorted(set(regular)-set(turbo)),
        turbo_only=sorted(set(turbo)-set(regular)),
        matching_variants=sorted(n for n in common if regular[n]['sha256']==turbo[n]['sha256']),
        different_variants=sorted(n for n in common if regular[n]['sha256']!=turbo[n]['sha256']),
        no_sin_named_modules=not any('sin' in n.lower() for n in regular),
        installed_libraries=regular,turbo_libraries=turbo,
        warning='CreateInterface export and interface-looking strings do NOT prove SiN ABI compatibility or runnable Quest gameplay.')

def analyze(path:Path)->dict:
    with zipfile.ZipFile(path) as wrapper:
        if any(n.startswith(INSTALLED) for n in wrapper.namelist()):
            result=inspect_apk(wrapper);result['container']='APK'
        else:
            apk_names=[n for n in wrapper.namelist() if n.lower().endswith('.apk')]
            if len(apk_names)!=1:raise AuditError('ZIP must contain exactly one APK')
            with tempfile.TemporaryFile() as tmp:
                with wrapper.open(apk_names[0]) as source:
                    shutil.copyfileobj(source,tmp,8*1024*1024)
                tmp.seek(0)
                with zipfile.ZipFile(tmp) as inner:
                    result=inspect_apk(inner)
            result['container']='ZIP containing APK'
            result['inner_apk_name']=apk_names[0]
    result['source_size_bytes']=path.stat().st_size
    with path.open('rb') as source:
        result['source_sha256']=hashlib.file_digest(source,'sha256').hexdigest()
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--apk-or-zip',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    result=analyze(a.apk_or_zip)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items()
        if k not in ('installed_libraries','turbo_libraries')},indent=2))
if __name__=='__main__':main()
