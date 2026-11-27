#!/usr/bin/env python3
"""Audit original ServerGameEnts001 registration and virtual layout, without raw DLL redistribution."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_recombinant_minor_metric_gate import PE32, Refused, require, SERVER_SHA, synthetic_test

NAME_VA=0x22443314
REG_VA, REG_BYTES = 0x223BF210,21
REG_SHA='6c46b07d21b9fa3c4a216b491ec0719e94c4b1dd6d2a72bbf9b13bb9df79209a'
REG_GLOBAL, REG_HELPER = 0x225723E4,0x221FB5D0
FACTORY_VA, FACTORY_BYTES = 0x221E82C0,6
FACTORY_SHA='81f3c5629ce13db9b52fddf5328c9d8301d28f7d02a510d2108ede5d9d3ea532'
OBJECT_VA,VTABLE_VA=0x22510654,0x22443030
VTABLE_SHA='802c3d045ace1464abe8259852c6654c2c4a2327b56be820085cca8e609038ae'
# offset, method VA, exact complete original length, SHA-256
METHODS=(
 (0x00,0x221E9360,31,'6d003e7b41562fcfcf5f3741ad6888bd72f881fac026b5ea76dd8f976d115e32'),
 (0x04,0x221E82D0,12,'dca33cf1628220c76b1919029a5e882dfe903aa93934aee599ef63f66e59c78f'),
 (0x08,0x221E9380,299,'8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1'),
 (0x0C,0x221E82E0,14,'0b0caf62afd15971ebdd8778c492e03519e45a2c93c03b42aceec070630cb0ac'),
 (0x10,0x221E94B0,22,'c702a6d4ebf9c7ec08865a2dfa7b1010364af129604919721528675c3c3fac53'),
 (0x14,0x221E82F0,28,'189b466407658510bb27d05b63609600b2e338a8c920d5b5997cb2002869bbe3'),
 (0x18,0x221E94D0,354,'e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176'),
)

def verify_entries(pe, vtable, entries):
    for slot,addr,size,digest in entries:
        require(pe.u32(vtable+slot)==addr, 'method vtable entry changed: '+hex(slot))
        require(hashlib.sha256(pe.at(addr,size)).hexdigest()==digest,
                'full method bytes changed: '+hex(addr))
        require(pe.at(addr+size,1)==b'\xcc', 'method INT3 boundary changed: '+hex(addr))

def verify(raw: bytes, verify_whole_file: bool = True) -> dict:
    if verify_whole_file:
        require(hashlib.sha256(raw).hexdigest()==SERVER_SHA,'original server.dll SHA mismatch')
    pe=PE32(raw)
    require(pe.base==0x22000000,'unexpected image base')
    require(pe.section(REG_VA)=='.text' and pe.section(FACTORY_VA)=='.text',
            'registration and factory not both code')
    require(pe.section(VTABLE_VA)=='.rdata' and pe.section(NAME_VA)=='.rdata',
            'interface name or vtable section mismatch')
    require(pe.at(NAME_VA,len('ServerGameEnts001')+1)==b'ServerGameEnts001\x00',
            'factory interface name changed')
    require(hashlib.sha256(pe.at(REG_VA,REG_BYTES)).hexdigest()==REG_SHA,
            'registration function bytes changed')
    require(pe.at(REG_VA+REG_BYTES,1)==b'\xcc','registration boundary changed')
    require(pe.at(REG_VA,1)==b'\x68' and pe.u32(REG_VA+1)==NAME_VA,
            'registration name argument changed')
    require(pe.at(REG_VA+5,1)==b'\x68' and pe.u32(REG_VA+6)==FACTORY_VA,
            'registered factory thunk changed')
    require(pe.at(REG_VA+10,1)==b'\xb9' and pe.u32(REG_VA+11)==REG_GLOBAL,
            'registration singleton identity changed')
    require(pe.at(REG_VA+15,1)==b'\xe8' and
            REG_VA+20+struct.unpack('<i',pe.at(REG_VA+16,4))[0]==REG_HELPER,
            'original factory registration call changed')
    require(hashlib.sha256(pe.at(FACTORY_VA,FACTORY_BYTES)).hexdigest()==FACTORY_SHA,
            'factory method bytes changed')
    require(pe.at(FACTORY_VA+FACTORY_BYTES,1)==b'\xcc','factory boundary changed')
    require(pe.at(FACTORY_VA,1)==b'\xb8' and pe.u32(FACTORY_VA+1)==OBJECT_VA and
            pe.at(FACTORY_VA+5,1)==b'\xc3','factory singleton return changed')
    require(pe.u32(OBJECT_VA)==VTABLE_VA, 'interface singleton vptr changed')
    require(hashlib.sha256(pe.at(VTABLE_VA,28)).hexdigest()==VTABLE_SHA,
            'seven-slot vtable digest changed')
    require(pe.u32(VTABLE_VA+28)==0,'unexpected eighth slot')
    require(pe.u32(0x22443054)==0x221E63F0,'adjacent ServerGameClients003 vtable changed')
    verify_entries(pe,VTABLE_VA,METHODS)
    require(pe.at(0x221E82D0,1)==b'\x8b' and pe.u32(0x221E82D5)==0x225721E8,
            'slot+4 owner global store changed')
    require(pe.u32(0x221E94B8+2)==0x16c, 'slot+10 candidate offset changed')
    return {
       'status':'ORIGINAL_2006_SERVERGAMEENTS001_FACTORY_AND_VTABLE_AUTHENTICATED',
       'server_sha256':SERVER_SHA,
       'registration_va':hex(REG_VA),'factory_va':hex(FACTORY_VA),
       'singleton_va':hex(OBJECT_VA),'vtable_va':hex(VTABLE_VA),
       'method_count':len(METHODS),
       'complete_method_bytes':sum(x[2] for x in METHODS),
       'method_entries':[{'slot':hex(s),'va':hex(va),'bytes':length,'sha256':digest}
                         for s,va,length,digest in METHODS],
       'limits':'Factory and boundaries authenticated; large methods and retail engine lifecycle remain incomplete.'
    }

def self_test() -> None:
    synthetic_test()
    class Fake:
        def __init__(self):
            self.memory={0x1000:b'\x90\xc3\xcc',0x2000:struct.pack('<I',0x1000)}
        def at(self,addr,n):
            for start,data in self.memory.items():
                if start<=addr and addr+n<=start+len(data):
                    return data[addr-start:addr-start+n]
            raise Refused('missing synthetic VA')
        def u32(self,addr):return struct.unpack('<I',self.at(addr,4))[0]
    fake=Fake()
    records=((0,0x1000,2,hashlib.sha256(b'\x90\xc3').hexdigest()),)
    verify_entries(fake,0x2000,records)
    for target,mutated in [(0x1000,b'\x90\x90\xcc'),
                           (0x2000,struct.pack('<I',0x1004))]:
        f=Fake();f.memory[target]=mutated
        try:verify_entries(f,0x2000,records)
        except Refused:pass
        else:raise AssertionError('synthetic method corruption was admitted')

def original_corruption_test(raw: bytes) -> None:
    image=PE32(raw)
    for at in (REG_VA,FACTORY_VA,VTABLE_VA,OBJECT_VA,METHODS[0][1],METHODS[6][1]):
        for _,start,size,off in image.sections:
            if start<=at<start+size:
                altered=bytearray(raw)
                altered[off+at-start] ^= 1
                try:verify(bytes(altered),verify_whole_file=False)
                except Refused:pass
                else:raise AssertionError('original corruption admitted: '+hex(at))
                break

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path,help='owner-local original server.dll path')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test()
    if not args.server:
        require(args.self_test,'provide --server or --self-test')
        print(json.dumps({'status':'SYNTHETIC_GAMEENTS_FACTORY_CORRUPTION_TEST_PASSED'}))
        return
    raw=args.server.read_bytes()
    result=verify(raw)
    if args.self_test:
        original_corruption_test(raw)
        result['corruption_test']='passed'
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=='__main__':
    try:main()
    except (Refused,OSError,struct.error) as error:
        raise SystemExit('REFUSED: '+str(error))
