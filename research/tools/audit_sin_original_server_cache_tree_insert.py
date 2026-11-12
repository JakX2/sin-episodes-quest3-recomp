#!/usr/bin/env python3
"""Phase193: original 2006 SiN server.dll paired 112-byte tree insertion wrappers.

Original commercial DLLs and full function machine-code bodies remain private.
This audit pins complete original function SHA256, normalized twin SHA256,
sparse native witnesses and exact relative-call edges.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_post_emitter import PE32, VerificationError

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGIONS={
 'scene':(0x221ACD00,112,'b878d9830b6e5cee73e17681d2c89c518f45c8f8f1f5811aab516c42f14e0640','8b442408'),
 'modelsounds':(0x22146EF0,112,'2bc1a6156b3b845d14935402d317b8ec6f285f1d9858a67fd4054a780db821af','5333dbf6'),
}
WITNESSES={
 0x00:'8b442408',0x04:'53',0x05:'8b5c2408',0x0B:'8bf1',
 0x0E:'8d4c2418',0x13:'8d542418',0x19:'8bce',0x20:'8bce',
 0x27:'8b4c2410',0x2B:'8bf8',0x2D:'8b442414',0x31:'5051578bce',
 0x3B:'6683461201',0x40:'0fb7c7',0x43:'8d1440',0x46:'8b4604',
 0x49:'8d44d008',0x4D:'85c0',0x4F:'7416',0x51:'8b0b8908',
 0x55:'8b5304',0x5B:'8b4b08',0x61:'8b530c',0x67:'668bc7',
 0x6A:'5f5e5b',0x6D:'c20800',
}
CALL_TARGETS={
 0x221ACD00:{0x1B:0x221A6510,0x22:0x221A6890,0x36:0x221AA890},
 0x22146EF0:{0x1B:0x22140700,0x22:0x22140900,0x36:0x221441B0},
}
INCOMING_CALLS={0x221AD06F:0x221ACD00,0x2214980C:0x22146EF0}
NORMALIZED_SHA='dd1f7514e89298739816f025baf317f3b87424ee8a637cda3ed3289acecf2725'

def rel32(source:int,target:int)->bytes:
    return b'\xe8'+struct.pack('<i',target-source-5)

def verify_native(pe:PE32,*,check_hash:bool=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError('Phase193 expected 2006 server original image base')
    normalized=[]
    for name,(va,size,digest,following) in REGIONS.items():
        if pe.section(va)!='.text':
            raise VerificationError('Phase193 function outside .text')
        body=bytearray(pe.at(va,size))
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError('Phase193 full original native function SHA mismatch: '+name)
        for off,hexcode in WITNESSES.items():
            expected=bytes.fromhex(hexcode)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError('Phase193 sparse x86 witness mismatch at %#x'%(va+off))
        for off,target in CALL_TARGETS[va].items():
            if pe.at(va+off,5)!=rel32(va+off,target):
                raise VerificationError('Phase193 outgoing CALL mismatch at %#x'%(va+off))
            body[off+1:off+5]=bytes(4)
        if pe.at(va+size,4)!=bytes.fromhex(following):
            raise VerificationError('Phase193 expected next aligned function prefix missing')
        normalized.append(bytes(body))
    for site,target in INCOMING_CALLS.items():
        if pe.at(site,5)!=rel32(site,target):
            raise VerificationError('Phase193 incoming callback CALL mismatch at %#x'%site)
    if normalized[0]!=normalized[1]:
        raise VerificationError('Phase193 twin insertion bodies differ outside 3 CALL displacements')
    if check_hash and hashlib.sha256(normalized[0]).hexdigest()!=NORMALIZED_SHA:
        raise VerificationError('Phase193 normalized whole original insertion body hash mismatch')
    return {
       'new_complete_original_function_count':2 if check_hash else 0,
       'new_complete_original_x86_bytes':224 if check_hash else 0,
       'sparse_native_witnesses':2*len(WITNESSES),
       'outgoing_exact_direct_calls':sum(len(x) for x in CALL_TARGETS.values()),
       'incoming_exact_direct_calls':len(INCOMING_CALLS),
       'normalized_identical':True,
       'normalized_original_SHA256':NORMALIZED_SHA if check_hash else None,
       'native_index_stride':24,'native_key_copy_offset':8,'native_key_copy_length':16,
       'native_live_count_increment_offset':0x12,'native_return_cleanup_bytes':8,
       'notes':['index allocation, search-position and insertion/link helpers are native opaque calls',
                'full allocator index goes to link, low 16 bits index storage and return',
                'key DWORD at node +0x14 is later overwritten by Phase190 payload assignment',
                'host bounds check not present in native x86; no playable replacement DLL'],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise VerificationError('Phase193 original dual DLL full SHA mismatch')
    return verify_native(PE32(server))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+'\n'
    if args.output:args.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()
