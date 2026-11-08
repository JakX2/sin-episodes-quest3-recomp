#!/usr/bin/env python3
"""Read-only 2006 SiN server.dll bone preparation and cache-update audit.

The original binary is owner-supplied. This checker includes only SHA-256
witnesses, addresses, instruction signatures and metadata, not game code.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS = {
    'BonePreparationStateMachine':(0x220eb8d0,0x220eb9ec,'1967c6985ea3943cb498a36f0da3b7d35132fbf40011ebef078c5e832b772dbf'),
    'CompactCacheReverseUpdate':(0x22100450,0x221004b1,'757c7ed176aed5cfdc0af6b2c312400aa75f72be1fb09feb651e9a083f51ffe4'),
    'CacheFreshnessComparison':(0x220fc5a0,0x220fc5b9,'4e04df9d5ff71668fc822766eee904f89d6a63f272c8559d30f74bc07e03d7d8'),
    'CacheHandleResolver':(0x22101ae0,0x22101af0,'e42940c4cf5597fbef25aaefc9b05db496aa96c923133552878ff3282ab4542a'),
    'CacheHandleRelease':(0x220fc5f0,0x220fc600,'d4c0d7b1912cc03169515416e2759732bfc8cbeb42926fa85eb7a40b8eab1ed3'),
    'CacheAllocationRegistry':(0x22104bc0,0x22104c12,'cbaee30b2cb2794fa1f21eceea91d6e88ae19c102050a142bea9f154b9c9a430'),
}
CALLS = {
    0x220eb8d5:0x223a6760,
    0x220eb8df:0x22139a10,
    0x220eb918:0x22101ae0,
    0x220eb937:0x220fc5a0,
    0x220eb954:0x220fc5f0,
    0x220eb994:0x22100450,
    0x220eb9ca:0x22104bc0,
    0x220eb9d6:0x22101ae0,
    0x22100481:0x22268ef0,
    0x22104be5:0x222725d0,
    0x22104bef:0x22272510,
    0x22104bf7:0x22102350,
    0x22104c0a:0x222714a0,
    0x220fc5fa:0x222722b0,
    0x22101aea:0x222723d0,
}
ANCHORS={
    0x220eb8d0:'b810180000',         # 0x1810-byte local pose buffer frame
    0x220eb8fa:'83f803',             # original mdldata type = 3
    0x220eb911:'8b8778040000',       # CBaseAnimating cached-handle +0x478
    0x220eb92f:'68cdcccc3d',         # 0.1f freshness threshold
    0x220eb93c:'84c0',               # fresh result skip calculation
    0x220eb944:'817e0400030000',     # expired cache must have tag 0x300
    0x220eb95e:'89b778040000',       # clear entity cached handle
    0x220eb966:'6800030000',         # bone mask 0x300
    0x220eb972:'ff92b4020000',       # virtual bone solve at +0x2B4
    0x220eb978:'85f6',               # existing cache versus new cache
    0x220eb985:'8b839c000000',       # studio bone count +0x9C
    0x220eb9c2:'c744241c00030000',  # allocation tag 0x300
    0x220eb9d0:'898778040000',       # install created handle at +0x478
    0x22100456:'0fb7470e0fb74f10', # compact matrix and reverse index offsets
    0x22100460:'6639770c',           # 16-bit cached bone count
    0x22100470:'0fbf447500',         # signed int16 cached -> source bone
    0x22100490:'83c330',             # advance 48 bytes per matrix
    0x22100497:'8b4c241c890f',      # update timestamp at cache+0
    0x220fc5a0:'d9442404',          # x87 fld(now)
    0x220fc5a4:'d821',              # x87 fsub cached timestamp
    0x220fc5a6:'d85c2408',          # x87 compare tolerance
    0x220fc5ac:'f6c441',            # unordered/greater comparison flags
}
# Same eight inherited RTTI tables independently verified in Phase 110.
VTABLES = {
    'CBaseAnimating':0x223fe6e4,
    'CBaseAnimatingOverlay':0x223ff6ac,
    'CBaseFlex':0x223ff994,
    'CBaseCombatCharacter':0x22459e5c,
    'CBasePlayer':0x2245222c,
    'CHL2_Player':0x224aec3c,
    'CAI_BaseNPC':0x223da7ac,
    'CNPC_Jessica':0x22497a14,
}
def reader(pe,va,n):
    if hasattr(pe,'read'):return pe.read(va,n)
    return pe.read_va(va,n)
def base(pe):return pe.base if hasattr(pe,'base') else pe.image_base
def readptr(pe,va):return struct.unpack('<I',reader(pe,va,4))[0]
def target(pe,at):
    encoded=reader(pe,at,5)
    if encoded[0]!=0xe8:raise ValueError('wrong near-CALL opcode at '+hex(at))
    return at+5+struct.unpack('<i',encoded[1:])[0]
def verify(binary):
    fullsha=hashlib.sha256(binary).hexdigest()
    if fullsha!=SERVER_SHA:raise ValueError('original SiN 2006 server.dll SHA-256 mismatch')
    pe=PE32(binary)
    if base(pe)!=0x22000000:raise ValueError('original PE32 image base mismatch')
    for name,(start,end,sha) in FUNCTIONS.items():
        if hashlib.sha256(reader(pe,start,end-start)).hexdigest()!=sha:
            raise ValueError('function witness mismatch: '+name)
    for at,to in CALLS.items():
        if target(pe,at)!=to:raise ValueError('original CALL mismatch '+hex(at))
    for at,encoded in ANCHORS.items():
        expected=bytes.fromhex(encoded)
        if reader(pe,at,len(expected))!=expected:
            raise ValueError('instruction witness mismatch '+hex(at))
    for name,table in VTABLES.items():
        if readptr(pe,table+0x2b4)!=0x220ef510:
            raise ValueError('inherited bone preparation virtual mismatch: '+name)
    return {'original_server_sha256':fullsha,'whole_function_hashes':len(FUNCTIONS),
            'call_targets':len(CALLS),'instruction_anchors':len(ANCHORS),
            'inherited_virtual_tables':len(VTABLES),
            'bone_preparation':'0x220EB8D0','bone_setup_virtual':'0x220EF510',
            'cache_update':'0x22100450','cache_handle_entity_offset':'0x478',
            'freshness_seconds':'0.1f','expected_cache_tag':'0x300',
            'bone_cache_matrix_stride_bytes':48,
            'proprietary_binary_included':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path,required=True)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (ValueError,OSError) as e:p.exit(1,'FAIL: '+str(e)+'\n')
if __name__=='__main__':main()
