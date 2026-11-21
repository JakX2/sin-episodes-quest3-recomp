#!/usr/bin/env python3
"""Phase537: original SHA-pinned MobGrunt vtable +0x844 branch-decomp audit.

Reads owner-local game DLLs. No original code is output, included, or uploaded.
Checks the actual complete method, direct CALLs, return IDs, source field
witnesses, and four schedule-name/ID registry pairs in the 2006 server DLL.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
FN = 0x2235D2C0
FN_LEN = 0x2E2
FN_SHA = '254800571c72c5a26cf163f745631d1e79c89ea5f707c302d682726be66f42dc'
AUX_FN = 0x2235A3A0
AUX_LEN = 0xCE
AUX_SHA = 'b86979db517e208a3f46a0ca0386a30f5b9066b08050c38de14b14cec6605343'
AUX_DISTANCE_CONSTANT = 0x223F02BC
PRIMARY_VTABLE = 0x2249AAA4
VTABLE_BYTE_SLOT = 0x844
DISTANCE_CONSTANT_ADDRESS = 0x22490CF8

# Each E8 rel32 at an instruction boundary in the 738-byte original routine.
CALL_EDGES = (
    (0x2235D2C5, 0x220397C0),
    (0x2235D2D7, 0x220397C0),
    (0x2235D307, 0x220AA2D0),
    (0x2235D31A, 0x220A9210),
    (0x2235D329, 0x220A9210),
    (0x2235D342, 0x220A9210),
    (0x2235D36D, 0x220397C0),
    (0x2235D38A, 0x22054D40),
    (0x2235D3A6, 0x2235A470),
    (0x2235D3AF, 0x220397C0),
    (0x2235D3DA, 0x220397C0),
    (0x2235D3E7, 0x220397C0),
    (0x2235D3F4, 0x220397C0),
    (0x2235D409, 0x220397C0),
    (0x2235D442, 0x2235A560),
    (0x2235D453, 0x220397C0),
    (0x2235D504, 0x220397C0),
    (0x2235D515, 0x220397C0),
    (0x2235D522, 0x220397C0),
    (0x2235D52F, 0x220397C0),
    (0x2235D53A, 0x222AA640),
    (0x2235D551, 0x220092D0),
    (0x2235D559, 0x220092D0),
    (0x2235D560, 0x220ADAD0),
    (0x2235D56F, 0x220397C0),
    (0x2235D590, 0x2235A3A0),
)

# Immediate return values from x86 `mov eax,imm32` blocks. The two zero
# returns use xor eax,eax and are verified separately.
RETURN_SITES = (
    (0x2235D394, 0x2A),
    (0x2235D39C, 0x19),
    (0x2235D3CF, 0x50),
    (0x2235D3FE, 0x30),
    (0x2235D439, 0x0D),
    (0x2235D49E, 0x58),
    (0x2235D4FB, 0x2D),
    (0x2235D587, 0x59),
    (0x2235D596, 0x57),
)

# Each exact condition ID push preceding a direct call to the same original
# condition-test function. These are *shared raw IDs*, not named conditions.
CONDITION_WITNESSES = (
    (0x2235D2C1, 0x1D), (0x2235D2D3, 0x19),
    (0x2235D369, 0x0A), (0x2235D3AB, 0x14),
    (0x2235D3D6, 0x04), (0x2235D3E3, 0x03),
    (0x2235D3F0, 0x16), (0x2235D405, 0x10),
    (0x2235D44F, 0x0C), (0x2235D502, 0x0A),
    (0x2235D511, 0x14), (0x2235D51E, 0x25),
    (0x2235D52B, 0x1E), (0x2235D56B, 0x25),
)

# Confirm both original registry name and corresponding machine-code ID.
# name_write, id_write, call_site, registry_string_addr, ID, label
REGISTRATIONS = (
    (0x22360143,0x2236014B,0x22360153,0x22493278,0x50,'SCHED_GRUNT_SUPPRESS'),
    (0x2235FF91,0x2235FF99,0x2235FFA1,0x22493EC0,0x57,'SCHED_GRUNT_ASSAULT'),
    (0x2235FFCF,0x2235FFD7,0x2235FFDF,0x22493C90,0x58,'SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE'),
    (0x2236000D,0x22360015,0x2236001D,0x22493A58,0x59,'SCHED_GRUNT_PRESS_ATTACK'),
)

# Vtable virtual dispatches that have separate effect/guard roles.
# Some use FF90 and others FF92 (EAX/EDX vptr), and a fixed 32-bit offset.
VIRTUAL_DISPATCHES = (
    (0x2235D2E8, 0x154),
    (0x2235D332, 0x154),
    (0x2235D351, 0x584),
    (0x2235D363, 0x588),
    (0x2235D37C, 0x78C),
    (0x2235D3C0, 0x79C),
    (0x2235D42A, 0x154),
    (0x2235D472, 0x154),
    (0x2235D480, 0x154),
    (0x2235D493, 0x7A0),
    (0x2235D4A9, 0x154),
    (0x2235D4B5, 0x214),
    (0x2235D4C1, 0x214),
    (0x2235D549, 0x154),
    (0x2235D57C, 0x79C),
)

class Refused(ValueError):
    pass

def require(yes, msg):
    if not yes:
        raise Refused(msg)

class PE32:
    def __init__(self, raw):
        self.raw = raw
        require(raw[:2] == b'MZ', 'missing DOS MZ header')
        pe = struct.unpack_from('<I', raw, 0x3C)[0]
        require(raw[pe:pe+4] == b'PE\0\0', 'missing PE signature')
        coff = pe+4
        require(struct.unpack_from('<H',raw,coff)[0] == 0x14C,'requires i386 PE')
        n = struct.unpack_from('<H',raw,coff+2)[0]
        sz = struct.unpack_from('<H',raw,coff+16)[0]
        opt = coff+20
        require(struct.unpack_from('<H',raw,opt)[0] == 0x10B,'requires PE32')
        self.base = struct.unpack_from('<I',raw,opt+28)[0]
        self.sections = []
        for i in range(n):
            off = opt+sz+i*40
            name = raw[off:off+8].split(b'\0')[0].decode('ascii','replace')
            vs,rva,rawsize,rawptr = struct.unpack_from('<IIII',raw,off+8)
            require(rawptr+rawsize<=len(raw),'section extends beyond PE file')
            self.sections.append((name,self.base+rva,vs,rawsize,rawptr))

    def at(self,va,n):
        for name,start,virtual_size,rawsize,off in self.sections:
            if start<=va and va+n<=start+min(virtual_size,rawsize):
                return self.raw[off+va-start:off+va-start+n]
        raise Refused('original VA outside raw-backed section: '+hex(va))

    def u32(self,va):
        return struct.unpack('<I',self.at(va,4))[0]

    def section(self,va):
        for name,start,size,_,_ in self.sections:
            if start<=va<start+size:
                return name
        return None

def call_at(pe,site,target):
    raw=pe.at(site,5)
    require(raw[0]==0xE8 and site+5+struct.unpack_from('<i',raw,1)[0]==target,
            f'wrong CALL rel32 {site:#x} expected {target:#x}')

def verify(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'original server SHA mismatch')
    require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'original client SHA mismatch')
    pe=PE32(server)
    require(pe.base==0x22000000,'unexpected server image base')
    require(pe.section(FN)=='.text','function outside .text')
    require(hashlib.sha256(pe.at(FN,FN_LEN)).hexdigest()==FN_SHA,'complete original +0x844 method SHA mismatch')
    require(pe.u32(PRIMARY_VTABLE+VTABLE_BYTE_SLOT)==FN,'Grunt primary vtable slot+0x844 mismatch')
    require(pe.section(AUX_FN)=='.text' and
            hashlib.sha256(pe.at(AUX_FN,AUX_LEN)).hexdigest()==AUX_SHA,
            'complete original assault helper 0x2235A3A0 SHA mismatch')
    require(pe.at(AUX_FN+AUX_LEN,1)==b'\xcc', 'assault helper padding boundary changed')
    call_at(pe,0x2235A3BE,0x22031A80)
    call_at(pe,0x2235A466,0x220A35C0)
    require(pe.at(0x2235A3B2,3)==b'\x83\xf8\x01',
            'assault helper sample <=1 decision changed')
    require(pe.at(0x2235A419,6)==b'\xd8\x1d'+struct.pack('<I',AUX_DISTANCE_CONSTANT),
            'assault helper squared distance reference changed')
    require(struct.unpack('<f',pe.at(AUX_DISTANCE_CONSTANT,4))[0]==4000000.0,
            'assault helper 4m squared-distance threshold changed')
    require(pe.at(0x2235A43F,5)==b'\x68\x81\x40\x00\x00',
            'assault helper 0x4081 virtual argument changed')
    require(pe.at(0x2235A45B,4)==b'\x6a\x02\x6a\x00',
            'assault helper sentence arguments changed')
    for site,target in CALL_EDGES:
        require(FN<=site<FN+FN_LEN,'call witness outside original routine')
        call_at(pe,site,target)
    for site, value in RETURN_SITES:
        require(pe.at(site,5)==b'\xb8'+struct.pack('<I',value),f'wrong return schedule at {site:#x}')
    for site in (0x2235D2CE,0x2235D59D):
        require(pe.at(site,2)==b'\x33\xc0','original zero return not preserved')
    for site,id_num in CONDITION_WITNESSES:
        require(pe.at(site,2)==bytes((0x6A,id_num)),f'condition query changed {site:#x}')
    for site,virtual in VIRTUAL_DISPATCHES:
        insn=pe.at(site,6)
        require(insn[:2] in (b'\xff\x90',b'\xff\x92') and
                insn[2:]==struct.pack('<I',virtual),f'virtual slot witness changed {site:#x}')
    for name_site,id_site,regcall,name_ptr,sched_id,name in REGISTRATIONS:
        # mov [esp+imm8],imm32; stack slots differ for registration contexts
        name_bytes=pe.at(name_site,8)
        id_bytes=pe.at(id_site,8)
        require(name_bytes[:3]==b'\xc7\x44\x24' and
                name_bytes[4:]==struct.pack('<I',name_ptr),f'schedule name reference at {name_site:#x}')
        require(id_bytes[:3]==b'\xc7\x44\x24' and
                id_bytes[4:]==struct.pack('<I',sched_id),f'schedule ID at {id_site:#x}')
        require(pe.at(name_ptr,len(name)+1)==name.encode('ascii')+b'\0',f'schedule label mismatch {name}')
        call_at(pe,regcall,0x2208C590)
    require(pe.at(0x2235D462,9)==b'\x8b\x06\xc6\x86\x64\x0c\x00\x00\x01',
            'original 0xC64 flag set changed')
    require(pe.at(0x2235D46B,7)==b'\xc6\x86\x66\x0c\x00\x00\x00',
            'original 0xC66 flag clear changed')
    require(pe.at(0x2235D486,7)==b'\xf6\x80\xbd\x00\x00\x00\x80',
            'target flag 0xBD mask 0x80 changed')
    require(pe.at(0x2235D4D9,2)==b'\xd8\xc9', 'x87 distance square witness altered')
    require(pe.at(0x2235D4E7,6)==b'\xd8\x1d'+struct.pack('<I',DISTANCE_CONSTANT_ADDRESS),
            'x87 squared-distance threshold changed')
    threshold=struct.unpack('<f',pe.at(DISTANCE_CONSTANT_ADDRESS,4))[0]
    require(threshold==9000000.0,'original squared-distance threshold changed')
    require(pe.at(FN+FN_LEN-1,1)==b'\xc3','fallback function does not end in ret')
    return {
        'status':'ORIGINAL_GRUNT_844_FALLBACK_AUTHENTICATED',
        'server_sha256':SERVER_SHA,
        'original_method_va':hex(FN),
        'length_bytes':FN_LEN,
        'second_original_method_va':hex(AUX_FN),
        'second_method_length_bytes':AUX_LEN,
        'second_method_sha256':AUX_SHA,
        'two_method_total_bytes':FN_LEN+AUX_LEN,
        'original_method_sha256':FN_SHA,
        'verified_rel32_call_edges':len(CALL_EDGES),
        'distinct_rel32_targets':len({t for _,t in CALL_EDGES}),
        'verified_immediate_return_sites':len(RETURN_SITES),
        'raw_return_values':sorted({0,*[v for _,v in RETURN_SITES]}),
        'verified_shared_condition_pushes':len(CONDITION_WITNESSES),
        'verified_virtual_call_offsets':len(VIRTUAL_DISPATCHES),
        'verified_original_schedule_names':{hex(id_num):name for *_,id_num,name in REGISTRATIONS},
        'squared_distance_threshold':threshold,
        'assault_helper_speech_distance_threshold':4000000.0,
        'limits':'No verified semantic names for shared condition IDs or unmapped schedule IDs; no independent engine runtime.'
    }

def self_test():
    assert len(CALL_EDGES)==26 and len({x[0] for x in CALL_EDGES})==26
    assert FN_LEN+AUX_LEN==944
    assert len(RETURN_SITES)==9 and len(REGISTRATIONS)==4
    assert {k for _,k in RETURN_SITES}=={0x2A,0x19,0x50,0x30,0x0D,0x58,0x2D,0x59,0x57}
    require(FN_LEN==0x2E2,'incorrect verified function length')
    site=0x41414200;target=0x41415555
    raw=b'\xe8'+struct.pack('<i',target-site-5)
    class Tiny:
        def __init__(self,raw):self.raw=raw
        def at(self,va,n):assert va==site;return self.raw[:n]
    call_at(Tiny(raw),site,target)
    for i in range(5):
        corrupt=bytearray(raw);corrupt[i]^=0x01
        try:call_at(Tiny(bytes(corrupt)),site,target)
        except Refused:pass
        else:raise AssertionError(f'corrupted call byte {i} accepted')
    try:verify(b'not a server',b'not a client')
    except Refused:pass
    else:raise AssertionError('invalid DLL hashes accepted')
    print('PASS Phase537 synthetic CALL mutations and original SHA refusal')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-zip',type=Path,help='owner-private ZIP with original server.dll and client.dll')
    parser.add_argument('--self-test',action='store_true',help='run binary-free corruption tests')
    args=parser.parse_args()
    if not args.self_test and not args.bin_zip:
        parser.error('supply --self-test and/or --bin-zip')
    if args.self_test:self_test()
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            server=z.read('server.dll');client=z.read('client.dll')
        print(json.dumps(verify(server,client),indent=2,sort_keys=True))

if __name__=='__main__':main()