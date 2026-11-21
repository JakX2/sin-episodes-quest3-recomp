#!/usr/bin/env python3
"""Read-only SHA authentication of original 2006 SiN Grunt event dispatch/voice.

Does not publish or execute any part of the commercial DLL. Independent synthetic
mutation tests run with no original file and reject altered direct CALL edges.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE = 0x22000000
VTABLE = 0x2249AAA4
TABLE = 0x22052D14
METHODS = {
    0x223588B0: (149,'3569c4d343b38984df800a8688a594749fa8239707b1a4fbeee739e2b0f4b785','lost-enemy short/long phrase gate'),
    0x22358950: (29, '301bdcf5d10c75091894883514cacd3dc2b47948a3669dcbf81ed60f2996879a','refind-enemy phrase request'),
    0x22358970: (70, '87da0a39820aa483ac0ef3dc81187b4cf7eb325aec47b2793097c6d4f0df9c2a','go-alert phrase cooldown'),
    0x223589C0: (29, '5356fbf7af4e628566bf219932f46dd065da293e8f4506d0bfab6a5e55db1f47','announce phrase request'),
    0x22353F90: (6, '885aa912ed01cc73213d84315e8265bed2dfc71f0f02b9786b4aa9f216601c0a','GRUNT_REFIND_ENEMY getter'),
    0x22353FA0: (6, '38079625b3de1259cd5d03edeacb3fc01698138beb46a84b7dd83497d9f6e71d','GRUNT_GO_ALERT getter'),
    0x22353FF0: (6, 'a6e8f9905c54751b5be528e9ed71a144060a2dbc8a4b177678c41fe893b2d111','GRUNT_ANNOUNCE getter'),
    0x223541B0: (6, '2e69f1f5a67aca0826124ef50a6f25986f91e27d9e966861935d110045e48943','GRUNT_LOST_SHORT getter'),
    0x223541C0: (6, 'e7820746ecc3651c2eb523618d38b3431743a5137be091470bd5eb07f7ae6192','GRUNT_LOST_LONG getter'),
}
# Exact jump-table entry and handler block, no per-entry whole-function claim.
EVENTS = {
    0x50: (0x2205256A, 0x594, 0x223589C0),
    0x51: (0x2205254C, 0x590, 0x22358970),
    0x52: (0x22052579, 0x598, 0x2235AE90),
    0x53: (0x2205255B, 0x58C, 0x2235AFF0),
    0x54: (0x2205259D, 0x5A8, 0x2235AE30),
}
SLOTS = {
    0x590: 0x22358970, 0x594: 0x223589C0,
    0x598: 0x2235AE90, 0x58C: 0x2235AFF0,
    0x5A0: 0x223588B0, 0x5A4: 0x22358950,
    0x5A8: 0x2235AE30,
    0x7B0: 0x22353F90, 0x7B4: 0x22353FA0,
    0x7C8: 0x22353FF0, 0x838: 0x223541B0, 0x83C: 0x223541C0,
}
CALLS = {
    0x223588E0: 0x220317A0,
    0x22358916: 0x220A35C0,
    0x22358966: 0x220A35C0,
    0x2235899B: 0x220A35C0,
    0x223589D6: 0x220A35C0,
}
INDIRECT_CALLS = {
    0x22052550: 0x590, 0x2205256E: 0x594,
    0x2205257D: 0x598, 0x2205255F: 0x58C,
    0x220525C5: 0x5A8,
    0x2203056C: 0x5A4, 0x2203ED50: 0x5A0,
    0x22045798: 0x594, 0x22351C5F: 0x590,
    0x223588F9: 0x838, 0x22358905: 0x83C,
    0x22358955: 0x7B0, 0x2235898A: 0x7B4,
    0x223589C5: 0x7C8,
}
WORDS = {
    0x223588B0:'a190215722',0x223588BB:'d89e880c0000',
    0x223588C3:'f6c441',0x223588C6:'7b7b',
    0x223588CA:'ff9254010000',0x223588E8:'d81d148f3d22',
    0x223588F0:'f6c441',0x223588F3:'740c',
    0x2235890B:'6a02',0x2235890D:'6a00',
    0x2235891B:'85c0',0x2235891D:'7c24',
    0x22358927:'6800007041',0x2235892C:'680000a040',
    0x2235893D:'d99e880c0000',
    0x22358953:'8b06',0x2235895B:'6a02',0x2235895D:'6a02',
    0x22358978:'56',0x2235897B:'d89e7c0c0000',
    0x22358983:'f6c441',0x22358986:'752c',
    0x22358990:'6a02',0x22358992:'6a02',
    0x223589A8:'d805148f3d22',0x223589AE:'d99e7c0c0000',
    0x223589CB:'6a02',0x223589CD:'6a00',
    0x22050150:'81ec40070000',0x22050161:'8b1f',
    0x22050163:'8d43ff',0x22050166:'3d89000000',
    0x22050173:'ff2485142d0522',
    0x2205259D:'d94704',0x220525A2:'d97c2410',
    0x220525AD:'80cc0c',0x220525B4:'d96c2414',
    0x220525B8:'db5c2414',0x220525C0:'50',
    0x220525C1:'d96c2414',
}
STRINGS = {
    0x2248CD84: b'GRUNT_REFIND_ENEMY',
    0x2248CD98: b'GRUNT_GO_ALERT',
    0x2248CDF8: b'GRUNT_ANNOUNCE',
    0x2248CF7C: b'GRUNT_LOST_SHORT',
    0x2248CF90: b'GRUNT_LOST_LONG',
}

class PE32:
    def __init__(self,data):
        assert data[:2]==b'MZ','not MZ'
        pe=struct.unpack_from('<I',data,0x3c)[0]
        assert data[pe:pe+4]==b'PE\x00\x00','not PE'
        count=struct.unpack_from('<H',data,pe+6)[0]
        opt=pe+24
        assert struct.unpack_from('<H',data,opt)[0]==0x10b,'not PE32'
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        sh=opt+struct.unpack_from('<H',data,pe+20)[0]
        self.sections=[]
        for i in range(count):
            off=sh+i*40
            name=data[off:off+8].split(b'\0')[0]
            virtual_size,va,raw_size,raw=struct.unpack_from('<IIII',data,off+8)
            self.sections.append((name,self.base+va,self.base+va+min(virtual_size,raw_size),raw))
        self.data=data
    def section(self,va):
        for name,start,end,raw in self.sections:
            if start<=va<end:return name
        raise AssertionError(f'outside PE section {va:#x}')
    def read(self,va,n):
        for name,start,end,raw in self.sections:
            if start<=va and va+n<=end:
                return self.data[raw+(va-start):raw+(va-start)+n]
        raise AssertionError(f'outside PE section {va:#x}+{n}')

def edge(pe,site,target):
    code=pe.read(site,5)
    assert code[0]==0xe8 and site+5+struct.unpack('<i',code[1:])[0]==target,f'bad E8 at {site:#x}'

def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_SHA,'wrong client.dll identity'
    assert hashlib.sha256(server).hexdigest()==SERVER_SHA,'wrong server.dll identity'
    pe=PE32(server)
    assert pe.base==BASE
    ranges=[]
    for va,(size,sha,label) in METHODS.items():
        assert pe.section(va)==b'.text'
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==sha,f'wrong native method {label}'
        assert pe.read(va+size,1)==b'\xcc',f'wrong function boundary {va:#x}'
        assert pe.read(va+size-1,1)==b'\xc3',f'wrong native return {va:#x}'
        ranges.append((va,va+size))
    ranges.sort()
    assert all(p[1]<=q[0] for p,q in zip(ranges,ranges[1:])), 'method overlap'
    for off,va in SLOTS.items():assert struct.unpack('<I',pe.read(VTABLE+off,4))[0]==va,f'bad Grunt vtable {off:#x}'
    for site,target in CALLS.items():edge(pe,site,target)
    for site,slot in INDIRECT_CALLS.items():
        op=pe.read(site,6)
        assert op[:2] in (b'\xff\x90',b'\xff\x92',b'\xff\x93') and struct.unpack('<I',op[2:])[0]==slot,f'wrong vcall {site:#x}'
    for event,(handler,slot,grunt_func) in EVENTS.items():
        addr=TABLE+4*(event-1)
        assert pe.section(addr)==b'.text'
        assert struct.unpack('<I',pe.read(addr,4))[0]==handler,f'wrong event table code {event:#x}'
        assert struct.unpack('<I',pe.read(VTABLE+slot,4))[0]==grunt_func
    for site,h in WORDS.items():assert pe.read(site,len(bytes.fromhex(h)))==bytes.fromhex(h),f'opcode witness {site:#x}'
    for va,st in STRINGS.items():
        assert pe.section(va)==b'.rdata'
        assert pe.read(va,len(st)+1)==st+b'\0',f'wrong string {st!r}'
    assert struct.unpack('<f',pe.read(0x223D8F14,4))[0]==10.0
    assert sum(n for n,_,_ in METHODS.values())==307
    return {'new_complete_methods':len(METHODS),'new_original_server_bytes':307,
            'native_vtable_entries':len(SLOTS),'event_cases':len(EVENTS),
            'direct_calls':len(CALLS),'indirect_calls':len(INDIRECT_CALLS),
            'instruction_witnesses':len(WORDS),'original_voice_tokens':len(STRINGS)}

def self_test():
    class Small:
        def __init__(self,site,payload):self.site,self.payload=site,payload
        def read(self,site,n):return self.payload[site-self.site:site-self.site+n]
    for site,target in CALLS.items():
        original=b'\xe8'+struct.pack('<i',target-site-5)
        edge(Small(site,original),site,target)
        for i in range(5):
            fake=bytearray(original);fake[i]^=0x01
            try:edge(Small(site,bytes(fake)),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'mutated E8 accepted {site:#x}:{i}')
    try:audit(b'not original',b'not original')
    except AssertionError:pass
    else:raise AssertionError('accepted nonoriginal DLL pair')
    return 'SELF_TEST_OK'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test',action='store_true')
    parser.add_argument('--bin-zip',type=Path)
    args=parser.parse_args()
    if not args.self_test and not args.bin_zip:parser.error('provide --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z: result=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_ORIGINAL_GRUNT_EVENTS_PHASE580_AUTHENTICATED',result)
if __name__=='__main__':main()