#!/usr/bin/env python3
"""Read-only native provenance of SiN 2006 action-registration refresh/access."""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS={
  0x2211b7d0: (25,'f24d61f7f37b4272c44c76b97b7ad4fd467d6ecdb46ff70d659c92e6e97edff3'),
  0x2211bd00: (153,'5535f05bdbc9f26f0c70793c170c2e32c75108aed1f560cc2b572b99d70a52eb'),
  0x2211c110: (97,'bf68f439d4aa80d59b785cf34f2f999de12b1273118b0e369d994baf33430c2a'),
  0x220eaf70: (117,'1b87d88fa667991d994cdd5ef5d077371bc095489c5a7bdc00097445830edf94'),
  0x220eaff0: (80,'57ae6d9372fd9b88c412fa6fe318bcfd23d0f8239282536f9674ca7466e96db9'),
}
CALLS={
  0x2211b7e0: 0x2211d460,
  0x2211bd3e: 0x2211d460,
  0x2211bd5b: 0x2211d470,
  0x2211c129: 0x2211d460,
  0x2211c14b: 0x2211d470,
  0x220eaf7e: 0x2211bd00,
  0x220eaf93: 0x2211b7d0,
  0x220eaf9f: 0x2211bd00,
  0x220eafaf: 0x220e6cb0,
  0x220eafbd: 0x220e6fe0,
  0x220eaffe: 0x2211b7d0,
  0x220eb009: 0x220eaf70,
  0x220eb014: 0x2211bd00,
  0x220eb01a: 0x220ea730,
  0x220eb027: 0x2211b7d0,
  0x220eb038: 0x2211c180,
}
INBOUND={
  0x220eb1d1: 0x2211c110,
  0x220eb1df: 0x220eaff0,
  0x220eb1f7: 0x2211b7d0,
  0x220eb20a: 0x2211bd00,
  0x220eb219: 0x220eaf70,
  0x220eb29a: 0x2211b7d0,
}
WITNESSES={
  0x220eaf70: '53',
  0x220eaf7e: 'e87d0d0300',
  0x220eaf85: '804b0d10',
  0x220eaf93: 'e838080300',
  0x220eaf9f: 'e85c0d0300',
  0x220eafa4: '8b7008',
  0x220eafa7: '03f0',
  0x220eafae: '56',
  0x220eafaf: 'e8fcbcffff',
  0x220eafbd: 'e81ec0ffff',
  0x220eafc5: '894310',
  0x220eafde: 'bedc9d3d22',
  0x220eaff0: '57',
  0x220eaff5: '85ff',
  0x220eaffe: 'e8cd070300',
  0x220eb009: 'e862ffffff',
  0x220eb014: 'e8e70c0300',
  0x220eb01a: 'e811f7ffff',
  0x220eb027: 'e8a4070300',
  0x220eb030: 'a154884e22',
  0x220eb038: 'e843110300',
  0x220eb03f: 'c3',
  0x2211b7d0: '83b95001000000',
  0x2211b7d9: '8b81bc000000',
  0x2211b7e0: 'e87b1c0000',
  0x2211b7e5: '8b400c',
  0x2211bd00: '83b95001000000',
  0x2211bd09: '8b442404',
  0x2211bd11: '3b81bc000000',
  0x2211bd1b: '8d14c500000000',
  0x2211bd2c: '2bd0',
  0x2211bd2e: '8b81c0000000',
  0x2211bd34: '8d0490',
  0x2211bd3c: '56',
  0x2211bd3e: 'e81d170000',
  0x2211bd4b: 'c1e604',
  0x2211bd58: '034f50',
  0x2211bd5b: 'e810170000',
  0x2211bd66: '85f6',
  0x2211bd76: '8b91c0000000',
  0x2211bd8f: '8d0482',
  0x2211bd96: 'c20400',
  0x2211c110: '57',
  0x2211c11c: '8b87c4000000',
  0x2211c129: 'e832130000',
  0x2211c12e: '8bafc4000000',
  0x2211c13b: '395e5c',
  0x2211c14b: 'e820130000',
  0x2211c150: '8b80c4000000',
  0x2211c15a: '8be8',
  0x2211c165: '3b5e5c',
  0x2211c170: 'c3',
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
        if va==0x220eaff0:
            assert pe.read(va+n,5)==bytes.fromhex('568b742408'), 'original unpadded next method boundary'
        else:
            assert pe.read(va+n,1)==b'\xcc', 'original INT3 boundary'
    for site,dest in {**CALLS,**INBOUND}.items():direct(pe,site,dest)
    for va,hx in WITNESSES.items():assert pe.read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'witness drift {va:#x}'
    return dict(complete_native_methods=len(METHODS),native_bytes=sum(x[0] for x in METHODS.values()),original_e8_calls=len(CALLS),phase585_crosscalls=len(INBOUND),opcode_witnesses=len(WITNESSES))
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
    return 'PHASE586_SYNTHETIC_MUTATION_OK'
def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--self-test',action='store_true');a.add_argument('--bin-zip',type=Path)
    opt=a.parse_args()
    if not opt.self_test and not opt.bin_zip:a.error('choose --self-test or --bin-zip')
    if opt.self_test:print(self_test())
    if opt.bin_zip:
        with zipfile.ZipFile(opt.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_ORIGINAL_REGISTRY_REFRESH_PHASE586',r)
if __name__=='__main__':main()