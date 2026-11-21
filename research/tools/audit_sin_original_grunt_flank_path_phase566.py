#!/usr/bin/env python3
"""Original-only SHA-pinned 2006 Grunt flank-distance native function audit.

Proprietary DLLs are read privately and never emitted or modified. This audit
checks full native methods, direct call edges, authentic short field witnesses,
and the original x87 square-root function pointer, not playable parity.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge

METHODS = {
    0x2209D370: (53, '8f12ce1dc95011ea1289656287f5a4522322713532878c693aa43e0221acee29', 'backward cumulative waypoint distance recomputation'),
    0x2209D410: (49, '9d1f0effa105e5bf3c2a65cfb7a32c1a075cea1dca4cb2cf3ff916d9d56bd071', 'cached waypoint length getter with negative-one sentinel'),
    0x220B2A20: (26, '921a3f9d537e074580ceb7905ad98a4711f0742875c1c8f2280ffc1cee25517c', 'last point on +0x28 forward chain'),
    0x2206D9F0: (124, '89d5af4b7e6ff60288f9d3a636f4ac6f658ce898df290c3c1f67b377300a0526', 'distance in XY or XYZ based on numeric flag'),
    0x222680E0: (7, '6a9288ec74ac316568f99c1082f7fe146f57cf2af3a34ba4c989052209a3d807', 'float argument to original x87 fsqrt'),
}
CALLS={
    0x2209D38C:0x2206D9F0,
    0x2209D413:0x220B2A20,
    0x2209D426:0x2209D370,
}
CROSSCALLS={0x2235AE4A:0x2209D410}
WITNESSES={
    0x2209D375:'c7461400000000',
    0x2209D37C:'837e2c00',
    0x2209D386:'8b4624',
    0x2209D391:'d84614',
    0x2209D399:'d95f14',
    0x2209D39C:'837f2c00',
    0x2209D41C:'817814000080bf',
    0x2209D42E:'8b36',
    0x2209D434:'d94614',
    0x2209D439:'d90590253d22',
    0x220B2A20:'8b01',
    0x220B2A27:'8b4828',
    0x220B2A30:'8bc1',
    0x220B2A32:'8b4828',
    0x2206D9F3:'837c241c00',
    0x2206DA00:'d900',
    0x2206DA02:'d821',
    0x2206DA0B:'d86104',
    0x2206DA19:'d86108',
    0x2206DA34:'750a',
    0x2206DA36:'d9442408',
    0x2206DA40:'d944240c',
    0x2206DA5F:'ff15c4b15122',
    0x222680E0:'d9442404d9fac3',
}


def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'not owner original client.dll'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'not owner original server.dll'
    pe=PE32(server)
    assert pe.base==0x22000000
    spans=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,label
        assert pe.read(va+size,1)==b'\xCC',f'bad function boundary {va:#x}'
        assert pe.read(va+size-1,1)==b'\xc3',f'bad original RET {va:#x}'
        spans.append((va,va+size))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])), 'overlap/double count'
    for site,target in {**CALLS,**CROSSCALLS}.items():edge(pe,site,target)
    for va,hx in WITNESSES.items():
        val=bytes.fromhex(hx)
        assert pe.read(va,len(val))==val,f'bad instruction witness {va:#x}'
    # Original function pointer: 2006 image-side code at the imported-math
    # dispatch cell calls original x87 fsqrt 0x222680E0.
    assert struct.unpack('<I',pe.read(0x2251B1C4,4))[0]==0x222680E0
    # Original no-points fallback = +0.0f, and the Grunt's specific flank bar.
    assert struct.unpack('<f',pe.read(0x223D2590,4))[0]==0.0
    assert struct.unpack('<f',pe.read(0x223F8EFC,4))[0]==240.0
    return dict(new_complete_methods=len(METHODS),new_original_server_bytes=sum(x[0] for x in METHODS.values()),
                direct_native_calls=len(CALLS),prior_phase_crosscalls=len(CROSSCALLS),
                instruction_field_witnesses=len(WITNESSES),original_math_pointer=hex(0x222680E0),
                original_grunt_flank_threshold=240.0)


def self_test():
    class MockPE:
        def __init__(self,start,bytes_):self.start,self.data=start,bytes_
        def read(self,addr,n):return self.data[addr-self.start:addr-self.start+n]
    for site,target in {**CALLS,**CROSSCALLS}.items():
        raw=b'\xe8'+struct.pack('<i',target-(site+5))
        edge(MockPE(site,raw),site,target)
        for i in range(5):
            corrupt=bytearray(raw);corrupt[i]^=1
            try:edge(MockPE(site,corrupt),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'mutated original call accepted: {site:#x}')
    try:audit(b'MZwrong',b'MZwrong')
    except AssertionError:pass
    else:raise AssertionError('unsupported DLL accepted')
    return 'SELF_TEST_OK'


def main():
    args=argparse.ArgumentParser(description=__doc__)
    args.add_argument('--self-test',action='store_true')
    args.add_argument('--bin-zip',type=Path)
    opts=args.parse_args()
    if not (opts.self_test or opts.bin_zip):args.error('supply --self-test and/or --bin-zip')
    if opts.self_test:print(self_test())
    if opts.bin_zip:
        with zipfile.ZipFile(opts.bin_zip) as archive:
            print('SIN_GRUNT_FLANK_PATH_PHASE566_AUTHENTICATED',audit(archive.read('client.dll'),archive.read('server.dll')))
if __name__=='__main__':main()