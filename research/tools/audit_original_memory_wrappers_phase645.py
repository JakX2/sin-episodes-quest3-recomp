#!/usr/bin/env python3
"""Authenticate complete 2006 SiN server.dll allocation + tag wrappers.

Private owner DLLs are required for authentic checks. --self-test uses invented
bytes only. No proprietary original executable data is checked into source.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_symbol_slot_allocation_phase639 import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256,
    check_full,check_edge,check_pin)

METHODS=(
 ('tag_discovery',0x2226e290,90,'84049bbc203aa4d0f0fa107d0294be44dd7075dcd827a13f7c2fda933f9f0659'),
 ('allocation_wrapper',0x2226e390,79,'8c8192a3d478efa799525609e38affe36e9548e5ee449e57aadc914237e13d1a'),
 ('reallocation_wrapper',0x2226e400,90,'fccf3ba0ea064576d8806a024df924f2281e8829d202ae99c023010f2283d110'),
 ('free_wrapper',0x2226e3e0,18,'4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc'),
)
CALLS=(
 (0x2226e3a3,0x2226e290,'allocate_discovers_tag'),
 (0x2226e413,0x2226e290,'reallocate_discovers_tag'),
 (0x22280e30,0x2226e390,'slot_grow_allocates'),
 (0x22280e1b,0x2226e400,'slot_grow_reallocates'),
 (0x22280db0,0x2226e3e0,'slot_release_frees'),
)
BRANCHES=(
 (0x2226e2a1,0x2226e2e4,'74','tag_probe_false'),
 (0x2226e39b,0x2226e3af,'75','allocate_cache_initialized'),
 (0x2226e3ad,0x2226e3b4,'eb','allocate_cache_join'),
 (0x2226e3b6,0x2226e3ca,'75','allocate_tagged_dispatch'),
 (0x2226e40b,0x2226e41f,'75','realloc_cache_initialized'),
 (0x2226e41d,0x2226e424,'eb','realloc_cache_join'),
 (0x2226e426,0x2226e43f,'75','realloc_tagged_dispatch'),
)
# Address-specific native instruction fingerprints from original .text.
# Entire method SHA256s still authenticate *every* byte, including gaps.
PIN_TEXT='''
2226e290:a124213d22 2226e295:8b08 2226e297:8b11
2226e299:83ec1c 2226e29c:ff525c 2226e29f:84c0
2226e2a1:7441 2226e2a3:56 2226e2a4:6804010000
2226e2a9:6a00 2226e2ab:ff1524203d22 2226e2b1:50
2226e2b2:ff1520203d22 2226e2b8:8bf0 2226e2ba:6a1c
2226e2bc:8d442408 2226e2c0:50 2226e2c1:68d8855822
2226e2c6:ff151c203d22 2226e2cc:8b4c2408
2226e2d0:6804010000 2226e2d5:56 2226e2d6:51
2226e2d7:ff1518203d22 2226e2dd:8bc6
2226e2e4:33c0 2226e2e6:83c41c 2226e2e9:c3
2226e390:b801000000 2226e395:8405e0855822
2226e39b:7512 2226e39d:0905e0855822
2226e3a3:e8e8feffff 2226e3a8:a3dc855822
2226e3ad:eb05 2226e3af:a1dc855822
2226e3b4:85c0 2226e3b6:7512
2226e3b8:a124213d22 2226e3bd:8b08
2226e3bf:8b442404 2226e3c3:8b11
2226e3c5:50 2226e3c6:ff5204
2226e3ca:8b0d24213d22 2226e3d0:8b09
2226e3d2:8b11 2226e3d4:6a00
2226e3d6:50 2226e3d7:8b44240c
2226e3db:50 2226e3dc:ff12 2226e3de:c3
2226e3e0:a124213d22 2226e3e5:8b08
2226e3e7:8b442404 2226e3eb:8b11
2226e3ed:50 2226e3ee:ff5214 2226e3f1:c3
2226e400:b801000000 2226e405:8405e8855822
2226e40b:7512 2226e40d:0905e8855822
2226e413:e878feffff 2226e418:a3e4855822
2226e41d:eb05 2226e41f:a1e4855822
2226e424:85c0 2226e426:7517
2226e428:a124213d22 2226e42d:8b08
2226e42f:8b442408 2226e433:8b11
2226e435:50 2226e436:8b442408
2226e43a:50 2226e43b:ff520c
2226e43f:8b0d24213d22 2226e445:8b09
2226e447:8b11 2226e449:6a00
2226e44b:50 2226e44c:8b442410
2226e450:50 2226e451:8b442410
2226e455:50 2226e456:ff5208 2226e459:c3
'''
PINS=tuple((int(e.split(':',1)[0],16),e.split(':',1)[1])
           for e in PIN_TEXT.split())

def check_branch(read,site,target,opcode_hex):
    op=bytes.fromhex(opcode_hex)
    data=read(site,len(op)+1)
    if len(data)!=len(op)+1 or not data.startswith(op):
        raise VerificationError(f'wrong x86 branch opcode at {site:#x}')
    relative=struct.unpack_from('<b',data,len(op))[0]
    if site+len(data)+relative!=target:
        raise VerificationError(f'wrong x86 branch target at {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
            hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError('original retail whole DLL SHA256 identities mismatch')
    pe=PEImage(server)
    if pe.base!=0x22000000:raise VerificationError('wrong original image base')
    for name,va,n,sha in METHODS:
        if pe.section(va)!='.text':raise VerificationError(f'{name} outside original .text')
        check_full(pe.at,va,n,sha)
        if pe.at(va+n,1)!=b'\xcc':raise VerificationError(f'{name} lacks INT3 end boundary')
    for src,dst,role in CALLS:
        if pe.section(src)!='.text' or pe.section(dst)!='.text':
            raise VerificationError(f'call {role} outside original .text')
        check_edge(pe.at,src,dst)
    for src,dst,opcode,_ in BRANCHES:check_branch(pe.at,src,dst,opcode)
    for va,opcodes in PINS:check_pin(pe.at,va,opcodes)
    return {
        'status':'PHASE645_RETAIL_ORIGINAL_ALLOCATOR_WRAPPERS_VERIFIED',
        'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
        'methods':[dict(name=name,va=hex(va),bytes=n,sha256=sha)
                   for name,va,n,sha in METHODS],
        'complete_original_method_bytes':sum(x[2] for x in METHODS),
        'original_direct_rel32_edges':len(CALLS),
        'original_branch_edges':len(BRANCHES),
        'address_pinned_instruction_witnesses':len(PINS),
        'manager_global_va':'0x223d2124',
        'allocate_cached_tag_va':'0x225885dc',
        'allocate_tag_flag_va':'0x225885e0',
        'reallocate_cached_tag_va':'0x225885e4',
        'reallocate_tag_flag_va':'0x225885e8',
        'tag_discovery_imports':['GetProcessHeap','HeapAlloc','VirtualQuery','GetModuleFileNameA'],
        'original_underlying_memory_manager_reimplemented':False,
        'original_retail_game_process_executed':False,
        'android_quest_gameplay_executed':False,
    }

def self_test():
    for _,va,n,_ in METHODS:
        synthetic=bytes(((i*27+n+17)&255) for i in range(n))
        digest=hashlib.sha256(synthetic).hexdigest()
        check_full(lambda _va,size:synthetic[:size],va,n,digest)
        for i in range(n):
            changed=bytearray(synthetic);changed[i]^=1
            try:check_full(lambda _va,size:bytes(changed[:size]),va,n,digest)
            except VerificationError:pass
            else:raise AssertionError('mutated original-free function byte accepted')
    src,dst=0x71000000,0x71002000
    rel=b'\xe8'+struct.pack('<i',dst-src-5)
    check_edge(lambda _va,n:rel[:n],src,dst)
    for i in range(5):
        changed=bytearray(rel);changed[i]^=1
        try:check_edge(lambda _va,n:bytes(changed[:n]),src,dst)
        except VerificationError:pass
        else:raise AssertionError('invalid original rel32 edge accepted')
    for op in ('74','75','eb'):
        original=bytes.fromhex(op)+struct.pack('<b',8)
        check_branch(lambda _va,n:original[:n],src,src+len(original)+8,op)
        for i in range(len(original)):
            changed=bytearray(original);changed[i]^=1
            try:check_branch(lambda _va,n:bytes(changed[:n]),src,
                             src+len(original)+8,op)
            except VerificationError:pass
            else:raise AssertionError('wrong original conditional target accepted')
    check_pin(lambda _va,n:bytes.fromhex('ff5204')[:n],src,'ff5204')
    try:check_pin(lambda _va,n:bytes.fromhex('ff5214')[:n],src,'ff5204')
    except VerificationError:pass
    else:raise AssertionError('wrong memory-manager vtable method accepted')
    try:authenticate(b'unrelated server',b'unrelated client')
    except VerificationError:pass
    else:raise AssertionError('unrelated DLLs accepted as retail')
    print('PASS Phase645 invented full-body SHA, branch, rel32, vtable slot, retail identity refusal')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        p.error('supply original owner --server and --client DLL paths, or --self-test')
    try:report=authenticate(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        p.exit(1,'PHASE645 RETAIL AUDIT REFUSED: '+str(exc)+'\n')
    s=json.dumps(report,indent=2)+'\n'
    if args.output:args.output.write_text(s,encoding='utf-8')
    else:print(s,end='')
if __name__=='__main__':main()
