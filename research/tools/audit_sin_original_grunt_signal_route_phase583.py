#!/usr/bin/env python3
"""Read-only SHA audit of original SiN 2006 NPC action id mapping/routing.

These are genuine gamecode NPC signal dependencies of Grunt event 0x8D/0x91.
No proprietary machine code bytes or engine resources are emitted.
"""
import argparse, hashlib, struct, zipfile
from pathlib import Path
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS={
    0x22049A60:(229,'4ff325f5f5722b910e681e00b6b0466894baad8a1f92df3228a63b53b0e2f893'),
    0x22049B50:(202,'3dada046704eacb555a0aceff7ea9b3c067a0790835d83b00b90bad4059fb302'),
}
CALLS={
    0x22049A7A:0x22049870,0x22049A8D:0x2204FB20,
    0x22049A98:0x220EEBD0,0x22049AE9:0x220E6CE0,
    0x22049B2C:0x220EEBD0,
    0x22049B53:0x220EB8A0,0x22049BB2:0x22022A90,
    0x22049BBC:0x22022A90,0x22049BF6:0x22049A60,
    0x22049C0E:0x220417D0,
}
PRIOR_INBOUND={0x22049CCB:0x22049A60}
WITNESSES={
    0x22049A60:'8b442410',0x22049A74:'c707ffffffff',
    0x22049A7F:'83fb0f',0x22049A86:'894500',
    0x22049A9D:'83f8ff',0x22049AA0:'8907',
    0x22049AA8:'393524815422',
    0x22049ABB:'8b1590215722',0x22049AC4:'d8251c815422',
    0x22049AD0:'dfe0',0x22049AD2:'f6c441',
    0x22049B1B:'837d000a',0x22049B21:'6a06',
    0x22049B25:'c7450006000000',
    0x22049B33:'833fff',0x22049B38:'c70700000000',
    0x22049B42:'c21000',
    0x22049B50:'56',0x22049B5A:'0f84b6000000',
    0x22049B6B:'3bd8',0x22049B77:'83fb02',
    0x22049B7C:'81be800800009a000000',
    0x22049BA3:'8b4e0c',
    0x22049BD8:'8dbe8c080000',0x22049BDF:'8d9e88080000',
    0x22049BE6:'8dae84080000',
    0x22049BF0:'898680080000',
    0x22049C02:'52',0x22049C0E:'e8bd7bffff',
    0x22049C17:'c20400',
}
class PE:
    def __init__(self,data):
        assert data[:2]==b'MZ'
        pe=struct.unpack_from('<I',data,0x3C)[0]
        assert data[pe:pe+4]==b'PE\x00\x00'
        ns=struct.unpack_from('<H',data,pe+6)[0]
        opt=pe+24
        assert struct.unpack_from('<H',data,opt)[0]==0x10B
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        sec=opt+struct.unpack_from('<H',data,pe+20)[0]
        self.sections=[]
        for i in range(ns):
            x=sec+i*40;name=data[x:x+8].split(b'\0')[0]
            vsz,va,rsz,roff=struct.unpack_from('<4I',data,x+8)
            self.sections.append((name,self.base+va,self.base+va+min(vsz,rsz),roff))
        self.data=data
    def read(self,addr,n):
        for name,lo,hi,roff in self.sections:
            if lo<=addr and addr+n<=hi:
                return self.data[roff+addr-lo:roff+addr-lo+n]
        raise AssertionError(f'Unmapped native VA {addr:#x}')
    def section(self,addr):
        for name,lo,hi,roff in self.sections:
            if lo<=addr<hi:return name
        raise AssertionError(f'Unmapped section {addr:#x}')
def direct(pe,site,target):
    b=pe.read(site,5)
    assert b[0]==0xE8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'call drift at {site:#x}'
def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'original client identity mismatch'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'original server identity mismatch'
    pe=PE(server);assert pe.base==0x22000000
    for va,(n,sha) in METHODS.items():
        assert pe.section(va)==b'.text'
        assert hashlib.sha256(pe.read(va,n)).hexdigest()==sha,f'original method {va:#x} modified'
        assert pe.read(va+n,1)==b'\xCC',f'missing original INT3 {va:#x}'
    for site,tgt in {**CALLS,**PRIOR_INBOUND}.items():direct(pe,site,tgt)
    for va,hx in WITNESSES.items():
        b=bytes.fromhex(hx);assert pe.read(va,len(b))==b,f'opcode witness {va:#x}'
    return {'new_complete_original_methods':len(METHODS),'new_original_bytes':sum(n for n,_ in METHODS.values()),
            'original_e8_calls':len(CALLS),'prior_incoming_calls':len(PRIOR_INBOUND),'opcode_witnesses':len(WITNESSES)}
def self_test():
    class Fake:
        def __init__(self,site,b):self.site,self.b=site,b
        def read(self,addr,n):return self.b[addr-self.site:addr-self.site+n]
    for site,tgt in {**CALLS,**PRIOR_INBOUND}.items():
        valid=b'\xE8'+struct.pack('<i',tgt-site-5)
        direct(Fake(site,valid),site,tgt)
        for j in range(5):
            bad=bytearray(valid);bad[j]^=1
            try:direct(Fake(site,bad),site,tgt)
            except AssertionError:pass
            else:raise AssertionError(f'corrupt rel32 accepted {site:#x}:{j}')
    try:audit(b'fake',b'fake')
    except AssertionError:pass
    else:raise AssertionError('accepted unauthenticated retail originals')
    return 'SELF_TEST_OK'
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    args=p.parse_args()
    if not (args.self_test or args.bin_zip):p.error('specify --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_GRUNT_SIGNAL_ROUTE_PHASE583_AUTHENTICATED',r)
if __name__=='__main__':main()