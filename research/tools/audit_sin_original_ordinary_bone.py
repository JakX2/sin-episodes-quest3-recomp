#!/usr/bin/env python3
"""Owner-only SHA-256 and x86 opcode witnesses for SiN 2006 ordinary bones.

No copyrighted original machine-code bytes, models or disassembly are output.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'UnparentedMaskedBoneBuilder':(0x220FE0C0,0x220FE20A,
  'de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666'),
 'QuaternionWithTranslation':(0x2226D250,0x2226D278,
  '5155a696a20f9433b990e8be627790cfb833d545d835a8046647123eb2b7b280'),
 'AnglesAndPositionMatrix':(0x2226CC20,0x2226CC4A,
  '7cb35b38f51f106e70930bf8f3637477c0c6e2990f37c3a966ad895613e04ce3'),
}
CALLS={
 0x220EF7AF:0x220FE0C0,
 0x220FE144:0x2226CC20,
 0x220FE1A6:0x2226D250,
 0x220FE1EE:0x22269520,
}
ANCHORS={
 0x220FE0C0:'8b442418',     # target bone index, -1 all
 0x220FE0D3:'83f8ff',       # compare sentinel
 0x220FE0D8:'3b819c000000', # studio bone count +0x9c
 0x220FE0EF:'8bb19c000000', # all bones loop
 0x220FE100:'8901',         # order array push bone index
 0x220FE110:'8d0cc500000000', # bone record stride 216
 0x220FE123:'8b44cb04',     # bone parent index from record +4
 0x220FE141:'52',           # angle+origin matrix output buffer
 0x220FE149:'8d6eff',       # reverse chain order from stack
 0x220FE17A:'8584d3a0000000', # studio record boneFlags +0xa0 mask
 0x220FE1AB:'8b4704',       # direct parent index in studio record
 0x220FE1B1:'83f8ff',       # root parent == -1
 0x220FE1F6:'83ed01',       # decrement ancestor index
 0x220FE203:'81c460020000', # 0x260-byte local frame
}
def read(pe,va,n):
    reader=getattr(pe,"read",None)
    return reader(va,n) if callable(reader) else pe.read_va(va,n)
def branch(pe,va):
    b=read(pe,va,5)
    if b[0]!=0xe8:raise ValueError('expected near CALL at '+hex(va))
    return va+5+struct.unpack_from('<i',b,1)[0]
def verify(data):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=SERVER_SHA:raise ValueError('original server DLL SHA-256 mismatch')
    pe=PE32(data)
    base=getattr(pe,"image_base",getattr(pe,"base",None))
    if base!=0x22000000:raise ValueError('original preferred image base mismatch')
    for name,(lo,hi,sha) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=sha:
            raise ValueError('complete-function witness failed: '+name)
    for address,target in CALLS.items():
        if branch(pe,address)!=target:raise ValueError('relative call changed: '+hex(address))
    for address,encoding in ANCHORS.items():
        expected=bytes.fromhex(encoding)
        if read(pe,address,len(expected))!=expected:
            raise ValueError('opcode anchor mismatch: '+hex(address))
    return {'original_server_sha256':digest,
            'full_function_hashes':len(FUNCTIONS),
            'call_targets':len(CALLS),'instruction_anchors':len(ANCHORS),
            'original_ordinary_constructor':'0x220FE0C0',
            'bone_record_stride':216,'studio_mask_offset':'0xA0',
            'bone_count_offset':'0x9C','single_bone_chain':'parent-to-child',
            'original_binary_included':False}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',required=True,type=Path)
    args=parser.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as exc:parser.exit(1,'FAIL: '+str(exc)+'\n')
if __name__=='__main__':main()
