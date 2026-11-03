#!/usr/bin/env python3
"""SHA-pinned original SiN 2006 cache-header and client cleanup read-only audit.

Requires exact owner-held client.dll/server.dll; no original executable
library bytes are exported or published. Chains Phase88 source verification.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_shutdown_caches import audit as phase88_audit
from audit_sin_original_factories import PE32,VerificationError

REGIONS={
 'client.dll':{
  'client_single_global_release':(0x241e5050,27,'b46aa3159ec436d2a49e839506758499c47fd13919dc8de46790e75fe01c58b6'),
  'client_owned_pointer_array_release':(0x24143e80,97,'121ec6b42c0d35770e68281a5eb0d24b891266a747b69a4ccf497d005b94eeff'),
 },
 'server.dll':{
  'scene_cache_reader':(0x221ad3e0,576,'7b9db352473a8fc2d5343473b673ac52c89a8c98320847b0b4dd451dcf8e4e69'),
  'model_sounds_cache_reader':(0x221498c0,576,'94b55dbf7e21d7b45362d277505a8a7c02537667c3c4c4afbeeded9c40ec2218'),
 }
}
SERVER_SITES={
 0x221ad45e:'837c241802',0x2214993e:'837c241802',
 0x221ad488:'3b4638',0x22149968:'3b4638',
 0x221ad4a7:'8b4644',0x22149987:'8b4644',
 0x221ad4aa:'3944241c',0x2214998a:'3944241c',
 0x221ad4b0:'3bc5',0x22149990:'3bc5',
 0x221ad4ed:'396c2418',0x221499cd:'396c2418',
 0x221ad5f7:'c6463c01',0x22149ad7:'c6463c01',
 0x221ad5fd:'b001',0x22149add:'b001',
 0x221ad616:'32c0',0x22149af6:'32c0',
 0x221ad569:'8d0440',0x22149a49:'8d0440',
 0x221ad56c:'8d6cc108',0x22149a4c:'8d6cc108',
 0x221ad5d9:'ff500c',0x22149ab9:'ff500c',
 0x221ad5f4:'ff5244',0x22149ad4:'ff5244',
}
CLIENT_SITES={
 0x241e5050:'8b0d58863724',0x241e5056:'85c9',0x241e5058:'7406',
 0x241e505a:'8b01',0x241e505c:'6a01',0x241e505e:'ff10',
 0x241e5060:'c7055886372400000000',0x241e506a:'c3',
 0x24143e8f:'8b7e24',0x24143e92:'83ef01',
 0x24143ea0:'8b4618',0x24143ea3:'8b0cb8',
 0x24143eaa:'8b11',0x24143eac:'6a01',0x24143eae:'ff12',
 0x24143eb0:'83ef01',0x24143eb3:'79eb',
 0x24143eb5:'837e20ff',0x24143eb9:'895e24',
 0x24143ec5:'50',0x24143ece:'895e18',
 0x24143ed1:'895e1c',0x24143ed4:'8b4618',
 0x24143ed7:'894628',0x24143edb:'885e2c',
}
CALLS={
 'client.dll':{0x240ff41d:0x241e5050,0x240ff484:0x24143e80,
               0x24143e8a:0x241f3530,0x24143ec6:0x2416f940},
 'server.dll':{0x221ad55e:0x221acfd0,0x22149a3e:0x22149770},
}
CACHE=(
 ('scene',0x22503ee0,0x2242ed68,b'scene.cache\x00',3),
 ('model_sounds',0x224f0388,0x2240bc9c,b'modelsounds.cache\x00',2),
)

def verify_one(pe,module,hash_check=True):
    regions={}
    for name,(va,size,digest) in REGIONS[module].items():
        if hash_check and hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
            raise VerificationError('Phase89 original SHA-256 region mismatch: '+name)
        regions[name]={'va':hex(va),'size':size,
                       'sha256':digest if hash_check else None}
    sites=CLIENT_SITES if module=='client.dll' else SERVER_SITES
    for va,raw in sites.items():
        if pe.at(va,len(bytes.fromhex(raw)))!=bytes.fromhex(raw):
            raise VerificationError('Phase89 original opcode mismatch at %#x'%va)
    for va,target in CALLS[module].items():
        if pe.at(va,5)!=b'\xe8'+struct.pack('<i',target-va-5):
            raise VerificationError('Phase89 CALL mismatch at %#x'%va)
    report={'module':module,'regions':regions,'opcode_anchors':len(sites),
            'direct_calls':len(CALLS[module])}
    if module=='server.dll':
        caches={}
        for name,instance,pointer,literal,version in CACHE:
            if pe.at(instance+0x34,8)!=struct.pack('<II',pointer,version):
                raise VerificationError('Phase89 singleton fields mismatch: '+name)
            if pe.at(pointer,len(literal))!=literal:
                raise VerificationError('Phase89 filename mismatch: '+name)
            caches[name]={'filename':literal[:-1].decode('ascii'),
                'singleton_va':hex(instance),'format_version':version}
        report.update({
            'cache_singletons':caches,
            'header_length_bytes':16,
            'four_little_endian_fields':[
                {'offset':0,'meaning':'fixed tag','value':2},
                {'offset':4,'meaning':'format version, 3 for scene, 2 for modelsounds'},
                {'offset':8,'meaning':'callback-derived source key, comparison skipped if expected 0'},
                {'offset':12,'meaning':'signed item count'},
            ],
            'mismatch_behavior':'Invalid header closes handle, sets singleton +0x3C and often returns success to game Init',
            'record_table_stride_bytes':24,
            'payload_schema':'Full per-record disk format not recovered; no arbitrary file parsing claimed',
        })
    else:
        report.update({
            'global_release_function_va':'0x241E5050',
            'global_pointer_va':'0x24378658',
            'release_virtual_offset':'0x00',
            'release_argument':1,
            'pointer_array_release_va':'0x24143E80',
            'pointer_array_instance_va':'0x243623D0',
            'pointer_array_fields':{'buffer':'+0x18','count':'+0x24',
                 'ownership_sentinel':'+0x20','auxiliary_state':'+0x2C'},
            'reverse_release':True,'heap_free_skipped_when_sentinel_is_minus_one':True,
        })
    report['limitations']='Verified original x86 flow only, no complete game DLLs, disk format or SourceVR ARM64 ABI.'
    return report

def audit(client,server):
    phase88_audit(client,server)
    return {'client':verify_one(PE32(client),'client.dll'),
            'server':verify_one(PE32(server),'server.dll')}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({name:{'regions':len(part['regions']),
        'opcodes':part['opcode_anchors'],'calls':part['direct_calls']}
        for name,part in result.items()},indent=2))
if __name__=='__main__': main()
