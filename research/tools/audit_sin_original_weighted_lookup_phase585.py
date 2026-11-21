#!/usr/bin/env python3
"""Original 2006 SiN server.dll weighted registration lookup, read-only SHA audit."""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS={0x220EB1C0:(241,'c96a31109689d9493ca04cbd1e4d9dcceedcca2bd9bc65660b3a3b0e1ebbbec1')}
CALLS={
    0x220eb1d1:0x2211c110, 0x220eb1df:0x220eaff0,
    0x220eb1f7:0x2211b7d0, 0x220eb20a:0x2211bd00,
    0x220eb219:0x220eaf70, 0x220eb268:0x2228bc00,
    0x220eb29a:0x2211b7d0,
}
INBOUND={0x220eebfd:0x220eb1c0,0x220eec1d:0x220eb1c0}
WITNESSES={
    0x220eb1c0:'51',0x220eb1c6:'85ff',0x220eb1ca:'33c0',
    0x220eb1d1:'e83a0f0300',0x220eb1d6:'3b0554884e22',
    0x220eb1df:'e80cfeffff',0x220eb1ed:'c744240cffffff',
    0x220eb1f7:'e8d4050300',0x220eb1fc:'85c0',
    0x220eb20a:'e8f10a0300',0x220eb211:'f6460d10',
    0x220eb217:'53',0x220eb219:'e852fdffff',
    0x220eb221:'8b44241c',0x220eb225:'394610',
    0x220eb22d:'395c2420',0x220eb233:'85ff',
    0x220eb237:'895c2410',0x220eb249:'8bc7',
    0x220eb24d:'8bc7',0x220eb251:'03e8',
    0x220eb253:'833d6c87552200',0x220eb263:'68a0e23f22',
    0x220eb268:'e893091a00',0x220eb278:'ff15ac213d22',
    0x220eb281:'85ed',0x220eb285:'85ff',
    0x220eb28b:'3bc7',0x220eb28f:'895c2410',
    0x220eb293:'8b4c2418',0x220eb29a:'e831050300',
    0x220eb2a1:'0f8c5cffffff',0x220eb2a7:'8b442410',0x220eb2b0:'c3',
}
class PE:
    def __init__(self,data):
        assert data[:2]==b'MZ'; p=struct.unpack_from('<I',data,0x3c)[0]
        assert data[p:p+4]==b'PE\x00\x00'; n=struct.unpack_from('<H',data,p+6)[0]
        opt=p+24; assert struct.unpack_from('<H',data,opt)[0]==0x10b
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        sec=opt+struct.unpack_from('<H',data,p+20)[0]
        self.s=[]
        for j in range(n):
            x=sec+j*40;name=data[x:x+8].split(b'\x00')[0]
            vsz,va,rsz,roff=struct.unpack_from('<4I',data,x+8)
            self.s.append((name,self.base+va,self.base+va+min(vsz,rsz),roff))
        self.data=data
    def read(self,addr,n):
        for name,lo,hi,roff in self.s:
            if lo<=addr and addr+n<=hi:return self.data[roff+addr-lo:roff+addr-lo+n]
        raise AssertionError(f'not mapped original image address {addr:#x}')
    def section(self,addr):
        for name,lo,hi,_ in self.s:
            if lo<=addr<hi:return name
        raise AssertionError('unmapped')
def direct(pe,site,target):
    b=pe.read(site,5)
    assert b[0]==0xE8 and site+5+struct.unpack('<i',b[1:])[0]==target, f'original call mismatch {site:#x}'
def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client DLL not owner original'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'server DLL not owner original'
    pe=PE(server);assert pe.base==0x22000000
    for va,(n,digest) in METHODS.items():
        assert pe.section(va)==b'.text'
        assert hashlib.sha256(pe.read(va,n)).hexdigest()==digest
        assert pe.read(va+n,1)==b'\xcc'
    for site,dest in {**CALLS,**INBOUND}.items():direct(pe,site,dest)
    for va,hx in WITNESSES.items():assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'witness drift {va:#x}'
    return dict(complete_native_methods=1,native_bytes=241,original_e8_calls=len(CALLS),phase584_crosscalls=len(INBOUND),opcode_witnesses=len(WITNESSES))
def self_test():
    class Fake:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,site,n):return self.data[site-self.site:site-self.site+n]
    for site,target in {**CALLS,**INBOUND}.items():
        good=b'\xe8'+struct.pack('<i',target-site-5)
        direct(Fake(site,good),site,target)
        for j in range(5):
            bad=bytearray(good);bad[j]^=1
            try:direct(Fake(site,bad),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'corruption accepted {site:#x}:{j}')
    try:audit(b'bad',b'bad')
    except AssertionError:pass
    else:raise AssertionError('bad binaries accepted')
    return 'PHASE585_SYNTHETIC_MUTATION_OK'
def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--self-test',action='store_true');a.add_argument('--bin-zip',type=Path)
    opt=a.parse_args()
    if not opt.self_test and not opt.bin_zip:a.error('choose --self-test or --bin-zip')
    if opt.self_test:print(self_test())
    if opt.bin_zip:
        with zipfile.ZipFile(opt.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_ORIGINAL_WEIGHTED_LOOKUP_PHASE585',r)
if __name__=='__main__':main()