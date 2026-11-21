#!/usr/bin/env python3
"""Phase569 original PE32 auditor: Grunt route candidate-to-active promotion.
Original commercial binaries are owner-private, read-only, and never emitted.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, SERVER_HASH, CLIENT_HASH, edge

METHODS={
 0x22075480:(81,'fe29569ef963232204929ba6b34afe9231defe600c49a45d0f46411646d11551','reset candidate and conditional virtual refresh'),
 0x220754E0:(139,'c656b3404e31a18bcc6c7a0565f3a2cc3df9feb973a23e0b152a567517ad79fa','transfer candidate first node to active route'),
 0x220B2CF0:(22,'f80c12d876d67ed678b264d92cc0e901eef9e35d31ea209343761fcb365aca36','free linked route nodes and clear pointer'),
 0x2209D000:(50,'a7b13eb20aa9fe59fadbd580a02812bad11ef485c1e1eda746bf3fc68d84f497','mark last waypoint with terminal marker and endpoint coordinates'),
 0x2209D040:(37,'deecc5f623a17f579381567e40e29782cf627c5965e8aac8817475ffadceb0ce','store route endpoint as origin plus argument vector'),
 0x2209CEF0:(17,'6a09a9fdf9bc245627b133c7ecaea0c46dfcd352d5fca99ff8140de8bbef12a3','write route raw +0x24 and clear raw +0x28'),
 0x22074770:(43,'ce6bd4dd2a17df571f25e9a805616f6aaaff6ace7557dbbbe03bcc3ce5e4cb06','generation-checked navigator handle +0x10 resolution'),
}
CALLS={
 0x2207548D:0x220B2CF0,
 0x220754EF:0x22075480,
 0x2207550B:0x220B2CF0,
 0x22075522:0x2209D160,
 0x2207552C:0x220B2A10,
 0x22075536:0x2209D0C0,
 0x22075540:0x2209D0C0,
 0x2207555E:0x2209CEF0,
 0x220B2CF6:0x220B2C80,
}
CROSSCALLS={
 0x220753EF:0x2209D000,
 0x22075365:0x2209D040,
 0x22075444:0x2209D040,
}
WITNESSES={
 0x22075483:'8b4e28',0x22075486:'c7462c000080bf',
 0x22075492:'8b4624',0x22075495:'8b00',
 0x2207549B:'8b4024',0x2207549E:'83f803',0x220754A3:'83f801',
 0x220754AE:'83792c00',0x220754BC:'ff5254',
 0x220754C3:'8b0d90215722',0x220754CC:'89562c',
 0x220754E3:'8b4628',0x220754EA:'833800',
 0x22075501:'8b7e24',0x22075504:'833f00',
 0x22075510:'c7472cffffffff',0x2207551C:'6a01',
 0x2207552A:'6a00',0x22075534:'6a00',0x2207553E:'6a04',
 0x22075545:'8b4630',0x22075550:'894208',
 0x22075553:'8b4634',0x22075563:'b001',0x22075567:'32c0',
 0x220B2CF3:'8b06',0x220B2CFE:'c70600000000',
 0x2209D016:'8b10',0x2209D027:'c6413001',0x2209D02B:'83482008',
 0x2209D044:'c6413001',0x2209D048:'d94114',0x2209D04B:'d800',
 0x2209CEF4:'894124',0x2209CEF7:'c74128ffffffff',
 0x22074770:'8b4110',0x22074780:'81e1ff0f0000',0x2207478D:'c1e80c',
 0x22074790:'394104',0x22074795:'8b01',
}
def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'unexpected original client.dll'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'unexpected original server.dll'
    pe=PE32(server);assert pe.base==0x22000000
    intervals=[]
    for va,(length,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        raw=pe.read(va,length)
        assert hashlib.sha256(raw).hexdigest()==sha, f'whole method corrupted: {label}'
        assert pe.read(va+length,1)==b'\xcc',f'no authentic padding at {va:#x}'
        assert raw[-1]==0xc3 or raw[-3:] in (b'\xc2\x04\x00',b'\xc2\x08\x00'),label
        intervals.append((va,va+length))
    intervals.sort()
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:])), 'overlap'
    for site,target in {**CALLS,**CROSSCALLS}.items():edge(pe,site,target)
    for va,hx in WITNESSES.items():
        assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'opcode witness mismatch: {va:#x}'
    return {'original_complete_methods':len(METHODS),
            'original_code_bytes':sum(v[0] for v in METHODS.values()),
            'native_E8_calls':len(CALLS), 'incoming_native_E8_calls':len(CROSSCALLS),
            'instruction_witnesses':len(WITNESSES)}
def self_test():
    class MockPE:
        def __init__(self,base,data):self.base=base;self.data=data
        def read(self,address,n):return self.data[address-self.base:address-self.base+n]
    for site,target in {**CALLS,**CROSSCALLS}.items():
        native=b'\xe8'+struct.pack('<i',target-(site+5))
        edge(MockPE(site,native),site,target)
        for i in range(5):
            mutated=bytearray(native);mutated[i]^=1
            try:edge(MockPE(site,mutated),site,target)
            except AssertionError:pass
            else:raise AssertionError('corrupted E8 accepted')
    try:audit(b'not original',b'not original')
    except AssertionError:pass
    else:raise AssertionError('wrong binary accepted')
    return 'PHASE569_SELFTEST_OK'
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path);p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if not(args.self_test or args.bin_zip):p.error('pass --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            print('PHASE569_ORIGINAL_VERIFIED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()