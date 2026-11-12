#!/usr/bin/env python3
"""Phase192 owner-only SHA audit of two original 2006 cache-key comparators.

No original full native bodies, game binaries, or extracted content is included.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
CODE_SHA='77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4'
FUNCTIONS={
    'scene':(0x221A2F40,0x22503EE4),
    'modelsounds':(0x2213A3F0,0x224F038C),
}
# Only 11 of 17 native instruction bytes are embedded, disjoint evidence.
WITNESSES={0x00:'8b442404',0x08:'2408',0x0A:'3b0a',0x0E:'f7d8',0x10:'c3'}
LENGTH=17
BOUNDARY=b'\xcc'*15

class VerificationError(ValueError):pass

class PE32:
    def __init__(self,b:bytes):
        if len(b)<0x100 or b[:2]!=b'MZ':raise VerificationError('invalid DOS header')
        pe=struct.unpack_from('<I',b,0x3c)[0]
        if pe+24>len(b) or b[pe:pe+4]!=b'PE\0\0':raise VerificationError('invalid PE signature')
        machine,n=struct.unpack_from('<HH',b,pe+4)
        optsz=struct.unpack_from('<H',b,pe+20)[0];opt=pe+24
        if machine!=0x14c or not 1<=n<=96 or opt+optsz+40*n>len(b) or struct.unpack_from('<H',b,opt)[0]!=0x10b:
            raise VerificationError('expected bounded original i386 PE32')
        self.base=struct.unpack_from('<I',b,opt+28)[0]
        self.b=b;self.sections=[]
        for i in range(n):
            off=opt+optsz+40*i
            name=b[off:off+8].split(b'\0')[0].decode('ascii','replace')
            vsz,rva,size,raw=struct.unpack_from('<IIII',b,off+8)
            self.sections.append((name,rva,vsz,size,raw))
    def at(self,va:int,length:int)->bytes:
        if length<0:raise VerificationError('negative length')
        rva=va-self.base
        for _,r,vsz,sz,off in self.sections:
            if r<=rva and rva-r+length<=sz and off+rva-r+length<=len(self.b):
                return self.b[off+rva-r:off+rva-r+length]
        raise VerificationError('native VA out of file-backed region: %#x'%va)
    def section(self,va:int)->str:
        rva=va-self.base
        for name,r,vsz,sz,_ in self.sections:
            if r<=rva<r+max(vsz,sz):return name
        return ''

def verify_native(pe:PE32,*,check_hash=True)->dict:
    if pe.base!=0x22000000:raise VerificationError('server image base mismatch')
    native=[]
    for name,(va,slot) in FUNCTIONS.items():
        if pe.section(va)!='.text' or pe.section(slot)!='.data':
            raise VerificationError('comparator location/slot section mismatch')
        code=pe.at(va,LENGTH)
        if check_hash and hashlib.sha256(code).hexdigest()!=CODE_SHA:
            raise VerificationError('original complete comparator SHA256 mismatch: '+name)
        for off,hexcode in WITNESSES.items():
            witness=bytes.fromhex(hexcode)
            if code[off:off+len(witness)]!=witness:
                raise VerificationError('original opcode witness mismatch at %#x'%(va+off))
        if pe.at(va+LENGTH,len(BOUNDARY))!=BOUNDARY:
            raise VerificationError('original comparator code boundary mismatch')
        if pe.at(slot,4)!=struct.pack('<I',va):
            raise VerificationError('original repository comparator address mismatch: '+name)
        native.append(code)
    if native[0]!=native[1]:
        raise VerificationError('native scene/model-sounds comparators differ')
    return {
        'new_native_functions':2 if check_hash else 0,
        'new_original_x86_bytes':34 if check_hash else 0,
        'complete_function_sha256':CODE_SHA if check_hash else None,
        'byte_identical_method_bodies':True,
        'native_comparator_slots':{name:hex(slot) for name,(_,slot) in FUNCTIONS.items()},
        'comparator_code_addresses':{name:hex(va) for name,(va,_) in FUNCTIONS.items()},
        'sparse_witnesses':len(FUNCTIONS)*len(WITNESSES),
        'exact_comparator_pointer_fields':2,
        'return_semantics':'unsigned first DWORD lhs < unsigned first DWORD rhs => EAX=1 else EAX=0',
        'limitations':['Other key descriptor fields not compared, native key ownership remains opaque',
                       'No original Source engine module loading demonstrated'],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise VerificationError('original full DLL hashes mismatch')
    return verify_native(PE32(server))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    report=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+'\n'
    if a.output:a.output.write_text(report,encoding='utf8')
    else: print(report,end='')
if __name__=='__main__':main()
