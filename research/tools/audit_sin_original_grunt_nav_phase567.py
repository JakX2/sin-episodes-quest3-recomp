#!/usr/bin/env python3
"""Phase 567: original 2006 Grunt navigation component and route ownership.

Only byte hashes, short opcode witnesses, source-bound crosscall edges and
vtable pointers are stored here. Original DLLs remain owner-private.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge

METHODS = {570629456: (276,
             '575ffffba81df6bc80a78e347c3c8323f73412852556a9d632f79cf8ef75ebbd',
             'original shared AI component setup, virtual slots +3FC/+404/+408/+400/+40C/+410/+414'),
 570629968: (50,
             '8d099947aff7f4bb4f70dbc940af5c91543412ecec895fc4617d2f5a3da638aa',
             '0x9C-byte nav-component allocation plus constructor call'),
 570900624: (15,
             '8f9128e602be535749c7fb3db522ed78d9af3ab0c8974e2ca1dde8fb8fb4eb79',
             'wrapper normalization of route flag +0x44'),
 570900640: (21,
             '38155a2c8150d8ec632f8923e4f18feb2ce220019dc86f3aca2983b5d596cbfd',
             'component has nonempty waypoint chain'),
 570902208: (46,
             '759c3ddf7c122d6e55634db106e4843f6453d50c340f37853b8506071e7b5ff6',
             'waypoint copy initialization with links null and cache invalid'),
 570902432: (272,
             'efa418a1dc175deb2e47b3f6cc1e0a45c8b830a5def42e55186c616ea1a3cb66',
             'navigation component ctor, alloc 0x6C route and store +0x24'),
 571068656: (4,
             '75315a13a7503cb9a126675518a19ff201faa8c510e8a849d0f971d9c61bbb2b',
             'route +0x44 raw flag getter'),
 571069360: (89,
             '323005e7e1a163240b53764b5574b093479a375108b49539049488a030b2fa10',
             'route wrapper initializer with empty first node and sentinel state'),
 571923920: (10,
             '69beb53321d2f642bfcdaa9f2968f086157c785785fdd2b0d5448433f2579527',
             'Grunt vtable +0x40C allocates and zeros byte +0x38')}
CALLS = {570629641: 570867472,
 570629670: 570848464,
 570629705: 570965200,
 570629723: 571125968,
 570629977: 572974992,
 570630005: 570902432,
 570900627: 571068656,
 570902506: 572974992,
 570902520: 571069360,
 570902546: 572974992,
 571923920: 570629968}
WITNESSES = {570629461: 'ff90fc030000',
 570629469: '898698080000',
 570629481: 'ff9204040000',
 570629501: 'ff9008040000',
 570629509: '8986fc080000',
 570629521: 'ff9200040000',
 570629541: 'ff900c040000',
 570629549: '8986f8080000',
 570629561: 'ff9210040000',
 570629569: '898600090000',
 570629581: 'ff9014040000',
 570629589: '8986d8090000',
 570629646: '8b86f8080000',
 570629669: '50',
 570629675: '8b8ef8080000',
 570629689: '52',
 570629970: '689c000000',
 570629998: 'f3ab',
 570630002: '56',
 570900624: '8b4924',
 570900632: 'f7d8',
 570900634: '1bc0',
 570900640: '8b4124',
 570900647: '833800',
 570902213: '894818',
 570902225: 'b90c000000',
 570902232: 'f3a5',
 570902237: 'c74014000080bf',
 570902244: '89482c',
 570902247: '894828',
 570902440: '894604',
 570902443: 'c74608409c3d22',
 570902450: 'c70654933e22',
 570902456: 'c746082c933e22',
 570902466: '894660',
 570902480: '897e64',
 570902498: '6a6c',
 570902514: '3bc3',
 570902531: '894624',
 570902534: '895e20',
 570902564: '894628',
 570902590: 'c746680000803e',
 570902600: 'c7467000004040',
 570902610: 'c746780000a040',
 570902617: '8b0decb15122',
 570902653: '897e48',
 570902656: '89be98000000',
 570902672: '895e14',
 570902687: 'c78690000000ff',
 571068656: '8b4144',
 571069367: '8910',
 571069372: '89485c',
 571069409: '895004',
 571069424: 'c7404cffff7f7f',
 571069431: '894824',
 571069434: '894828',
 571069440: 'c74020cdcccc3d',
 571923925: 'c6403800'}
# Original source-generated Grunt primary vtable entry +0x40C.
VTABLE_PTR=0x2249AAA4+0x40C
GROUP_ROUTE_CROSS=0x2235AE41  # original Grunt event1 reads NPC +0x8F8

def audit(client, server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, 'not original client.dll'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH, 'not original server.dll'
    pe=PE32(server)
    assert pe.base==0x22000000
    spans=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,f'bad native method {label}'
        # 0x220747A0 has a direct following method, no INT3; it ends ret 4.
        if va==0x220747A0:
            assert pe.read(va+size-3,3)==b'\xc2\x04\x00'
            assert pe.read(va+size,3)==b'\xb0\x01\xc2'
        else:
            assert pe.read(va+size,1)==b'\xcc', f'bad native boundary {va:#x}'
            assert pe.read(va+size-1,1)==b'\xc3' or pe.read(va+size-3,3)==b'\xc2\x04\x00'
        spans.append((va,va+size))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])), 'overlapping native functions'
    for site,target in CALLS.items():edge(pe,site,target)
    for va,hx in WITNESSES.items():
        assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx), f'instruction {va:#x}'
    assert struct.unpack('<I',pe.read(VTABLE_PTR,4))[0]==0x2216DDD0
    assert pe.read(GROUP_ROUTE_CROSS,6)==bytes.fromhex('8b86f8080000')
    assert pe.read(0x2235AE47,3)==bytes.fromhex('8b4824')
    return dict(complete_methods=len(METHODS),unique_original_bytes=sum(row[0] for row in METHODS.values()),
                native_calls=len(CALLS),instruction_witnesses=len(WITNESSES),
                Grunt_nav_vtable='0x2216DDD0',component_allocation=0x9c,route_allocation=0x6c)

def self_test():
    class MockPE:
        def __init__(self,start,content):self.start,self.content=start,content
        def read(self,va,size):return self.content[va-self.start:va-self.start+size]
    for site,target in CALLS.items():
        source=b'\xe8'+struct.pack('<i',target-(site+5))
        edge(MockPE(site,source),site,target)
        for offset in range(5):
            mutate=bytearray(source)
            mutate[offset]^=1
            try:edge(MockPE(site,mutate),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'mutated E8 accepted {site:#x}')
    try:audit(b'non-original',b'non-original')
    except AssertionError:pass
    else:raise AssertionError('non-original binary accepted')
    return 'PHASE567_SELF_TEST_OK'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test',action='store_true')
    parser.add_argument('--bin-zip',type=Path)
    args=parser.parse_args()
    if not(args.self_test or args.bin_zip):parser.error('specify --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as original:
            print('PHASE567_ORIGINAL_VERIFIED',audit(original.read('client.dll'),original.read('server.dll')))

if __name__=='__main__':main()