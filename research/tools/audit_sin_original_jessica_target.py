#!/usr/bin/env python3
"""Read-only binary witnesses for Jessica's inherited world-collision center.

Run against the owner-provided, unchanged original 2006 SiN server.dll.
No proprietary DLL bytes are ever emitted or uploaded by this script.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CENTER_IMPL=0x2214BD40
FUNCTION_HASHES={
    'virtual_world_center':(0x2214BD40,0x2214BD8A,'822a832ca640788d3cd4bb94736413f3dcaa5a7f222ae914fca753cac0d99fb3'),
    'collision_bounds_midpoint':(0x2214B020,0x2214B0C2,'179d3cefecab711e90546ca6910931114f005f4768244115402ef9d4528e81e6'),
    'local_to_world_conversion':(0x2214B0D0,0x2214B16E,'82621ce3eb60687f54bde9720027b33c4fa5cf8e7d0a9b411f9bce7d9b33d369'),
}
# Vtable identity comes from MSVC x86 complete object locator / type descriptors,
# not guessed contiguous pointer arrays or Source 2013 ABI numbering.
VTABLES={
    'CBasePlayer':(0x2245222C,0x224C8498,0x224E5E60),
    'CHL2_Player':(0x224AEC3C,0x224D3FF0,0x2251E69C),
    'CNPC_Jessica':(0x22497A14,0x224D224C,0x22535B0C),
}
CALLS={0x2214BD78:0x2214B020,0x2214BD80:0x2214B0D0,
       0x2214B133:0x22268B20}
ANCHORS={
    0x22362509:bytes.fromhex('ff9214020000'), # player vtable +0x214
    0x2214BD4D:bytes.fromhex('8db9fc000000'), # entity +0xFC collision prop
    0x2214BD63:bytes.fromhex('83e07f'),       # 128-slot ring index
    0x2214B03E:bytes.fromhex('8d5114'),       # collision maxs +0x14
    0x2214B05A:bytes.fromhex('83c108'),       # collision mins +0x08
    0x2214B0D3:bytes.fromhex('f6462440'),     # collision flag bit 0x40
    0x2214B0D9:bytes.fromhex('8b4620'),       # solid type +0x20
    0x2214B0DC:bytes.fromhex('83f802'),       # special type 2
    0x2214B0E1:bytes.fromhex('85c0'),         # special type 0
    0x2214B0E7:bytes.fromhex('ff5024'),       # virtual local angle getter
    0x2214B125:bytes.fromhex('ff5228'),       # virtual entity transform getter
    0x2214B141:bytes.fromhex('8b16'),         # simple translation branch
}
def checked_pointer(pe,va):
    return struct.unpack('<I',pe.read(va,4))[0]
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=SERVER_SHA:raise ValueError('original server.dll SHA-256 mismatch')
    pe=PE32(data)
    if pe.base!=0x22000000:raise ValueError('unexpected original image base')
    for name,(start,end,expected) in FUNCTION_HASHES.items():
        if hashlib.sha256(pe.read(start,end-start)).hexdigest()!=expected:
            raise ValueError('code witness mismatch: '+name)
    for name,(table,col,type_desc) in VTABLES.items():
        if checked_pointer(pe,table-4)!=col:
            raise ValueError('RTTI vtable locator mismatch: '+name)
        if checked_pointer(pe,col+12)!=type_desc:
            raise ValueError('RTTI type descriptor mismatch: '+name)
        if checked_pointer(pe,table+0x214)!=CENTER_IMPL:
            raise ValueError('virtual slot +0x214 mismatch: '+name)
        raw=('.?AV'+name+'@@').encode('ascii')+b'\x00'
        if pe.read(type_desc+8,len(raw))!=raw:
            raise ValueError('RTTI decorated name mismatch: '+name)
    for at,target in CALLS.items():
        if pe.branch_target(at,0xe8)!=target:
            raise ValueError('call target mismatch at '+hex(at))
    for va,pattern in ANCHORS.items():
        if pe.read(va,len(pattern))!=pattern:
            raise ValueError('instruction witness mismatch at '+hex(va))
    if pe.read(0x223D9778,4)!=struct.pack('<f',0.5):
        raise ValueError('collision midpoint scale is not 0.5')
    return {'original_server_sha256':sha,
            'player_vtable_slot_bytes':0x214,
            'shared_vtable_target':hex(CENTER_IMPL),
            'types_verified':list(VTABLES),
            'complete_function_hashes':len(FUNCTION_HASHES),
            'relative_call_targets':len(CALLS),
            'instruction_anchors':len(ANCHORS),
            'collision_midpoint_scale':0.5,
            'original_game_binary_included':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as e:p.exit(1,'FAIL: '+str(e)+'\n')
if __name__=='__main__':main()
