#!/usr/bin/env python3
"""Phase 568: SHA-pinned original 2006 route mutation and length extensions.
Owner-private commercial DLLs are read-only; this script emits no game bytes.
"""
import argparse,hashlib,struct,zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge
METHODS={570902256: (27,
             '3b69acfa2835dd84abec9e5a68302cf56562cf61235dff19bd68d5115c44382f',
             'waypoint neighbor bidirectional unlink helper'),
 571068608: (45,
             '8cfdf325e21cda507e7a6ca21bcb8913ea9871c1be43ce79d3a3d2351f8e0ef4',
             'route state transition flag and original engine-time field'),
 571068768: (80,
             '551b6913d69c63152c9d0c309dfe14910a3bf71892ce8751bd54051d1fc02ebc',
             'route first-node replacement and optional last-node destination initialization'),
 571069520: (101,
             '21ed2b0389af568f08d820811302984a9f6e424e71344717e9a4469c0e7c38ed',
             'route length plus one external vector segment, first-node dimensionality'),
 571070240: (128,
             'ec3ec3b9014d2b232569782d9b7400ddd46791c2843b4ea5333a83d227b1daa7',
             'route head advancement, native node unlink/free and terminal distance invalidation'),
 571157008: (9,
             'aa1d0523d277f6a14d7950c488982e29fa08fe5d88274b15a712c4427302720a',
             'set route raw first-node pointer')}
CALLS={571068776: 571157008,
 571068783: 571157024,
 571069531: 571157024,
 571069550: 571069296,
 571069591: 570874352,
 571070311: 572976960,
 571070319: 571157008,
 571070326: 571157024}
CROSSCALLS={570901120: 571070240, 570903477: 571069520, 570905890: 571068768, 570905910: 571068608}
WITNESSES={570902256: '8b4128',
 570902263: '8b512c',
 570902276: '8b4928',
 571068608: '83794400',
 571068614: 'a190215722',
 571068626: '89514c',
 571068629: 'c6414001',
 571068643: 'c6414000',
 571068647: '895144',
 571068768: '8b442404',
 571068776: 'e8a3580100',
 571068783: 'e8ac580100',
 571068792: '807c240c00',
 571068797: 'c74014000080bf',
 571068806: 'c6463000',
 571068810: '83482008',
 571068818: 'd900',
 571068820: 'd84614',
 571068844: '5e',
 571069520: '51',
 571069525: '8b3e',
 571069540: '817814000080bf',
 571069550: 'e8fdfeffff',
 571069564: '8b4614',
 571069573: 'c7442408000000',
 571069585: '8b4724',
 571069591: 'e85405fdff',
 571069596: 'd8442414',
 571070240: '56',
 571070245: '85c0',
 571070249: 'f6402008',
 571070256: '8b7828',
 571070263: 'f6402004',
 571070269: '8b4810',
 571070286: '8b502c',
 571070299: '8b5028',
 571070305: '50',
 571070317: '8bce',
 571070335: '5f',
 571070345: '6894cb3e22',
 571070361: '834e2008',
 571157008: '8b442404',
 571157012: '8901'}

# Three original adjacent method boundaries are supported by exact RET and
# next method entry bytes, *not* guessed INT3 padding.
BOUNDARY_NEXT={
  0x2209D160:bytes.fromhex('8b442404'),
  0x2209D720:bytes.fromhex('5333dbf6'),
}
def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'not original client'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'not original server'
    pe=PE32(server)
    assert pe.base==0x22000000
    ranges=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,label
        if va in BOUNDARY_NEXT:
            assert pe.read(va+size,len(BOUNDARY_NEXT[va]))==BOUNDARY_NEXT[va]
        else:
            assert pe.read(va+size,1)==b'\xcc',f'bad native boundary {va:#x}'
        assert pe.read(va+size-1,1)==b'\xc3' or pe.read(va+size-3,3) in (b'\xc2\x04\x00',b'\xc2\x08\x00')
        ranges.append((va,va+size))
    ranges.sort()
    assert all(a[1]<=b[0] for a,b in zip(ranges,ranges[1:])), 'overlap'
    for site,tgt in {**CALLS,**CROSSCALLS}.items():edge(pe,site,tgt)
    for va,hx in WITNESSES.items():
        raw=bytes.fromhex(hx)
        assert pe.read(va,len(raw))==raw,f'original field/opcode witness {va:#x}'
    assert struct.unpack('<f',pe.read(0x223D2590,4))[0]==0.0
    return dict(complete_methods=len(METHODS),original_code_bytes=sum(x[0] for x in METHODS.values()),
                direct_calls=len(CALLS),crosscalls=len(CROSSCALLS),opcode_witnesses=len(WITNESSES))

def self_test():
    class MockPE:
        def __init__(self,start,data):self.start,self.data=start,data
        def read(self,va,n):return self.data[va-self.start:va-self.start+n]
    for site,target in {**CALLS,**CROSSCALLS}.items():
        native=b'\xe8'+struct.pack('<i',target-(site+5))
        edge(MockPE(site,native),site,target)
        for i in range(5):
            mutant=bytearray(native);mutant[i]^=1
            try:edge(MockPE(site,mutant),site,target)
            except AssertionError:pass
            else:raise AssertionError('corrupt original rel32 accepted')
    try:audit(b'not a client',b'not a server')
    except AssertionError:pass
    else:raise AssertionError('unsupported binary accepted')
    return 'PHASE568_SELF_TEST_OK'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-zip',type=Path)
    parser.add_argument('--self-test',action='store_true')
    opt=parser.parse_args()
    if not(opt.self_test or opt.bin_zip):parser.error('use --self-test and/or --bin-zip')
    if opt.self_test:print(self_test())
    if opt.bin_zip:
        with zipfile.ZipFile(opt.bin_zip) as owner:
            print('PHASE568_ORIGINAL_VERIFIED',audit(owner.read('client.dll'),owner.read('server.dll')))
if __name__=='__main__':main()