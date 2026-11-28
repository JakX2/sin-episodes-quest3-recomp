#!/usr/bin/env python3
"""Phase1731: private original CWorld dual 12-byte-vector insertion ABI.

Reads only the SHA-pinned unchanged original 2006 PE32 client.dll.
No executable bytes are shipped or included in generated metadata.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA,CTOR_VA,CTOR_LEN,CTOR_SHA,executable_text,read,sha_ok,need,
)
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8

SOURCE_INSERT_VA=0x24061220
SOURCE_INSERT_LEN=121
SOURCE_INSERT_SHA='3e3868e86685fa86f4bcbb796a35de8b8277cddb8b69e17f203a673757edb948'
SOURCE_GROW_VA=0x24060870
SOURCE_GROW_LEN=115
SOURCE_GROW_SHA='2dc1e61743a119f0043dfc270e07363ff9d3d7946a27e68855145a1fca9407a9'
SOURCE_ALLOC_VA=0x2416f8f0
SOURCE_REALLOC_VA=0x2416f960
SOURCE_MEMMOVE_VA=0x24248b50
SOURCE_ELEMENT_BYTES=12
SOURCE_CAPACITY=3
SOURCE_ALLOCATION_BYTES=36
SOURCE_FIRST_CALL_OFFSET=398
SOURCE_SECOND_CALL_OFFSET=436


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,'whole client.dll SHA mismatch')
    base,code=executable_text(original)
    ctor=read(base,code,CTOR_VA,CTOR_LEN)
    insert=read(base,code,SOURCE_INSERT_VA,SOURCE_INSERT_LEN)
    grow=read(base,code,SOURCE_GROW_VA,SOURCE_GROW_LEN)
    need(sha_ok(ctor,CTOR_LEN,CTOR_SHA),'CWorld source constructor SHA drift')
    need(sha_ok(insert,SOURCE_INSERT_LEN,SOURCE_INSERT_SHA),'121-byte vector insertion SHA drift')
    need(sha_ok(grow,SOURCE_GROW_LEN,SOURCE_GROW_SHA),'115-byte capacity growth SHA drift')
    need(insert[-3:]==b'\xc2\x08\x00' and grow[-3:]==b'\xc2\x04\x00',
         'original source stdcall/thiscall stack cleanup drift')
    need(read(base,code,SOURCE_INSERT_VA+SOURCE_INSERT_LEN,7)==b'\xcc'*7,
         '121B vector insert INT3 padding drift')
    need(read(base,code,SOURCE_GROW_VA+SOURCE_GROW_LEN,13)==b'\xcc'*13,
         '115B vector growth INT3 padding drift')
    for off in (SOURCE_FIRST_CALL_OFFSET,SOURCE_SECOND_CALL_OFFSET):
        expected_e8(base,code,CTOR_VA+off,SOURCE_INSERT_VA)
    expected_e8(base,code,SOURCE_INSERT_VA+25,SOURCE_GROW_VA)
    expected_e8(base,code,SOURCE_GROW_VA+79,SOURCE_REALLOC_VA)
    expected_e8(base,code,SOURCE_GROW_VA+101,SOURCE_ALLOC_VA)
    expected_e8(base,code,SOURCE_INSERT_VA+74,SOURCE_MEMMOVE_VA)
    # Native vector layout, len+1 versus capacity, capacity growth,
    # 12-byte element displacement and three DWORD copy.
    need(insert[:15]==bytes.fromhex('568bf18b460c8b4e048d50013bd157'),
         'vector insert count/capacity branch no longer matches source')
    need(insert[30:34]==bytes.fromhex('83 46 0c 01'),
         'source insertion no longer increases length by one')
    need(insert[43:55]==bytes.fromhex('2bc783e80185c0894e107e1b'),
         'original insert tail/no-shift branch or pointer alias drift')
    need(insert[94:111]==bytes.fromhex('8b4c24108b1189108b51048950048b4908'),
         'original insert lost two first 32-bit element copies')
    need(grow[0:14]==bytes.fromhex('568bf18b4e0883f9ff74648b4604'),
         'source vector growth-mode/initial state drift')
    need(grow[49:56]==bytes.fromhex('c7460403000000'),
         'zero capacity no longer grows to three slots')
    need(grow[93:101]==bytes.fromhex('8d044003c003c050'),
         'grow routine no longer multiplies capacity by 12 bytes')
    # CWorld ctor sets local 12-byte payloads [2,&owner+0x220,&owner+0x22c]
    # and [2,&owner+0x25c,&owner+0x268] with initial index=size(0) then size(1).
    need(ctor[362:368]==bytes.fromhex('8d8e20020000') and
         ctor[379:382]==bytes.fromhex('8d7e10') and
         ctor[390:398]==bytes.fromhex('c744241802000000'),
         'original first vector pointer/element setup drift')
    need(ctor[410:416]==bytes.fromhex('8d8e5c020000') and
         ctor[424:428]==bytes.fromhex('896c2420') and
         ctor[428:436] == bytes.fromhex('c744241802000000'),
         'original second vector pointer/element setup drift')
    need(SOURCE_ALLOCATION_BYTES==SOURCE_CAPACITY*SOURCE_ELEMENT_BYTES,
         'source initial growth geometry drift')
    print('PASS Phase1731 owner original 2006 PE32: both CWorld E8 append '
          'calls, 121B insertion, 115B growth, 36B allocation and native '
          '12B element-copy geometry; source CRT allocator/reallocator and '
          'memmove are never executed')


def self_test()->None:
    need(SOURCE_INSERT_LEN==121 and SOURCE_GROW_LEN==115 and
         SOURCE_FIRST_CALL_OFFSET==398 and SOURCE_SECOND_CALL_OFFSET==436,
         'source lengths and call offsets drift')
    need(not sha_ok(b'\x90'*121,121,SOURCE_INSERT_SHA) and
         not sha_ok(b'\x90'*115,115,SOURCE_GROW_SHA),
         'fabricated source helper bytes accepted')
    print('PASS Phase1731 original-free 2x12B append/36B growth source fingerprints')


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    need(args.self_test or args.client is not None,'use --self-test or --client /private/client.dll')
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=='__main__':
    try:main()
    except (ValueError,OSError) as exc:
        print(f'FAIL Phase1731: {exc}',file=sys.stderr)
        sys.exit(1)
