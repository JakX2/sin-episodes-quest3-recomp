#!/usr/bin/env python3
"""Phase184 read-only original 2006 client KeyValues object lifetime verification.

Owner-local PE32 DLLs only; commit neither the binaries nor whole original
function bodies. Function SHA256 checks cover three complete real x86 regions.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS = {
    'kv_name_constructor': (0x24165010, 61, 'f87d6d3f9161a19024319d7e4e2bfd5cef780bd85850b523ab4807d95844a8df'),
    'kv_deleting_destructor': (0x24165050, 35, '03b60f2d8adfedcfced3fc591eb1e8c1f07df082d727e21dbafbf4958e99865d'),
    'kv_recursive_contents_destructor': (0x24166620, 135, '8aa139f82c4f1499a950f2782e457327f5d9d95749a340027bbef554b1788c6e'),
}
# Independent, non-contiguous native opcode/layout witnesses.
OPCODES = {
    0x24165010:'33c0',
    0x24165015:'c706ffffffff',
    0x2416501b:'894610',
    0x2416501e:'89461c',
    0x24165021:'894618',
    0x24165024:'894620',
    0x24165027:'894604',
    0x2416502a:'894608',
    0x2416502d:'89460c',
    0x24165030:'884624',
    0x24165033:'ff1530622724',
    0x24165039:'8b4c2408',
    0x24165042:'ff520c',
    0x24165045:'8906',
    0x2416504a:'c20400',
    0x24165050:'568bf1',
    0x24165058:'f644240801',
    0x2416505d:'740e',
    0x2416505f:'ff1530622724',
    0x2416506a:'ff5208',
    0x24165070:'c20400',
    0x24166620:'53',
    0x24166621:'8b1d30622724',
    0x2416662b:'8b751c',
    0x24166633:'8b7e18',
    0x24166638:'c7461800000000',
    0x24166644:'ffd3',
    0x2416664b:'ff5208',
    0x24166654:'8b7518',
    0x24166660:'3bf5',
    0x24166664:'8b7e18',
    0x24166669:'c7461800000000',
    0x24166675:'ffd3',
    0x2416667c:'ff5208',
    0x24166685:'8b4504',
    0x2416668e:'8b4d08',
    0x24166694:'897504',
    0x2416669f:'5f',
    0x241666a0:'897508',
    0x241666a6:'c3',
}
DIRECT_CALLS = {
    0x24165053:0x24166620,
    0x2416663f:0x24166620,
    0x24166670:0x24166620,
    0x24166689:0x2416fd60,
    0x24166697:0x2416fd60,
}
INCOMING_CALLS = {
    0x241927bb:0x24165010, # Phase181 manifest constructs its record
    0x24166057:0x24166620, # Phase181 release bridge
}

class VerificationError(ValueError):
    pass

class PEImage:
    """Read-only PE32 VA -> raw DLL section byte mapper."""
    def __init__(self, raw:bytes):
        self.raw=raw
        if len(raw)<0x100 or raw[:2]!=b'MZ':
            raise VerificationError('expected original MZ PE32 image')
        pe=struct.unpack_from('<I',raw,0x3c)[0]
        if pe+24>len(raw) or raw[pe:pe+4]!=b'PE\x00\x00':
            raise VerificationError('invalid original PE signature')
        machine,nsects=struct.unpack_from('<HH',raw,pe+4)
        optsize=struct.unpack_from('<H',raw,pe+20)[0]
        opt=pe+24
        if machine!=0x14c or not(1<=nsects<=96) or opt+optsize+nsects*40>len(raw):
            raise VerificationError('not an original x86 PE32 executable')
        if struct.unpack_from('<H',raw,opt)[0]!=0x10b:
            raise VerificationError('expected PE32 optional header')
        self.base=struct.unpack_from('<I',raw,opt+28)[0]
        self.sections=[]
        for i in range(nsects):
            x=opt+optsize+i*40
            name=raw[x:x+8].split(b'\x00')[0].decode('ascii','replace')
            vsize,va,size,offset=struct.unpack_from('<IIII',raw,x+8)
            self.sections.append((name,va,vsize,size,offset))
    def at(self, address:int,size:int)->bytes:
        if size<0:raise VerificationError('negative PE read')
        rva=address-self.base
        for _,va,_,rawsize,off in self.sections:
            if va<=rva and rva-va+size<=rawsize:
                start=off+rva-va
                if start+size<=len(self.raw):return self.raw[start:start+size]
        raise VerificationError('native VA outside raw PE sections %#x'%address)
    def section_of(self,address:int)->str:
        rva=address-self.base
        for name,va,vsize,sz,_ in self.sections:
            if va<=rva<va+max(sz,vsize):return name
        return ''

def verify_native(client:PEImage, *,check_hash:bool=True)->dict:
    if client.base!=0x24000000:
        raise VerificationError('unexpected original client base')
    for label,(address,size,digest) in REGIONS.items():
        if client.section_of(address)!='.text':
            raise VerificationError(label+' original region not executable')
        if check_hash and hashlib.sha256(client.at(address,size)).hexdigest()!=digest:
            raise VerificationError(label+' complete native function SHA256 mismatch')
    for address,witness in OPCODES.items():
        value=bytes.fromhex(witness)
        if client.at(address,len(value))!=value:
            raise VerificationError('original lifetime opcode mismatch at %#x'%address)
    for address,target in {**DIRECT_CALLS,**INCOMING_CALLS}.items():
        value=b'\xe8'+struct.pack('<i',target-address-5)
        if client.at(address,5)!=value:
            raise VerificationError('original lifetime CALL target mismatch at %#x'%address)
    return {
        'three_complete_native_sha256_verified':check_hash,
        'unique_original_functions':len(REGIONS),
        'total_original_function_bytes':sum(n for _,n,_ in REGIONS.values()),
        'opcode_witnesses':len(OPCODES),
        'original_direct_calls':len(DIRECT_CALLS),
        'incoming_original_calls':len(INCOMING_CALLS),
        'native_layout':{'symbol':0,'narrow':4,'wide':8,'number':12,'type':16,'peer':24,'child':28,'fallback':32,'escape_byte':36},
        'limitations':['KeyValuesSystem vtable ABI and free implementation remain opaque',
                       'C++ host model adds cycle/depth checks and severs root links',
                       'Original Windows game executable has not loaded reconstructed gameplay modules']
    }

def audit(client_raw:bytes,server_raw:bytes)->dict:
    if hashlib.sha256(client_raw).hexdigest()!=CLIENT_SHA or hashlib.sha256(server_raw).hexdigest()!=SERVER_SHA:
        raise VerificationError('owner original client/server full DLL SHA256 mismatch')
    return {'phase':184,'client_keyvalues_lifetime':verify_native(PEImage(client_raw))}

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else: print(report,end='')
if __name__=='__main__':main()