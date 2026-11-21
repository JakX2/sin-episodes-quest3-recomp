#!/usr/bin/env python3
"""SHA-pinned, original-only SiN 2006 server sentence pipeline audit (Phase 558).

Only metadata is printed. No original commercial bytes are copied to output.
The binary is supplied locally with --bin-zip. The independent self-test
requires no original files.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
CLIENT_HASH='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_HASH='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

class PE32:
    def __init__(self,data):
        if data[:2]!=b'MZ':raise ValueError('not DOS MZ')
        off=struct.unpack_from('<I',data,0x3c)[0]
        if data[off:off+4]!=b'PE\0\0':raise ValueError('not PE')
        if struct.unpack_from('<H',data,off+4)[0]!=0x14c:raise ValueError('not x86 PE')
        opt=off+24
        if struct.unpack_from('<H',data,opt)[0]!=0x10b:raise ValueError('not PE32')
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        n=struct.unpack_from('<H',data,off+6)[0]
        sec=opt+struct.unpack_from('<H',data,off+20)[0]
        self.sections=[]
        self.data=data
        for i in range(n):
            p=sec+40*i
            name=data[p:p+8].split(b'\0')[0]
            vsize,rva,rawsize,rawoff=struct.unpack_from('<IIII',data,p+8)
            self.sections.append((name,self.base+rva,rawsize,rawoff))
    def read(self,addr,n):
        for name,va,rawsize,rawoff in self.sections:
            if va<=addr and addr+n<=va+rawsize:
                return self.data[rawoff+addr-va:rawoff+addr-va+n]
        raise ValueError('VA outside backed PE section')
    def section(self,addr):
        for name,va,rawsize,rawoff in self.sections:
            if va<=addr<va+rawsize:return name
        return None

def check_edge(pe,site,target,opcode):
    raw=pe.read(site,5)
    assert raw[0]==opcode and site+5+struct.unpack_from('<i',raw,1)[0]==target, f'bad rel32 call {site:#x} -> {target:#x}'


# Each body extends from entry to the exact end of its last legitimate
# instruction/data region, excluding following 0xCC padding. 0x220A3410's
# 4-entry jump table at 0x220A34DC is INCLUDED in its 220-byte interval.
METHODS = {
  0x220A32C0: (167, 'acc55a05ec231b12ee39ff24f4446d6b02aa018b31536ff9cafad02858512f74', 'conditional sentence diagnostic'),
  0x220A3370: (156, '514871b1c9c004298f641eb10bbf409693a614326956f83fcbad2a14f9b31427', 'queued sentence lifecycle check'),
  0x220A3410: (220, '4d4e5ccf3f514e0c8295e5105b054444e2e06c9a31ac9a986b9ee857562c6cc4', 'sentence mode gate incl jump table'),
  0x220A34F0: (203, '6688daf8fd51c81d86a498d4d7de9e3e81f5f56034ad8e4aca23b327d35de24b', 'sentence queue creation and diagnostics'),
  0x220A35C0: (131, '5d6bd2e685efd1123def77e7df7178a84a9acd184c73f8682f5cc0f199408653', 'sentence mode/argument dispatcher'),
}

CALLS = {
  0x220A331A: 0x220381B0,
  0x220A33CA: 0x221B37C0,
  0x220A33EC: 0x220A32C0,
  0x220A346C: 0x220A9480,
  0x220A34AC: 0x220A9480,
  0x220A34BC: 0x220AA2D0,
  0x220A3561: 0x221B3840,
  0x220A357B: 0x220A32C0,
  0x220A35A9: 0x220A32C0,
  0x220A35C8: 0x220A3410,
  0x220A35EF: 0x220A34F0,
  0x220A3611: 0x220A34F0,
  0x220A3636: 0x220A32C0,
}
# Original Grunt member at +0xCC0 invokes this shared sentence dispatcher;
# related native wrapper also invokes it. These are references, not new methods.
CROSS_CALLS = {0x2235A52D:0x220A35C0, 0x220A367F:0x220A35C0}
GATE_JUMP_TABLE = (0x220A34D6, 0x220A342B, 0x220A3448, 0x220A347F)
WITNESSES = {
  0x220A32C5:'8b402c',        # Original diagnostic selector +0x2C
  0x220A3373:'837e0cff',      # queued index == -1
  0x220A3382:'d94610',        # x87 queued timestamp +0x10
  0x220A3398:'ff9284050000',  # owner virtual +0x584
  0x220A33B6:'ff5214',        # sentence vtable virtual +0x14
  0x220A33BE:'ff5010',        # sentence vtable virtual +0x10
  0x220A33CA:'e8f1031100',    # queued output helper
  0x220A33DD:'50',            # emitted result to engine virtual +0x80
  0x220A33FC:'ff9288050000',  # owner virtual +0x588
  0x220A3403:'c7460cffffffff',# clear queued index
  0x220A3417:'b903000000',    # gate mode upper 3
  0x220A3424:'ff2485dc340a22',# jump through 4-entry native table
  0x220A342E:'398e50080000',  # owner state +0x850 == 3
  0x220A3436:'8b86b4000000',  # owner flags +0xB4
  0x220A3460:'8b8ec4090000',  # owner related +0x9C4
  0x220A346A:'6a01',          # related helper argument 1
  0x220A34B9:'56',            # owner to related predicate
  0x220A34D6:'b001',          # mode>3 defaults allow
  0x220A34F4:'81ec80000000',  # native 128-byte local temp, unbounded copy loop
  0x220A3504:'8a08',          # original byte copy
  0x220A350C:'84c9',          # stop only at NUL, no length gate
  0x220A3515:'ff90b4050000',  # owner optional virtual +0x5B4
  0x220A354C:'ff5214',        # this vtable virtual +0x14
  0x220A3554:'ff5010',        # this vtable virtual +0x10
  0x220A3561:'e8da021100',    # engine sentence queue
  0x220A3597:'ff9080000000',  # engine virtual +0x80
  0x220A35C8:'e843feffff',    # dispatch -> gating
  0x220A35D1:'83c8ff',        # rejected => -1
  0x220A35E3:'895e0c',        # clear active queue id to -1
  0x220A35FF:'57',            # non-default arg to owner +0x584
  0x220A361B:'6800000040',    # postqueue flag 0x40000000
  0x220A3623:'ff9288050000',  # owner virtual +0x588
  0x220A3631:'6850df3e22',    # CULL diagnostic
}
STRINGS = {
  0x223EDF08:b'SENTENCE [%d %.2f] %s: %s\n',
  0x223EDF24:b'Speaking [from QUEUE]',
  0x223EDF3C:b'Speaking',
  0x223EDF48:b'BOGUS',
  0x223EDF50:b'CULL',
}

def audit(client, server):
    assert hashlib.sha256(client).hexdigest() == CLIENT_HASH, 'unexpected client original identity'
    assert hashlib.sha256(server).hexdigest() == SERVER_HASH, 'unexpected server original identity'
    pe=PE32(server)
    assert pe.base == 0x22000000, 'wrong server PE image base'
    intervals=[]
    for va,(size,digest,label) in METHODS.items():
        assert pe.section(va)==b'.text', f'{label} outside original .text'
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==digest, f'{label}: original method digest mismatch'
        assert pe.read(va+size,1)==b'\xcc', f'{label}: missing original INT3 boundary'
        intervals.append((va,va+size))
    intervals.sort()
    assert all(x[1]<=y[0] for x,y in zip(intervals,intervals[1:])), 'overlap or recount'
    for site,target in {**CALLS,**CROSS_CALLS}.items():check_edge(pe,site,target,0xE8)
    assert struct.unpack('<4I',pe.read(0x220A34DC,16))==GATE_JUMP_TABLE, 'native gate jump targets mismatch'
    for va,hx in WITNESSES.items():
        assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'witness differs at {va:#x}'
    for va,literal in STRINGS.items():
        assert pe.read(va,len(literal)+1)==literal+b'\0',f'diagnostic literal differs {va:#x}'
    assert sum(size for size,*_ in METHODS.values())==877
    return dict(methods=5,unique_server_method_bytes=877,verified_internal_direct_calls=len(CALLS),
                external_grunt_related_calls=len(CROSS_CALLS),mode_jump_table=4,
                opcode_witnesses=len(WITNESSES),string_literals=len(STRINGS),
                original_server_sha256=SERVER_HASH)

def selftest():
    class Fake:
        def __init__(self,site,blob):self.site,self.blob=site,blob
        def read(self,addr,n):
            assert addr==self.site and n==5
            return self.blob
    for site,target in {**CALLS,**CROSS_CALLS}.items():
        good=bytes([0xe8])+struct.pack('<i',target-site-5)
        check_edge(Fake(site,good),site,target,0xe8)
        for i in range(5):
            bad=bytearray(good);bad[i]^=1
            try:check_edge(Fake(site,bytes(bad)),site,target,0xe8)
            except AssertionError:pass
            else:raise AssertionError('invalid rel32 edge accepted')
    assert len(set(GATE_JUMP_TABLE))==4, 'unexpected duplicate mode destinations'
    try:audit(b'MZ invalid',b'MZ invalid')
    except AssertionError:pass
    else:raise AssertionError('unsupported original binaries accepted')
    return 'SELF_TEST_OK'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-zip',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if not args.bin_zip and not args.self_test:parser.error('pass --self-test and/or --bin-zip')
    if args.self_test:print(selftest())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:out=audit(z.read('client.dll'),z.read('server.dll'))
        print('ORIGINAL_SIN_GRUNT_SENTENCE_PHASE558_AUTHENTICATED',out)
if __name__=='__main__':main()