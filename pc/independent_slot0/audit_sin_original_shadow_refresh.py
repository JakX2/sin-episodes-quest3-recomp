#!/usr/bin/env python3
"""Phase 491: source-authenticated 125-byte ShadowMgr linked refresh helper.

Private --client/--archive checks original SHA-pinned 2006 PE32 DLL.
Public --self-test fabricates all instruction windows, mutates key
branches and checks refusal. No original game code or assets published.
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

REFRESH=0x24115E80
REFRESH_SIZE=125
REFRESH_SHA="64b3d9cf23200b29329f83836b2507c243dfe9b49ac122aa86a2a5af2973dc66"
ACTIVE_HELPER=0x24113EA0
INACTIVE_HELPER=0x24113F60
FINALIZE_HELPER=0x24114A90
MARKERS=(
    (0x24115E84,"668b6f2c"), # head index low16 at object+0x2C
    (0x24115E88,"6681fdffff"), # 0xFFFF sentinel
    (0x24115E8D,"c687ce00000001"), # entry guard byte+0xCE=1
    (0x24115E9C,"c1e604"), # index times 16, then subtract idx
    (0x24115E9F,"2bf0"),
    (0x24115EA4,"03f603f603f6"), # then times 8 -> 120 bytes
    (0x24115EAC,"f6400801"), # active route bit1 at entry+8
    (0x24115EB4,"668b4004"), # external 16-bit ID at entry+4
    (0x24115EBE,"8b11"),
    (0x24115EC1,"ff5238"), # external virtual byte+0x38
    (0x24115EC9,"e8d2dfffff"), # active helper
    (0x24115ED3,"e888e0ffff"), # inactive helper
    (0x24115EDB,"668b6c0e76"), # next index entry+0x76
    (0x24115EE0,"6681fdffff"), # sentinel comparison again
    (0x24115EE8,"8d8fdc000000"), # object auxiliary this+0xDC
    (0x24115EEE,"e89debffff"), # finalization helper
    (0x24115EF3,"c687ce00000000"), # guard cleared
    (0x24115EFA,"5f5dc3"), # return
)
CALLS=(
    (0x24115EC9,ACTIVE_HELPER),
    (0x24115ED3,INACTIVE_HELPER),
    (0x24115EEE,FINALIZE_HELPER),
)
# These two authenticated original ShadowMgr methods call Refresh.
PARENT_CALLS=(
    (0x24115F2D,0x24115F00),
    (0x24115F4A,0x24115F40),
)
def validate_region(data,verify_hash=True):
    if len(data)!=REFRESH_SIZE:
        raise ValueError("original ShadowMgr refresh size mismatch")
    if verify_hash and hashlib.sha256(data).hexdigest()!=REFRESH_SHA:
        raise ValueError("original ShadowMgr refresh method SHA mismatch")
    for at,raw in MARKERS:
        op=bytes.fromhex(raw)
        if data[at-REFRESH:at-REFRESH+len(op)]!=op:
            raise ValueError("original shadow refresh opcode differs at %#x"%at)
    for at,target in CALLS:
        rel=target-at-5
        op=b"\xe8"+struct.pack("<i",rel)
        if data[at-REFRESH:at-REFRESH+5]!=op:
            raise ValueError("original shadow refresh CALL differs at %#x"%at)
    return {
        "original_full_refresh_SHA_verified":bool(verify_hash),
        "original_method_bytes":REFRESH_SIZE,
        "original_linked_entry_stride_bytes":120,
        "original_link_index_bits":16,
        "null_link_sentinel":65535,
        "head_field_offset":"0x2C",
        "entry_external_id_offset":"0x04",
        "entry_route_flag_offset":"0x08",
        "next_entry_offset":"0x76",
        "guard_field_offset":"0xCE",
        "guard_set_then_cleared":True,
        "finalize_helper_va":hex(FINALIZE_HELPER),
        "original_branch_helper_vas":[hex(ACTIVE_HELPER),hex(INACTIVE_HELPER)],
        "original_external_virtual_byte_offset":"0x38",
        "actual_engine_shadow_updates_performed":False
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("whole original 2006 client.dll SHA differs")
    pe=parse_image(client)
    if pe[0]!=0x24000000:
        raise ValueError("unexpected original image base")
    out=validate_region(region_at_va(client,pe,REFRESH,REFRESH_SIZE))
    for callsite,parent in PARENT_CALLS:
        # The enclosing virtual setter at 0x24115F00 / 0x24115F40
        # must call this exact refresh implementation via CALL rel32.
        src=region_at_va(client,pe,callsite,5)
        expect=b"\xe8"+struct.pack("<i",REFRESH-callsite-5)
        if src!=expect:
            raise ValueError("original shadow setter no longer invokes refresh")
    out.update(
        original_client_SHA_verified=True,
        two_SHA_verified_parent_setter_call_sites=True,
        user_supplied_original_binary_exported=False
    )
    return out
def self_test():
    fabricated=bytearray(b"\xcc"*REFRESH_SIZE)
    for va,raw in MARKERS:
        op=bytes.fromhex(raw)
        fabricated[va-REFRESH:va-REFRESH+len(op)]=op
    output=validate_region(fabricated,False)
    assert output["original_linked_entry_stride_bytes"]==120
    for corrupted_at in (
        0x24115E84,0x24115E8D,0x24115E9C,
        0x24115EA4,0x24115EAC,0x24115EC1,
        0x24115EC9,0x24115ED3,0x24115EDB,
        0x24115EEE,0x24115EF3
    ):
        broken=bytearray(fabricated)
        broken[corrupted_at-REFRESH]^=1
        try:validate_region(broken,False)
        except ValueError:pass
        else:raise ValueError("fabricated corrupt shadow refresh accepted")
    return {
        "original_free_shadow_refresh_audit":"PASS",
        "source_opcode_and_control_witnesses":len(MARKERS),
        "selected_direct_original_call_targets":len(CALLS),
        "no_original_dll_access":True
    }
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    g=ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--self-test",action="store_true")
    g.add_argument("--client",type=Path)
    g.add_argument("--archive",type=Path)
    args=ap.parse_args()
    if args.self_test:result=self_test()
    elif args.client:result=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            result=private(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original ShadowMgr linked-refresh SHA audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
