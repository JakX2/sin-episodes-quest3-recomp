#!/usr/bin/env python3
"""Phase190: verify original SiN 2006 scene/model-sounds record identity callbacks.

No original DLL bytes, complete function bodies, or proprietary assets are
stored in this verifier. The full file hashes are required for owner audit.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_post_emitter import PE32, VerificationError

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGIONS = {
    'scene': (0x221ACFD0, 190,
              '48004fea0993479cec22accfbf75523c49b3e7f0cebb5a05ef228e178d4c0804',
              0x1c, 0x2242DCA0),
    'modelsounds': (0x22149770, 187,
                    'c79d38ec382e6477cf1872553addccc307629672bcafaf5a5f7e2c22e97b8e49',
                    0x18, 0x2240ABC8),
}
# Sparse native x86 opcode witnesses. These intentionally are not full bodies.
COMMON = {
    0x00: '83ec10',
    0x03: '8b542414',
    0x0b: '8b0da4215722',
    0x1c: 'c7442414feffffff',
    0x24: 'c7442418ffffffff',
    0x2e: '52ff90d4000000',  # FS vtable +0xD4, one file handle
    0x49: '0fb7c0',
    0x4c: '3dffff0000',      # missing uint16 index sentinel
}
DETAIL = {
 'scene': {
    0x51:'7562',0x53:'6a1c',0x61:'c700a0dc4222',
    0xa4:'0fb7c0',0xaa:'8d1449',0xad:'8b4e08',
    0xb0:'895cd114',0xb5:'5f5e5d',0xb8:'83c410c20400',
 },
 'modelsounds': {
    0x51:'755f',0x53:'6a18',0x61:'c700c8ab4022',
    0xa1:'0fb7c0',0xa7:'8d1449',0xaa:'8b4e08',
    0xad:'895cd114',0xb2:'5f5e5d',0xb5:'83c410c20400',
 },
}
CALLS = {
 0x221AD014:0x221A7FB0,0x221AD025:0x2226E790,
 0x221AD060:0x221A6620,0x221AD06F:0x221ACD00,
 0x221497B4:0x22142670,0x221497C5:0x2226E790,
 0x221497FD:0x22140810,0x2214980C:0x22146EF0,
}
INCOMING = {0x221AD55E:0x221ACFD0,0x22149A3E:0x22149770}

def rel_call(src: int, dst: int)->bytes:
    return b'\xe8'+struct.pack('<i',dst-src-5)

def verify_native(pe: PE32, *, verify_hash: bool=True)->dict:
    if pe.base != 0x22000000:
        raise VerificationError('Phase190 original server PE image base mismatch')
    for key,(va,length,digest,allocation,vtable) in REGIONS.items():
        if verify_hash and hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
            raise VerificationError('Phase190 complete original function SHA mismatch: '+key)
        for off,code in {**COMMON,**DETAIL[key]}.items():
            expected=bytes.fromhex(code)
            if pe.at(va+off,len(expected)) != expected:
                raise VerificationError('Phase190 opcode mismatch at %#x'%(va+off))
        if pe.at(va+0x53,2) != b'\x6a'+bytes([allocation]):
            raise VerificationError('Phase190 record allocation size mismatch')
        if pe.at(va+0x61,6) != b'\xc7\x00'+struct.pack('<I',vtable):
            raise VerificationError('Phase190 record vtable initializer mismatch')
        if pe.section(vtable) != '.rdata':
            raise VerificationError('Phase190 record vtable section mismatch')
    for src,dst in {**CALLS,**INCOMING}.items():
        if pe.at(src,5)!=rel_call(src,dst):
            raise VerificationError('Phase190 direct rel32 CALL mismatch at %#x'%src)
    return {
      'new_complete_native_functions':len(REGIONS) if verify_hash else 0,
      'new_complete_x86_bytes':sum(v[1] for v in REGIONS.values()) if verify_hash else 0,
      'sparse_instruction_witnesses':2*len(COMMON)+sum(len(v) for v in DETAIL.values()),
      'exact_rel32_call_edges':len(CALLS)+len(INCOMING),
      'filesystem_vtable_byte_offset':0xD4,
      'lookup_missing_index_sentinel':0xFFFF,
      'record_index_slot_stride_bytes':24,
      'records':{k:{'va':hex(v[0]),'code_length':v[1],
                      'allocation_bytes':v[3],'vtable_va':hex(v[4])}
                 for k,v in REGIONS.items()},
      'limits':['Original record contents, native helper implementations and FS virtual +0xD4 semantics remain opaque',
                'No game-loadable Windows replacement DLL is implemented'],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA or hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise VerificationError('Phase190 original whole-DLL SHA mismatch')
    return verify_native(PE32(server))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    result=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+'\n'
    if args.output:args.output.write_text(result,encoding='utf-8')
    else: print(result,end='')

if __name__=='__main__':main()
