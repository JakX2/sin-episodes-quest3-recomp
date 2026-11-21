#!/usr/bin/env python3
"""Read-only original 2006 SiN Grunt-event signal and angle-normalization audit.

The event 0x8D/0x92 branch bytes are a subrange of the already authenticated
Phase581 method, and are never miscounted as newly recovered code bytes.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE = 0x22000000
METHODS = {
    0x22049C20: (181,'1662be502c6a43a12394b61206a00c54178d6e15e29e415defb8fabe6f55f524'),
    0x22028200: (60, '0804a4997f4b278c9b17c985396aeb16edcefaead374be03c3d72c0e421842e0'),
}
CALLS={
    0x22049C5C:0x22022A90,
    0x22049C66:0x22022A90,
    0x22049CA4:0x220EB8A0,
    0x22049CCB:0x22049A60,
}
INBOUND={
    0x22351C75:0x22049C20,
    0x22351FC7:0x22049C20,
    0x22352084:0x22049C20,
    0x22351F5E:0x22028200,
}
BRANCH_CALLS={
    0x22351D86:0x2214C730,
    0x22351D8F:0x22032220,
    0x22351D98:0x220303B0,
    0x22351DA1:0x22030390,
    0x22351DAC:0x22143610,
    0x22351FBE:0x22141A00,
}
WITNESS={
    0x22049C20:'5657',
    0x22049C26:'83ff02',
    0x22049C2B:'0f849f000000',
    0x22049C31:'a190d05422',
    0x22049C3C:'f686a900000010',
    0x22049C80:'85ff',
    0x22049C84:'8b16',
    0x22049C89:'ff92dc040000',
    0x22049C94:'81ff9a000000',
    0x22049C9A:'89be80080000',
    0x22049CAD:'8d868c080000',
    0x22049CD2:'c20400',
    0x22028200:'51d9442408',
    0x22028205:'d80d0c8e3d22',
    0x2202820B:'d97c2408',
    0x22028214:'80cc0c',
    0x2202821A:'d92c24',
    0x2202821D:'db1c24',
    0x22028223:'25ffff0000',
    0x22028228:'d96c2408',
    0x22028234:'d80d088e3d22',
    0x2202823B:'c3',
    0x22351D7F:'6800040000',
    0x22351D94:'6a04',
    0x22351D9D:'6a01',
    0x22351DB9:'6800002041',
    0x22351DBE:'680000a040',
    0x22351DCF:'d99e200c0000',
    0x22351F45:'68b4000000',
    0x22351F4A:'684cffffff',
    0x22351F72:'680000003f',
    0x22351F77:'68cdcccc3d',
    0x22351FC3:'6a19',
    0x22351FCC:'c6863c0c000000',
    0x22351FDB:'680000a040',
    0x22351FE0:'6800000040',
    0x22351FF1:'d99e380c0000',
}
CONSTS={
    0x223D8E0C:182.04444885253906,
    0x223D8E08:0.0054931640625,
    0x223D48EC:0.01745329238474369,
    0x2244579C:220.0,
}

class PE32:
    def __init__(self,data):
        assert data[:2]==b'MZ'
        at=struct.unpack_from('<I',data,0x3C)[0]
        assert data[at:at+4]==b'PE\0\0'
        n=struct.unpack_from('<H',data,at+6)[0]
        off=at+24
        assert struct.unpack_from('<H',data,off)[0]==0x10B
        self.base=struct.unpack_from('<I',data,off+28)[0]
        self.sections=[]
        section_off=off+struct.unpack_from('<H',data,at+20)[0]
        for i in range(n):
            s=section_off+40*i
            name=data[s:s+8].split(b'\0')[0]
            vsize,vaddr,rsize,rptr=struct.unpack_from('<IIII',data,s+8)
            self.sections.append((name,self.base+vaddr,self.base+vaddr+min(vsize,rsize),rptr))
        self.data=data
    def read(self,va,n):
        for name,lo,hi,raw in self.sections:
            if lo<=va and va+n<=hi:
                return self.data[raw+va-lo:raw+va-lo+n]
        raise AssertionError(f'unmapped original VA {va:#x}+{n}')
    def section(self,va):
        for name,lo,hi,_ in self.sections:
            if lo<=va<hi:return name
        raise AssertionError(f'unmapped section {va:#x}')

def check_call(pe,site,target):
    b=pe.read(site,5)
    assert b[0]==0xE8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'original E8 {site:#x} wrong'

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client original identity mismatch'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'server original identity mismatch'
    pe=PE32(server)
    assert pe.base==BASE
    for va,(length,sha) in METHODS.items():
        assert pe.section(va)==b'.text'
        assert hashlib.sha256(pe.read(va,length)).hexdigest()==sha,f'wrong original method body {va:#x}'
        assert pe.read(va+length,1)==b'\xCC',f'original method boundary {va:#x}'
    for site,dest in {**CALLS,**INBOUND,**BRANCH_CALLS}.items():check_call(pe,site,dest)
    for va,opcode in WITNESS.items():
        b=bytes.fromhex(opcode)
        assert pe.read(va,len(b))==b,f'original opcode {va:#x}'
    for va,value in CONSTS.items():
        assert pe.section(va)==b'.rdata'
        assert struct.unpack('<f',pe.read(va,4))[0]==value,f'original constant {va:#x}'
    # Prior Phase581 already pins the containing event handler, including these branches.
    expected='770d75ba487fb923eddd5f20bf200d72ac7dd3b66085d9aab007366c1838363d'
    assert hashlib.sha256(pe.read(0x22351C20,1330)).hexdigest()==expected
    return {'new_complete_native_methods':len(METHODS),
            'new_original_function_bytes':sum(n for n,_ in METHODS.values()),
            'new_native_e8_calls':len(CALLS),'incoming_event_e8_calls':len(INBOUND),
            'prior_handler_branch_calls':len(BRANCH_CALLS),
            'original_opcode_witnesses':len(WITNESS),'original_float_constants':len(CONSTS),
            'prior_phase581_handler_sha_still_verified':True}

def self_test():
    class Dummy:
        def __init__(self,va,b):self.va,self.b=va,b
        def read(self,va,n):return self.b[va-self.va:va-self.va+n]
    for site,target in {**CALLS,**INBOUND,**BRANCH_CALLS}.items():
        valid=b'\xE8'+struct.pack('<i',target-site-5)
        check_call(Dummy(site,valid),site,target)
        for i in range(5):
            changed=bytearray(valid);changed[i]^=0x1
            try:check_call(Dummy(site,changed),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'corrupt original rel32 accepted {site:#x}+{i}')
    try:audit(b'fake',b'fake')
    except AssertionError:pass
    else:raise AssertionError('fake original pair accepted')
    return 'SELF_TEST_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    opts=p.parse_args()
    if not(opts.self_test or opts.bin_zip):p.error('specify --self-test and/or --bin-zip')
    if opts.self_test:print(self_test())
    if opts.bin_zip:
        with zipfile.ZipFile(opts.bin_zip) as f:
            result=audit(f.read('client.dll'),f.read('server.dll'))
        print('SIN_GRUNT_EVENT_SIGNAL_PHASE582_AUTHENTICATED',result)
if __name__=='__main__':main()