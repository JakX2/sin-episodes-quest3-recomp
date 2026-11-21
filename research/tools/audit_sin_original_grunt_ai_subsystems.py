#!/usr/bin/env python3
"""Verify seven original 2006 SiN NPC CAI_* factories and 0x22031D50.

Reads only exact SHA-pinned original client/server DLLs in owner's bin.zip.
Outputs RTTI/class/layout metadata, never native machine-code bytes.
Public self-test uses invented addresses and does not need game content.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
GRUNT_VTABLE = 0x2249AAA4
MAIN = (0x22031D50, 276,
        "575ffffba81df6bc80a78e347c3c8323f73412852556a9d632f79cf8ef75ebbd")
# Runtime dispatch order, including six vptr RTTI names and the seventh.
# Each tuple: primary slot, owner field, factory VA, byte count,
# entire factory method SHA256, first native allocation bytes,
# RTTI vtable pointer, MSVC x86 decorated RTTI class name.
SPECS = (
 (0x3FC,0x898,0x22048F90,71,"a1b65c0362271bacc5e54150039605d895e3e8d35dbeadcc9bad9c03e2ce057e",0x78,0x223DAF60,".?AVCAI_Senses@@"),
 (0x404,0x908,0x2216E9C0,50,"d16d6e905d81fd38306a17fb56c5a48916964f8a1f94f5c242672a0dc2807378",0xF0,0x2241680C,".?AVCAI_BlendedMotor@@"),
 (0x408,0x8FC,0x22031ED0,50,"1e94cd9d1f559ee584e61377e802b6d18d1efc7dfe91e4ad99d037097a620fb4",0xD4,0x223E88F0,".?AVCAI_LocalNavigator@@"),
 (0x400,0x904,0x22031EA0,47,"1e31431da29aaf886e164345b7eacb17b74a2c0d659519c7d355250355265e2b",0x44,0x223E9018,".?AVCAI_MoveProbe@@"),
 (0x40C,0x8F8,0x2216DDD0,10,"69beb53321d2f642bfcdaa9f2968f086157c785785fdd2b0d5448433f2579527",0x9C,0x223E9354,".?AVCAI_Navigator@@"),
 (0x410,0x900,0x220379F0,62,"2ad1abdc04b6149e36b7761ba0b101a7f91004a2e4fc052da0ebbe6bc55c6fab",0x18,0x223D9C68,".?AVCAI_Pathfinder@@"),
 (0x414,0x9D8,0x22031F10,50,"05cd186561ac3c01b36d5cb2cd01f875e093b3809a8a128ea67efb5554b3a6a2",0x10,0x223D975C,".?AVCAI_TacticalServices@@"),
)
STORES = (
 (0x22031D5D,0x898),(0x22031D71,0x908),
 (0x22031D85,0x8FC),(0x22031D99,0x904),
 (0x22031DAD,0x8F8),(0x22031DC1,0x900),
 (0x22031DD5,0x9D8),
)
# Exact original relative CALL edges, including the distinct Navigator wrapper.
EDGES = (
 (0x22359213,0x22031D50),
 (0x22031E09,0x2206BF10),
 (0x22031E26,0x220674D0),
 (0x22031E49,0x22083CD0),
 (0x22031E5B,0x220AB0D0),
 (0x2216DDD0,0x22031F50),
 (0x22031F75,0x220747A0),
 (0x22048FB2,0x22044DC0),
 (0x2216E9E5,0x2216EA00),
 (0x22031EF5,0x22067970),
 (0x22031EC2,0x22071EC0),
)

class Refusal(ValueError):
    pass

def check(condition,why):
    if not condition: raise Refusal(why)

class PE:
    def __init__(self, data):
        self.data=data
        check(data[:2]==b'MZ',"not DOS/PE")
        pe=struct.unpack_from('<I',data,0x3C)[0]
        check(data[pe:pe+4]==b'PE\0\0',"bad PE signature")
        coff=pe+4
        machine,count=struct.unpack_from('<HH',data,coff)
        opt_size=struct.unpack_from('<H',data,coff+16)[0]
        opt=coff+20
        check(machine==0x14C and struct.unpack_from('<H',data,opt)[0]==0x10B,
              "not original 32-bit x86 PE")
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        self.sections=[]
        for i in range(count):
            p=opt+opt_size+40*i
            name=data[p:p+8].split(b'\0')[0].decode('ascii','replace')
            vs,rva,raw_size,raw=struct.unpack_from('<IIII',data,p+8)
            check(raw+raw_size<=len(data),"bad section bounds")
            self.sections.append((name,self.base+rva,vs,raw_size,raw))
    def at(self,va,n):
        for name,start,vs,rs,raw in self.sections:
            if start<=va and va+n<=start+min(vs,rs):
                return self.data[raw+va-start:raw+va-start+n]
        raise Refusal("unmapped original VA: "+hex(va))
    def section_at(self,va):
        for name,start,vs,_,_ in self.sections:
            if start<=va<start+vs:return name
        return None
    def u32(self,va):
        return struct.unpack('<I',self.at(va,4))[0]

def verify_edges(read,edges):
    for site,target in edges:
        code=read(site,5)
        check(len(code)==5 and code[0]==0xE8,
              'missing direct rel32 CALL at '+hex(site))
        delta=struct.unpack_from('<i',code,1)[0]
        check(site+5+delta==target,
              'incorrect original direct target at '+hex(site))
    return len(edges)

def verify_rtti(pe,virtual_table,expected):
    check(pe.section_at(virtual_table)=='.rdata','RTTI vtable outside .rdata')
    col=pe.u32(virtual_table-4)
    check(pe.section_at(col)=='.rdata','MSVC x86 RTTI COL absent')
    type_descr=pe.u32(col+0xC)
    check(pe.section_at(type_descr) in ('.data','.rdata'),'RTTI type descriptor absent')
    check(pe.at(type_descr+8,len(expected)+1)==expected.encode()+b'\0',
          'incorrect original RTTI type '+expected)

def verify_original(client,server):
    check(hashlib.sha256(server).hexdigest()==SERVER_SHA,
          'original server.dll whole-file SHA256 mismatch')
    check(hashlib.sha256(client).hexdigest()==CLIENT_SHA,
          'original client.dll whole-file SHA256 mismatch')
    pe=PE(server)
    check(pe.base==0x22000000,'original server image base changed')
    va,n,expected=MAIN
    check(pe.section_at(va)=='.text' and
          hashlib.sha256(pe.at(va,n)).hexdigest()==expected,
          'original 276-byte AI initializer changed')
    for i,(slot,field,factory,length,digest,alloc,vptr,rtti) in enumerate(SPECS):
        check(pe.u32(GRUNT_VTABLE+slot)==factory,
              'original Grunt factory virtual slot changed: '+hex(slot))
        check(pe.section_at(factory)=='.text' and
              hashlib.sha256(pe.at(factory,length)).hexdigest()==digest,
              'original full factory method changed: '+hex(factory))
        check(pe.at(STORES[i][0],6)==b'\x89\x86'+struct.pack('<I',field),
              'original owner pointer store offset changed: '+hex(field))
        verify_rtti(pe,vptr,rtti)
    # Source post-factory context pointer is read three times.
    for site in (0x22031E31,0x22031E3D,0x22031E4E):
        check(pe.u32(site+2 if site!=0x22031E3D else site+1)==0x225490C8,
              'original shared initialization context pointer changed')
    # Original member +0x9E4 uses virtual slot+4 with owner as argument.
    check(pe.at(0x22031DE7,6)==b'\x8d\x8e\xe4\x09\x00\x00' and
          pe.at(0x22031DEE,3)==b'\xff\x52\x04',
          'embedded owner relay changed')
    checked=verify_edges(pe.at,EDGES)
    return {
       'status':'ORIGINAL_SIN_GRUNT_AI_SUBSYSTEMS_VERIFIED',
       'server_sha256':SERVER_SHA,'client_sha256':CLIENT_SHA,
       'main_initializer':hex(va),'main_initializer_length':n,
       'complete_methods_sha_verified':len(SPECS)+1,
       'complete_bytes_sha_verified':n+sum(v[3] for v in SPECS),
       'direct_call_edges':checked,
       'components':[
           {'rtti':spec[7], 'virtual_slot':hex(spec[0]),
            'owner_field':hex(spec[1]),'factory':hex(spec[2]),
            'first_allocation_bytes':spec[5]}
           for spec in SPECS],
       'embedded_owner_member_offset':'0x9e4',
       'shared_context_global_va':'0x225490c8',
       'limits': 'No native engine-owned CAI_* instances created; no playable independent server.dll proven.'
    }

def self_test():
    fake=((0x73000100,0x73004000),(0x73000200,0x73005000))
    buf={site:b'\xE8'+struct.pack('<i',target-site-5)
         for site,target in fake}
    assert verify_edges(lambda va,n:buf.get(va,b'')[:n],fake)==2
    for site in buf:
        for byte in range(5):
            changed=dict(buf)
            p=bytearray(changed[site]);p[byte]^=1
            changed[site]=bytes(p)
            try:verify_edges(lambda va,n:changed.get(va,b'')[:n],fake)
            except Refusal:pass
            else:raise AssertionError('forged original CALL accepted')
    try:verify_original(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError('wrong original hashes accepted')
    assert len(SPECS)==7 and len({x[1] for x in SPECS})==7
    assert [x[0] for x in SPECS]==[0x3FC,0x404,0x408,0x400,0x40C,0x410,0x414]
    print('PASS Phase534 10 synthetic rel32 mutations, exact seven factory slot/order contract, SHA refusal')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.bin_zip:p.error('use exact original --bin-zip or --self-test')
    try:
        with zipfile.ZipFile(args.bin_zip) as z:
            result=verify_original(z.read('client.dll'),z.read('server.dll'))
        print(json.dumps(result,indent=2))
    except (OSError,zipfile.BadZipFile,KeyError,Refusal,struct.error) as exc:
        p.exit(2,'ORIGINAL GRUNT AI EVIDENCE REFUSED: '+str(exc)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
