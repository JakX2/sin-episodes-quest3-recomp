#!/usr/bin/env python3
"""Phase183: read-only original 2006 client KeyValues tree, string/wide/numeric setters.

Pin complete owner-original client/server DLL SHA256 plus six complete native
function hashes, disjoint opcode fragments and actual rel32 CALL edges.
No proprietary DLLs or complete original machine code may be committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'keyvalues_find_or_create':(0x241656E0,267,'132b5ecb73db4bee8a0d930a26c35006115d25aceaf6d42b3e6a4558c46cfbc3'),
 'keyvalues_set_narrow_string':(0x24165AE0,126,'e23c92e211203861ea33fa597159207d0dc27582008b2dcce8d598699425258a'),
 'keyvalues_set_wide_string':(0x24165B60,117,'7e252f1ef17364fca5531956e1bda4cbeedc9032d4ead89c985bf6ba6e9f550d'),
 'keyvalues_set_type2_integer':(0x24165BE0,33,'e2c8ded0de6cb99c3c1fd37ec712b02f4c882e206abf7a4fbcb20c06647d2ccd'),
 'keyvalues_set_type3_float':(0x24165C10,33,'b4ee4b29519066a4b4ab518ab541e7659a4b2ae1bfca7411025a243165a81829'),
 'keyvalues_set_type4_pointer_bits':(0x24165C40,33,'91ba8d9a8fffdb393e9a7f239751eb1d8ff53db6d57071b1c302421954a69764'),
}
# Native fragments are sparse, separate from the independently pinned full-body hashes.
OPCODES = {
 0x241656E0:"558bec",
 0x241656E6:"81ec0c",
 0x241656F0:"85db",
 0x24165700:"803b00",
 0x24165709:"6a2f",
 0x2416571E:"2bc3",
 0x24165724:"c1e902",
 0x2416573D:"ff15",
 0x24165748:"ff520c",
 0x24165751:"8b421c",
 0x24165760:"3908",
 0x24165766:"8b4018",
 0x2416576D:"8b4a20",
 0x24165774:"6a00",
 0x24165780:"38450c",
 0x2416578D:"6a28",
 0x24165791:"ff5204",
 0x241657AA:"894718",
 0x241657B3:"89411c",
 0x241657BA:"895818",
 0x241657BD:"895a10",
 0x241657DD:"c20800",
 0x241657E8:"c20800",
 0x24165AE0:"8b442404",
 0x24165AE5:"6a01",
 0x24165AF3:"8b4b04",
 0x24165B0F:"85f6",
 0x24165B11:"c74308",
 0x24165B1A:"be0e",
 0x24165B2D:"2bc2",
 0x24165B38:"894304",
 0x24165B41:"c1e902",
 0x24165B52:"c74310",
 0x24165B5B:"c20800",
 0x24165B60:"8b442404",
 0x24165B73:"8b4b08",
 0x24165B8F:"85f6",
 0x24165B91:"c74304",
 0x24165B9A:"bebc",
 0x24165BA5:"8d6c0002",
 0x24165BB0:"43088b",
 0x24165BC9:"c74310",
 0x24165BE0:"8b442404",
 0x24165BF0:"8b4c2408",
 0x24165BF4:"89480c",
 0x24165BF7:"c74010",
 0x24165C10:"8b442404",
 0x24165C20:"8b4c2408",
 0x24165C24:"89480c",
 0x24165C27:"c74010",
 0x24165C40:"8b442404",
 0x24165C50:"8b4c2408",
 0x24165C54:"89480c",
 0x24165C57:"c74010",
}
# Independent original direct rel32 CALL sites: inner recursion, interning,
# node construction, payload free/alloc and the Phase182 GetString caller.
CALLS = {
 0x2416570C:0x24249550, 0x24165777:0x241656E0,
 0x2416579D:0x24165010, 0x241657D2:0x241656E0,
 0x24165AE8:0x241656E0, 0x24165AFA:0x2416FD60,
 0x24165B03:0x2416FD60, 0x24165B33:0x2416FCF0,
 0x24165B68:0x241656E0, 0x24165B7A:0x2416FD60,
 0x24165B83:0x2416FD60, 0x24165BA0:0x24249FA9,
 0x24165BAA:0x2416FCF0, 0x24165BE7:0x241656E0,
 0x24165C17:0x241656E0, 0x24165C47:0x241656E0,
}
INCOMING={0x241667CF:0x241656E0, 0x2416681A:0x24165AE0,
          0x2416684C:0x24165AE0, 0x2416688A:0x24165AE0}
DATA_STRINGS={0x242D330E:b'\0',0x242D37BC:b'\0\0'}

class VerificationError(ValueError):
    pass
class PEImage:
    def __init__(self,data:bytes):
        self.data=data
        if len(data)<0x200 or data[:2]!=b'MZ':raise VerificationError('not PE image')
        pe=struct.unpack_from('<I',data,0x3c)[0]
        if data[pe:pe+4]!=b'PE\0\0':raise VerificationError('missing PE signature')
        machine,num=struct.unpack_from('<HH',data,pe+4)
        opt_size=struct.unpack_from('<H',data,pe+20)[0]
        opt=pe+24
        if machine!=0x14c or not (1<=num<=96) or struct.unpack_from('<H',data,opt)[0]!=0x10b:
            raise VerificationError('not original x86 PE32')
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        self.sections=[]
        for i in range(num):
            s=opt+opt_size+i*40
            if s+40>len(data):raise VerificationError('truncated PE section')
            name=data[s:s+8].split(b'\0')[0].decode('ascii','replace')
            vsize,rva,rsize,raw=struct.unpack_from('<IIII',data,s+8)
            self.sections.append((name,rva,max(vsize,rsize),rsize,raw))
    def section_of(self,va:int)->str:
        rva=va-self.base
        return next((name for name,start,extent,rsize,raw in self.sections if start<=rva<start+extent),'')
    def at(self,va:int,size:int)->bytes:
        rva=va-self.base
        for name,start,extent,rsize,raw in self.sections:
            if start<=rva and 0<=rva-start and rva-start+size<=rsize:
                off=raw+rva-start
                if off+size<=len(self.data):return self.data[off:off+size]
        raise VerificationError(f'original VA out of PE file-backed section: {va:#x}')

def verify_native(pe:PEImage,*,check_hash:bool=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('unexpected client image base')
    for name,(va,size,sha) in REGIONS.items():
        if pe.section_of(va)!='.text' or pe.section_of(va+size-1)!='.text':
            raise VerificationError('original routine outside executable .text: '+name)
        if check_hash and hashlib.sha256(pe.at(va,size)).hexdigest()!=sha:
            raise VerificationError('complete original x86 function SHA mismatch: '+name)
    for va,hexstring in OPCODES.items():
        b=bytes.fromhex(hexstring)
        if pe.at(va,len(b))!=b:
            raise VerificationError(f'original x86 instruction witness mismatch {va:#x}')
    for va,target in {**CALLS,**INCOMING}.items():
        if pe.at(va,5)!=b'\xe8'+struct.pack('<i',target-(va+5)):
            raise VerificationError(f'original rel32 CALL edge mismatch {va:#x}')
    for va,string in DATA_STRINGS.items():
        if pe.section_of(va)!='.rdata' or pe.at(va,len(string))!=string:
            raise VerificationError(f'original empty value string mismatch {va:#x}')
    return {
        'complete_native_function_hashes':len(REGIONS) if check_hash else 0,
        'verified_complete_function_bytes':sum(size for _,size,_ in REGIONS.values()) if check_hash else 0,
        'sparse_instruction_witnesses':len(OPCODES),
        'direct_CALL_witnesses':len(CALLS),
        'incoming_CALL_witnesses':len(INCOMING),
        'original_empty_string_constants':len(DATA_STRINGS),
        'original_functions':{name:{'va':hex(va),'bytes':size} for name,(va,size,sha) in REGIONS.items()},
        'limits':['Only original KeyValues lookup and typed setter branches recovered.',
                  'Native KeyValuesSystem symbol intern, allocator and object constructors remain callbacks.',
                  'No game DLL or engine-runtime executable integration.']
    }

def audit(client:bytes,server:bytes)->dict:
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA or hashlib.sha256(server).hexdigest()!=SERVER_SHA:
        raise VerificationError('complete original client/server DLL SHA256 mismatch')
    return {'phase':183,'status':'original_client_keyvalues_find_and_setters_verified',
            'owner_original_client_and_server_sha256_matched':True,
            'client_keyvalues':verify_native(PEImage(client))}

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--client',required=True,type=Path)
    ap.add_argument('--server',required=True,type=Path)
    ap.add_argument('--output',type=Path)
    args=ap.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:args.output.write_text(data)
    else:print(data,end='')
if __name__=='__main__': main()