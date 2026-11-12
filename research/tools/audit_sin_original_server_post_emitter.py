#!/usr/bin/env python3
"""Phase187: original SiN 2006 server post-emitter AutoGameSystem gates.

Owner-original PE32 DLLs are never committed. This audit verifies five complete
native original x86 function digests, sparse opcodes, exact call/branch edges,
lookup strings and original global locations. No original parser is invented.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'

class VerificationError(ValueError):
    pass

class PE32:
    def __init__(self, image:bytes):
        self.image = image
        if len(image) < 0x100 or image[:2] != b'MZ':
            raise VerificationError('invalid original PE DOS header')
        pe = struct.unpack_from('<I',image,0x3c)[0]
        if pe+24 > len(image) or image[pe:pe+4] != b'PE\0\0':
            raise VerificationError('invalid original PE signature')
        machine, nsections = struct.unpack_from('<HH',image,pe+4)
        opt_size = struct.unpack_from('<H',image,pe+20)[0]
        if machine != 0x14c or nsections < 1 or nsections > 96 or pe+24+opt_size+nsections*40>len(image):
            raise VerificationError('not original i386 PE with bounded sections')
        opt=pe+24
        if struct.unpack_from('<H',image,opt)[0] != 0x10b:
            raise VerificationError('not original PE32')
        self.base=struct.unpack_from('<I',image,opt+28)[0]
        self.sections=[]
        for i in range(nsections):
            off=opt+opt_size+i*40
            name=image[off:off+8].split(b'\0')[0].decode('ascii','replace')
            vlen,rva,rawlen,raw=struct.unpack_from('<IIII',image,off+8)
            self.sections.append((name,rva,vlen,rawlen,raw))
    def at(self,va:int,length:int)->bytes:
        if length<0:raise VerificationError('invalid PE read length')
        rva=va-self.base
        for name,start,virtual,rawlen,off in self.sections:
            if start <= rva and rva-start+length<=rawlen and off+rva-start+length<=len(self.image):
                return self.image[off+rva-start:off+rva-start+length]
        raise VerificationError('original PE read outside mapped raw section %#x'%va)
    def section(self,va:int)->str:
        rva=va-self.base
        for name,start,virtual,rawlen,off in self.sections:
            if start<=rva<start+max(virtual,rawlen):return name
        return ''

REGIONS={
 'game_systems_dispatch':(0x221F5700,277,'1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530'),
 'repository_a_singleton_thunk':(0x221AD8A0,10,'83762183370ab5370d0c099c4350b20768c57043ed99066edb8e1b1804da49e0'),
 'repository_b_singleton_thunk':(0x22149B60,10,'47793d5818d074eb9ad8526102a131bb60fd50ab5fcaaa52aa5afe6e940852d9'),
 'developer_cvar_registration':(0x221E6450,35,'93b09228e176da1f87b61fb302a3e761389c8f67a788ec1f3e382ddb22df8470'),
 'registry_forwarder':(0x221F56E0,23,'76cce08b11d37afa67eeda003f01b78d06106290ed4205c165cb6704622534d5'),
}
# Disjoint native instruction witnesses, not a complete disassembly.
OPCODES={
 0x221F5700:'a1c02c5722',0x221F5707:'8b0d640a5122',
 0x221F5711:'0f84ba000000',0x221F5718:'8b1d28213d22',
 0x221F5720:'33c0',0x221F5726:'8b15580a5122',
 0x221F5730:'393c82',0x221F573E:'83f8ff',
 0x221F5743:'a15c0a5122',0x221F5757:'b9580a5122',
 0x221F576D:'83c101',0x221F5779:'890d640a5122',
 0x221F577F:'8915680a5122',0x221F578B:'8d0cb2',
 0x221F57A8:'8d04b2',0x221F57AF:'8938',
 0x221F57B3:'68383e4422',0x221F57C5:'8b7f04',
 0x221F57CA:'0f8550ffffff',0x221F57D5:'c705c02c572200000000',
 0x221F57DF:'c605b82c572201',0x221F57F0:'a1580a5122',
 0x221F57FA:'ff12',0x221F57FC:'84c0',0x221F57FE:'7410',
 0x221F580B:'5fb0015ec3',0x221F5810:'5f32c05ec3',
 0x221AD8A0:'b9e03e5022',0x22149B60:'b988034f22',
 0x221E6450:'6864015122',0x221E645A:'8b0da0215722',
 0x221E6465:'68341f4422',0x221E646A:'ff500c',
 0x221E646D:'a3581c5722',
 0x221F56E0:'8b0d640a5122',0x221F56E6:'8d442404',
 0x221F56EC:'b9580a5122',
}
CALLS={
 0x221F575C:0x221F51A0, # game-system registry capacity growth
 0x221F5794:0x223A5D10, # copy older pointer slots on insertion
 0x221E6455:0x2225B350, # cvar registration
 0x221F56F1:0x221F54F0, # vector forward
 0x221E7842:0x221E6450, # original server Init -> cvar bridge
 0x221E7927:0x221F5700, # original server Init -> game-system gate
 0x221E7930:0x221AD8A0, # Init -> repository singleton A
 0x221E7939:0x22149B60, # Init -> repository singleton B
 0x221E78D8:0x221F56E0, # Init -> registry forwarder
}
JMPS={
 0x221AD8A5:0x221AD3E0,
 0x22149B65:0x221498C0,
}
FAILURE_BRANCHES={
 0x221E792E:0x221E79A1,
 0x221E7937:0x221E79A1,
 0x221E7940:0x221E79A1,
}
STRINGS={
 0x22441F34:b'developer\0',
 0x224428F0:b'resource/gameevents.res\0',
 0x224428E4:b'sv_cheats\0',
 0x224428CC:b'resource/ModEvents.res\0',
 0x22443E38:b'AutoGameSystem already added to game system list!!!\n\0',
}
GLOBALS={
 'system_list_head':0x22572CC0,
 'system_ready':0x22572CB8,
 'registered_pointer_array':0x22510A58,
 'registered_count':0x22510A64,
 'registered_capacity':0x22510A5C,
 'developer_cvar_slot':0x22571C58,
}

def verify_native(pe,*,check_hash:bool=True)->dict:
    if pe.base!=0x22000000:raise VerificationError('original server image base mismatch')
    for name,va in GLOBALS.items():
        if pe.section(va)!='.data':raise VerificationError('original global not in data section '+name)
    for address,opcode in OPCODES.items():
        data=bytes.fromhex(opcode)
        if pe.at(address,len(data))!=data:
            raise VerificationError('Phase187 server opcode mismatch %#x'%address)
    for address,target in CALLS.items():
        expected=b'\xe8'+struct.pack('<i',target-(address+5))
        if pe.at(address,5)!=expected:
            raise VerificationError('Phase187 native CALL edge mismatch %#x'%address)
    for address,target in JMPS.items():
        expected=b'\xe9'+struct.pack('<i',target-(address+5))
        if pe.at(address,5)!=expected:
            raise VerificationError('Phase187 singleton JMP edge mismatch %#x'%address)
    for address,target in FAILURE_BRANCHES.items():
        relative=target-(address+2)
        if relative < -128 or relative > 127:
            raise VerificationError('Phase187 failure target too distant for original short JZ')
        if pe.at(address,2)!=b'\x74'+struct.pack('<b',relative):
            raise VerificationError('Phase187 shared Init failure edge mismatch %#x'%address)
    for address,string in STRINGS.items():
        if pe.section(address)!='.rdata' or pe.at(address,len(string))!=string:
            raise VerificationError('Phase187 native server resource string mismatch %#x'%address)
    if check_hash:
        for name,(address,length,sha) in REGIONS.items():
            if hashlib.sha256(pe.at(address,length)).hexdigest()!=sha:
                raise VerificationError('Phase187 entire original server method hash mismatch '+name)
    return {
      'native_complete_function_hashes':len(REGIONS) if check_hash else 0,
      'native_code_bytes_hashed':sum(n for _,n,_ in REGIONS.values()) if check_hash else 0,
      'native_opcode_witnesses':len(OPCODES),
      'native_rel32_CALL_edges':len(CALLS),
      'native_rel32_singleton_JMP_edges':len(JMPS),
      'native_three_shared_Init_false_branches':len(FAILURE_BRANCHES),
      'server_interface_strings':len(STRINGS),
      'opaque_repository_parser_entrypoints':['0x221AD3E0','0x221498C0'],
      'limits':['Two original repository parsers remain unported',
                'No real original-engine gameplay DLL startup demonstrated',
                'Portable linked-list cycle guard and std::vector storage are non-native protections']
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA or hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA:
        raise VerificationError('Phase187 original full server/client DLL SHA256 mismatch')
    pe=PE32(server_bytes)
    return {'original_server_post_emitter':verify_native(pe)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=audit(args.server.read_bytes(),args.client.read_bytes())
    out=json.dumps(result,indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(out,encoding='utf8')
    else: print(out,end='')
if __name__=='__main__':main()