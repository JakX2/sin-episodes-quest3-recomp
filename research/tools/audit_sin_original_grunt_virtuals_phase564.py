#!/usr/bin/env python3
"""SHA-pin original SiN 2006 Grunt vtable group/speech chain (Phase564).

Only accepts the owner-private exact original client+server DLL identities.
No commercial code bytes or disassembly are emitted.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import CLIENT_HASH, SERVER_HASH, PE32, edge

# Exact original .text body extents; names are editorial labels.
METHODS = {
 0x22044670:(13,'a60a7112693b99216a257966994ffb15a47d318cdc5ac6420364ca2be6f622cd','owner group virtual+0x690 assignment'),
 0x22044680:(3,'01cb47d078b4841b8408ec4fe278efa83115c6f6e101972987507d2b2b57dcf0','owner virtual+0x694 constant false'),
 0x22044840:(46,'729aaf252765a75428d0bb7ebb70479acc072ab5de17dd78127326351d480ea2','owner virtual+0x154 selected handle resolver'),
 0x22054C80:(28,'417ce57aa7b372fd1d435a4bd9402fe42370c986cf61e990b938a08e927f27f2','signed inclusive selected index range'),
 0x22054CA0:(51,'35eb22aff367f46e13187ef07092381ad94de0f67aeceb809c4e7a549cee001c','owner releases old group reservation and clears index'),
 0x220AAFF0:(91,'c727b891b3527e35fae31714120b19d54956f257f018011170469c953caa2e70','group bitmap previous-selection release'),
 0x22353FB0:(6,'4b11cfc6b58b896a9e45635b0aecd2a2bd7f23c20643008098b368b8dbcc73a0','GRUNT_LAST_OF_SQUAD immutable getter'),
 0x22353FC0:(6,'31ead634b449a8c6cc177f46cf3641d6a22e9e5ccda4e208ce9a2cdc9e596308','GRUNT_MAN_DOWN immutable getter'),
 0x22358590:(19,'010287fed98c2f063bf7d10504dfca1c1dd6e1cec5595f1411a5f6b6676ab820','owner group-join callback redirect with flag gate'),
 0x2235AF70:(113,'43c5bf4a22da09311edc96258ac037095c155b7d988363b044801962a79a9794','survivor group member removal dialogue'),
}
EXPECTED_VTABLE = {
 0x154:0x22044840, 0x678:0x22358590, 0x690:0x22044670,
 0x694:0x22044680, 0x750:0x2235AF70,
 0x7B8:0x22353FB0, 0x7BC:0x22353FC0,
}
VTABLE_START=0x2249AAA4
# All direct E8 calls within the ten new original methods.
CALLS={
 0x22054CC2:0x220AAFF0,
 0x220AB00D:0x220AA920,0x220AB037:0x220AA920,
 0x2235AF91:0x220A9480,0x2235AFB0:0x220A35C0,
 0x2235AFD8:0x220A35C0,
}
# Exact previous-method caller witnesses, no double-counted body bytes.
PRIOR_CALLS={
 0x22054D68:0x220AAEB0,  # Phase560 reservation method
 0x220AAEE3:0x220AA920, # Phase560 reservation's bitmap accessor
}
# Virtual call sites prove original table slot usages, not static direct edges.
VIRTUAL_CALLS={
 0x22054CB5:0x154, 0x22054D5B:0x154,
 0x220AA9B2:0x154,
 0x220AA17D:0x750,0x220AA195:0x690,0x220AA230:0x690,
 0x220AA2A4:0x678,
 0x2235AF9F:0x7B8,0x2235AFC7:0x7BC,
 0x2235AFBD:0x588,
}
# Source-bounded selective instruction and field offset bytes, not code dumps.
WITNESSES={
 0x22044670:'8b442404', 0x22044674:'8981c4090000', 0x2204467A:'c20400',
 0x22044680:'32c0c3',
 0x22044840:'8b81a0080000',0x22044846:'83f8ff',
 0x2204484B:'8b1520564f22',0x22044853:'81e1ff0f0000',
 0x22044860:'c1e80c',0x22044868:'8b01',
 0x22054C80:'8b81cc090000',0x22054C86:'3b442404',
 0x22054C8C:'3b442408',0x22054C92:'b001',0x22054C97:'32c0',
 0x22054CA3:'83bec409000000',0x22054CAC:'8b86cc090000',
 0x22054CC7:'c786cc090000ffffffff',
 0x220AAFF6:'83ffff',0x220AB005:'c1fe05',
 0x220AB012:'8bcf',0x220AB014:'83e11f',
 0x220AB022:'8538',0x220AB026:'6848e83e22',
 0x220AB040:'f7d7',0x220AB042:'2138',
 0x22353FB0:'b8a8cd4822',0x22353FC0:'b8bccd4822',
 0x22358590:'f681b600000001',0x22358599:'32c0',0x2235859B:'c20c00',
 0x2235AF73:'f686bf00000008',
 0x2235AF80:'83b89c00000000',0x2235AF8F:'6a01',
 0x2235AF96:'83f802',0x2235AFA5:'6a01',
 0x2235AFA7:'6aff',0x2235AFAA:'8d8ec00c0000',
 0x2235AFB7:'6a00',0x2235AFB9:'6a00',
 0x2235AFCD:'6a02',0x2235AFCF:'6a00',
}
TERMINALS={
 0x22044670:b'\xc2\x04\x00',0x22044680:b'\xc3',0x22044840:b'\xc3',
 0x22054C80:b'\xc2\x08\x00',0x22054CA0:b'\xc3',
 0x220AAFF0:b'\xc2\x08\x00',0x22353FB0:b'\xc3',
 0x22353FC0:b'\xc3',0x22358590:None,0x2235AF70:b'\xc2\x04\x00',
}
STRINGS={0x2248CDA8:b'GRUNT_LAST_OF_SQUAD',0x2248CDBC:b'GRUNT_MAN_DOWN'}


def check_jmp(pe,site,target):
    data=pe.read(site,5)
    assert data[0] == 0xE9 and site+5+struct.unpack('<i',data[1:])[0] == target, \
        f'bad original tail jump {site:#x}'


def audit(client,server):
    assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'invalid original client DLL SHA'
    assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'invalid original server DLL SHA'
    pe=PE32(server)
    assert pe.base==0x22000000
    intervals=[]
    for va,(size,digest,label) in METHODS.items():
        assert pe.section(va)==b'.text',label
        assert hashlib.sha256(pe.read(va,size)).hexdigest()==digest, f'whole original function changed: {label}'
        suffix=TERMINALS[va]
        if suffix is not None:
            assert pe.read(va+size-len(suffix),len(suffix))==suffix, f'nonmatching RET {va:#x}'
        else:
            check_jmp(pe,0x2235859E,0x22030520)
        assert pe.read(va+size,1)==b'\xcc', f'missing original INT3 after {label}'
        intervals.append((va,va+size))
    intervals.sort()
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
    for off,target in EXPECTED_VTABLE.items():
        assert struct.unpack('<I',pe.read(VTABLE_START+off,4))[0]==target, \
               f'Grunt primary vtable {off:#x}'
    for site,target in {**CALLS,**PRIOR_CALLS}.items(): edge(pe,site,target)
    for site,off in VIRTUAL_CALLS.items():
        op=pe.read(site,6)
        assert op[:2] in (b'\xff\x90',b'\xff\x92'),f'nonvirtual original call {site:#x}'
        assert struct.unpack('<I',op[2:])[0]==off,f'virtual offset {site:#x}'
    for site,hx in WITNESSES.items():
        seq=bytes.fromhex(hx)
        assert pe.read(site,len(seq))==seq,f'opcode/field witness {site:#x}'
    for va,lit in STRINGS.items():
        assert pe.section(va)==b'.rdata'
        assert pe.read(va,len(lit)+1)==lit+b'\0',f'original sentence token {va:#x}'
    assert sum(n for n,_,_ in METHODS.values())==376
    return {
        'full_native_methods':len(METHODS),'distinct_original_server_bytes':376,
        'direct_E8_calls':len(CALLS),'prior_xrefs':len(PRIOR_CALLS),
        'vtable_slots':len(EXPECTED_VTABLE),'virtual_calls':len(VIRTUAL_CALLS),
        'opcode_witnesses':len(WITNESSES),'original_squad_tokens':len(STRINGS),
        'tail_jumps':1,
    }


def self_test():
    class Stub:
        def __init__(self,address,code): self.address,self.code=address,code
        def read(self,where,length):return self.code[where-self.address:where-self.address+length]
    for site,target in {**CALLS,**PRIOR_CALLS}.items():
        original=b'\xe8'+struct.pack('<i',target-site-5)
        edge(Stub(site,original),site,target)
        for i in range(5):
            corrupt=bytearray(original);corrupt[i]^=1
            try:edge(Stub(site,corrupt),site,target)
            except AssertionError:pass
            else:raise AssertionError(f'accepted mutated call at {site:#x}')
    site=0x2235859E;dest=0x22030520
    good=b'\xe9'+struct.pack('<i',dest-site-5)
    check_jmp(Stub(site,good),site,dest)
    for i in range(5):
        bad=bytearray(good);bad[i]^=1
        try:check_jmp(Stub(site,bad),site,dest)
        except AssertionError:pass
        else:raise AssertionError('accepted mutated JMP')
    try:audit(b'MZnotoriginal',b'MZnotoriginal')
    except AssertionError:pass
    else:raise AssertionError('accepted wrong DLL identity')
    return 'SELF_TEST_OK'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--self-test',action='store_true')
    ap.add_argument('--bin-zip',type=Path)
    args=ap.parse_args()
    if not (args.self_test or args.bin_zip):ap.error('provide --self-test and/or --bin-zip')
    if args.self_test:print(self_test())
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            res=audit(z.read('client.dll'),z.read('server.dll'))
        print('SIN_GRUNT_VIRTUALS_PHASE564_AUTHENTICATED',res)
if __name__=='__main__':main()