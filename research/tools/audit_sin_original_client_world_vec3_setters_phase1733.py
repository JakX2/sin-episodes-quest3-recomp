#!/usr/bin/env python3
"""Phase1733: source-authenticate two original CWorld x87 Vec3 setters.

Private --client uses only the unchanged user-owned original 2006 PE32
client.dll; no original executable bytes are committed to the repository.
The original dirty propagator 0x24096290 remains BLOCKED for execution.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, executable_text, need, read, sha_ok,
)
from audit_sin_original_client_world_interpolator_zeroed_phase1729 import (
    pe_section,
)
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8

PRIMARY_VTABLE=0x242895EC
FINALIZER_VA=0x240C9F40
SOURCE_DIRTY_VA=0x24096290
SETTERS=(
    (0x240C51B0,0x220,1,
     '3f4bb4faf68496fd5f46ccde0790bcb116909152c50e74c7b124dd3b451fd5b3',0xE8),
    (0x240C5280,0x25C,2,
     'a0de64a8e7a4a04f9f85f423e922323163a13f73b36c010eacfe1a4942db953e',0xF0),
)
SETTER_BYTES=101


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         'whole original 2006 client.dll SHA mismatch')
    base,code=executable_text(original)
    rd_base,rd,_,_=pe_section(original,'.rdata')
    def get_vtable_dword(va:int)->int:
        off=va-rd_base
        need(0<=off<=len(rd)-4,'original CWorld .rdata vtable out of bounds')
        return struct.unpack_from('<I',rd,off)[0]
    need(get_vtable_dword(PRIMARY_VTABLE)==0x240CD4C0,
         'CWorld primary vtable first slot unexpectedly changed')
    final=read(base,code,FINALIZER_VA,414)
    need(final[95:101]==bytes.fromhex('ff90e8000000') and
         final[110:116]==bytes.fromhex('ff92f0000000'),
         'original 414B finalizer no longer invokes +E8,+F0 virtual setter pair')
    for va,field,dirty,sha,slot in SETTERS:
        need(get_vtable_dword(PRIMARY_VTABLE+slot)==va,
             f'original CWorld virtual slot {slot:#x} target drift')
        block=read(base,code,va,SETTER_BYTES)
        need(sha_ok(block,SETTER_BYTES,sha),
             f'whole 101B source Vec3 setter SHA drift at {va:#x}')
        need(block[:8]==bytes.fromhex('56578b7c240c8bf1') and
             block[-3:]==b'\xc2\x04\x00' and
             read(base,code,va+SETTER_BYTES,11)==b'\xcc'*11,
             'source method thiscall/RET4/padding ABI drift')
        # Three ordered x87 comparisons and TEST AH,0x44 distinguish
        # equal from unequal or NaN. The first two JP go to dirty branch,
        # and final JNP goes to the no-write return.
        need(block.count(b'\xf6\xc4\x44')==3 and
             block.count(b'\xf6\xc4\x44\x7a')==2 and
             block.count(b'\xf6\xc4\x44\x7b')==1,
             'x87 FUCOMPP/flags parity-based three-field comparisons drift')
        need(block.count(b'\xda\xe9\xdf\xe0')==3,
             'original setter no longer compares exactly 3 floats')
        for offset in (field,field+4,field+8):
            need(b'\xd9\x86'+struct.pack('<I',offset) in block,
                 f'source Vec3 component offset {offset:#x} changed')
        need(block[61:63]==bytes((0x6a,dirty)),
             'source dirty-propagation mask no longer matches channel')
        expected_e8(base,code,va+65,SOURCE_DIRTY_VA)
        for offset in (field,field+4,field+8):
            need(struct.pack('<I',offset) in block,
                 f'source dirty setter write offset {offset:#x} changed')
    print('PASS Phase1733 owner original client SHA, both CWorld primary'
          ' vtable slots +E8/+F0, two complete 101B methods with RET4'
          ' and 11 INT3, x87 FUCOMPP comparison of three fields,'
          ' conditional original dirty propagator 0x24096290 flags 1/2')


def self_test()->None:
    need(len(SETTERS)==2 and SETTER_BYTES==101 and
         SETTERS[0][1]+12<=1200 and SETTERS[1][1]+12<=1200,
         'original source setter inventory invalid')
    for _,_,_,sha,_ in SETTERS:
        need(not sha_ok(b'\x90'*SETTER_BYTES,SETTER_BYTES,sha),
             'fabricated source method accepted by SHA witness')
    print('PASS Phase1733 original-free setter identity/mutation refusal')


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,
                   help='private unchanged 2006 PE32 client.dll')
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    need(args.self_test or args.client is not None,
         'use --self-test or --client /private/client.dll')
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=='__main__':
    try:main()
    except (OSError,ValueError) as exc:
        print(f'FAIL Phase1733: {exc}',file=sys.stderr)
        sys.exit(1)
