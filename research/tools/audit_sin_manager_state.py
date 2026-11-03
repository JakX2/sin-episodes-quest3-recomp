#!/usr/bin/env python3
"""Verify original 2006 SiN server difficulty-manager archive call paths.

Only exact SHA-pinned PE32 original binaries are accepted. Produces metadata,
never original executable bytes. Archive serializer does not necessarily mean
game save/load timing or an understood Source file format.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

class VerificationError(ValueError):
    pass

class PEImage:
    def __init__(self, data: bytes):
        self.data=data
        if data[:2]!=b'MZ':
            raise VerificationError('not an MZ executable')
        pe=struct.unpack_from('<I',data,0x3c)[0]
        if data[pe:pe+4]!=b'PE\0\0':
            raise VerificationError('missing PE signature')
        if struct.unpack_from('<H',data,pe+24)[0]!=0x10b:
            raise VerificationError('expected Windows x86 PE32')
        base=struct.unpack_from('<I',data,pe+24+0x1c)[0]
        count=struct.unpack_from('<H',data,pe+6)[0]
        sections=pe+24+struct.unpack_from('<H',data,pe+20)[0]
        self.mappings=[]
        for i in range(count):
            h=sections+i*40
            _,rva,size,offset=struct.unpack_from('<IIII',data,h+8)
            self.mappings.append((base+rva,base+rva+size,offset))
    def at(self,va:int,size:int)->bytes:
        for first,last,offset in self.mappings:
            if first<=va and va+size<=last:
                start=offset+va-first
                return self.data[start:start+size]
        raise VerificationError('VA outside mapped section: %#x'%va)

def check(pe,site:int,raw:bytes):
    if pe.at(site,len(raw))!=raw:
        raise VerificationError('original archive opcode mismatch at %#x'%site)

def direct_call(pe,site:int,target:int):
    check(pe,site,b'\xe8'+struct.pack('<i',target-site-5))

SERVER_BYTES={
    0x222b9082:'c74424100e000000', # 14 scalar write header
    0x222b90a8:'ff5240',
    0x222b90ae:'83fe0e',
    0x222b90cc:'ff5034',          # count virtual
    0x222b90cf:'0fbff8',          # sign-extend from ax
    0x222b90ec:'ff5244',          # array virtual read
    0x222b90f7:'83fe0e',         # max 14
    0x222b910d:'83c601',
    0x222b91c2:'c744241014000000', # 20 scalar header
    0x222b91d0:'6a01',           # selector 1
    0x222b91ed:'6a03',           # selector 3
    0x222b91ea:'ff5240',
    0x222b9207:'ff5240',
    0x222b920d:'83ff0a',         # max 10 pairs
    0x222b922c:'ff5034',
    0x222b922f:'0fbff8',
    0x222b924c:'ff5244',
    0x222b9254:'2bc2',           # signed count / 2
    0x222b9258:'d1ff',
    0x222b9260:'83fe0a',
    0x222b9269:'6a01',
    0x222b9276:'6a03',
    0x222b92b4:'bb06000000',    # six advisor objects
    0x222b92bf:'ff5028',        # archive write group count
    0x222b92c2:'83c648',        # advisor pointers at manager+0x48
    0x222b92cb:'ff521c',
    0x222b92d4:'ff5510',
    0x222b92da:'83eb01',
    0x222b92fd:'ff5034',
    0x222b9311:'83ff06',        # max 6 on read
    0x222b931c:'ff521c',
    0x222b9325:'ff5508',
    0x222b932c:'ff5220',
    0x222bb68a:'e8f1f5ffff',
    0x222bbf54:'8b4624',
}
CALLS={
    0x222b9091:0x222d33e0, # metric getter
    0x222b9104:0x222d3340, # mutable metric cell
    0x222b91d3:0x222c07a0, # modifier select 1
    0x222b91f0:0x222c07a0, # modifier select 3
    0x222b926c:0x222c0810, # restore select 1
    0x222b9279:0x222c0810, # restore select 3
    0x222bb690:0x222b9070, # state writer -> 14 metrics
    0x222bb69b:0x222b92a0, # state writer -> 6 advisor objects
    0x222bb6a3:0x222b91b0, # state writer -> 10 pairs
    0x222bbf6d:0x222b90c0, # state reader -> 14 metrics
    0x222bbf78:0x222b92f0, # state reader -> 6 advisor objects
    0x222bbf80:0x222b9220, # state reader -> 10 pairs
}
def verify_original(server_pe):
    for site,encoded in SERVER_BYTES.items():
        check(server_pe,site,bytes.fromhex(encoded))
    for site,target in CALLS.items():
        direct_call(server_pe,site,target)
    return {
        'original_x86_addresses':{
            'manager_state_writer':'0x222BB680',
            'manager_state_reader':'0x222BBF50',
            'metric_writer':'0x222B9070','metric_reader':'0x222B90C0',
            'modifier_writer':'0x222B91B0','modifier_reader':'0x222B9220',
            'advisor_writer':'0x222B92A0','advisor_reader':'0x222B92F0',
        },
        'metrics':{'written_f32':14,'max_restored':14},
        'success_modifiers':{
            'written_f32':20,'written_pairs':10,'max_restored_pairs':10,
            'selectors':[1,3],'restore_pair_count':'signed scalar count / 2 truncating toward zero'
        },
        'opaque_advisors':{'written_objects':6,'max_restored':6},
        'verified_instruction_sites':len(SERVER_BYTES),
        'verified_direct_call_edges':len(CALLS),
        'caveat':'Verified archive vtable calls do not establish full Source save/load timing, field names or scoring formulas.',
    }

def audit(client:bytes,server:bytes):
    ch=hashlib.sha256(client).hexdigest()
    sh=hashlib.sha256(server).hexdigest()
    if ch!=CLIENT_SHA or sh!=SERVER_SHA:
        raise VerificationError('original SiN 2006 DLL SHA-256 mismatch')
    result=verify_original(PEImage(server))
    result['original_dll_sha256']={'client.dll':ch,'server.dll':sh}
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
if __name__=='__main__':
    main()
