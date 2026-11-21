#!/usr/bin/env python3
"""Read-only original SiN Episodes 2006 Grunt mode-0 spatial helper audit.

The owner-private retail DLL pair must remain local. This source publishes only
exact function hashes, field-level witnesses and call edges, no proprietary code.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_HASH = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_HASH = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE = 0x22000000
METHODS = {
    0x22071A20: (460, '406d61d26d0955d7e350adf2cfdd756a2bdac148a0a630d8843865554c3afd3e', 'mode-0 signed status and first-stage clearance'),
    0x2206F940: (68, '15fff7a2c9dd55774a638b1f9090d86b43f4d233e736e860a2d257100fa267d5', 'alternate clearance wrapper'),
    0x2204C980: (11, '603a2e12dd734e40d381885c907dbfbe41b1ef2c6a0a8d8166ed2168523640a1', 'Grunt navigation+0x8f8 tail trampoline'),
    0x22076910: (117, '9243ed71b618ba5f96a9164e7ead48314ac3e3e8a835d5812921aa6d26afedd4', 'serial-checked nav+0x7c metric selector'),
}
CALLS = {
    0x22071AA8: 0x2204C980,
    0x22071ABD: 0x2206EF20,
    0x22071B41: 0x2206F940,
    0x22071B78: 0x22071040,
    0x22071B99: 0x22035100,
    0x22071BA4: 0x22035090,
    0x2206F961: 0x2204C980,
    0x2206F97A: 0x2206EF20,
}
PRIOR_INCOMING = {0x22071D8D: 0x22071A20}  # authenticated Phase573 caller
TAIL_JUMPS = {0x2204C986: 0x22076910}
WITNESSES = {
    0x22071A20:'83ec18', 0x22071A23:'8b442420',
    0x22071A2D:'8b7c242c', 0x22071A88:'d95e20',
    0x22071A8B:'8b5d04',0x22071A95:'68000034c4',
    0x22071AA8:'e8d3aefdff',0x22071AB1:'8b5c243c',
    0x22071ABD:'e85ed4ffff',0x22071AC2:'84c0',
    0x22071AC6:'8b4620',0x22071AC9:'894624',
    0x22071AF2:'a1b8855822',0x22071B0C:'c706feffffff',
    0x22071B2A:'c21c00',0x22071B2D:'f644243c04',
    0x22071B41:'e8faddffff',0x22071B48:'8b442430',
    0x22071B78:'e8c3f4ffff',0x22071B7D:'8b7c2438',
    0x22071BB7:'3b7e1c',0x22071BC0:'896e24',
    0x22071BE9:'c21c00',0x2206F940:'8b44240c',
    0x2206F94E:'68000034c4',0x2206F955:'ff92f8050000',
    0x2206F961:'e81ad0fdff',0x2206F97A:'e8a1f5ffff',
    0x2206F981:'c20c00',0x2204C980:'8b89f8080000',
    0x2204C986:'e9859f0200',0x22076910:'8b417c',
    0x22076913:'83f8ff',0x22076921:'81e2ff0f0000',
    0x22076931:'394204',0x22076963:'ff9220010000',
    0x2207696D:'d905a0943e22',0x22076975:'d905148f3d22',
    0x2207697D:'d905f8223d22',0x22076984:'c3',
}
FLOATS = {0x223E94A0:2.6, 0x223D8F14:10.0, 0x223D22F8:1.0}

class PE32:
    def __init__(self, raw):
        if raw[:2] != b'MZ': raise AssertionError('invalid MZ')
        pos=struct.unpack_from('<I',raw,0x3c)[0]
        if raw[pos:pos+4] != b'PE\0\0': raise AssertionError('invalid PE')
        if struct.unpack_from('<H',raw,pos+4)[0] != 0x14c: raise AssertionError('not x86')
        if struct.unpack_from('<H',raw,pos+24)[0] != 0x10b: raise AssertionError('not PE32')
        self.base=struct.unpack_from('<I',raw,pos+24+28)[0]
        count=struct.unpack_from('<H',raw,pos+6)[0]
        secstart=pos+24+struct.unpack_from('<H',raw,pos+20)[0]
        self.sections=[]
        for i in range(count):
            off=secstart+40*i
            name=raw[off:off+8].split(b'\0')[0]
            _,rva,rsz,roff=struct.unpack_from('<IIII',raw,off+8)
            self.sections.append((name,self.base+rva,rsz,roff))
        self.raw=raw
    def read(self,addr,n):
        for name,va,size,off in self.sections:
            if va<=addr and addr+n<=va+size:
                return self.raw[off+addr-va:off+addr-va+n]
        raise AssertionError(f'VA outside mapped raw section: {addr:#x}')
    def section(self,addr):
        for name,va,n,_ in self.sections:
            if va<=addr<va+n:return name
        return None

def control_edge(pe,site,target,opcode):
    data=pe.read(site,5)
    assert data[0]==opcode, f'expected {opcode:#x} control transfer at {site:#x}'
    assert site+5+struct.unpack('<i',data[1:])[0]==target, f'wrong target at {site:#x}'

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, 'client.dll not owner original'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH, 'server.dll not owner original'
    pe=PE32(server)
    assert pe.base==BASE
    spans=[]
    for site,(length,digest,meaning) in METHODS.items():
        assert pe.section(site)==b'.text',f'outside .text: {meaning}'
        assert hashlib.sha256(pe.read(site,length)).hexdigest()==digest, f'native body mismatch: {meaning}'
        assert pe.read(site+length,1)==b'\xcc', f'wrong function/data boundary: {site:#x}'
        spans.append((site,site+length))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
    assert pe.read(0x22071BE9,3)==bytes.fromhex('c21c00')
    assert pe.read(0x2206F981,3)==bytes.fromhex('c20c00')
    assert pe.read(0x22076984,1)==b'\xc3'
    for site,target in {**CALLS,**PRIOR_INCOMING}.items():control_edge(pe,site,target,0xe8)
    for site,target in TAIL_JUMPS.items():control_edge(pe,site,target,0xe9)
    for site,witness in WITNESSES.items():
        code=bytes.fromhex(witness)
        assert pe.read(site,len(code))==code,f'code witness changed at {site:#x}'
    for va,value in FLOATS.items():
        assert pe.section(va)==b'.rdata'
        assert pe.read(va,4)==struct.pack('<f',value),f'original float literal mismatch at {va:#x}'
    return dict(full_native_methods=len(METHODS),distinct_server_bytes=sum(n for n,_,_ in METHODS.values()),
                native_calls=len(CALLS),prior_dispatch_callers=len(PRIOR_INCOMING),
                native_tail_jumps=len(TAIL_JUMPS),opcode_witnesses=len(WITNESSES),float_literals=len(FLOATS))

def self_test():
    class Stub:
        def __init__(self,site,buf): self.site,self.buf=site,buf
        def read(self,va,n):
            assert self.site<=va and va+n<=self.site+len(self.buf)
            return self.buf[va-self.site:va-self.site+n]
    for opcode,edges in [(0xe8,{**CALLS,**PRIOR_INCOMING}),(0xe9,TAIL_JUMPS)]:
        for site,target in edges.items():
            good=bytes([opcode])+struct.pack('<i',target-site-5)
            control_edge(Stub(site,good),site,target,opcode)
            for i in range(5):
                bad=bytearray(good);bad[i]^=1
                try:control_edge(Stub(site,bad),site,target,opcode)
                except AssertionError:pass
                else:raise AssertionError(f'mutated control edge accepted: {site:#x}')
    try:audit(b'not owner',b'not owner')
    except AssertionError:pass
    else:raise AssertionError('wrong DLL pair accepted')
    return 'PHASE574_SYNTHETIC_PASS'

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--self-test',action='store_true')
    a.add_argument('--bin-zip',type=Path)
    args=a.parse_args()
    if not(args.self_test or args.bin_zip):a.error('specify --self-test and/or --bin-zip')
    if args.self_test: print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            print('PHASE574_ORIGINAL_PASS',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()