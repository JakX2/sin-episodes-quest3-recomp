#!/usr/bin/env python3
"""Read-only SHA-authenticated SiN 2006 original client surface-index calls.

Only tiny *sparse* x86 opcode witnesses are published, never proprietary
function bodies. This proves virtual byte offset +0x0C on the genuine
original VPhysicsSurfaceProps001 global, which the 2004 SDK names
GetSurfaceIndex. Slot2 SurfacePropCount is still ONLY SDK-guided.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
BASE = 0x24000000
SURFACE_GLOBAL = 0x243743A8
# Each item is a short sparse original code slice: NOT a full function,
# not a disassembly dump and not an alternate executable distribution.
WITNESSES = {
    0x240B7EBA: bytes.fromhex("8b0da84337248b1103d853ff520c"),
    0x24191E33: bytes.fromhex("8b0da84337248b0152ff500c"),
    0x241A7367: bytes.fromhex("8b0da8433724"),
    0x241A7386: bytes.fromhex("8b0152ff500c"),
    # Cross-check old 2004 SDK +0x14 GetSurfaceData layout on same global.
    0x24184F57: bytes.fromhex("8b0da84337248b0103f552ff5014"),
}
INDEX_CALLS = (0x240B7EC5, 0x24191E3C, 0x241A7389)
SURFACE_DATA_CALL = 0x24184F62

class AuditError(ValueError):
    pass

def u16(data: bytes, i: int) -> int:
    if i < 0 or i+2 > len(data):
        raise AuditError("truncated PE u16")
    return struct.unpack_from("<H", data, i)[0]

def u32(data: bytes, i: int) -> int:
    if i < 0 or i+4 > len(data):
        raise AuditError("truncated PE u32")
    return struct.unpack_from("<I", data, i)[0]

class PE32:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise AuditError("missing DOS PE header")
        nt = u32(data, 0x3c)
        if nt+24 > len(data) or data[nt:nt+4] != b"PE\\0\\0":
            raise AuditError("missing PE signature")
        nsections = u16(data, nt+6)
        opt_len = u16(data, nt+20)
        opt = nt+24
        if u16(data,opt) != 0x10b:
            raise AuditError("original is not PE32")
        self.base = u32(data,opt+28)
        if self.base != BASE:
            raise AuditError("original client image base mismatch")
        section_start = opt+opt_len
        if not (1 <= nsections <= 32):
            raise AuditError("invalid PE section count")
        self.sections=[]
        for i in range(nsections):
            p=section_start+i*40
            if p+40 > len(data):
                raise AuditError("truncated PE section")
            name=data[p:p+8].rstrip(b"\\0")
            virtual_size=u32(data,p+8)
            rva=u32(data,p+12)
            raw_size=u32(data,p+16)
            offset=u32(data,p+20)
            if offset+raw_size > len(data):
                raise AuditError("section extends outside PE")
            self.sections.append((name,rva,virtual_size,offset,raw_size))

    def read(self, va: int, size: int) -> bytes:
        if not 0<size<64 or va<self.base:
            raise AuditError("invalid opcode witness")
        rva=va-self.base
        for name,start,vsize,offset,rsize in self.sections:
            if name == b".text" and start <= rva and rva+size <= start+rsize:
                p=offset+(rva-start)
                return self.data[p:p+size]
        raise AuditError("witness not within original executable section")

def verify_witnesses(read):
    for va, expected in WITNESSES.items():
        if read(va,len(expected)) != expected:
            raise AuditError("original surface index ABI opcode mismatch at %#x" % va)
    return {
        "client_surface_global": hex(SURFACE_GLOBAL),
        "index_vtable_offset_bytes": 12,
        "index_callsite_VAs": [hex(x) for x in INDEX_CALLS],
        "surface_data_vtable_offset_bytes": 20,
        "surface_data_callsite_VA": hex(SURFACE_DATA_CALL),
        "original_sparse_witnesses":len(WITNESSES),
        "sparse_witness_bytes":sum(map(len,WITNESSES.values())),
        "slot2_surface_count": "SDK2004-guided only, not a demonstrated original callsite",
        "game_init": "NOT demonstrated",
    }

def verify_original(data: bytes):
    if hashlib.sha256(data).hexdigest() != CLIENT_SHA:
        raise AuditError("original client.dll full SHA256 mismatch")
    return verify_witnesses(PE32(data).read)

def self_test():
    expected=dict(WITNESSES)
    def read(va,size):
        return expected[va][:size]
    verify_witnesses(read)
    for va in WITNESSES:
        fixture=dict(expected)
        mutant=bytearray(fixture[va])
        mutant[len(mutant)//2]^=1
        fixture[va]=bytes(mutant)
        try:
            verify_witnesses(lambda a,n:fixture[a][:n])
        except AuditError:
            pass
        else:
            raise AssertionError("mutated x86 witness not rejected")
    try:
        verify_original(b"MZ"+b"\\0"*500)
    except AuditError:
        pass
    else:
        raise AssertionError("untrusted original hash accepted")
    print("PASS original-free SiN 2006 surface index witness mutations and SHA refusal")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--client",type=Path,help="owner-only authentic original client.dll path")
    group.add_argument("--archive",type=Path,help="owner-only original SE1/bin.zip")
    group.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.archive:
        with zipfile.ZipFile(args.archive) as z:
            data=z.read("client.dll")
    else:
        data=args.client.read_bytes()
    print(json.dumps(verify_original(data),indent=2,sort_keys=True))

if __name__=="__main__":
    try:main()
    except (AuditError,OSError,KeyError,zipfile.BadZipFile) as e:
        print("REFUSED:",str(e),file=sys.stderr)
        sys.exit(1)
