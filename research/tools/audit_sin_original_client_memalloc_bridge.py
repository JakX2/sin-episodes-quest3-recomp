#!/usr/bin/env python3
"""Phase185: verify original 2006 SiN x86 client.dll tier0 heap forwarding.

Original DLLs are owner-local, never committed. Pin complete function hashes,
original import names/IAT locations, debug-state and tier0 vtable dispatch.
The code models the *boundary* only; the tier0.dll heap is not in client.dll.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'

class VerificationError(ValueError):
    pass

class PE32:
    def __init__(self, image:bytes):
        self.image=image
        if len(image)<0x100 or image[:2]!=b'MZ':raise VerificationError('invalid DOS header')
        pe=struct.unpack_from('<I',image,0x3c)[0]
        if pe+24>len(image) or image[pe:pe+4]!=b'PE\0\0':raise VerificationError('invalid PE signature')
        machine,nsections=struct.unpack_from('<HH',image,pe+4)
        sz=struct.unpack_from('<H',image,pe+20)[0]
        opt=pe+24
        if machine!=0x14c or struct.unpack_from('<H',image,opt)[0]!=0x10b or nsections>96:
            raise VerificationError('not x86 PE32')
        if opt+sz+nsections*40>len(image):raise VerificationError('short PE sections')
        self.base=struct.unpack_from('<I',image,opt+28)[0]
        self.import_rva=struct.unpack_from('<I',image,opt+104)[0]
        self.sections=[]
        for k in range(nsections):
            off=opt+sz+k*40
            name=image[off:off+8].split(b'\0')[0].decode('ascii','replace')
            virtual_size,rva,raw_size,raw_off=struct.unpack_from('<IIII',image,off+8)
            self.sections.append((name,rva,virtual_size,raw_size,raw_off))

    def at(self,va:int,n:int)->bytes:
        if n<0:raise VerificationError('negative PE read')
        rva=va-self.base
        for name,start,virt,raw,off in self.sections:
            if start<=rva and rva-start+n<=raw:
                index=off+rva-start
                if index+n<=len(self.image):return self.image[index:index+n]
        raise VerificationError('original PE read outside raw section %#x'%va)

    def section(self,va:int)->str:
        rva=va-self.base
        for name,start,virt,raw,off in self.sections:
            if start<=rva<start+max(virt,raw):return name
        return ''

    def cstr(self,va:int,limit=256)->str:
        buf=bytearray()
        for i in range(limit):
            char=self.at(va+i,1)[0]
            if char==0:return buf.decode('ascii')
            buf.append(char)
        raise VerificationError('unterminated PE import string')

    def import_iat(self)->dict[tuple[str,str],int]:
        if not self.import_rva:raise VerificationError('missing native import table')
        imported={}
        for k in range(256):
            desc=self.at(self.base+self.import_rva+20*k,20)
            oft,stamp,chain,name_rva,iat_rva=struct.unpack('<IIIII',desc)
            if desc==bytes(20):return imported
            if not name_rva or not iat_rva:raise VerificationError('malformed PE imports')
            module=self.cstr(self.base+name_rva).lower()
            source=oft or iat_rva
            for i in range(2048):
                thunk=struct.unpack('<I',self.at(self.base+source+i*4,4))[0]
                if not thunk:break
                if thunk&0x80000000:continue
                symbol=self.cstr(self.base+thunk+2)
                key=(module,symbol)
                if key in imported:raise VerificationError('duplicate imported symbol')
                imported[key]=self.base+iat_rva+i*4
            else:raise VerificationError('import thunk loop too long')
        raise VerificationError('import descriptor loop too long')

REGIONS={
 'alloc_cached_a':(0x2416FCF0,79,'84fffc04db883f146d64edf1f474b61b5fe03cb36e95b5fcd717ece17e793acf'),
 'debug_alloc_direct_a':(0x2416FD40,27,'06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17'),
 'free_a':(0x2416FD60,18,'c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73'),
 'alloc_cached_b':(0x2416FD80,79,'44318a97f116b1b629cb2fe1794e14c28ed58b61a7606b7784a4e7b785c20d08'),
 'debug_alloc_direct_b':(0x2416FDD0,27,'06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17'),
 'free_b':(0x2416FDF0,18,'c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73'),
 'allocator_present':(0x2416FE10,14,'e0cbbd04cffa5347279a626191f42bafb3f112ce195ffd26d344f03b26a896e4'),
}
# Intentionally disjoint fragments; no whole original function bytes published.
OPCODES={
 0x2416FCF0:'b801000000',
 0x2416FCF5:'84050c293724',
 0x2416FCFB:'7512',
 0x2416FCFD:'09050c293724',
 0x2416FD08:'a308293724',
 0x2416FD0D:'eb05',
 0x2416FD0F:'a108293724',
 0x2416FD14:'85c0',
 0x2416FD16:'7512',
 0x2416FD18:'a1cc612724',
 0x2416FD26:'ff5204',
 0x2416FD2A:'8b0dcc612724',
 0x2416FD3C:'ff12',
 0x2416FD3E:'c3',
 0x2416FD40:'a1cc612724',
 0x2416FD58:'ff12',
 0x2416FD5A:'c3',
 0x2416FD60:'a1cc612724',
 0x2416FD6E:'ff5214',
 0x2416FD71:'c3',
 0x2416FD80:'b801000000',
 0x2416FD85:'84050c293724',
 0x2416FD8D:'09050c293724',
 0x2416FD98:'a308293724',
 0x2416FDA8:'a1cc612724',
 0x2416FDB6:'ff5204',
 0x2416FDCC:'ff12',
 0x2416FDD0:'a1cc612724',
 0x2416FDE8:'ff12',
 0x2416FDEA:'c3',
 0x2416FDF0:'a1cc612724',
 0x2416FDFE:'ff5214',
 0x2416FE01:'c3',
 0x2416FE10:'8b0dcc612724',
 0x2416FE16:'33c0',
 0x2416FE1A:'0f95c0',
 0x2416FE1D:'c3',
}
CALLS={0x2416FD03:0x2416F7F0,0x2416FD93:0x2416F7F0}
IMPORTS={('tier0.dll','g_pMemAlloc'):0x242761CC,
         ('vstdlib.dll','KeyValuesSystem'):0x24276230}

def verify_imports(imports:dict[tuple[str,str],int])->None:
    for k,va in IMPORTS.items():
        if imports.get(k)!=va:raise VerificationError('original import/IAT mismatch '+repr(k))

def verify_native(pe:PE32,*,whole_hash=True,parse_imports=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('original client image base mismatch')
    if parse_imports:verify_imports(pe.import_iat())
    for va,hexcode in OPCODES.items():
        fragment=bytes.fromhex(hexcode)
        if pe.at(va,len(fragment))!=fragment:
            raise VerificationError('original heap opcode witness mismatch %#x'%va)
    for source,dest in CALLS.items():
        relative=b'\xe8'+struct.pack('<i',dest-source-5)
        if pe.at(source,5)!=relative:
            raise VerificationError('original debug resolver CALL mismatch %#x'%source)
    if whole_hash:
        for name,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError('original complete heap bridge function SHA mismatch '+name)
    return {
      'verified_native_functions':len(REGIONS) if whole_hash else 0,
      'complete_native_region_bytes':sum(n for _,n,_ in REGIONS.values()) if whole_hash else 0,
      'native_opcode_witnesses':len(OPCODES),
      'native_direct_call_edges':len(CALLS),
      'original_tier0_import_iat':'0x242761cc',
      'original_vstdlib_keyvalues_import_iat':'0x24276230',
      'native_cached_debug_owner':'0x24372908',
      'native_debug_flags':'0x2437290c',
      'native_debug_owner_resolver':'0x2416f7f0',
      'limits':[
        'Only client.dll forwarding and cached debug branch recovered: imported tier0.dll IMemAlloc is not decompiled.',
        'KeyValuesSystem is an import from vstdlib.dll, not an in-DLL implementation.',
        'No executable Windows Source engine game module, map or player simulation demonstrated.'
      ]
    }

def audit(client:bytes,server:bytes)->dict:
    if len(client)!=3592192 or len(server)!=6049792:
        raise VerificationError('unexpected original DLL length')
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA or hashlib.sha256(server).hexdigest()!=SERVER_SHA:
        raise VerificationError('original complete DLL SHA256 mismatch')
    return {'phase':185,'client_allocator_bridge':verify_native(PE32(client))}

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding='utf-8')
    else:print(result,end='')

if __name__=='__main__':main()