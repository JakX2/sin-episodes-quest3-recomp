#!/usr/bin/env python3
"""Authenticate source-bounded original 2006 SiN Grunt speech reaction methods.

Only owner-private SHA-matching client.dll and server.dll are accepted.
No commercial native code/disassembly bytes are output or redistributed.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import CLIENT_HASH, SERVER_HASH, PE32, edge

VTABLE_START = 0x2249AAA4
METHODS = {
 0x22353FD0:(6,'b73c01b91523a4ea5ed232948f7e33d737622673f95edce26156ae6a8ccf1746','GRUNT_DIE literal getter'),
 0x22353FE0:(6,'b25fb3090b44d9579a39a0c558fbe2206af2244a5bb5984340699cf0620a1cca','GRUNT_MUTANTDEATH literal getter'),
 0x223540C0:(6,'abed7bd812cacd4b23cfe219e3ee543c4c14c30f80720d74bb1e04141d4d27cb','GRUNT_FLANK literal getter'),
 0x22354170:(6,'e17d4c73112e6e6c8380d680b8c95c41e72022f80014f2cd6b27c3faf168ae83','GRUNT_PAIN first literal getter'),
 0x22354180:(6,'021b4ee4ca707a049660bc9e8ae29fc8bf11d6b852a522705aa529af78228896','GRUNT_TAUNT literal getter'),
 0x22354190:(6,'e17d4c73112e6e6c8380d680b8c95c41e72022f80014f2cd6b27c3faf168ae83','GRUNT_PAIN second literal getter'),
 0x2235AE30:(85,'d4cf7a90c31cc3f6d7fb9412e5570cf975bbf130c9b70967bc4a7dcce32a68a1','original Grunt event 0/1 speech and metric gate'),
 0x2235AE90:(220,'5256cbba36900560183a9203d0ac492a3f737abb3d028459defc187b6e07b9f8','original Grunt pain/taunt thresholds and 1-second voice timer'),
 0x2235AFF0:(91,'ec1585edc3137e2a5ff13d66729306b9ea340f3fa0fa37d7ddc5086187ac254c','original Grunt die/mutantdeath selection'),
}
VTABLE = {
 0x5A8:0x2235AE30, 0x598:0x2235AE90, 0x58C:0x2235AFF0,
 0x7C0:0x22353FD0, 0x7C4:0x22353FE0, 0x7FC:0x223540C0,
 0x828:0x22354170,0x82C:0x22354180,0x830:0x22354190,
}
CALLS = {
 0x2235AE4A:0x2209D410,
 0x2235AE71:0x220A35C0,
 0x2235AE7C:0x2235A3A0,
 0x2235AF51:0x220A35C0,
 0x2235B02A:0x220A35C0,
 0x2235B044:0x220A35C0,
}
VIRTUAL_CALLS={
 0x2235AE60:0x7FC,
 0x2235AECE:0x828,
 0x2235AF0B:0x82C,
 0x2235AF3A:0x830,
 0x2235B019:0x7C0,
 0x2235B033:0x7C4,
}
STRINGS={
 0x2248CDCC:b'GRUNT_DIE',0x2248CDD8:b'GRUNT_MUTANTDEATH',
 0x2248CEB4:b'GRUNT_FLANK',0x2248CF58:b'GRUNT_PAIN',0x2248CF64:b'GRUNT_TAUNT',
}
CONSTANTS = {
 0x223F8EFC:('<f',240.0),
 0x223E9138:('<d',0.9),
 0x223DA478:('<d',0.25),
 0x223D22F8:('<f',1.0),
}
WITNESSES={
 0x2235AE30:'8b442404',0x2235AE34:'83e800',0x2235AE3C:'83e801',
 0x2235AE41:'8b86f8080000',0x2235AE4F:'d81dfc8e3f22',
 0x2235AE57:'f6c441',0x2235AE66:'6a02',0x2235AE68:'6a00',
 0x2235AE6B:'8d8ec00c0000',
 0x2235AE93:'8b8ebc000000',0x2235AE99:'f7c100000010',
 0x2235AEA5:'a190215722',0x2235AEAD:'d89e780c0000',
 0x2235AEB5:'f6c441',0x2235AEBE:'f7c100000008',
 0x2235AED4:'db869c000000',0x2235AEDA:'8b8e88090000',
 0x2235AEE2:'dab698000000',0x2235AEEC:'dc1538913e22',
 0x2235AEF4:'f6c441',0x2235AEFD:'81c900000080',
 0x2235AF03:'898e88090000',0x2235AF15:'f7c100000040',
 0x2235AF1D:'dc1d78a43d22',0x2235AF25:'f6c405',
 0x2235AF2C:'81c900000040',0x2235AF32:'898e88090000',
 0x2235AF46:'6a00',0x2235AF48:'6aff',
 0x2235AF4B:'8d8ec00c0000',0x2235AF5E:'d805f8223d22',
 0x2235AF64:'d99e780c0000',
 0x2235AFF3:'f686bf00000010',0x2235AFFC:'80beb10c000000',
 0x2235B005:'80be440c000000',0x2235B00E:'83be680c00000a',
 0x2235B01F:'6a00',0x2235B021:'6aff',
 0x2235B039:'6a00',0x2235B03B:'6aff',
}


def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'client.dll wrong identity'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'server.dll wrong identity'
    pe=PE32(server)
    assert pe.base==0x22000000
    spans=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,f'original method mutated {label}'
        assert pe.read(va+size,1)==b'\xcc',f'original method boundary {label}'
        terminal=b'\xc2\x04\x00' if va==0x2235AE30 else b'\xc3'
        assert pe.read(va+size-len(terminal),len(terminal))==terminal,f'terminal {label}'
        spans.append((va,va+size))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
    for off,func in VTABLE.items():
        assert struct.unpack('<I',pe.read(VTABLE_START+off,4))[0]==func,f'vtable+{off:#x}'
    for site,target in CALLS.items():edge(pe,site,target)
    for site,offset in VIRTUAL_CALLS.items():
        d=pe.read(site,6)
        assert d[:2] in (b'\xff\x90',b'\xff\x92') and struct.unpack('<I',d[2:])[0]==offset, f'vcall {site:#x}'
    for site,hx in WITNESSES.items():
        d=bytes.fromhex(hx)
        assert pe.read(site,len(d))==d,f'instruction witness {site:#x}'
    for va,s in STRINGS.items():
        assert pe.section(va)==b'.rdata'
        assert pe.read(va,len(s)+1)==s+b'\x00',f'original voice token at {va:#x}'
    for va,(fmt,value) in CONSTANTS.items():
        assert struct.unpack(fmt,pe.read(va,struct.calcsize(fmt)))[0]==value,f'float/double original {va:#x}'
    assert sum(size for size,_,_ in METHODS.values())==432
    return {'new_complete_methods':len(METHODS),'new_original_server_bytes':432,
            'vtable_slots':len(VTABLE),'native_direct_calls':len(CALLS),
            'native_virtual_call_sites':len(VIRTUAL_CALLS),
            'opcode_witnesses':len(WITNESSES),'original_tokens':len(STRINGS),
            'original_float_and_double_constants':len(CONSTANTS)}


def self_test():
    class Fake:
        def __init__(self,addr,data):self.addr,self.data=addr,data
        def read(self,addr,n):return self.data[addr-self.addr:addr-self.addr+n]
    for site,target in CALLS.items():
        original=b'\xe8'+struct.pack('<i',target-site-5)
        edge(Fake(site,original),site,target)
        for i in range(5):
            bad=bytearray(original);bad[i]^=1
            try:edge(Fake(site,bad),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'mutated E8 accepted {site:#x}')
    try:audit(b'MZnotowner',b'MZnotowner')
    except AssertionError:pass
    else:raise AssertionError('accepted unsupported original')
    return 'SELF_TEST_OK'


def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--self-test',action='store_true')
    a.add_argument('--bin-zip',type=Path)
    opts=a.parse_args()
    if not (opts.self_test or opts.bin_zip):a.error('choose --self-test and/or --bin-zip')
    if opts.self_test:print(self_test())
    if opts.bin_zip:
        with zipfile.ZipFile(opts.bin_zip) as z:
            result=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_GRUNT_REACTIONS_PHASE565_AUTHENTICATED',result)
if __name__=='__main__':main()