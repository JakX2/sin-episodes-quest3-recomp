#!/usr/bin/env python3
"""Read-only byte witnesses for SiN 2006 parented bone merge and name matching.

Original DLL supplied privately by the game owner. Prints only hashes and
numeric metadata, never original copyrighted executable bytes or disassembly.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'ParentBoneMerge':(0x220ED290,0x220ED3C2,'c6e4dd1d5df71287141c1984818e1d1ba99cbda114d6d4f9e306f4804b7d63e6'),
 'StudioFindBoneByName':(0x220FFB30,0x220FFBB8,'f3a538d6b42c332d9163373ad4c7366d8f4893ad701585d27288df5ed185f65b'),
 'CaseInsensitiveCompare':(0x223A5574,0x223A55B7,'d108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29'),
 'QuaternionToMatrix':(0x2226AF60,0x2226B03B,'e0ba6a46c016a127c8304d883923c432b0e46fa567a9613bef593c450b06f2d5'),
 'QuaternionMatrixWithPosition':(0x2226D250,0x2226D278,'5155a696a20f9433b990e8be627790cfb833d545d835a8046647123eb2b7b280'),
 'UnparentedComparison':(0x220FE0C0,0x220FE20A,'de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666'),
}
CALLS={
 0x220EF764:0x220ED290,
 0x220ED29A:0x22139A10,
 0x220ED2F2:0x2226CC20,
 0x220ED32A:0x220FFB30,
 0x220ED33E:0x221004C0,
 0x220ED349:0x22268EF0,
 0x220ED35E:0x2226D250,
 0x220ED38E:0x22269520,
 0x220FFB8D:0x223A5574,
 0x2226D25A:0x2226AF60,
 0x223A5591:0x223A72DA,
 0x223A559D:0x223A72DA,
}
ANCHORS={
 0x220ED290:'8b4c241c',          # seventh argument: parent entity
 0x220ED2D4:'8b8c2480000000',  # child world origin
 0x220ED2E3:'8bb7a0000000',    # child bone descriptors hdr+0xa0
 0x220ED2FA:'399f9c000000',    # child studiohdr bone count +0x9c
 0x220ED320:'8b0e',            # name relative offset from bone +0
 0x220ED33E:'e87d310100',      # compact parent bone cache lookup
 0x220ED363:'8b4604',          # child bone parent index +4
 0x220ED396:'8b44247c',        # loop loads studio count input
 0x220ED3A8:'81c6d8000000',  # 216-byte studio bone stride
 0x220FFB34:'8b88a0000000',  # hdr bones offset +0xa0
 0x220FFB3B:'8b986c010000',  # sorted-index offset +0x16c
 0x220FFB44:'8bb89c000000',  # studio bone count +0x9c
 0x220FFB6C:'0fb6041e',      # byte sorted bone-index entry
 0x220FFB7F:'8b04d1',        # relative bone name offset
 0x220FFB8D:'e8e2592a00',    # case insensitive string comparison
 0x220FFBA0:'8d7eff',        # binary-search high bound decrement
 0x220FFBA9:'5d83c8ff',      # missing name -> -1
 0x220FFBB0:'b6041e',        # found sorted index -> bone index
 0x2226D250:'8b54240c',    # matrix output
 0x2226D265:'894a0c',      # translation column 3
}
def read(pe,va,n):
    m=getattr(pe,'read',None)
    return m(va,n) if callable(m) else pe.read_va(va,n)
def base(pe):return pe.base if hasattr(pe,'base') else pe.image_base
def target(pe,va):
    b=read(pe,va,5)
    if b[0]!=0xe8:raise ValueError('expected relative call '+hex(va))
    return va+5+struct.unpack_from('<i',b,1)[0]
def verify(data):
    actual=hashlib.sha256(data).hexdigest()
    if actual!=SERVER_SHA:raise ValueError('2006 original server.dll digest mismatch')
    pe=PE32(data)
    if base(pe)!=0x22000000:raise ValueError('unexpected original image base')
    for name,(lo,hi,digest) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=digest:
            raise ValueError('whole-function hash changed: '+name)
    for va,expected in CALLS.items():
        if target(pe,va)!=expected:raise ValueError('call target changed: '+hex(va))
    for va,encoded in ANCHORS.items():
        b=bytes.fromhex(encoded)
        if read(pe,va,len(b))!=b:
            raise ValueError('instruction anchor changed: '+hex(va))
    return {'original_sha256':actual,'whole_function_hashes':len(FUNCTIONS),
            'near_call_targets':len(CALLS),'opcode_anchors':len(ANCHORS),
            'parent_merge':'0x220ED290','bone_name_lookup':'0x220FFB30',
            'quaternion_matrix':'0x2226D250','model_bone_stride':216,
            'sorted_name_index_offset':'0x16C','bone_count_offset':'0x9C',
            'original_binary_distributed':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (ValueError,OSError) as e:p.exit(1,'FAIL: '+str(e)+'\n')
if __name__=='__main__':main()
