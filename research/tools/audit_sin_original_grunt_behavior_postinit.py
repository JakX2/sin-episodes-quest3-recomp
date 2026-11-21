#!/usr/bin/env python3
"""Verify exact 2006 original MobGrunt 8 behavior registrations and sentence setup.

Owner-private original server/client DLLs MUST match their original full-file
SHA256 digests. Only structural metadata is printed. No DLL bytes distributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
GRUNT_VTABLE=0x2249AAA4
# Complete original methods: entry, exclusive byte length, full SHA256.
METHODS=(
 (0x22359210, 57,  '848486f30a6c0d1bf820eecbb544c451f91a362070ca3693f500165b74fbfbc6'),
 (0x2235F6E0, 438, '771d728cd8620187a0938a5afaa18fbfee645622c7375cf57b9ff459c90a88b7'),
 (0x22360D80, 338, '20ba9979f88e567a350b3378178dd03eef942667b891a83d4510fd539059356b'),
 (0x22358FA0, 238, '2ed0ff737d7007e37e45e0c20078e35dbc5b3b6f774b1c894d4f68a6b425b925'),
 (0x22353F80, 6,   '3457d8872b6b7b3e4cda7708e95e121a47a6676e7e1d7303d6fa1d5c0824df3a'),
 (0x22055670, 98,  'c1c733febe7e5b2ebb04d647a0e6174cf86c3dd041505dd87c0240cf31e8d296'),
 (0x220CDBD0, 10,  '01caaa228fe4f4c7fe4269994a79702b290762107c5e6b67c17e8d8525fc7585'),
)
# Behavior-append order: subobject field, exact constructor, RTTI vptr,
# constructor witness VA of `mov dword ptr [this], vptr`, type string.
SPECS=(
 (0xCFC,0x220DEC20,0x223F956C,0x220DEC31,'.?AVCAI_RappelBehavior@@'),
 (0xCD8,0x22393E00,0x224ABEC4,0x22393E0C,'.?AVCAI_FuncTankBehavior@@'),
 (0xD20,0x22399CD0,0x224AE624,0x22399CE0,'.?AVCAI_TakecoverBehavior@@'),
 (0xDA0,0x223925C0,0x224AB88C,0x223925D1,'.?AVCAI_BeserkBehavior@@'),
 (0xF88,0x22391BB0,0x224AB494,0x22391BBC,'.?AVCAI_ActBusyBehavior@@'),
 (0xDB8,0x220D0C80,0x223F5544,0x220D0C91,'.?AVCAI_AssaultBehavior@@'),
 (0xDE8,0x220E2850,0x223FA1B4,0x220E289C,'.?AVCAI_StandoffBehavior@@'),
 (0xEB8,0x220D6690,0x223F6B7C,0x220D669C,'.?AVCAI_FollowBehavior@@'),
)
# Constructor dispatches are not in registration order, verify separately.
CTOR_CALLS=(
 (0x22360E17,0x22393E00),(0x22360E22,0x220DEC20),
 (0x22360E2E,0x22399CD0),(0x22360E39,0x223925C0),
 (0x22360E44,0x220D0C80),(0x22360E52,0x220E2850),
 (0x22360E6C,0x220D6690),(0x22360E77,0x22391BB0),
)
APPEND_CALLS=(0x2235F704,0x2235F737,0x2235F76A,0x2235F79D,
              0x2235F7D0,0x2235F803,0x2235F836,0x2235F869)
BEHAVIOR_FIELDS=(0x2235F6F8,0x2235F72B,0x2235F75E,0x2235F791,
                 0x2235F7C4,0x2235F7F7,0x2235F82A,0x2235F85D)
# Every per-behavior virtual slot+4 and owner-subobject store after append.
OWNER_CALLS=(0x2235F70E,0x2235F741,0x2235F774,0x2235F7A7,
             0x2235F7DA,0x2235F80D,0x2235F840,0x2235F873)
OWNER_STORES=(0x2235F71F,0x2235F752,0x2235F785,0x2235F7B8,
              0x2235F7EB,0x2235F81E,0x2235F851,0x2235F881)

class Refusal(ValueError):
    pass

def require(ok, reason):
    if not ok:raise Refusal(reason)

class PE:
    def __init__(self,raw):
        self.raw=raw
        require(raw[:2]==b'MZ','not an MZ file')
        e=struct.unpack_from('<I',raw,0x3C)[0]
        require(raw[e:e+4]==b'PE\0\0','PE signature')
        coff=e+4
        count=struct.unpack_from('<H',raw,coff+2)[0]
        opts=struct.unpack_from('<H',raw,coff+16)[0]
        opt=coff+20
        require(struct.unpack_from('<H',raw,coff)[0]==0x14C and
                struct.unpack_from('<H',raw,opt)[0]==0x10B,'not Intel PE32')
        self.base=struct.unpack_from('<I',raw,opt+28)[0]
        self.sections=[]
        for i in range(count):
            p=opt+opts+i*40
            name=raw[p:p+8].split(b'\0')[0].decode('ascii','replace')
            vs,rva,rs,off=struct.unpack_from('<IIII',raw,p+8)
            require(off+rs<=len(raw),'section overrun')
            self.sections.append((name,self.base+rva,vs,rs,off))
    def at(self,addr,n):
        for name,start,vs,rs,off in self.sections:
            if start<=addr and addr+n<=start+min(vs,rs):
                return self.raw[off+addr-start:off+addr-start+n]
        raise Refusal('unmapped original address '+hex(addr))
    def u32(self,addr):return struct.unpack('<I',self.at(addr,4))[0]
    def section(self,addr):
        for name,start,vs,rs,off in self.sections:
            if start<=addr<start+vs:return name
        return None

def verify_call(read,site,target):
    raw=read(site,5)
    require(raw[:1]==b'\xE8' and len(raw)==5,'missing rel32 CALL '+hex(site))
    require(site+5+struct.unpack_from('<i',raw,1)[0]==target,
            'incorrect rel32 CALL target '+hex(site))

def verify_rtti(pe,vtable,name):
    require(pe.section(vtable)=='.rdata','RTTI vtable outside .rdata')
    col=pe.u32(vtable-4)
    require(pe.section(col)=='.rdata','RTTI COL outside .rdata')
    td=pe.u32(col+0xC)
    require(pe.section(td) in ('.data','.rdata'),'RTTI TD untrusted')
    require(pe.at(td+8,len(name)+1)==name.encode()+b'\0',
            'RTTI class mismatch: '+name)

def verify_original(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'server.dll SHA256 mismatch')
    require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client.dll SHA256 mismatch')
    pe=PE(server)
    require(pe.base==0x22000000,'unexpected server.dll image base')
    for a,n,sha in METHODS:
        require(pe.section(a)=='.text' and
                hashlib.sha256(pe.at(a,n)).hexdigest()==sha,
                'complete original function SHA mismatch at '+hex(a))
    require(pe.u32(GRUNT_VTABLE+0x3F8)==0x22359210,
            'Grunt post-construction virtual missing')
    require(pe.u32(GRUNT_VTABLE+0x76C)==0x2235F6E0,
            'Grunt behavior registration virtual missing')
    require(pe.u32(GRUNT_VTABLE+0x7AC)==0x22353F80,
            'Grunt sentence key virtual missing')
    for site,target in [(0x22359213,0x22031D50),
                        (0x22359240,0x22358FA0),*CTOR_CALLS]:
        verify_call(pe.at,site,target)
    for i,(field,ctor,vt,witness,rtti) in enumerate(SPECS):
        verify_rtti(pe,vt,rtti)
        expected=struct.pack('<I',vt)
        require(expected in pe.at(witness,8),'constructor vptr witness mismatch '+rtti)
        require(pe.u32(vt+4)==0x220CDBD0,'behavior bind virtual mismatch '+rtti)
        verify_call(pe.at,APPEND_CALLS[i],0x22055670)
        # lea ebx,[esi+field], a six-byte exact instruction.
        require(pe.at(BEHAVIOR_FIELDS[i],6)==b'\x8d\x9e'+struct.pack('<I',field),
                'incorrect registration subobject '+hex(field))
        require(pe.at(OWNER_CALLS[i],3)==b'\xff\x52\x04',
                'behavior owner +4 dispatch mismatch')
        # Most stores use [ebx+0x0C], last uses same at a slightly
        # different address after popping edi.
        require(pe.at(OWNER_STORES[i],3)==b'\x89\x43\x0c',
                'behavior link+0x0C store missing')
    require(pe.at(0x2235F6E5,6)==b'\x8b\x8e\x28\x0c\x00\x00',
            'collection size field +0xC28 changed')
    require(pe.at(0x2235F6EC,6)==b'\x8d\xbe\x1c\x0c\x00\x00',
            'collection object +0xC1C changed')
    require(pe.at(0x22353F80,6)==b'\xb8'+struct.pack('<I',0x2248CD64)+b'\xc3',
            'sentence virtual key return changed')
    key=b'NPC_Grunt.SentenceParameters\0'
    require(pe.at(0x2248CD64,len(key))==key,'sentence parameter key changed')
    require(pe.at(0x2235923A,6)==b'\x8d\x8e\xc0\x0c\x00\x00',
            'embedded sentence state offset changed')
    return {
        'status':'ORIGINAL_MOBGRUNT_EIGHT_BEHAVIORS_AUTHENTICATED',
        'original_server_sha256':SERVER_SHA,
        'complete_original_methods_verified':len(METHODS),
        'verified_complete_method_bytes':sum(n for _,n,_ in METHODS),
        'original_constructor_calls_verified':len(CTOR_CALLS),
        'behavior_array_offset':'0xC1C',
        'behavior_array_count_field':'0xC28',
        'owner_reference_subobject':'0xC14',
        'sentence_key':'NPC_Grunt.SentenceParameters',
        'behaviors':[{'rtti':rtti,'owner_offset':hex(field),
                     'constructor':hex(ctor),'original_vtable':hex(vt)}
                     for field,ctor,vt,_,rtti in SPECS],
        'limits':'No replacement NPC creation, original engine boot or Android execution claimed.'
    }

def self_test():
    assert len(SPECS)==8 and len({x[0] for x in SPECS})==8
    assert [x[0] for x in SPECS]==[0xCFC,0xCD8,0xD20,0xDA0,
                                 0xF88,0xDB8,0xDE8,0xEB8]
    a,b=0x70001234,0x70004000
    valid=b'\xe8'+struct.pack('<i',b-a-5)
    verify_call(lambda addr,n:valid[:n],a,b)
    for i in range(5):
        m=bytearray(valid);m[i]^=0x01
        try:verify_call(lambda addr,n:bytes(m[:n]),a,b)
        except Refusal:pass
        else:raise AssertionError('bad synthetic CALL accepted at '+str(i))
    try:verify_original(b'different',b'different')
    except Refusal:pass
    else:raise AssertionError('invalid PE was accepted')
    print('PASS 8 behavior metadata records; five rel32 corruptions rejected; original SHA refusal')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.bin_zip:p.error('provide --bin-zip or --self-test')
    try:
        with zipfile.ZipFile(args.bin_zip) as z:
            result=verify_original(z.read('server.dll'),z.read('client.dll'))
        print(json.dumps(result,indent=2))
        return 0
    except (OSError,zipfile.BadZipFile,KeyError,Refusal,struct.error) as ex:
        p.exit(2,'PHASE535 ORIGINAL EVIDENCE REFUSED: '+str(ex)+'\n')
if __name__=='__main__':raise SystemExit(main())