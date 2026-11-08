#!/usr/bin/env python3
"""Read-only original SiN 2006 recursive pose-bone matrix witnesses.
Never exports raw DLL bytes or original disassembly. Original DLL stays private.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

ORIGINAL_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'RecursiveBoneWorldMatrix':(0x22100F30,0x22101027,
  '70dca7987b0eaba1e1f63be816ee588554bec4e33ce13eedd14401166e533f9f'),
 'SecondLoopSamplerDispatch':(0x22103D70,0x22103E3A,
  '8bc552e59e43d7ad60cf1af4efbec235a7a7d6bafcea4815103af20adcc45b0a'),
 'SecondLoopTrackIncrement':(0x221048A6,0x221048D5,
  '23d05658771f0375ca0b892c99330abd06259c7ed6b606a884c201f98fa1a302'),
 'NativeQuaternionToMatrixPrefix':(0x2226D250,0x2226D280,
  '263ddbc5da874eb8741da1b3dd157adbf4dd85a19d1585892a01e01059869b38'),
 'NativeMatrixMultiplyPrefix':(0x22269520,0x22269560,
  'fedaaf8609fafa512f904aa8461f5934ea9b4342418f5ad30f1e58470589bd21'),
}
CALLS={
 0x22100F7F:0x2226D250,
 0x22100FC1:0x22269520,
 0x22100FEE:0x22100F30,
 0x2210100A:0x22269520,
 0x22103DC8:0x22100F30,
 0x22103DDD:0x2226D250,
 0x22103DED:0x22268F10,
 0x22103E10:0x22269520,
}
ANCHORS={
 0x22100F35:'8b742450',  # requested bone index at argument +0x10
 0x22100F3B:'83e11f',    # bit index &31
 0x22100F4B:'c1e805',    # bitset word index /32
 0x22100F4E:'851c81',    # cached-bit test
 0x22100F5C:'0f85bf000000', # early return on previously cached
 0x22100F88:'8b90a0000000', # studio bone record offset +0xA0
 0x22100FA7:'83ffff',    # parent == -1 root case
 0x22100FCD:'0918',      # completed bitset OR
 0x22100FF3:'8d0476c1e004', # pose output 48-byte matrix stride
 0x2210101D:'0918',      # completed parented bitset OR
 0x22103D70:'8b7500',    # track array pointer
 0x22103D73:'d9443e5c',  # track influence +0x5C
 0x22103D84:'0f851c0b0000', # skip inactive to next track
 0x221048B1:'81c7e4000000', # track stride 0xE4
 0x221048C2:'0f8ca8f4ffff', # second loop backedge
}
def read(pe,address,count):return pe.read_va(address,count)
def target(pe,address):
    b=read(pe,address,5)
    if b[0]!=0xE8:raise ValueError('not relative CALL: '+hex(address))
    return address+5+struct.unpack('<i',b[1:])[0]
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=ORIGINAL_SHA:raise ValueError('not the owner-verified original server.dll')
    pe=PE32(data)
    if pe.image_base!=0x22000000:raise ValueError('unexpected image base')
    for name,(start,end,expected) in REGIONS.items():
        if hashlib.sha256(read(pe,start,end-start)).hexdigest()!=expected:
            raise ValueError('code-region hash mismatch: '+name)
    for at,expected in CALLS.items():
        if target(pe,at)!=expected:raise ValueError('call target mismatch: '+hex(at))
    for at,encoded in ANCHORS.items():
        expected=bytes.fromhex(encoded)
        if read(pe,at,len(expected))!=expected:
            raise ValueError('opcode witness mismatch: '+hex(at))
    return {'sha256':sha,'code_region_hashes':len(REGIONS),
            'relative_calls':len(CALLS),'opcode_anchors':len(ANCHORS),
            'recursive_function':'0x22100F30','second_pass_entry':'0x22103D70',
            'cached_bitset_word':'bone >> 5','cached_bitset_bit':'bone & 31',
            'original_code_or_assets_distributed':False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as ex:p.exit(1,'FAIL: '+str(ex)+'\n')
if __name__=='__main__':main()
