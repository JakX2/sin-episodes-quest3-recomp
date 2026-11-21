#!/usr/bin/env python3
"""SHA-audited original 2006 server physics surface sound-index scan.

Authenticates the original 352-byte function 0x2220D9B0 including ten
unsigned-short surface sound fields (offsets 0x2c..0x3e), two Count calls,
one GetSurfaceData call, ten GetString virtual calls and ten calls to the
same original sound-cache forwarding helper. Emits NO original bytecode.

Public CI uses --self-test and invented bytes; proprietary original
server.dll is read ONLY when explicitly supplied to --server.
"""
import argparse
import hashlib
import struct
from pathlib import Path

from audit_sin_original_server_physics_bootstrap import (
    AuditError, OriginalPE, SERVER_SHA
)

REGION_VA = 0x2220D9B0
REGION_SIZE = 352
REGION_SHA = "77fbcb512867a57087c25c63f97bf902fcc7149dec58f8d71920569699bc237e"
PRECACHE_FORWARD_VA = 0x221B6170
SOUND_OFFSETS = tuple(range(0x2c,0x40,2))
FIELD_NAMES = (
    "stepleft","stepright","impactSoft","impactHard","scrapeSmooth",
    "scrapeRough","bulletImpact","rolling","breakSound","strainSound"
)

def inspect_region(body, require_digest=True):
    if len(body)!=REGION_SIZE:
        raise AuditError("original server sound scan length differs from 352")
    if require_digest and hashlib.sha256(body).hexdigest()!=REGION_SHA:
        raise AuditError("complete original server sound scan SHA mismatch")
    count=b"\xff\x50\x08"
    surface=b"\xff\x52\x14"
    string=b"\xff\x52\x18"
    if body.count(count)!=2 or body.count(surface)!=1 or body.count(string)!=10:
        raise AuditError("original SurfacePropCount/GetSurfaceData/GetString call count mismatch")
    if body[-1]!=0xc3:
        raise AuditError("original sound reference scan return changed")
    previous=-1
    for offset in SOUND_OFFSETS:
        opcode=bytes((0x0f,0xb7,0x46,offset))
        if body.count(opcode)!=1:
            raise AuditError("original sound field %+x no longer exact"%offset)
        position=body.find(opcode)
        if position<=previous:
            raise AuditError("original ten surface sound fields out of order")
        previous=position
    cursor=0
    all_calls=[]
    while True:
        pos=body.find(string,cursor)
        if pos<0:break
        # Original +0x18 GetString result is pushed directly to helper.
        if body[pos+3:pos+5]!=b"\x50\xe8":
            raise AuditError("original GetString forwarder call changed")
        rel=struct.unpack_from("<i",body,pos+5)[0]
        callsite=REGION_VA+pos+4
        target=callsite+5+rel
        if target!=PRECACHE_FORWARD_VA:
            raise AuditError("original string forwarder target changed")
        all_calls.append(pos)
        cursor=pos+3
    if len(all_calls)!=10:
        raise AuditError("original ten GetString forwarding calls mismatch")
    return {
        "whole_function_sha256":REGION_SHA,
        "full_original_function_bytes":REGION_SIZE,
        "original_va":hex(REGION_VA),
        "count_vtable_byte_offset":8,
        "data_vtable_byte_offset":20,
        "getstring_vtable_byte_offset":24,
        "sound_fields_hex":[hex(x) for x in SOUND_OFFSETS],
        "sound_field_names":FIELD_NAMES,
        "nonzero_name_forwarder_va":hex(PRECACHE_FORWARD_VA),
        "original_sound_helper_recreated":False,
        "native_engine_sound_reads_attempted":False,
    }

def inspect_original(server_bytes):
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise AuditError("full original server.dll SHA-256 mismatch")
    return inspect_region(OriginalPE(server_bytes).read(REGION_VA,REGION_SIZE))

def fake_region():
    # Synthetic individual instruction witnesses only, not copied binary.
    fake=bytearray(b"\x90"*REGION_SIZE)
    fake[0:3]=b"\xff\x50\x08"
    fake[16:19]=b"\xff\x52\x14"
    fake[340:343]=b"\xff\x50\x08"
    fake[-1]=0xc3
    for i,offset in enumerate(SOUND_OFFSETS):
        base=32+29*i
        fake[base:base+4]=bytes((0x0f,0xb7,0x46,offset))
        fake[base+8:base+11]=b"\xff\x52\x18"
        fake[base+11:base+13]=b"\x50\xe8"
        callsite=REGION_VA+base+12
        struct.pack_into("<i",fake,base+13,PRECACHE_FORWARD_VA-callsite-5)
    return fake

def self_test():
    sample=fake_region()
    inspect_region(sample,False)
    for at in (0,16,44,32+9*29+3,32+8*29+8,350):
        mutated=bytearray(sample)
        mutated[at]^=1
        try:inspect_region(mutated,False)
        except AuditError:pass
        else:raise AssertionError("mutated sound index signature accepted at "+str(at))
    try:inspect_region(sample,True)
    except AuditError:pass
    else:raise AssertionError("fake original sound scan accepted as SHA-authenticated")
    try:inspect_original(b"MZ"+b"\x00"*512)
    except AuditError:pass
    else:raise AssertionError("fake server PE accepted")
    print("PASS original-free SiN server 352-byte sound-index scan: exact ten fields/virtual slots/forwarder targets; no original code or audio effects")

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    args=ap.add_mutually_exclusive_group(required=True)
    args.add_argument("--server",type=Path)
    args.add_argument("--self-test",action="store_true")
    cfg=ap.parse_args()
    if cfg.self_test:self_test()
    else:
        import json
        print(json.dumps(inspect_original(cfg.server.read_bytes()),
                         indent=2,sort_keys=True))
if __name__=="__main__":
    main()
