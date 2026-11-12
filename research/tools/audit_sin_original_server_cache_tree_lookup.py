#!/usr/bin/env python3
"""Phase191: original paired server cache-index tree lookups. No DLL bytes committed."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGIONS={
 'scene':(0x221A7FB0,260,'8e6977a45630ac69cadd8b7597f752bdd24946981832ffc1e5eaa31274661038',0x2256E1F0),
 'modelsounds':(0x22142670,260,'8cc7e151e2ee8b7fcc7c8202b6a880b8542969c7a5bbeb424cbcc89d9b79a912',0x22558D50),
}
# Only these 14 native DWORD absolute address operands differ between bodies.
SENTINEL_OFFSETS={
 0x40:8,0x53:8,0x5a:0,0x61:2,0x68:4,0x6e:6,0x85:0,
 0xa2:8,0xb5:8,0xbb:0,0xc1:2,0xc7:4,0xce:6,0xdf:0,
}
NORMALIZED_SHA='9dc7a270eed6024f374820355e3097d25f96d64547b3c38d29d25c502732ec41'
# Sparse complete x86 instruction witnesses; not a full native disassembly.
WITNESSES={
 0x00:'55578bf9',0x04:'668b6f10',0x08:'6681fdffff',0x0d:'0f84e9000000',
 0x13:'538b5c241056',0x20:'0fb7c5',0x23:'8d3440',0x26:'8b4704',
 0x2f:'8d443008',0x33:'5053ff17',0x37:'83c40884c0',0x3c:'7450',
 0x47:'b9ffff0000',0x4c:'b801000000',0x72:'6681fdffff',
 0x77:'740b',0x79:'8b4704',0x7c:'668b2c30',0x80:'03c6eb63',
 0x89:'668b28eb59',0x8e:'8b4f04',0x91:'8d443108',0x95:'5350ff17',
 0x99:'83c40884c0',0x9e:'7452',0xa7:'b8ffff0000',
 0xae:'b901000000',0xd2:'663be8',0xd5:'7407',
 0xd7:'8b4704',0xda:'03c6eb05',0xe3:'668b6802',
 0xe7:'6681fdffff',0xec:'0f852effffff',
 0xf2:'5e5b5f668bc55dc20800',0xfc:'5f668bc55dc20800',
}
INDIRECT_COMPARE_OFFSETS=(0x35,0x97)
RET8_OFFSETS=(0xf9,0x101)
class VerificationError(ValueError):pass

class PE32:
    def __init__(self,b:bytes):
        if len(b)<0x100 or b[:2]!=b'MZ':raise VerificationError('missing PE32 DOS header')
        pe=struct.unpack_from('<I',b,0x3c)[0]
        if pe+24>len(b) or b[pe:pe+4]!=b'PE\0\0':raise VerificationError('bad PE signature')
        machine,n=struct.unpack_from('<HH',b,pe+4)
        opt_size=struct.unpack_from('<H',b,pe+20)[0];opt=pe+24
        if machine!=0x14c or not 1<=n<=96 or pe+24+opt_size+40*n>len(b) or struct.unpack_from('<H',b,opt)[0]!=0x10b:
            raise VerificationError('not bounded Windows i386 PE32')
        self.base=struct.unpack_from('<I',b,opt+28)[0];self.b=b;self.sections=[]
        for i in range(n):
            p=opt+opt_size+40*i
            name=b[p:p+8].split(b'\0')[0].decode('ascii','replace')
            vsz,rva,rawlen,raw=struct.unpack_from('<IIII',b,p+8)
            self.sections.append((name,rva,vsz,rawlen,raw))
    def at(self,va:int,length:int)->bytes:
        rva=va-self.base
        for _,s,vsz,n,off in self.sections:
            if s<=rva and 0<=length and rva-s+length<=n and off+rva-s+length<=len(self.b):
                return self.b[off+rva-s:off+rva-s+length]
        raise VerificationError('native VA not file-backed %#x'%va)
    def section(self,va:int)->str:
        rva=va-self.base
        for name,start,vsz,rawlen,_ in self.sections:
            if start<=rva<start+max(vsz,rawlen):return name
        return ''

def verify_native(pe:PE32,*,check_hash:bool=True)->dict:
    if pe.base!=0x22000000:raise VerificationError('original server image base mismatch')
    normalized=[]
    for name,(va,size,digest,sentinel) in REGIONS.items():
        if pe.section(va)!='.text':raise VerificationError('not native code')
        body=bytearray(pe.at(va,size))
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError('whole original lookup function digest mismatch '+name)
        for off,hexcode in WITNESSES.items():
            expected=bytes.fromhex(hexcode)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError('native lookup instruction mismatch %#x'%(va+off))
        for off in INDIRECT_COMPARE_OFFSETS:
            if body[off:off+2]!=b'\xff\x17':raise VerificationError('indirect compare mismatch')
        for off in RET8_OFFSETS:
            if body[off:off+3]!=b'\xc2\x08\x00':raise VerificationError('RET 8 mismatch')
        for off,delta in SENTINEL_OFFSETS.items():
            address=struct.unpack_from('<I',body,off)[0]
            if address!=sentinel+delta or pe.section(address)!='.data':
                raise VerificationError('native sentinel address mismatch %#x'%(va+off))
            body[off:off+4]=bytes(4)
        normalized.append(bytes(body))
        if pe.at(va+size,2)!=b'\xcc\xcc':
            raise VerificationError('native function boundary mismatch')
    if normalized[0]!=normalized[1]:
        raise VerificationError('paired lookups differ outside 14 absolute addresses')
    if check_hash and hashlib.sha256(normalized[0]).hexdigest()!=NORMALIZED_SHA:
        raise VerificationError('normalized original code hash mismatch')
    return {
      'new_sha_verified_function_count':2 if check_hash else 0,
      'new_sha_verified_native_bytes':520 if check_hash else 0,
      'independent_sparse_instruction_witnesses':len(WITNESSES)*2,
      'indirect_comparator_calls':4,'ret8_exits':4,
      'absolute_sentinel_references':len(SENTINEL_OFFSETS)*2,
      'identical_after_only_14_dword_relocations':True,
      'full_normalized_sha256':NORMALIZED_SHA if check_hash else None,
      'native_node_stride':24,'root_index_offset':0x10,
      'node_children_offsets':[0,2],'node_key_offset':8,
      'missing_index':0xffff,
      'limitations':['Original comparator/key format and tree allocation remain opaque',
                     'Host-only cycle protection is not native x86 behavior',
                     'No Windows engine-hosted playable replacement DLL demonstrated'],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise VerificationError('owner-original complete DLL hashes mismatch')
    return verify_native(PE32(server))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    s=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+'\n'
    if a.output:a.output.write_text(s,encoding='utf8')
    else:print(s,end='')
if __name__=='__main__':main()
