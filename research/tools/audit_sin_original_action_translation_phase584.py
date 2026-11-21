#!/usr/bin/env python3
"""Read-only native SHA auditor: original 2006 SiN Grunt action-resolution chain.

Original PE bytes are inspected privately and never printed or included in Git.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
METHODS = {
    0x22049870: (483,'1fd33bf43c063f042614cebee270dc29990aea8cda358ad39e461b72fbe1d315'),
    0x220EEBD0: (90,'313f85010f1a33727341d3896097c366180f6d1340c81f91baf78989ba7ab0c5'),
    0x220417D0: (359,'aa4f92800ea9cb148416f4cb5f135ce1d4acd342b216f178d4f324c67fff5487'),
}
CALLS = {
    0x220498fa:0x220eb8a0, 0x22049906:0x220eebd0,
    0x22049963:0x223a5639, 0x22049975:0x22042940,
    0x22049980:0x222aa640, 0x220499a1:0x22022a90,
    0x220499ca:0x22042ab0, 0x220499da:0x2202e600,
    0x220499ee:0x2202e600, 0x22049a10:0x2202e600,
    0x220eebd8:0x22139a10, 0x220eebfd:0x220eb1c0,
    0x220eec1d:0x220eb1c0,
    0x2204181b:0x22022a90, 0x22041829:0x22022a90,
    0x22041839:0x220ecc90, 0x22041844:0x22022a90,
    0x22041850:0x220ecc90, 0x22041857:0x22022a90,
    0x220418b7:0x220405c0, 0x220418bf:0x220f2300,
    0x220418c7:0x220eede0, 0x220418d7:0x222a5470,
    0x220418e2:0x220f2300, 0x22041900:0x22140bd0,
}
PRIOR_INBOUND = {
    0x22049A7A:0x22049870, 0x22049A98:0x220EEBD0,
    0x22049C0E:0x220417D0,
}
WITNESSES = {
    0x220417d0:'8b44240c', 0x220417da:'89867c080000', 0x220417e6:'83792c00',
    0x2204187a:'39beec030000', 0x22041896:'ff90e0040000', 0x220418a9:'ff92e0040000',
    0x220418b7:'e804edffff', 0x220418bf:'e83c0a0b00', 0x220418c7:'e814d50a00',
    0x220418d7:'e8943b2600', 0x220418e2:'e8190a0b00', 0x220418f7:'ff9230060000',
    0x22041905:'8b7c2418', 0x22041916:'ff92e4040000', 0x2204191c:'89be78080000',
    0x22041922:'8bb608090000', 0x2204192c:'ff504c', 0x22041934:'c21000',
    0x22049870:'83ec0c', 0x22049891:'ff903c030000', 0x220498b8:'ff9238030000',
    0x220498d0:'ff923c030000', 0x220498e0:'83fb05', 0x220498e7:'83ff0f',
    0x220498fa:'e8a11f0a00', 0x22049906:'e8c5520a00', 0x22049923:'807c241200',
    0x220499d3:'3bfd', 0x220499e3:'8b5c2418', 0x220499f7:'8b442420',
    0x22049a05:'8b7c2420', 0x22049a19:'83ff0a', 0x22049a36:'83ff06',
    0x22049a50:'c20800', 0x220eebd2:'8bb9ec030000', 0x220eebd8:'e833ae0400',
    0x220eebef:'83f803', 0x220eebf4:'8b4c240c', 0x220eec02:'83c40c',
    0x220eec13:'ff5228', 0x220eec22:'83c40c', 0x220eec27:'c20400',
}

class OriginalPE:
    def __init__(self,data):
        assert data[:2]==b'MZ','not original MZ image'
        hdr=struct.unpack_from('<I',data,0x3c)[0]
        assert data[hdr:hdr+4]==b'PE\0\0','not a PE image'
        sections=struct.unpack_from('<H',data,hdr+6)[0]
        opt=hdr+24
        assert struct.unpack_from('<H',data,opt)[0]==0x10b,'expected PE32'
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        first=opt+struct.unpack_from('<H',data,hdr+20)[0]
        self.sections=[]
        for i in range(sections):
            j=first+i*40
            name=data[j:j+8].rstrip(b'\0')
            vsz,va,rsz,offset=struct.unpack_from('<4I',data,j+8)
            self.sections.append((name,self.base+va,self.base+va+min(vsz,rsz),offset))
        self.data=data
    def read(self,addr,n):
        for name,lo,hi,offset in self.sections:
            if lo<=addr and addr+n<=hi:
                return self.data[offset+addr-lo:offset+addr-lo+n]
        raise AssertionError(f'not original mapped PE RVA: {addr:#x}')
    def section(self,addr):
        for name,lo,hi,_ in self.sections:
            if lo<=addr<hi:return name
        raise AssertionError('outside PE')

def direct(pe,source,dest):
    raw=pe.read(source,5)
    assert raw[0]==0xE8 and source+5+struct.unpack('<i',raw[1:])[0]==dest, f'original E8 drift {source:#x}'

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client.dll SHA does not match owner original'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'server.dll SHA does not match owner original'
    pe=OriginalPE(server)
    assert pe.base==0x22000000
    for va,(n,digest) in METHODS.items():
        assert pe.section(va)==b'.text'
        assert hashlib.sha256(pe.read(va,n)).hexdigest()==digest, f'function digest drift {va:#x}'
        assert pe.read(va+n,1)==b'\xCC',f'original method boundary drift {va:#x}'
    for site,tgt in {**CALLS,**PRIOR_INBOUND}.items(): direct(pe,site,tgt)
    for va,hx in WITNESSES.items():
        assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'native opcode drift {va:#x}'
    return {'complete_native_methods':len(METHODS),'distinct_native_bytes':sum(x[0] for x in METHODS.values()),
            'direct_e8_edges':len(CALLS),'incoming_prior_edges':len(PRIOR_INBOUND),
            'opcode_witnesses':len(WITNESSES)}

def self_test():
    class Mock:
        def __init__(self,addr,raw):self.addr=addr;self.raw=raw
        def read(self,addr,n):return self.raw[addr-self.addr:addr-self.addr+n]
    for site,dest in {**CALLS,**PRIOR_INBOUND}.items():
        raw=b'\xe8'+struct.pack('<i',dest-site-5)
        direct(Mock(site,raw),site,dest)
        for j in range(5):
            bad=bytearray(raw);bad[j]^=1
            try:direct(Mock(site,bad),site,dest)
            except AssertionError:pass
            else:raise AssertionError(f'mutation not detected at {site:#x}+{j}')
    try:audit(b'fake',b'fake')
    except AssertionError:pass
    else:raise AssertionError('accepted non-original DLL')
    return 'PHASE584_SYNTHETIC_MUTATION_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if not args.self_test and not args.bin_zip:p.error('pass --self-test or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            d=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_ORIGINAL_ACTION_TRANSLATION_PHASE584',d)
if __name__=='__main__':main()