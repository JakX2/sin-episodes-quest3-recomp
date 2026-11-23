#!/usr/bin/env python3
"""Audit three complete native SiN Episodes (2006) rotation/fixup methods.

Original PE32 binaries are used only with --server/--client in owner-private
checks. The public self-test uses made-up bytes, not proprietary game bytes.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct

SERVER_SHA256 = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA256 = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
IMAGE_BASE = 0x22000000
METHODS = (
    ('rotate_left', 0x2228e710, 235,
     '5b96eff8753dab4996f3b011a5e4bbd6d8f6ccbfec341d22c9833d94d3759b0a'),
    ('rotate_right', 0x2228e800, 238,
     '70f1138c52c9aa4ca3a4ce23218620b65935fced6026547bf222ce8a1365e572'),
    ('insert_rebalance', 0x2228edf0, 1387,
     '69086eef8636a0041aa364549620a2c406af077bb3c451e5f3e0fd97e419ac8c'),
)
# Every original direct call from these three methods is accounted for.
ROTATE_LEFT_RESOLVE_CALLS = (
    0x2228e71b, 0x2228e727, 0x2228e744, 0x2228e753, 0x2228e770,
    0x2228e790, 0x2228e79c, 0x2228e7a9, 0x2228e7be,
)
ROTATE_RIGHT_RESOLVE_CALLS = (
    0x2228e80b, 0x2228e816, 0x2228e833, 0x2228e843, 0x2228e861,
    0x2228e881, 0x2228e88d, 0x2228e89b, 0x2228e8b1,
)
FIXUP_CALLS = (
    (0x2228f13c, 0x2228e710), (0x2228f144, 0x2228e100),
    (0x2228f150, 0x2228e100), (0x2228f17a, 0x2228e800),
    (0x2228f2f3, 0x2228e800), (0x2228f2fb, 0x2228e100),
    (0x2228f307, 0x2228e100), (0x2228f331, 0x2228e710),
)
CALLS = tuple((va, 0x2228e100) for va in
              (*ROTATE_LEFT_RESOLVE_CALLS, *ROTATE_RIGHT_RESOLVE_CALLS)) + FIXUP_CALLS


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from('<H', data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from('<I', data, offset)[0]


def pe_text(data: bytes) -> tuple[int, bytes]:
    if data[:2] != b'MZ':
        raise ValueError('not a DOS/PE file')
    pe = u32(data, 0x3c)
    if data[pe:pe+4] != b'PE\0\0' or u16(data, pe+4) != 0x14c:
        raise ValueError('not PE32 i386')
    opt = pe+24
    if u16(data, opt) != 0x10b or u32(data, opt+28) != IMAGE_BASE:
        raise ValueError('unexpected PE32 image base')
    count = u16(data, pe+6)
    section = opt+u16(data, pe+20)
    for i in range(count):
        s = section+i*40
        if data[s:s+8].rstrip(b'\0') != b'.text':
            continue
        size, rva, raw_size, raw_pointer = struct.unpack_from('<IIII', data, s+8)
        if not size or not raw_size or raw_pointer+raw_size > len(data):
            raise ValueError('invalid .text bounds')
        return IMAGE_BASE+rva, data[raw_pointer:raw_pointer+raw_size]
    raise ValueError('missing .text')


def get(text_va: int, raw: bytes, va: int, count: int) -> bytes:
    start=va-text_va
    if start < 0 or start+count > len(raw):
        raise ValueError('method/address outside executable .text')
    return raw[start:start+count]


def rel32_target(text_va: int, raw: bytes, va: int) -> int:
    code=get(text_va,raw,va,5)
    if code[0] != 0xe8:
        raise ValueError(f'not a direct rel32 call: {va:#x}')
    return (va+5+struct.unpack_from('<i',code,1)[0]) & 0xffffffff


def audit(server: bytes, client: bytes) -> tuple[int, int]:
    if hashlib.sha256(server).hexdigest() != SERVER_SHA256:
        raise ValueError('wrong original server.dll hash')
    if hashlib.sha256(client).hexdigest() != CLIENT_SHA256:
        raise ValueError('wrong original client.dll hash')
    text_va, raw=pe_text(server)
    for name, va, size, expected in METHODS:
        original=get(text_va,raw,va,size)
        if hashlib.sha256(original).hexdigest()!=expected:
            raise ValueError(f'incomplete/mismatched original {name}')
        if original[-3:] != b'\xc2\x04\x00' or get(text_va,raw,va+size,1)!=b'\xcc':
            raise ValueError(f'invalid original {name} RET 4/INT3 boundary')
    for call_va, target in CALLS:
        if rel32_target(text_va,raw,call_va)!=target:
            raise ValueError(f'original call edge mismatch at {call_va:#x}')
    return sum(method[2] for method in METHODS),len(CALLS)


def self_test() -> None:
    # Synthetic rel32 and branch mutation, no retail bytes involved.
    fake=bytearray(b'\x90'*64)
    fake[10]=0xe8
    struct.pack_into('<i',fake,11,30-(10+5))
    assert rel32_target(0,bytes(fake),10)==30
    fake[11] ^= 0x01
    assert rel32_target(0,bytes(fake),10)!=30
    fake[10] = 0x90
    try:
        rel32_target(0,bytes(fake),10)
    except ValueError:
        pass
    else:
        raise AssertionError('call opcode mutation survived')
    invented=b'\x90'*80+b'\xc2\x04\x00'
    digest=hashlib.sha256(invented).hexdigest()
    changed=bytearray(invented);changed[21]^=1
    assert digest != hashlib.sha256(changed).hexdigest()
    try:
        pe_text(b'not a PE')
    except ValueError:
        pass
    else:
        raise AssertionError('non-PE accepted')
    print('Phase654 synthetic-only audit self-test PASS')


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--client',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.server is None or args.client is None:
        parser.error('provide --server and --client, or --self-test')
    byte_count,edge_count=audit(args.server.read_bytes(),args.client.read_bytes())
    print(f'Phase654 owner-retail PASS: {byte_count} complete original bytes, '
          f'{edge_count} direct x86 call edges, 3 RET4/INT3 method boundaries')


if __name__=='__main__':
    main()
