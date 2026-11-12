#!/usr/bin/env python3
"""Phase186: original 2006 SiN PE32 allocator debug-owner resolution.

Original DLLs are supplied locally and MUST NOT be committed. Pin four complete
client.dll function hashes, sparse x86 witnesses, five relevant Windows/tier0
IAT imports, and five native rel32 CALLs to the owner resolver.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
class VerificationError(ValueError): pass

class PE32:
    def __init__(self, data: bytes):
        self.data=data
        if len(data)<0x100 or data[:2]!=b'MZ':raise VerificationError('not a DOS image')
        pe=struct.unpack_from('<I',data,0x3c)[0]
        if pe+24>len(data) or data[pe:pe+4]!=b'PE\0\0':raise VerificationError('not a PE image')
        machine,sections=struct.unpack_from('<HH',data,pe+4)
        opt=pe+24;opt_len=struct.unpack_from('<H',data,pe+20)[0]
        if machine!=0x14c or not (1<=sections<=96) or opt+opt_len+sections*40>len(data):
            raise VerificationError('not an x86 PE32 image')
        if struct.unpack_from('<H',data,opt)[0]!=0x10b:raise VerificationError('not PE32 optional header')
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        self.import_rva=struct.unpack_from('<I',data,opt+104)[0]
        self.sections=[]
        for i in range(sections):
            pos=opt+opt_len+40*i
            name=data[pos:pos+8].split(b'\0')[0].decode('ascii','replace')
            vsz,rva,raw_len,offset=struct.unpack_from('<IIII',data,pos+8)
            self.sections.append((name,rva,max(vsz,raw_len),raw_len,offset))

    def at(self,va:int,length:int)->bytes:
        rva=va-self.base
        if length<0:raise VerificationError('negative native read')
        for name,start,virtual_size,raw_len,offset in self.sections:
            if start<=rva and rva-start+length<=raw_len:
                f=offset+rva-start
                if f+length<=len(self.data):return self.data[f:f+length]
        raise VerificationError(f'original raw PE address outside image {va:#x}')

    def cstr(self,va:int,max_len=260)->str:
        s=bytearray()
        for off in range(max_len):
            x=self.at(va+off,1)[0]
            if x==0:return s.decode('ascii')
            s.append(x)
        raise VerificationError('import name not terminated')

    def imports(self)->dict[tuple[str,str],int]:
        if not self.import_rva:raise VerificationError('no import directory')
        found={}
        for i in range(256):
            d=self.at(self.base+self.import_rva+20*i,20)
            if d==bytes(20):return found
            oft,_,_,name_rva,iat_rva=struct.unpack('<IIIII',d)
            if not name_rva or not iat_rva:raise VerificationError('malformed imports')
            module=self.cstr(self.base+name_rva).lower()
            for j in range(2048):
                va=self.base+(oft or iat_rva)+4*j
                thunk=struct.unpack('<I',self.at(va,4))[0]
                if not thunk:break
                if thunk&0x80000000:continue
                symbol=self.cstr(self.base+thunk+2)
                key=(module,symbol)
                if key in found:raise VerificationError('duplicate import')
                found[key]=self.base+iat_rva+4*j
            else:raise VerificationError('import thunk array too large')
        raise VerificationError('import descriptor array too large')

REGIONS={
 'debug_owner_resolver':(0x2416F7F0,90,'c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0'),
 'single_size_esi':(0x2416F850,71,'e42a8f93b8166c2510985587937279800f654aa5ec7483aa6efec9cfaa6f200c'),
 'two_register_values':(0x2416F8A0,74,'fd969723fbe917bc9107e2920679fed8dcfd5c91809fe57db7330651d4dbe7ac'),
 'single_size_stack':(0x2416F8F0,79,'e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a'),
}
# Sparse, intentionally disjoint fragments from original .text. Original full
# disassembly/code bytes are not redistributed; whole SHA256 pins boundaries.
OPCODES={
 0x2416F7F0:'a1cc612724',0x2416F7FC:'ff525c',0x2416F7FF:'84c0',
 0x2416F801:'7441',0x2416F804:'6804010000',0x2416F809:'6a00',
 0x2416F80B:'ff1598602724',0x2416F812:'ff159c602724',
 0x2416F81A:'6a1c',0x2416F81C:'8d442408',0x2416F821:'6804293724',
 0x2416F826:'ff15a0602724',0x2416F82C:'8b4c2408',
 0x2416F830:'6804010000',0x2416F837:'ff15a4602724',
 0x2416F83D:'8bc6',0x2416F843:'c3',0x2416F844:'33c0',0x2416F849:'c3',
 0x2416F850:'b801000000',0x2416F855:'84050c293724',
 0x2416F85D:'09050c293724',0x2416F868:'a308293724',
 0x2416F86F:'a108293724',0x2416F878:'a1cc612724',
 0x2416F882:'ff5204',0x2416F894:'ff12',0x2416F896:'c3',
 0x2416F8A0:'b801000000',0x2416F8A5:'840514293724',
 0x2416F8AD:'090514293724',0x2416F8B8:'a310293724',
 0x2416F8BF:'a110293724',0x2416F8C8:'a1cc612724',
 0x2416F8D3:'ff520c',0x2416F8E6:'ff5208',0x2416F8E9:'c3',
 0x2416F8F0:'b801000000',0x2416F8F5:'84050c293724',
 0x2416F8FD:'09050c293724',0x2416F908:'a308293724',
 0x2416F90F:'a108293724',0x2416F918:'a1cc612724',
 0x2416F91F:'8b442404',0x2416F926:'ff5204',
 0x2416F937:'8b44240c',0x2416F93C:'ff12',0x2416F93E:'c3',
}
CALLS={
 0x2416F863:0x2416F7F0,
 0x2416F8B3:0x2416F7F0,
 0x2416F903:0x2416F7F0,
 0x2416FD03:0x2416F7F0, # previously recovered Phase185
 0x2416FD93:0x2416F7F0, # previously recovered Phase185
}
IMPORTS={
 ('tier0.dll','g_pMemAlloc'):0x242761CC,
 ('kernel32.dll','GetProcessHeap'):0x24276098,
 ('kernel32.dll','HeapAlloc'):0x2427609C,
 ('kernel32.dll','VirtualQuery'):0x242760A0,
 ('kernel32.dll','GetModuleFileNameA'):0x242760A4,
}

def verify_native(pe:PE32,*,check_hash=True,check_imports=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('unexpected client PE32 image base')
    if check_imports:
        imp=pe.imports()
        for key,va in IMPORTS.items():
            if imp.get(key)!=va:raise VerificationError(f'original imported API IAT mismatch {key}')
    for va,code in OPCODES.items():
        if pe.at(va,len(bytes.fromhex(code)))!=bytes.fromhex(code):
            raise VerificationError(f'original instruction witness mismatch {va:#x}')
    for source,target in CALLS.items():
        code=b'\xe8'+struct.pack('<i',target-source-5)
        if pe.at(source,5)!=code:
            raise VerificationError(f'original owner-resolver CALL mismatch {source:#x}')
    if check_hash:
        for name,(va,size,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
                raise VerificationError('original full function hash mismatch '+name)
    return {
      'original_complete_function_hashes':len(REGIONS) if check_hash else 0,
      'original_function_byte_total':sum(r[1] for r in REGIONS.values()) if check_hash else 0,
      'original_sparse_opcode_witnesses':len(OPCODES),
      'original_rel32_call_edges':len(CALLS),
      'original_imported_api_sites':len(IMPORTS),
      'resolver':hex(0x2416F7F0),
      'resolver_uses':['IMemAlloc virtual +0x5c','GetProcessHeap','HeapAlloc','VirtualQuery','GetModuleFileNameA'],
      'limits':['VirtualQuery success and HeapAlloc failure are not guarded by native code',
                'Neither tier0.dll allocator nor Source engine vtables have been implemented',
                'Host-neutral tests do not prove original SiN executable loading replacement DLLs'],
    }


def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA or hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('original full DLL file SHA256 mismatch')
    return {'phase':186,'original_client_debug_owner':verify_native(PE32(client_bytes))}


def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--client',type=Path,required=True)
    a.add_argument('--server',type=Path,required=True)
    a.add_argument('--output',type=Path)
    args=a.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()