#!/usr/bin/env python3
"""Owner-only read-only byte witnesses for the 2006 SiN original bone cache.
No original executable code, assets or original disassembly is redistributed.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FUNCTIONS={
 'CBaseAnimatingBoneGetter':(0x220ED1B0,0x220ED230,'4fea7931f33e8d0825a422c77b46fe46653b049ceea1e09344f7a7c1bd4e1165'),
 'BoneCacheRemap':(0x221004C0,0x221004E3,'7fc9d4fdffc06c20875ad7c526541110430b8d1359af39481ff872d7f40be908'),
 'EntityWorldMatrixFallback':(0x220EBE70,0x220EBE8D,'1139b829a7af912eb57a128d8cbfd4e0957959d4772223504d46ee522e026762'),
 'Matrix3x4Copy':(0x22268EF0,0x22268F04,'eda2988761a27e6b7a0f9a1fe41d482da5c805a3a6bc23ee934769a34c83be6e'),
}
CALLS={
 0x220ED1B4:0x22139A10,
 0x220ED1F2:0x220EB8D0,
 0x220ED1FA:0x221004C0,
 0x220ED20A:0x220EBE70,
 0x220ED210:0x22268EF0,
 0x220ED223:0x22268EF0,
 0x220EBE80:0x2213FA60,
}
ANCHORS={
 0x220ED1CB:'83f803',              # studio resource type 3
 0x220ED1E4:'85f6',                # negative bone rejected
 0x220ED1E8:'3bb09c000000',        # upper bound hdr+0x9c
 0x220ED1FF:'85c0',                # cached matrix null?
 0x220ED203:'8b442410',            # output pointer, fallback path
 0x221004C0:'8b442404',            # requested bone index
 0x221004C4:'0fbf444114',          # signed short remapping
 0x221004C9:'85c0',                # cache remap negative?
 0x221004CD:'0fb7510e',            # cache matrix offset +0xe
 0x221004D1:'8d0440',              # idx * 3
 0x221004D4:'c1e004',              # idx * 48
 0x221004D9:'8d441014',            # base+0x14+offset+idx*48
 0x220EBE73:'8b86b8000000',        # entity dirty flags
 0x220EBE79:'c1e80b',              # shift flag
 0x220EBE85:'8d8668020000',        # entity world matrix +0x268
 0x22268EFA:'b90c000000',          # 12 dwords
 0x22268EFF:'f3a5',                # rep movsd
}
def read(pe,va,n):
    fn=getattr(pe,'read',None)
    return (fn if callable(fn) else pe.read_va)(va,n)
def base(pe):return pe.base if hasattr(pe,'base') else pe.image_base
def call_target(pe,va):
    b=read(pe,va,5)
    if b[0]!=0xe8:raise ValueError('not a near CALL at '+hex(va))
    return va+5+struct.unpack('<i',b[1:])[0]
def verify(data):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=SERVER_SHA:raise ValueError('original server.dll does not match pinned SHA-256')
    pe=PE32(data)
    if base(pe)!=0x22000000:raise ValueError('original image base changed')
    for name,(lo,hi,function_digest) in FUNCTIONS.items():
        if hashlib.sha256(read(pe,lo,hi-lo)).hexdigest()!=function_digest:
            raise ValueError('function witness mismatch: '+name)
    for address,target in CALLS.items():
        if call_target(pe,address)!=target:
            raise ValueError('CALL target changed: '+hex(address))
    for address,encoded in ANCHORS.items():
        expected=bytes.fromhex(encoded)
        if read(pe,address,len(expected))!=expected:
            raise ValueError('instruction changed: '+hex(address))
    return {'original_sha256':digest,'full_function_hashes':len(FUNCTIONS),
            'relative_calls':len(CALLS),'instruction_anchors':len(ANCHORS),
            'bone_count_hdr_offset':'0x9C','signed_remap_offset':'0x14',
            'matrix_relative_offset':'0xE','matrix_stride_bytes':48,
            'world_fallback_entity_offset':'0x268',
            'original_game_bytes_distributed':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path,required=True)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as ex:p.exit(1,'FAIL: '+str(ex)+'\n')
if __name__=='__main__':main()
