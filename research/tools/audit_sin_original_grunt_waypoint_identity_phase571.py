#!/usr/bin/env python3
"""Phase571 original SiN 2006 waypoint identity-splice, constructor auditor.

Authenticates owner-private original PE32 DLLs, complete native method hashes,
branch and field witnesses; never dumps game code to stdout or to repository.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32, CLIENT_HASH, SERVER_HASH, edge

NEW_METHODS = {
    0x220B29F0: (26, '9aded3ff164add9dd409b2ee9acd656d9598f55ff0ef903a75f5c31f61c3bc4c', 'tail from node'),
    0x220B2A40: (26, '921a3f9d537e074580ceb7905ad98a4711f0742875c1c8f2280ffc1cee25517c', 'tail from holder'),
    0x220B2A60: (70, '48efdcc32e024ed3b76d009f2114c365f1e3265acd06218af1f0ee272cf0da94', 'default 48-byte node constructor'),
    0x220B2AB0: (84, 'd6d353f6e1f91498e5f0b95be0bc672eaa2ba2383a685a4f82842ada02aa668c', 'five-argument 48-byte node constructor'),
}
PARENT_SPLICE = (0x220B2B10, 184,
 '572d5112f9b4963c0ff9c6869ecb770f3e47ac4e32befd11f3f17a6ea31f2c94')
# Calls originating in prior-phase functions. The one at 0x220777E8
# links nav +0x54 candidate generation to the waypoint constructor.
CROSS_EDGES = {0x220777E8: 0x220B2AB0, 0x220B2B60: 0x2226EF40}
WITNESSES = {
 0x220B29F0:'8bc1',0x220B29F2:'8b4828',0x220B2A00:'8bc1',0x220B2A09:'c3',
 0x220B2A40:'8b01',0x220B2A47:'8b4828',0x220B2A59:'c3',
 0x220B2A60:'8bd1',0x220B2A62:'c74218ffffffff',
 0x220B2A69:'c7421cffffffff',0x220B2A73:'b90c000000',0x220B2A7A:'f3ab',
 0x220B2A7C:'a1ecb15122',0x220B2A81:'8902',
 0x220B2A94:'c74210ffffffff',0x220B2A9B:'c74214000080bf',
 0x220B2AB0:'8bd1',0x220B2AB2:'83c8ff',0x220B2AC5:'f3ab',
 0x220B2AC7:'8b442408',0x220B2ADF:'8b442410',
 0x220B2AE3:'894224',0x220B2AED:'8b4c2414',
 0x220B2AF1:'894210',0x220B2AF4:'894a20',0x220B2AF7:'c74214000080bf',
 0x220B2B1B:'3918',0x220B2B2D:'836620f7',
 0x220B2B31:'83f8ff',0x220B2B39:'744e',0x220B2B3B:'3b4110',
 0x220B2B3E:'7549',0x220B2B40:'8b7928',0x220B2B47:'8b412c',
 0x220B2B4A:'89472c',0x220B2B54:'8b5128',0x220B2B57:'895028',
 0x220B2B65:'8b4e28',0x220B2B6F:'89592c',
 0x220B2B74:'8938',0x220B2B78:'8b7f2c',
 0x220B2B7F:'895f28',0x220B2B84:'89712c',
 0x220B2B89:'8b5628',0x220B2B96:'3bcb',
 0x220B2B9C:'8b492c',0x220B2BA3:'895928',
 0x220B2BA8:'89722c',0x220B2BBA:'83492008',
 0x220B2BC1:'834e2008',
}

def audit(client,server):
    assert hashlib.sha256(client).hexdigest() == CLIENT_HASH, 'incorrect original client.dll'
    assert hashlib.sha256(server).hexdigest() == SERVER_HASH, 'incorrect original server.dll'
    pe = PE32(server)
    assert pe.base == 0x22000000
    spans=[]
    for addr,(n,digest,name) in NEW_METHODS.items():
        assert pe.section(addr)==b'.text',name
        data=pe.read(addr,n)
        assert hashlib.sha256(data).hexdigest()==digest,name
        assert pe.read(addr+n,1)==b'\xcc',f'bad native end for {name}'
        if addr==0x220B2AB0:
            assert data[-3:]==b'\xc2\x14\x00',name
        else:
            assert data[-1:]==b'\xc3',name
        spans.append((addr,addr+n))
    spans.sort()
    assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])), 'overlap'
    addr,n,digest=PARENT_SPLICE
    assert hashlib.sha256(pe.read(addr,n)).hexdigest()==digest, 'Phase570 original splice changed'
    assert pe.read(addr+n,1)==b'\xcc'
    for src,dst in CROSS_EDGES.items():edge(pe,src,dst)
    for va,h in WITNESSES.items():
        b=bytes.fromhex(h)
        assert pe.read(va,len(b))==b,f'original instruction mismatch at {va:#x}'
    return dict(new_full_methods=len(NEW_METHODS), new_distinct_bytes=sum(t[0] for t in NEW_METHODS.values()),
                revisited_phase570_splice_bytes=n,original_call_edges=len(CROSS_EDGES),
                opcode_field_witnesses=len(WITNESSES))

def self_test():
    class PE:
        def __init__(self,at,data):self.at=at;self.data=data
        def read(self,va,n):return self.data[va-self.at:va-self.at+n]
    for src,dst in CROSS_EDGES.items():
        raw=b'\xe8'+struct.pack('<i',dst-src-5)
        edge(PE(src,raw),src,dst)
        for idx in range(5):
            changed=bytearray(raw);changed[idx]^=1
            try:edge(PE(src,changed),src,dst)
            except AssertionError:pass
            else:raise AssertionError('corrupted native E8 accepted')
    try:audit(b'wrong',b'wrong')
    except AssertionError:pass
    else:raise AssertionError('wrong DLL identity accepted')
    return 'PHASE571_SYNTHETIC_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    args=p.parse_args()
    if not(args.self_test or args.bin_zip):p.error('supply --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            print('PHASE571_ORIGINAL_VERIFIED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()