#!/usr/bin/env python3
"""Authenticate original CGlobalState and CEnvGlobal method identities without exporting bytes."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE = 0x22000000
METHODS = {
    'global_clear': (0x221f4980, 69, '7c13db622884650619706d474b2f5165bb01bbf31bbf70574dad5519d0710c16'),
    'global_add': (0x221f4820, 120, '715322ee653347b965be3a75416460f9838ca6df6d5099b5abc48b803ab2b743'),
    'global_find': (0x221f45c0, 70, '1f5b5582c638d9a73c4b5b3c7c5988e1b66a6040362c67b9ed0788f912ebabaf'),
    'global_set_state': (0x221f4610, 40, 'fc140ed155bea5a3aaa49f3f72971ebeea2676d55dca637c2bc81f53bc692e56'),
    'global_set_level': (0x221f4640, 64, 'd025e320a72328acffa713b86f44296dbca800738a452743dba91dc5efa6bed8'),
    'symbol_clear': (0x2228f400, 49, 'a2421243cd3fb6f5bb6a352ef57826558f6768eeb197757074f3e2202d28ec78'),
    'tree_clear': (0x2228ec50, 132, 'be7e1d74e6cc2924a085a28c953618d1e8b21ac5da98245cc55cfa93634c21c6'),
    'case_sensitive_compare': (0x2228e910, 157, '8285c595abf524eb7a9878b5d83e7712974bda42b8662529e22e44e330dacf76'),
    'env_spawn': (0x22179470, 97, '04a2e897ed331d8b1d21e28c53a2615faabf1c39f99808a6a3cbabbfd1574592'),
    'env_on': (0x221794e0, 118, 'ffe6f0fea454bd62052b99ae3fc291c529087c0743f60de9f4934fb6403b1eb2'),
    'env_off': (0x22179560, 118, '5283971b3f6bb9a33be3e4d2e75a34b3e1ca9b052439f7e3051b29a7b68c4624'),
    'env_remove': (0x221795e0, 118, '99e8d875fe435466aec99342cfc5679a343c53cb43ef0bf9e0fe9fce667f9513'),
    'env_toggle': (0x22179660, 167, 'b1001430d4df71be63e695cc3fb01fbb5922e7b8babc4a4ee0b39902e89480a6'),
}
INPUTS = {0x224fc2e4: ('TurnOn', 'env_on'), 0x224fc318: ('TurnOff', 'env_off'),
          0x224fc34c: ('Remove', 'env_remove'), 0x224fc380: ('Toggle', 'env_toggle')}


def verify(data):
    if hashlib.sha256(data).hexdigest() != SHA:
        raise ValueError('wrong original server.dll SHA256')
    # The pinned PE has equal RVA/raw offsets for .text and .data here.
    # Check the section table before using that identity.
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0':
        raise ValueError('not PE')
    n = struct.unpack_from('<H', data, pe+6)[0]
    optional = struct.unpack_from('<H', data, pe+20)[0]
    if struct.unpack_from('<I', data, pe+24+28)[0] != BASE:
        raise ValueError('wrong image base')
    sections = []
    for i in range(n):
        off = pe+24+optional+i*40
        rva, size, raw = struct.unpack_from('<III', data, off+12)
        sections.append((rva, size, raw))
    def read(va, size):
        rva = va-BASE
        for start, length, raw in sections:
            if start <= rva and rva+size <= start+length:
                off = raw+rva-start
                if off+size <= len(data):
                    return data[off:off+size]
        raise ValueError('invalid file-backed address')
    def u32(va):
        return struct.unpack('<I', read(va, 4))[0]
    for name, (va, size, digest) in METHODS.items():
        if hashlib.sha256(read(va, size)).hexdigest() != digest:
            raise ValueError('method mismatch: '+name)
    for va, (name, method) in INPUTS.items():
        if u32(va) != METHODS[method][0] or read(u32(va-8), len(name)+1) != name.encode()+b'\0':
            raise ValueError('datamap mismatch: '+name)
    if u32(0x2241f7b8) != METHODS['env_spawn'][0]:
        raise ValueError('spawn vtable mismatch')
    for address, offset, name in [(0x224fc234, 0x394, 'globalstate'), (0x224fc29c, 0x39c, 'initialstate')]:
        if u32(address) != offset or read(u32(address+12), len(name)+1) != name.encode()+b'\0':
            raise ValueError('field mismatch: '+name)
    return {'original_sha256': SHA, 'method_count': len(METHODS), 'datamap_inputs': len(INPUTS),
            'datamap_fields': 2, 'spawn_vtable': True, 'original_bytes_exported': 0}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', required=True, type=Path)
    print(json.dumps(verify(parser.parse_args().server.read_bytes()), indent=2))
