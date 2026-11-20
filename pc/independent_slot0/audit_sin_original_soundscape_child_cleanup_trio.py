#!/usr/bin/env python3
"""Original 2006 soundscape three 83-byte child array cleanup routines.

Private --client/--archive: SHA-verify original retail client.dll,
all three COMPLETE 83B method bodies, six direct allocator CALLs,
and show they are byte-identical when two CALL displacements are
normalized. Public --self-test: independently fabricated bytes only.
No original code, game assets or memory are written or invoked.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA,parse_image,region_at_va,
)
ORIGINAL_ALLOCATOR_FREE=0x2416F940
SIZE=83
METHODS=(
    ("child-at-08-and-1C",0x240F3970,
     "da7aa4756e4619c54c29a8a2093621e74b2eec7bb687b9bff8d1cd89c676c005"),
    ("child-at-A0",0x240F3A20,
     "07714c3f94df41a0385d66d5816262ee1a366200ad9f20739530459b79f16f9f"),
    ("child-at-B4",0x240F3AD0,
     "1ad7502a6062d8dabbd7ef7b3fa766ba73209c2787ed6215da6ab2592016d071"),
)
# Every source method executes the same instruction sequence; the
# ONLY differences among the entire 83 bytes are the two relative
# CALL-to-free displacements at offsets +23 and +60.
NORMALIZE_REL32_OFFSETS=(23,60)
SELECTED_WINDOWS=(
    (0,"568bf1"),
    (3,"837e08ff"),
    (7,"c7460c00000000"),
    (14,"741c"),
    (16,"8b0685c0740f50"),
    (28,"83c404"),
    (31,"c70600000000"),
    (37,"c7460400000000"),
    (44,"837e08ff"),
    (48,"8b06894610741a85c0740f50"),
    (65,"83c404"),
    (68,"c70600000000"),
    (74,"c7460400000000"),
    (81,"5ec3"),
)
def normalized(raw):
    copy=bytearray(raw)
    for at in NORMALIZE_REL32_OFFSETS:
        copy[at:at+5]=b"\xe8\x00\x00\x00\x00"
    return bytes(copy)
def inspect(name,va,sha,raw,full):
    if len(raw)!=SIZE:
        raise ValueError("original source child cleanup size mismatch: "+name)
    if full and hashlib.sha256(raw).hexdigest()!=sha:
        raise ValueError("original source child cleanup SHA mismatch: "+name)
    for at,op in SELECTED_WINDOWS:
        token=bytes.fromhex(op)
        if raw[at:at+len(token)]!=token:
            raise ValueError("original child cleanup opcode mismatch: "+name+
                             " offset=%d"%at)
    for at in NORMALIZE_REL32_OFFSETS:
        callsite=va+at
        raw_call=b"\xe8"+struct.pack(
            "<i",ORIGINAL_ALLOCATOR_FREE-(callsite+5))
        if raw[at:at+5]!=raw_call:
            raise ValueError("original child cleanup free CALL differs: "+name)
    return normalized(raw)
def audit(raws,full):
    if len(raws)!=len(METHODS):
        raise ValueError("expected exactly three original child methods")
    norm=[]
    for (name,va,sha),raw in zip(METHODS,raws):
        norm.append(inspect(name,va,sha,raw,full))
    if len(set(norm))!=1:
        raise ValueError("three source child methods differ outside CALL rel32")
    return {
        "full_original_83B_sha_for_each_method_verified":bool(full),
        "original_method_count":len(METHODS),
        "original_method_vas":[hex(item[1]) for item in METHODS],
        "source_method_size":SIZE,
        "source_identical_except_two_relative_calls":True,
        "source_allocator_free_va":hex(ORIGINAL_ALLOCATOR_FREE),
        "source_two_direct_free_calls_per_method_verified":True,
        "source_flag_offset":"0x8",
        "source_pointer_offset":"0x0",
        "source_count_offsets":["0x4","0xC"],
        "source_mirror_pointer_offset":"0x10",
        "source_sentinel_minus_one_checks":2,
        "source_two_allocator_calls_but_at_most_one_reachable":True,
        "source_selected_opcode_witnesses":len(SELECTED_WINDOWS)*3,
        "real_engine_memory_deallocated":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("owner original retail client.dll SHA mismatch")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("original retail PE32 base differs")
    result=audit([region_at_va(client,image,va,SIZE)
                  for _,va,_ in METHODS],True)
    result.update(original_client_whole_sha_verified=True,
                  original_private_method_bytes_exported=False)
    return result
def self_test():
    raw=[]
    for _,va,_ in METHODS:
        fake=bytearray(bytes([0xCC])*SIZE)
        for at,op in SELECTED_WINDOWS:
            token=bytes.fromhex(op)
            fake[at:at+len(token)]=token
        for at in NORMALIZE_REL32_OFFSETS:
            fake[at:at+5]=b"\xe8"+struct.pack(
                "<i",ORIGINAL_ALLOCATOR_FREE-(va+at+5))
        raw.append(fake)
    assert audit(raw,False)["source_identical_except_two_relative_calls"]
    for i in range(3):
        for at,_ in SELECTED_WINDOWS:
            corrupted=[bytes(v) for v in raw]
            changed=bytearray(corrupted[i])
            changed[at]^=1
            corrupted[i]=bytes(changed)
            try:audit(corrupted,False)
            except ValueError:pass
            else:raise ValueError("damaged synthetic child method accepted")
        for at in NORMALIZE_REL32_OFFSETS:
            corrupted=[bytes(v) for v in raw]
            changed=bytearray(corrupted[i])
            changed[at+1]^=1
            corrupted[i]=bytes(changed)
            try:audit(corrupted,False)
            except ValueError:pass
            else:raise ValueError("damaged synthetic free target accepted")
    return {
        "original_free_three_soundscape_child_cleanup_audit":"PASS",
        "three_methods":len(METHODS),
        "relative_allocator_call_target_checks":6,
        "original_method_bytes_used":False,
    }
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    choices=ap.add_mutually_exclusive_group(required=True)
    choices.add_argument("--client",type=Path)
    choices.add_argument("--archive",type=Path)
    choices.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:out=self_test()
    elif args.client:out=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            out=private(z.read("client.dll"))
    print(json.dumps(out,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (ValueError,AssertionError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original three source child cleanup audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
