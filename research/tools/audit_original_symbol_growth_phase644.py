#!/usr/bin/env python3
"""Original SiN Episodes: Emergence server.dll 2006 array owner audit.

The public self-test contains only invented byte sequences. A genuine owner-only
retail audit requires exact original server.dll and client.dll fingerprints.
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
    ('slot_array_growth_22280dd0',0x22280dd0,110,
     '0b4c3d93fa9aef438bff1693d141d16c30ed3301154e5e1cd7fc15ae6732e9f1'),
    ('slot_array_release_22280da0',0x22280da0,39,
     '7aa2cb04909c586cb3f63da1279b2fa7e924d84e81583b465f0339775879e2af'),
)
CALLS=(
    (0x222810fc,0x22280dd0,'Phase639_slot_acquisition_calls_grow'),
    (0x22280e1b,0x2226e400,'grow_reallocate_wrapper'),
    (0x22280e30,0x2226e390,'grow_allocate_wrapper'),
    (0x22280db0,0x2226e3e0,'release_free_wrapper'),
)
BRANCHES=(
    (0x22280da7,0x22280dc5,'74','release_disabled_skip'),
    (0x22280dad,0x22280dbe,'74','release_pointer_null'),
    (0x22280dd9,0x22280e3a,'74','growth_disabled_skip'),
    (0x22280de6,0x22280e0d,'7d','signed_capacity_sufficient'),
    (0x22280ded,0x22280e01,'74','capacity_zero_special_case'),
    (0x22280df1,0x22280dfa,'74','growth_step_zero_doubles'),
    (0x22280df8,0x22280e08,'eb','increment_join'),
    (0x22280dff,0x22280e08,'eb','double_join'),
    (0x22280e0b,0x22280de8,'7c','signed_growth_retry'),
    (0x22280e11,0x22280e29,'74','allocate_when_null'),
)
PIN_WORDS='''
22280da0:56 22280da1:8bf1 22280da3:837e08ff 22280da7:741c
22280da9:8b06 22280dab:85c0 22280dad:740f 22280daf:50
22280db5:83c404 22280db8:c70600000000 22280dbe:c7460400000000
22280dc5:5e 22280dc6:c3
22280dd0:56 22280dd1:8bf1 22280dd3:8b4e08 22280dd6:83f9ff
22280dd9:745f 22280ddb:8b4604 22280dde:8b542408 22280de2:03d0
22280de4:3bc2 22280de6:7d25 22280de8:8b4604 22280deb:85c0
22280ded:7412 22280def:85c9 22280df1:7407 22280df3:03c1
22280df5:894604 22280df8:eb0e 22280dfa:03c0 22280dfc:894604
22280dff:eb07 22280e01:c7460402000000 22280e08:395604
22280e0b:7cdb 22280e0d:8b06 22280e0f:85c0 22280e11:7416
22280e13:8b4e04 22280e16:c1e104 22280e19:51 22280e1a:50
22280e20:83c408 22280e23:8906 22280e25:5e 22280e26:c20400
22280e29:8b5604 22280e2c:c1e204 22280e2f:52
22280e35:83c404 22280e38:8906 22280e3a:5e 22280e3b:c20400
'''
PINS=tuple((int(part.split(':')[0],16),part.split(':')[1])
           for part in PIN_WORDS.split())
assert len(PINS)==55

def check_branch(read,site,target,opcode_hex):
    code=bytes.fromhex(opcode_hex)
    width=4 if code[0]==0x0f else 1
    raw=read(site,len(code)+width)
    if len(raw)!=len(code)+width or raw[:len(code)]!=code:
        raise VerificationError(f'native branch opcode changed {site:#x}')
    relative=struct.unpack_from('<i' if width==4 else '<b',raw,len(code))[0]
    if site+len(raw)+relative!=target:
        raise VerificationError(f'native branch target changed {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
            hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError('owner original 2006 full DLL identities mismatch')
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError('incorrect retail PE32 server image base')
    for name,va,length,digest in METHODS:
        if pe.section(va)!='.text':
            raise VerificationError(f'original method {name} not executable .text')
        check_full(pe.at,va,length,digest)
        if pe.at(va+length,1)!=b'\xcc':
            raise VerificationError(f'original method {name} lacks INT3 boundary')
    for site,dest,role in CALLS:
        if pe.section(site)!='.text' or pe.section(dest)!='.text':
            raise VerificationError(f'original call edge {role} outside .text')
        check_edge(pe.at,site,dest)
    for site,dest,opcode,_ in BRANCHES:
        check_branch(pe.at,site,dest,opcode)
    for addr,hexbytes in PINS:
        check_pin(pe.at,addr,hexbytes)
    return {
        'status':'PHASE644_OWNER_RETAIL_SLOT_GROWTH_RELEASE_AUTHENTICATED',
        'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
        'methods':[{'name':name,'va':hex(va),'original_bytes':length,
                    'full_sha256':digest} for name,va,length,digest in METHODS],
        'total_original_method_bytes':sum(item[2] for item in METHODS),
        'original_rel32_calls':len(CALLS),
        'original_branch_edges':len(BRANCHES),
        'address_pinned_opcode_witnesses':len(PINS),
        'original_initial_allocate':'0x2226e390',
        'original_reallocate':'0x2226e400',
        'original_free':'0x2226e3e0',
        'allocation_wrappers_reconstructed':False,
        'original_game_process_executed':False,
        'android_quest_gameplay_executed':False,
    }

def self_test():
    for _,va,n,_ in METHODS:
        invented=bytes(((i*31+n+19)&0xff) for i in range(n))
        digest=hashlib.sha256(invented).hexdigest()
        check_full(lambda _v,k:invented[:k],va,n,digest)
        for i in range(n):
            changed=bytearray(invented);changed[i]^=1
            try:check_full(lambda _v,k:bytes(changed[:k]),va,n,digest)
            except VerificationError:pass
            else:raise AssertionError('accepted modified complete method byte')
    site,dest=0x71000000,0x71001000
    call=b'\xe8'+struct.pack('<i',dest-site-5)
    check_edge(lambda _va,n:call[:n],site,dest)
    for i in range(len(call)):
        changed=bytearray(call);changed[i]^=1
        try:check_edge(lambda _va,n:bytes(changed[:n]),site,dest)
        except VerificationError:pass
        else:raise AssertionError('accepted modified rel32 call')
    for opcode in ('74','7c','7d','eb'):
        branch=bytes.fromhex(opcode)+struct.pack('<b',10)
        check_branch(lambda _va,n:branch[:n],site,site+len(branch)+10,opcode)
        for i in range(len(branch)):
            changed=bytearray(branch);changed[i]^=1
            try:check_branch(lambda _va,n:bytes(changed[:n]),site,
                             site+len(branch)+10,opcode)
            except VerificationError:pass
            else:raise AssertionError('accepted altered signed branch')
    check_pin(lambda _va,n:bytes.fromhex('c1e104')[:n],site,'c1e104')
    try:check_pin(lambda _va,n:bytes.fromhex('c1e102')[:n],site,'c1e104')
    except VerificationError:pass
    else:raise AssertionError('accepted bad native 16-byte stride')
    try:authenticate(b'unrelated server',b'unrelated client')
    except VerificationError:pass
    else:raise AssertionError('accepted wrong full DLL identity')
    print('PASS Phase644 original-free invented full-method SHA, rel32, signed-branch and 16B stride mutation refusals')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--client',type=Path)
    parser.add_argument('--self-test',action='store_true')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        parser.error('provide ORIGINAL --server and --client, or --self-test')
    try:report=authenticate(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        parser.exit(1,'PHASE644 RETAIL AUDIT REFUSED: '+str(exc)+'\n')
    out=json.dumps(report,indent=2)+'\n'
    if args.output:args.output.write_text(out,encoding='utf-8')
    else:print(out,end='')
if __name__=='__main__':main()
