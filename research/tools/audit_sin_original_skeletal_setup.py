#!/usr/bin/env python3
"""Read-only, SHA-pinned witnesses for original SiN (2006) skeletal setup.

Requires the user's untouched legally owned original Windows PE32 server.dll.
Does not redistribute the original DLL, decompiled source, or disassembly.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

ORIGINAL_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'SkeletalDispatch':(0x220ef510,0x220ef7d7,'50a14a2a1da29a091a6f38fb899914a9111b6881712ef05e35ec3f57c4e44b00'),
 'ParentBoneBuild':(0x220ed290,0x220ed3c2,'c6e4dd1d5df71287141c1984818e1d1ba99cbda114d6d4f9e306f4804b7d63e6'),
 'UnparentedBoneBuild':(0x220fe0c0,0x220fe20a,'de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666'),
}
# Binary-checked relative calls, including original parent->cache recursion.
CALLS={
 0x220ef547:0x22139a10, 0x220ef599:0x2213fa60,
 0x220ef607:0x2213fa60, 0x220ef63c:0x22106840,
 0x220ef679:0x22103a60, 0x220ef6ab:0x22104d10,
 0x220ef70c:0x223a5bab, 0x220ef71c:0x220eb8d0,
 0x220ef737:0x2213fa60, 0x220ef764:0x220ed290,
 0x220ef77a:0x2213fa60, 0x220ef7af:0x220fe0c0,
 0x220ed29a:0x22139a10,0x220ed2f2:0x2226cc20,
 0x220ed32a:0x220ffb30,0x220ed33e:0x221004c0,
 0x220ed349:0x22268ef0,0x220ed35e:0x2226d250,
 0x220ed38e:0x22269520,
 0x220fe144:0x2226cc20,0x220fe1a6:0x2226d250,
 0x220fe1ee:0x22269520,
}
ANCHORS={
 0x220ef510:'81ec380e0000',  # skeletal setup stack frame
 0x220ef564:'83f803',        # MDL cache data type 3
 0x220ef57f:'8b96b8000000',  # world matrix dirty bit +0xB8
 0x220ef585:'8b8ed4030000',  # vertical offset field +0x3D4
 0x220ef59e:'39aed8030000',  # optional driver pointer +0x3D8
 0x220ef5ea:'0f84c2000000',  # skip pose driver path
 0x220ef5f6:'8386dc03000001',# increment +0x3DC driver counter
 0x220ef653:'ff90ac020000',  # virtual base pose +0x2AC
 0x220ef68c:'ff90b8020000',  # driver virtual hook +0x2B8
 0x220ef6cb:'ff90ac020000',  # same virtual call, no-driver path
 0x220ef6d1:'8b86f0000000',  # parent EHANDLE +0xF0
 0x220ef6d7:'83f8ff',        # invalid EHANDLE sentinel
 0x220ef6e4:'81e2ff0f0000',  # EHANDLE entity table slot bits
 0x220ef6f1:'c1e80c',        # generation bits
 0x220ef71c:'e8afc1ffff',    # recursive parent cache prepare
 0x220ef764:'e827dbffff',    # parent merged bone assembly
 0x220ef7af:'e80ce90000',    # unparented bone construction
 0x220ef7d4:'c20800',        # thiscall callee pops two arguments
 0x220ed2fa:'399f9c000000',  # parent builder uses studio bone count
 0x220ed33e:'e87d310100',    # parent bone compact remap
 0x220fe17a:'8584d3a0000000',# base builder tests bone mask
}
VTABLES={
 'CBaseAnimating':(0x223fe6e4,0x220f2ac0),
 'CBaseAnimatingOverlay':(0x223ff6ac,0x220f5430),
 'CBaseFlex':(0x223ff994,0x220f5430),
 'CBaseCombatCharacter':(0x22459e5c,0x220f5430),
 'CBasePlayer':(0x2245222c,0x220f5430),
 'CHL2_Player':(0x224aec3c,0x220f5430),
 'CAI_BaseNPC':(0x223da7ac,0x220f5430),
 'CNPC_Jessica':(0x22497a14,0x220f5430),
}
def read(pe,va,n):
    fn=getattr(pe,'read',None)
    return fn(va,n) if callable(fn) else pe.read_va(va,n)
def base(pe):return pe.base if hasattr(pe,'base') else pe.image_base
def pointer(pe,va):return struct.unpack('<I',read(pe,va,4))[0]
def call_target(pe,va):
    b=read(pe,va,5)
    if b[0]!=0xe8:raise ValueError('unexpected original CALL opcode at '+hex(va))
    return va+5+struct.unpack('<i',b[1:])[0]
def verify(data):
    actual=hashlib.sha256(data).hexdigest()
    if actual!=ORIGINAL_SHA:raise ValueError('original 2006 server.dll SHA-256 mismatch')
    pe=PE32(data)
    if base(pe)!=0x22000000:raise ValueError('original image base mismatch')
    for name,(lo,hi,digest) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=digest:
            raise ValueError('function witness mismatch '+name)
    for va,target in CALLS.items():
        if call_target(pe,va)!=target:raise ValueError('CALL edge mismatch '+hex(va))
    for va,hexbytes in ANCHORS.items():
        b=bytes.fromhex(hexbytes)
        if read(pe,va,len(b))!=b:raise ValueError('instruction anchor mismatch '+hex(va))
    for name,(table,base_pose) in VTABLES.items():
        if pointer(pe,table+0x2b4)!=0x220ef510:
            raise ValueError('skeletal dispatch virtual mismatch '+name)
        if pointer(pe,table+0x2ac)!=base_pose:
            raise ValueError('virtual base pose override mismatch '+name)
        if pointer(pe,table+0x2b8)!=0x220f0b80:
            raise ValueError('virtual driver hook mismatch '+name)
    return {'original_server_sha256':actual,
            'whole_function_hashes':len(FUNCTIONS),'call_edges':len(CALLS),
            'instruction_anchors':len(ANCHORS),'inherited_vtables':len(VTABLES),
            'skeletal_setup':'0x220EF510','parent_builder':'0x220ED290',
            'ordinary_builder':'0x220FE0C0','virtual_setup_slot':'0x2B4',
            'virtual_base_pose_slot':'0x2AC','virtual_hook_slot':'0x2B8',
            'original_game_bytes_included':False}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',required=True,type=Path)
    args=parser.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as err:parser.exit(1,'FAIL: '+str(err)+'\n')
if __name__=='__main__':main()
