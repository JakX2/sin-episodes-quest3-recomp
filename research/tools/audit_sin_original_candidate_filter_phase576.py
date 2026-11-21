#!/usr/bin/env python3
"""Read-only Phase 576 original SiN 2006 Grunt post-clearance authenticator.

Never patches binaries or distributes original machine code. Only prints
method boundaries/digests and bounded control-flow evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_BASE = 0x22000000
METHODS = {
    0x22071040: (2523, 'efc230a266706e2b6acb2de1081ae86d2b9abe9e3a43ef7dde011b1fe35e5bee'),
    0x22027C30: (91, '7bd375fadb682ca28763503174a92c69ed47dd02d4551d7ac1c5b3d3eace0f48'),
    0x22139F60: (19, 'd5ea9253030765bcd70227e7860e4620758e52644047e564b5756b5d78eaef44'),
}
DIRECT_CALLS = {
    0x220710DE: 0x22027C30,
    0x22071158: 0x2206F500,
    0x22071193: 0x2206F500,
    0x220711B2: 0x2225C840,
    0x22071226: 0x2204C980,
    0x2207123C: 0x22066270,
    0x220713D3: 0x2206E870,
    0x220713EF: 0x2206FC20,
    0x22071598: 0x2225C1A0,
    0x220715B2: 0x2225C1A0,
    0x220715C8: 0x2206FB70,
    0x22071634: 0x2206F500,
    0x22071671: 0x2225C0F0,
    0x220716C2: 0x2225C0F0,
    0x220716E1: 0x2225C840,
    0x22071822: 0x22139F60,
    0x22071894: 0x2225C0F0,
    0x220718AC: 0x2226E3E0,
    0x220718E7: 0x22066290,
    0x22071912: 0x22066290,
    0x22071A04: 0x2226E3E0,
}
INCOMING_CALLS = {0x22071B78: 0x22071040}
WITNESSES = {
    0x22071040:'81ecf0000000', 0x22071054:'3bd8',
    0x22071056:'b90e000000', 0x22071062:'f3ab',
    0x2207107B:'894324', 0x2207109C:'8903',
    0x220710AF:'894334', 0x220710E3:'d95320',
    0x220710F5:'f6c444', 0x220710F8:'7a0e',
    0x220710FC:'b001', 0x22071105:'c21800',
    0x2207110F:'f6c101', 0x2207111C:'dc1d18e63d22',
    0x22071124:'f6c405', 0x22071127:'7b46',
    0x22071130:'dc1d00903e22', 0x22071138:'f6c441',
    0x2207113D:'c7842410010000', 0x22071148:'f6c102',
    0x22071161:'be02000000', 0x22071168:'be01000000',
    0x2207116F:'33f6', 0x22071178:'8b842414010000',
    0x2207117F:'83e008', 0x220713E0:'8d542430',
    0x220715F2:'8b442464', 0x220715F9:'83f810',
    0x22071600:'0f8cdafdffff', 0x22071619:'894514',
    0x2207161C:'89452c', 0x220717DF:'89b734020000',
    0x2207182B:'b8fdffffff', 0x2207184A:'83c0fe',
    0x22071859:'8903', 0x2207185B:'d95b24',
    0x220718B7:'32c0', 0x220718C0:'c21800',
    0x220718C3:'f6842414010000',
    0x22071992:'c703feffffff', 0x22071A0F:'b001',
    0x22071A18:'c21800',
    0x22027C30:'56', 0x22027C4C:'ff15c4b15122',
    0x22027C52:'d90590253d22', 0x22027C61:'f6c444',
    0x22027C64:'7b16', 0x22027C7B:'c3',
    0x22027C7C:'c7460400000000', 0x22027C8A:'c3',
    0x22139F60:'56', 0x22139F65:'ff90f4000000',
    0x22139F6B:'f6d8', 0x22139F6D:'1bc0',
    0x22139F6F:'23c6', 0x22139F72:'c3',
}
LITERALS = {
    0x223D2590: ('<f', 0.0),
    0x223DE618: ('<d', 0.001),
    0x223E9000: ('<d', 99.999),
    0x223D6314: ('<f', 100.0),
    0x223DDFD0: ('<d', 16.0),
    0x223D8FE8: ('<d', 0.1),
    0x223D8F00: ('<d', 0.5),
}


def original_pe_reader(data: bytes):
    assert data[:2] == b'MZ', 'Not a PE MZ image'
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    assert data[pe:pe+4] == b'PE\0\0', 'Invalid PE signature'
    assert struct.unpack_from('<H', data, pe+4)[0] == 0x14C, 'Not original x86 machine'
    assert struct.unpack_from('<H', data, pe+24)[0] == 0x10B, 'Not original PE32'
    base = struct.unpack_from('<I', data, pe+52)[0]
    assert base == SERVER_BASE, 'Not expected preferred image base'
    section_off = pe + 24 + struct.unpack_from('<H', data, pe+20)[0]
    sections = []
    for i in range(struct.unpack_from('<H', data, pe+6)[0]):
        q = section_off + i*40
        label = data[q:q+8].split(b'\0')[0]
        virtual_size, rva, raw_size, raw_off = struct.unpack_from('<IIII', data, q+8)
        sections.append((label, base+rva, min(virtual_size,raw_size), raw_off))
    def read(addr: int, size: int) -> bytes:
        for label, start, section_size, raw_off in sections:
            if start <= addr and addr+size <= start+section_size:
                result = data[raw_off+(addr-start):raw_off+(addr-start)+size]
                assert len(result) == size
                return result
        raise AssertionError(f'Original VA outside mapped PE file: {addr:#x} + {size}')
    return read, sections


def call_target(read, addr, expected):
    raw = read(addr, 5)
    assert raw[0] == 0xE8, f'Original CALL opcode missing: {addr:#x}'
    target = addr + 5 + struct.unpack_from('<i', raw, 1)[0]
    assert target == expected, f'Original CALL mismatch at {addr:#x}: {target:#x}'


def audit(client: bytes, server: bytes):
    assert hashlib.sha256(client).hexdigest() == CLIENT_SHA, 'Wrong original client.dll identity'
    assert hashlib.sha256(server).hexdigest() == SERVER_SHA, 'Wrong original server.dll identity'
    read, sections = original_pe_reader(server)
    for start, (length, digest) in METHODS.items():
        assert any(name == b'.text' and begin <= start and start+length <= begin+size
                   for name, begin, size, _ in sections)
        assert hashlib.sha256(read(start, length)).hexdigest() == digest, f'Changed method {start:#x}'
        assert read(start+length,1) == b'\xCC', f'Function boundary changed {start:#x}'
    for site, target in {**DIRECT_CALLS, **INCOMING_CALLS}.items():
        call_target(read,site,target)
    for addr, hex_data in WITNESSES.items():
        expected = bytes.fromhex(hex_data)
        assert read(addr,len(expected)) == expected, f'Opcode witness changed {addr:#x}'
    for addr, (fmt, value) in LITERALS.items():
        data = struct.pack(fmt, value)
        assert read(addr,len(data)) == data, f'Literal changed at {addr:#x}'
    assert struct.unpack('<I',read(0x2251B1C4,4))[0] == 0x222680E0, 'sqrt pointer changed'
    assert read(0x22071A20,3) == bytes.fromhex('83ec18'), 'Next method entry changed'
    assert read(0x22027C90,3) == bytes.fromhex('8bc1c3'), 'Planar-normalization neighbor changed'
    assert read(0x22139F80,3) == bytes.fromhex('568bf1'), 'Entity gate neighbor changed'
    return dict(methods=len(METHODS),native_bytes=sum(x[0] for x in METHODS.values()),
                direct_calls=len(DIRECT_CALLS),incoming_calls=len(INCOMING_CALLS),
                opcode_witnesses=len(WITNESSES),literal_constants=len(LITERALS),
                sqrt_pointer='0x222680E0', candidate_loop_upper_bound=16)


def selftest():
    class Stub:
        def __init__(self, addr, contents):
            self.addr, self.contents = addr, contents
        def __call__(self, addr, size):
            assert addr == self.addr and size == 5
            return self.contents
    for site, target in {**DIRECT_CALLS, **INCOMING_CALLS}.items():
        code = b'\xe8'+struct.pack('<i', target-(site+5))
        call_target(Stub(site,code),site,target)
        for pos in range(5):
            altered = bytearray(code)
            altered[pos] ^= 1
            try:
                call_target(Stub(site,altered),site,target)
            except AssertionError:
                pass
            else:
                raise AssertionError(f'Mutation not caught: {site:#x}+{pos}')
    try:
        audit(b'not original', b'not original')
    except AssertionError:
        pass
    else:
        raise AssertionError('Unapproved original pair accepted')
    assert sum(x[0] for x in METHODS.values()) == 2633
    return 'PHASE576_SYNTHETIC_PASS'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--bin-zip', type=Path)
    args = parser.parse_args()
    if not (args.self_test or args.bin_zip):
        parser.error('Specify --self-test and/or --bin-zip')
    if args.self_test:
        print(selftest())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as archive:
            print('PHASE576_ORIGINAL_PASS', audit(archive.read('client.dll'),archive.read('server.dll')))

if __name__ == '__main__':
    main()