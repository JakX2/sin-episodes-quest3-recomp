#!/usr/bin/env python3
"""Original 2006 SiN soundscape 223-byte auxiliary cleanup, vtable slot5.

Private --client/--archive SHA-verifies the original retail client.dll,
whole 223-byte helper at 0x240F3D10, 5-byte direct slot5 thunk at
0x240F3E10, selected source opcodes and both engine free CALL targets.
Public --self-test uses fabricated code bytes and mutated witnesses.
No original proprietary code body is published or invoked.
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

HELPER=0x240F3D10
HELPER_SIZE=223
HELPER_SHA="41ca09e9768fe5a683d57fdbe8ccac0e157ce557efc101e8e103aec3c804cfee"
THUNK=0x240F3E10
THUNK_SIZE=5
THUNK_SHA="9dddea87b38bb2b72eaa40f5738b6c57dbb1ef3fa62d3ca201da507a79fcaeec"
VTABLE=0x24293C34
ORIGINAL_FREE=0x2416F940
ORIGINAL_HANDLE_TABLE=0x242FA734
WINDOWS=(
    (0x240F3D10,"53568bf18b869c000000"),
    (0x240F3D20,"3bc3"),
    (0x240F3D24,"8b1534a72f24"),
    (0x240F3D2C,"81e1ff0f0000"),
    (0x240F3D39,"c1e80c"),
    (0x240F3D4B,"ff10"),
    (0x240F3D4D,"899e9c000000"),
    (0x240F3D53,"399e94000000"),
    (0x240F3D61,"ff12"),
    (0x240F3D63,"899e94000000"),
    (0x240F3D69,"399ea8000000"),
    (0x240F3D6F,"89beac000000"),
    (0x240F3D82,"e8b9bb0700"),
    (0x240F3D8A,"89bea0000000"),
    (0x240F3D90,"89bea4000000"),
    (0x240F3D9C,"8986b0000000"),
    (0x240F3DA2,"399ebc000000"),
    (0x240F3DA8,"89bec0000000"),
    (0x240F3DBB,"e880bb0700"),
    (0x240F3DC3,"89beb4000000"),
    (0x240F3DCF,"89beb8000000"),
    (0x240F3DD6,"898ec4000000"),
    (0x240F3DDE,"c3"),
    (0x240F3DE6,"8996c4000000"),
    (0x240F3DEE,"c3"),
)
CALL_SITES=(0x240F3D82,0x240F3DBB)
def audit(helper,thunk,whole):
    if len(helper)!=HELPER_SIZE or len(thunk)!=THUNK_SIZE:
        raise ValueError("original soundscape auxiliary method length mismatch")
    if whole:
        if hashlib.sha256(helper).hexdigest()!=HELPER_SHA:
            raise ValueError("original aux-cleanup full 223B method SHA mismatch")
        if hashlib.sha256(thunk).hexdigest()!=THUNK_SHA:
            raise ValueError("original aux-cleanup slot5 thunk SHA mismatch")
    for site,raw in WINDOWS:
        pattern=bytes.fromhex(raw)
        at=site-HELPER
        if helper[at:at+len(pattern)]!=pattern:
            raise ValueError("original source auxiliary cleanup witness changed at %#x"%site)
    for site in CALL_SITES:
        rel=ORIGINAL_FREE-site-5
        if helper[site-HELPER:site-HELPER+5]!=b"\xe8"+struct.pack("<i",rel):
            raise ValueError("source original allocator free CALL target changed")
    if thunk!=b"\xe9"+struct.pack("<i",HELPER-THUNK-5):
        raise ValueError("source original slot5 tail jump target changed")
    return {
        "original_aux_cleanup_full_sha_verified":bool(whole),
        "original_slot5_thunk_full_sha_verified":bool(whole),
        "original_aux_cleanup_bytes":HELPER_SIZE,
        "original_aux_cleanup_va":hex(HELPER),
        "original_slot5_va":hex(THUNK),
        "original_allocator_free_va":hex(ORIGINAL_FREE),
        "original_packed_handle_table_global_va":hex(ORIGINAL_HANDLE_TABLE),
        "source_handle_9c_index_mask":"0xFFF",
        "source_handle_9c_generation_shift":12,
        "source_handle_94_distinct_sentinel_only_check":True,
        "source_array_flag_offsets":["0xA8","0xBC"],
        "source_always_reset_count_offsets":["0xAC","0xC0"],
        "source_optional_array_buffer_offsets":["0xA0","0xB4"],
        "source_array_mirror_offsets":["0xB0","0xC4"],
        "source_selected_opcode_witnesses":len(WINDOWS),
        "two_direct_allocator_free_call_targets_checked":True,
        "original_memory_or_engine_handle_calls_performed":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("original user retail 2006 client.dll SHA mismatch")
    pe=parse_image(client)
    if pe[0]!=0x24000000:
        raise ValueError("original client.dll image base mismatch")
    slot5=struct.unpack("<I",region_at_va(client,pe,VTABLE+20,4))[0]
    if slot5!=THUNK:
        raise ValueError("original SoundscapeSystem vtable slot5 changed")
    result=audit(region_at_va(client,pe,HELPER,HELPER_SIZE),
                 region_at_va(client,pe,THUNK,THUNK_SIZE),True)
    result["original_client_whole_sha_authenticated"]=True
    result["original_system_vtable_slot5_authenticated"]=True
    result["original_binary_bytes_exported"]=False
    return result
def synthetic():
    fake=bytearray(bytes([0xCC])*HELPER_SIZE)
    for site,raw in WINDOWS:
        token=bytes.fromhex(raw)
        fake[site-HELPER:site-HELPER+len(token)]=token
    thunk=b"\xe9"+struct.pack("<i",HELPER-THUNK-5)
    out=audit(fake,thunk,False)
    assert out["source_selected_opcode_witnesses"]==len(WINDOWS)
    for site,_ in WINDOWS:
        damaged=bytearray(fake)
        damaged[site-HELPER]^=1
        try:audit(damaged,thunk,False)
        except ValueError:pass
        else:raise ValueError("corrupted fabricated auxiliary method accepted")
    damaged_thunk=bytearray(thunk)
    damaged_thunk[1]^=1
    try:audit(fake,damaged_thunk,False)
    except ValueError:pass
    else:raise ValueError("corrupted fabricated source slot5 JMP accepted")
    return {
        "original_free_soundscape_aux_cleanup_audit":"PASS",
        "selected_source_opcode_witnesses":len(WINDOWS),
        "two_source_allocator_call_targets_verified":True,
        "source_slot5_thunk_verified":True,
        "private_original_binary_bytes_used":False,
    }
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    group.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:result=synthetic()
    elif args.client:result=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            result=private(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (ValueError,AssertionError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original soundscape auxiliary cleanup audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
