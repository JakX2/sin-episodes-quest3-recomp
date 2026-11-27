#!/usr/bin/env python3
"""Audit the owner-local original SiN Minor +0x398 method without redistributing DLL bytes."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
METHOD_SHA = 'e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7'
METHOD_VA, METHOD_SIZE = 0x2237E8C0, 0x38
VTABLE_VA, VTABLE_SLOT = 0x224A2974, 0x398
INHERITED_VTABLES=(0x2249F65C,0x224A0D8C)
INHERITED_FALSE=0x22044730
CALL_SITES=(0x22042F15,0x222A7A17,0x222AC311)
OWNER_GLOBAL, THRESHOLD_VA = 0x22572198, 0x223D8F14

class Refused(ValueError):
    pass

def require(ok: bool, why: str) -> None:
    if not ok:
        raise Refused(why)

class PE32:
    def __init__(self, data: bytes):
        self.data = data
        require(len(data) >= 0x100 and data[:2] == b'MZ', 'missing DOS header')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        require(pe + 24 <= len(data) and data[pe:pe+4] == b'PE\x00\x00', 'missing NT header')
        co = pe + 4
        require(struct.unpack_from('<H', data, co)[0] == 0x14c, 'not Intel i386')
        count = struct.unpack_from('<H', data, co+2)[0]
        size = struct.unpack_from('<H', data, co+16)[0]
        op = co+20
        require(op+size <= len(data) and size >= 0x60, 'truncated optional header')
        require(struct.unpack_from('<H', data, op)[0] == 0x10b, 'not PE32')
        self.base = struct.unpack_from('<I', data, op+28)[0]
        table = op+size
        require(count > 0 and table+count*40 <= len(data), 'truncated sections')
        self.sections = []
        for i in range(count):
            off = table+i*40
            name = data[off:off+8].split(b'\x00')[0].decode('ascii', 'replace')
            size_virtual, rva, size_raw, raw = struct.unpack_from('<IIII', data, off+8)
            require(raw+size_raw <= len(data), 'section outside file')
            self.sections.append((name,self.base+rva,min(size_virtual,size_raw),raw))

    def at(self, va: int, length: int) -> bytes:
        for _,start,size,raw in self.sections:
            if length >= 0 and start <= va and va+length <= start+size:
                return self.data[raw+va-start:raw+va-start+length]
        raise Refused('unmapped VA '+hex(va))

    def u32(self, va: int) -> int:
        return struct.unpack('<I', self.at(va,4))[0]

    def section(self, va: int) -> str | None:
        for name,start,size,_ in self.sections:
            if start <= va < start+size:
                return name
        return None

def verify(raw: bytes, verify_whole_file: bool = True) -> dict:
    if verify_whole_file:
        require(hashlib.sha256(raw).hexdigest() == SERVER_SHA, 'server.dll SHA-256 mismatch')
    pe = PE32(raw)
    require(pe.base == 0x22000000, 'unexpected PE image base')
    require(pe.section(METHOD_VA) == '.text', 'method not in .text')
    require(pe.section(VTABLE_VA+VTABLE_SLOT) == '.rdata', 'vtable not in .rdata')
    require(pe.u32(VTABLE_VA+VTABLE_SLOT) == METHOD_VA, 'original Minor +398 vtable mismatch')
    require(all(pe.u32(vt+VTABLE_SLOT)==INHERITED_FALSE for vt in INHERITED_VTABLES),
            'base/Major +398 inherited dispatch changed')
    require(pe.at(INHERITED_FALSE,5) == b'\x32\xc0\xc2\x04\x00',
            'original inherited false stub changed')
    for site in CALL_SITES:
        require(pe.at(site,6) == b'\xff\x90\x98\x03\x00\x00',
                'shared virtual +398 callsite changed: '+hex(site))
    method = pe.at(METHOD_VA,METHOD_SIZE)
    require(hashlib.sha256(method).hexdigest() == METHOD_SHA, 'complete method digest changed')
    require(pe.at(METHOD_VA+METHOD_SIZE,1) == b'\xcc', 'method lacks expected INT3 terminator')
    require(pe.u32(THRESHOLD_VA) == 0x41200000, 'original threshold is not 10.0f')
    require(method[0:2] == b'\x8b\x0d' and pe.u32(METHOD_VA+2) == OWNER_GLOBAL,
            'original global owner load changed')
    require(method[8:10] == b'\xff\x90' and pe.u32(METHOD_VA+10) == 0x170,
            'global owner virtual +170 call changed')
    require(method[14:18] == b'\x84\xc0\x74\x05', 'early owner veto branch changed')
    require(method[23:30] == b'\x8b\x4c\x24\x04\xd9\x41\x2c',
            'argument+2c float read changed')
    require(method[30:32] == b'\xd8\x1d' and pe.u32(METHOD_VA+32) == THRESHOLD_VA,
            'x87 threshold comparison changed')
    require(method[38:41] == b'\xf6\xc4\x41', 'x87 status-mask check changed')
    require(method[43:48] == b'\xb8\x01\x00\x00\x00', 'true return changed')
    require(method[20:23] == method[48:51] == method[53:56] == b'\xc2\x04\x00',
            'original stdcall stack return size changed')
    return {
        'status':'ORIGINAL_2006_RECOMBINANT_MINOR_METRIC_GATE_AUTHENTICATED',
        'server_sha256':SERVER_SHA,
        'method_va':hex(METHOD_VA),
        'method_length':METHOD_SIZE,
        'method_sha256':METHOD_SHA,
        'original_vtable_slot':hex(VTABLE_SLOT),
        'manager_virtual_slot':hex(0x170),
        'authenticated_shared_vtable_calls':[hex(site) for site in CALL_SITES],
        'inherited_base_major_default':'false',
        'candidate_scalar_offset':hex(0x2c),
        'threshold_float':10.0,
        'comparison':'strict greater-than; unordered NaN is false',
        'limits':'Original scalar and manager meanings unresolved; native tests do not establish retail-engine execution.'
    }

def synthetic_test() -> None:
    # Independently test the VA/section parser on synthetic, original-free bytes.
    data = bytearray(0x400)
    data[:2] = b'MZ'
    struct.pack_into('<I',data,0x3c,0x80)
    data[0x80:0x84] = b'PE\x00\x00'
    struct.pack_into('<H',data,0x84,0x14c)
    struct.pack_into('<H',data,0x86,1)
    struct.pack_into('<H',data,0x94,0xe0)
    struct.pack_into('<H',data,0x98,0x10b)
    struct.pack_into('<I',data,0x98+28,0x22000000)
    sec=0x98+0xe0
    data[sec:sec+5]=b'.text'
    struct.pack_into('<IIII',data,sec+8,0x100,0x1000,0x100,0x200)
    data[0x200:0x204]=b'ABCD'
    pe=PE32(bytes(data))
    require(pe.at(0x22001000,4)==b'ABCD', 'synthetic PE address mapping failed')
    for mutation in ('mz','pe','section'):
        trial=bytearray(data)
        if mutation=='mz':trial[0]=0
        elif mutation=='pe':trial[0x80]=0
        else:struct.pack_into('<I',trial,sec+20,len(data)+1)
        try:PE32(bytes(trial))
        except Refused:pass
        else:raise AssertionError('synthetic corruption admitted: '+mutation)

def original_corruption_test(raw: bytes) -> None:
    pe=PE32(raw)
    verify(raw)
    for at in (METHOD_VA,VTABLE_VA+VTABLE_SLOT,THRESHOLD_VA,CALL_SITES[0],OWNER_GLOBAL):
        # The global owner itself is mutable .data and not a code witness.
        if at==OWNER_GLOBAL:continue
        for _,start,size,off in pe.sections:
            if start<=at<start+size:
                mutated=bytearray(raw)
                mutated[off+at-start] ^= 1
                try:verify(bytes(mutated),verify_whole_file=False)
                except Refused:pass
                else:raise AssertionError('original region corruption admitted: '+hex(at))
                break

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path,help='owner-local original server.dll path')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:synthetic_test()
    if not args.server:
        require(args.self_test,'provide --server or --self-test')
        print(json.dumps({'status':'SYNTHETIC_PE32_CORRUPTION_TEST_PASSED'}))
        return
    raw=args.server.read_bytes()
    evidence=verify(raw)
    if args.self_test:
        original_corruption_test(raw)
        evidence['corruption_test']='passed'
    print(json.dumps(evidence,indent=2,sort_keys=True))

if __name__=='__main__':
    try:main()
    except (Refused,OSError,struct.error) as error:
        raise SystemExit('REFUSED: '+str(error))
