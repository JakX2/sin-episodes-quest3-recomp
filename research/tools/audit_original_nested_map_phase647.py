#!/usr/bin/env python3
"""Audit complete owner-original 2006 SiN nested-map constructors and release.

Owner-provided server.dll/client.dll are private. --self-test uses invented
byte sequences only; no proprietary code or DLL data is distributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA256='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
METHODS=(
 ('nested_array_constructor',0x2228eab0,90,
  '0ab5d6c1640e57969dace5266e647d13832946ea0db14d8ab500ec3d981d8e58'),
 ('nested_map_constructor',0x2228f360,81,
  '4bd47b0db93a0629b8732063cb35968b49c37dadb68e65b46dbe590972984ee8'),
 ('nested_array_release',0x2228eb10,41,
  'd157aa7fc3dc00d0e282b9507a754499e5870ab4382005547b909d6f08f9db99'),
)
CALLS=(
 (0x2228f373,0x2228eab0,'nested_constructor_calls_array_ctor'),
 (0x2228eada,0x2226e390,'array_constructor_allocation'),
 (0x2228eb21,0x2226e3e0,'array_release_free'),
 (0x22282166,0x2228f360,'table_direct_constructor_calls_nested'),
 (0x22282257,0x2228f360,'table_factory_calls_nested'),
)
BRANCHES=(
 (0x2228ead0,0x2228eae5,'74','zero_capacity_skips_alloc'),
 (0x2228f391,0x2228f3a2,'74','selector_low_byte_zero'),
 (0x2228f395,0x2228f3ac,'75','nonzero_selector_existing_comparator'),
 (0x2228f3a4,0x2228f3ac,'75','zero_selector_existing_comparator'),
 (0x2228eb17,0x2228eb37,'74','disabled_release'),
 (0x2228eb1e,0x2228eb30,'74','null_array_release'),
)
COMPARATOR_WRITES=(
 (0x2228f397,0x2228e9b0,'nonzero_selector'),
 (0x2228f3a6,0x2228e910,'zero_selector'),
)
PINS_TEXT='''
2228eab0:8b44240c 2228eab4:56 2228eab5:8bf1
2228eab7:8b4c2408 2228eabb:8906 2228eabd:8b44240c
2228eac1:85c0 2228eac3:c7460400000000
2228eaca:894608 2228eacd:894e0c 2228ead0:7413
2228ead2:8d1440 2228ead5:03d2 2228ead7:03d2
2228ead9:52 2228eada:e8b1f8fdff 2228eadf:83c404
2228eae2:894604 2228eae5:b8ffff0000
2228eaea:66894610 2228eaee:66894614
2228eaf2:8b4604 2228eaf5:894618
2228eaf8:66c746120000 2228eafe:66c746160000
2228eb04:8bc6 2228eb06:5e 2228eb07:c20c00
2228eb10:56 2228eb11:8bf1 2228eb13:837e0cff
2228eb17:741e 2228eb19:8b4604 2228eb1c:85c0
2228eb1e:7410 2228eb20:50 2228eb21:e8baf8fdff
2228eb26:83c404 2228eb29:c7460400000000
2228eb30:c7460800000000 2228eb37:5e 2228eb38:c3
2228f360:8b442408 2228f364:53 2228f365:56
2228f366:33db 2228f368:53 2228f369:8bf1
2228f36b:8b4c2410 2228f36f:50 2228f370:51
2228f371:8bce 2228f373:e838f7ffff
2228f378:385c2414 2228f37c:895e1c
2228f37f:895e20 2228f382:c7462408000000
2228f389:895e28 2228f38c:895e2c
2228f38f:8bc6 2228f391:740f 2228f393:391e
2228f395:7515 2228f397:c706b0e92822
2228f39d:5e 2228f39e:5b 2228f39f:c20c00
2228f3a2:391e 2228f3a4:7506
2228f3a6:c70610e92822 2228f3ac:5e
2228f3ad:5b 2228f3ae:c20c00
'''
PINS=tuple((int(p.split(':')[0],16),p.split(':')[1]) for p in PINS_TEXT.split())
class AuditFailure(ValueError):pass
class RetailPE:
    def __init__(self,data:bytes):
        self.data=data
        if len(data)<0x100 or data[:2]!=b'MZ':raise AuditFailure('not a PE binary')
        pe=struct.unpack_from('<I',data,0x3c)[0]
        if data[pe:pe+4]!=b'PE\0\0':raise AuditFailure('invalid PE signature')
        coff=pe+4
        sections=struct.unpack_from('<H',data,coff+2)[0]
        optional_size=struct.unpack_from('<H',data,coff+16)[0]
        optional=coff+20
        if struct.unpack_from('<H',data,optional)[0]!=0x10b:
            raise AuditFailure('expected original PE32, not PE32+')
        self.base=struct.unpack_from('<I',data,optional+28)[0]
        self.sections=[]
        for i in range(sections):
            start=optional+optional_size+40*i
            name=data[start:start+8].rstrip(b'\x00').decode('ascii')
            virtual_size,rva,raw_size,raw_ptr=struct.unpack_from('<IIII',data,start+8)
            self.sections.append((name,self.base+rva,virtual_size,raw_size,raw_ptr))
    def section(self,va:int)->str:
        for name,begin,virtual,raw,_ in self.sections:
            if begin<=va<begin+min(virtual,raw):return name
        raise AuditFailure(f'address outside original PE32 sections: {va:#x}')
    def at(self,va:int,length:int)->bytes:
        for name,begin,virtual,raw,offset in self.sections:
            if begin<=va and va+length<=begin+min(virtual,raw):
                chunk=self.data[offset+va-begin:offset+va-begin+length]
                if len(chunk)!=length:raise AuditFailure(f'truncated {name} at {va:#x}')
                return chunk
        raise AuditFailure(f'method outside PE section at {va:#x}')

def check_call(read,site:int,target:int):
    code=read(site,5)
    if code[0]!=0xe8 or site+5+struct.unpack_from('<i',code,1)[0]!=target:
        raise AuditFailure(f'changed retail rel32 call at {site:#x}')
def check_branch(read,site:int,target:int,op:str):
    code=read(site,2)
    if code[0]!=int(op,16) or site+2+struct.unpack_from('<b',code,1)[0]!=target:
        raise AuditFailure(f'changed retail short branch at {site:#x}')
def check_pin(read,site:int,hexbytes:str):
    value=bytes.fromhex(hexbytes)
    if read(site,len(value))!=value:
        raise AuditFailure(f'original opcode witness changed at {site:#x}')
def check_comparator_install(read,site:int,target:int):
    op=read(site,6)
    if op[:2]!=b'\xc7\x06' or struct.unpack_from('<I',op,2)[0]!=target:
        raise AuditFailure(f'changed installed native comparator at {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise AuditFailure('owner-original complete 2006 server/client hashes required')
    pe=RetailPE(server)
    if pe.base!=0x22000000:raise AuditFailure('unexpected original server image base')
    for name,va,size,digest in METHODS:
        if pe.section(va)!='.text' or pe.section(va+size-1)!='.text':
            raise AuditFailure(f'{name} not in original executable .text')
        if hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
            raise AuditFailure(f'whole original method SHA mismatch: {name}')
        if pe.at(va+size,1)!=b'\xcc':
            raise AuditFailure(f'missing original INT3 boundary: {name}')
    for site,dest,_ in CALLS:
        if pe.section(site)!='.text' or pe.section(dest)!='.text':
            raise AuditFailure('call site not in original text')
        check_call(pe.at,site,dest)
    for site,dest,op,_ in BRANCHES:check_branch(pe.at,site,dest,op)
    for site,target,_ in COMPARATOR_WRITES:check_comparator_install(pe.at,site,target)
    for site,op in PINS:check_pin(pe.at,site,op)
    return {
        'status':'PHASE647_ORIGINAL_NESTED_MAP_CONSTRUCTION_AUTHENTICATED',
        'retail_server_sha256':SERVER_SHA256,'retail_client_sha256':CLIENT_SHA256,
        'methods':[{'name':name,'va':hex(va),'bytes':size,'sha256':digest}
                   for name,va,size,digest in METHODS],
        'complete_original_executable_bytes':sum(m[2] for m in METHODS),
        'native_direct_rel32_call_edges':len(CALLS),
        'native_short_branch_edges':len(BRANCHES),
        'original_comparator_installations':len(COMPARATOR_WRITES),
        'original_pinned_opcode_witnesses':len(PINS),
        'original_nested_record_stride_bytes':12,
        'original_f360_selector_low_byte_zero_comparator':'0x2228e910',
        'original_f360_selector_low_byte_nonzero_comparator':'0x2228e9b0',
        'original_2228f7a0_string_lookup_recovered':False,
        'original_2228e9b0_comparator_recovered':False,
        'original_retail_game_process_executed':False,
        'quest3_gameplay_executed':False,
    }

def self_test():
    for _,va,n,_ in METHODS:
        original_free=bytes(((i*19+n+7)&255) for i in range(n))
        digest=hashlib.sha256(original_free).digest()
        assert hashlib.sha256(original_free).digest()==digest
        for i in range(n):
            candidate=bytearray(original_free);candidate[i]^=1
            if hashlib.sha256(candidate).digest()==digest:
                raise AssertionError('changed invented complete method accepted')
    site,dest=0x71000000,0x71000400
    call=b'\xe8'+struct.pack('<i',dest-site-5)
    check_call(lambda _v,n:call[:n],site,dest)
    for i in range(5):
        c=bytearray(call);c[i]^=1
        try:check_call(lambda _v,n:bytes(c[:n]),site,dest)
        except AuditFailure:pass
        else:raise AssertionError('changed rel32 accepted')
    for op in ('74','75'):
        code=bytes.fromhex(op)+struct.pack('<b',8)
        check_branch(lambda _v,n:code[:n],site,site+10,op)
        for i in range(2):
            c=bytearray(code);c[i]^=1
            try:check_branch(lambda _v,n:bytes(c[:n]),site,site+10,op)
            except AuditFailure:pass
            else:raise AssertionError('changed jump accepted')
    for target in (0x2228e910,0x2228e9b0):
        code=b'\xc7\x06'+struct.pack('<I',target)
        check_comparator_install(lambda _v,n:code[:n],site,target)
        for i in range(6):
            c=bytearray(code);c[i]^=1
            try:check_comparator_install(lambda _v,n:bytes(c[:n]),site,target)
            except AuditFailure:pass
            else:raise AssertionError('altered comparator install accepted')
    try:authenticate(b'fake PE',b'fake DLL')
    except AuditFailure:pass
    else:raise AssertionError('accepted unrelated DLL')
    print('PASS Phase647 invented 212 complete-method byte mutations, rel32, branch, comparator-target, retail identity refusal')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--client',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        parser.error('provide owner-retail --server and --client, or --self-test')
    try:result=authenticate(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,ValueError,struct.error) as error:
        parser.exit(1,f'PHASE647 AUTHENTICATION REFUSED: {error}\n')
    output=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(output,encoding='utf-8')
    else:print(output,end='')
if __name__=='__main__':main()
