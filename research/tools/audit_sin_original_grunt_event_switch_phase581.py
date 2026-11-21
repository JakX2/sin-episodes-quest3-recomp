#!/usr/bin/env python3
"""Audit entire authenticated original Grunt-specific event-switch method.

Authenticates a complete 2006 PE32 method extent (which includes 2 data
jump-tables), plus actual event IDs, 44 direct CALL edges and original caller
links. Only checks retail binaries locally; it never outputs their code bytes.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE=0x22000000
METHOD=0x22351C20
LENGTH=1330
SHA='770d75ba487fb923eddd5f20bf200d72ac7dd3b66085d9aab007366c1838363d'
PTR_TABLE=0x223520B0
IDX_TABLE=0x223520D8
TARGETS={
  0:0x22351E37, 1:0x22351DDC, 2:0x22351C47, 3:0x22351F3D,
  4:0x22351FFE, 5:0x22351C82, 6:0x22352080, 7:0x22351D7F,
  8:0x223520A5, 9:0x2235209D,
}
EXCEPTIONS={
  0x1B:0, 0x4A:1, 0x8C:2, 0x8D:3, 0x8F:4,
  0x90:5, 0x91:6, 0x92:7, 0x94:8,
}
CALLS={
  0x22351C67:0x22350D30, 0x22351C75:0x22049C20,
  0x22351CA0:0x220092D0, 0x22351CBB:0x220092D0,
  0x22351CD3:0x220092D0, 0x22351CDA:0x22027DB0,
  0x22351D24:0x22012E50, 0x22351D31:0x220092D0,
  0x22351D38:0x2200DEB0, 0x22351D56:0x2206C670,
  0x22351D5F:0x2203D2D0, 0x22351D86:0x2214C730,
  0x22351D8F:0x22032220, 0x22351D98:0x220303B0,
  0x22351DA1:0x22030390, 0x22351DAC:0x22143610,
  0x22351DDE:0x220330F0, 0x22351DED:0x22039130,
  0x22351E04:0x22065DF0, 0x22351E0F:0x2204F680,
  0x22351E16:0x22039130, 0x22351E39:0x220330F0,
  0x22351E4E:0x220330F0, 0x22351E5B:0x22350F10,
  0x22351E64:0x220330F0, 0x22351E7D:0x2203D2D0,
  0x22351E86:0x2204F680, 0x22351E93:0x22039130,
  0x22351EA4:0x22039130, 0x22351EAB:0x220641F0,
  0x22351EB4:0x22032220, 0x22351EBD:0x22030390,
  0x22351EDB:0x2204C660, 0x22351EFA:0x2204F680,
  0x22351F03:0x220303B0, 0x22351F0A:0x22039130,
  0x22351F1E:0x2203D2D0, 0x22351F5E:0x22028200,
  0x22351FBE:0x22141A00, 0x22351FC7:0x22049C20,
  0x22352050:0x2200DEB0, 0x22352073:0x2203D2D0,
  0x22352084:0x22049C20, 0x223520A0:0x22050150,
}
WITNESSES={
  0x22351C20:'83ec6c',0x22351C26:'8b4c2474',
  0x22351C2C:'83c0e5',0x22351C2F:'83f879',
  0x22351C33:'0f8764040000',
  0x22351C39:'0fb680d8203522',
  0x22351C40:'ff2485b0203522',
  0x22351C47:'8b0db0215722',
  0x22351C4F:'6a04',0x22351C51:'6a01',
  0x22351C53:'ff5208',0x22351C56:'83f801',
  0x22351C59:'750a',0x22351C5F:'ff9090050000',
  0x22351C65:'8bce',
  0x22351C82:'8b16',0x22351C86:'ff9254010000',
  0x22351D7F:'6800040000',
  0x22351D94:'6a04',0x22351D9D:'6a01',
  0x22351FC3:'6a19',
  0x22352080:'6a1e',
  0x22352089:'8b8e68030000',
  0x22352090:'898e300c0000',
  0x2235209D:'51',0x223520A5:'5f',
  0x223520AD:'8d4900',
}
# Native PE32 code references in original retail .rdata; exact class is opaque.
REFERENCES=(0x2248B1F8,0x2248B990,0x2248C128)

class PE32:
    def __init__(self,data):
        assert data[:2]==b'MZ'
        off=struct.unpack_from('<I',data,0x3c)[0]
        assert data[off:off+4]==b'PE\0\0'
        n=struct.unpack_from('<H',data,off+6)[0]
        o=off+24
        assert struct.unpack_from('<H',data,o)[0]==0x10b
        self.base=struct.unpack_from('<I',data,o+28)[0]
        sh=o+struct.unpack_from('<H',data,off+20)[0]
        self.sections=[]
        for i in range(n):
            so=sh+i*40
            name=data[so:so+8].split(b'\0')[0]
            virt_size,virt_addr,raw_size,raw_pos=struct.unpack_from('<IIII',data,so+8)
            self.sections.append((name,self.base+virt_addr,self.base+virt_addr+min(virt_size,raw_size),raw_pos))
        self.data=data
    def section(self,va):
        for name,start,end,raw in self.sections:
            if start<=va<end:return name
        raise AssertionError(f'unmapped original VA {va:#x}')
    def read(self,va,size):
        for name,start,end,raw in self.sections:
            if start<=va and va+size<=end:
                return self.data[raw+va-start:raw+va-start+size]
        raise AssertionError(f'cross-section read at {va:#x}+{size}')

def direct(pe,site,target):
    b=pe.read(site,5)
    assert b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target,f'call {site:#x}'

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'wrong original client.dll'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'wrong original server.dll'
    pe=PE32(server)
    assert pe.base==BASE
    assert pe.section(METHOD)==b'.text'
    assert hashlib.sha256(pe.read(METHOD,LENGTH)).hexdigest()==SHA,'native handler body altered'
    assert pe.read(METHOD+LENGTH,1)==b'\xcc','method boundary not authentic'
    assert pe.section(PTR_TABLE)==b'.text' and pe.section(IDX_TABLE)==b'.text'
    for i,target in TARGETS.items():
        assert struct.unpack('<I',pe.read(PTR_TABLE+i*4,4))[0]==target,f'handler pointer {i}'
    assert PTR_TABLE==0x223520B0 and IDX_TABLE==PTR_TABLE+40
    assert IDX_TABLE+122==METHOD+LENGTH
    observed={}
    for idx in range(122):
        event_id=0x1B+idx
        code=pe.read(IDX_TABLE+idx,1)[0]
        assert code in TARGETS,f'unsupported switch code {code}'
        if code != 9:observed[event_id]=code
    assert observed==EXCEPTIONS,f'Grunt event IDs drift: {observed}'
    assert sum(pe.read(IDX_TABLE+i,1)==b'\x09' for i in range(122))==113
    for site,target in CALLS.items():direct(pe,site,target)
    for site,hx in WITNESSES.items():
        b=bytes.fromhex(hx)
        assert pe.read(site,len(b))==b,f'opcode witness {site:#x}'
    for ref in REFERENCES:
        assert pe.section(ref)==b'.rdata'
        assert struct.unpack('<I',pe.read(ref,4))[0]==METHOD
    # Indirect original go-alert virtual call, used by 0x8C conditional arm.
    assert pe.read(0x22351C5F,6)==b'\xff\x90\x90\x05\x00\x00'
    return {'new_complete_methods':1,'authenticated_original_method_bytes':LENGTH,
            'original_direct_calls':len(CALLS),'original_case_branches':len(TARGETS),
            'specific_grunt_event_ids':len(EXCEPTIONS),'fallback_cases':113,
            'instruction_witnesses':len(WITNESSES),'rdata_code_references':len(REFERENCES)}

def self_test():
    class Fake:
        def __init__(self,at,data):self.at,self.data=at,data
        def read(self,addr,n):return self.data[addr-self.at:addr-self.at+n]
    for site,target in CALLS.items():
        original=b'\xe8'+struct.pack('<i',target-site-5)
        direct(Fake(site,original),site,target)
        for j in range(5):
            bad=bytearray(original);bad[j]^=1
            try:direct(Fake(site,bad),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'altered call accepted {site:#x}:{j}')
    try:audit(b'not owner',b'not owner')
    except AssertionError:pass
    else:raise AssertionError('accepted invalid owner original pair')
    return 'SELF_TEST_OK'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    a=p.parse_args()
    if not(a.bin_zip or a.self_test):p.error('provide --bin-zip and/or --self-test')
    if a.self_test:print(self_test())
    if a.bin_zip:
        with zipfile.ZipFile(a.bin_zip) as z:result=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_GRUNT_EVENT_SWITCH_PHASE581_AUTHENTICATED',result)
if __name__=='__main__':main()