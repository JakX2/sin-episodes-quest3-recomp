#!/usr/bin/env python3
"""Phase 499: authenticate 2006 C_SoundscapeSystem constructor and vtable.

Original owner --client/--archive: exact SHA-pinned 120-byte ctor,
12-pointer vtable and 12-byte slot7 copy, plus selected instruction
witnesses and four independent one-byte RET handlers. No engine
functions are invoked and no proprietary source bodies are exported.
Public --self-test uses fabricated instruction windows only.
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
ORIGINAL_CTOR=0x240F3BC0
CTOR_BYTES=120
CTOR_SHA="0d9e47028fda189671b406f07549c147f3bfebebd24291e4460221c210ee96f0"
ORIGINAL_VTABLE=0x24293C34
VTABLE_BYTES=48
VTABLE_SHA="049c45c9002710aedfaef967c4c4aaebbc81bbc645528e4a5dea7c2735ed02b8"
ORIGINAL_SLOT7=0x240F3CA0
SLOT7_BYTES=12
SLOT7_SHA="f3271ecc3bb1ddf88123e9cdf1e011d9495c1790230626c6d67c1df782795565"
ORIGINAL_SUBOBJECT_VTABLE=0x2428A9B0
ORIGINAL_GLOBAL_POINTER=0x242FA458
ORIGINAL_VIRTUAL_SLOTS=(
    0x240F4160,0x240F3EE0,0x240F3DF0,0x240F3C40,
    0x240F3C80,0x240F3E10,0x240F3C90,0x240F3CA0,
    0x240F3CB0,0x240F3CC0,0x240F4D30,0x240F3E20,
)
DIRECT_RET_SLOTS=(4,6,8,9)
CTOR_WITNESSES=(
    (0x240F3BC0,"8bc1c700343c2924"),
    (0x240F3BC8,"33c9"),
    (0x240F3BCA,"894808"),
    (0x240F3BD0,"894810"),
    (0x240F3BDC,"894820"),
    (0x240F3BE8,"c74030b0a92824"),
    (0x240F3BEF,"83caff"),
    (0x240F3BF2,"89909c000000"),
    (0x240F3BF8,"8988a0000000"),
    (0x240F3C0A,"8988ac000000"),
    (0x240F3C1C,"8988b8000000"),
    (0x240F3C28,"8988c0000000"),
    (0x240F3C34,"895004"),
    (0x240F3C37,"c3"),
)
SLOT7_INSTRUCTIONS=(
    (0x240F3CA0,"a158a42f24"), # load global pointer
    (0x240F3CA5,"8b5004"), # source +4 dword
    (0x240F3CA8,"895104"), # write this +4
    (0x240F3CAB,"c3"), # return
)
def verify(method,start,size,sha,witnesses,whole):
    if len(method)!=size:
        raise ValueError("original soundscape method extent differs")
    if whole and hashlib.sha256(method).hexdigest()!=sha:
        raise ValueError("original soundscape method complete SHA mismatch")
    for va,raw in witnesses:
        pattern=bytes.fromhex(raw)
        if method[va-start:va-start+len(pattern)]!=pattern:
            raise ValueError("original source opcode mismatch at %#x"%va)
    return len(witnesses)
def verify_vtable(vtable,whole):
    if len(vtable)!=VTABLE_BYTES:
        raise ValueError("original soundscape vtable table size differs")
    if whole and hashlib.sha256(vtable).hexdigest()!=VTABLE_SHA:
        raise ValueError("original soundscape 12-slot vtable SHA mismatch")
    actual=struct.unpack("<12I",vtable)
    if actual!=ORIGINAL_VIRTUAL_SLOTS:
        raise ValueError("original soundscape vtable entries differ")
    return len(actual)
def audit(ctor,slot7,vtable,full):
    c=verify(ctor,ORIGINAL_CTOR,CTOR_BYTES,CTOR_SHA,CTOR_WITNESSES,full)
    g=verify(slot7,ORIGINAL_SLOT7,SLOT7_BYTES,SLOT7_SHA,
             SLOT7_INSTRUCTIONS,full)
    n=verify_vtable(vtable,full)
    return {
        "original_ctor_whole_120_byte_SHA_verified":bool(full),
        "original_slot7_whole_12_byte_SHA_verified":bool(full),
        "original_12_slot_vtable_SHA_verified":bool(full),
        "original_ctor_va":hex(ORIGINAL_CTOR),
        "original_vtable_va":hex(ORIGINAL_VTABLE),
        "original_slot7_va":hex(ORIGINAL_SLOT7),
        "original_init_slot":0,
        "original_shutdown_slot":1,
        "original_ICvar_lookup_slot":3,
        "source_single_ret_slots":list(DIRECT_RET_SLOTS),
        "original_slot7_reads_global_pointer_va":hex(ORIGINAL_GLOBAL_POINTER),
        "original_slot7_copies_global_plus_4_to_object_plus_4":True,
        "source_known_ctor_dword_zero_offsets":[
            hex(k) for k in list(range(0x8,0x30,4))+
                           list(range(0xA0,0xC8,4))],
        "source_known_ctor_minus_1_offsets":["0x4","0x9c"],
        "source_nested_subobject_vtable_va":hex(ORIGINAL_SUBOBJECT_VTABLE),
        "source_instruction_witnesses":c+g,
        "original_virtual_methods_authenticated":n,
        "original_object_size_known":False,
        "actual_engine_constructor_executed":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("SHA-256 original 2006 client.dll mismatch")
    pe=parse_image(client)
    if pe[0]!=0x24000000:
        raise ValueError("original PE32 image base differs")
    out=audit(
        region_at_va(client,pe,ORIGINAL_CTOR,CTOR_BYTES),
        region_at_va(client,pe,ORIGINAL_SLOT7,SLOT7_BYTES),
        region_at_va(client,pe,ORIGINAL_VTABLE,VTABLE_BYTES),
        True)
    for idx in DIRECT_RET_SLOTS:
        if region_at_va(client,pe,ORIGINAL_VIRTUAL_SLOTS[idx],1)!=b"\xc3":
            raise ValueError("original single RET method differs at slot %d"%idx)
    out.update(
        original_2006_client_whole_SHA_verified=True,
        original_four_single_ret_virtual_stubs_verified=True,
        original_private_binary_or_runtime_addresses_exported=False,
    )
    return out
def synthetic():
    fake_ctor=bytearray(bytes([0xCC])*CTOR_BYTES)
    fake_slot7=bytearray(bytes([0xCC])*SLOT7_BYTES)
    for base,buffer,witnesses in (
        (ORIGINAL_CTOR,fake_ctor,CTOR_WITNESSES),
        (ORIGINAL_SLOT7,fake_slot7,SLOT7_INSTRUCTIONS)):
        for va,raw in witnesses:
            pattern=bytes.fromhex(raw)
            buffer[va-base:va-base+len(pattern)]=pattern
    vtable=struct.pack("<12I",*ORIGINAL_VIRTUAL_SLOTS)
    r=audit(fake_ctor,fake_slot7,vtable,False)
    assert r["original_virtual_methods_authenticated"]==12
    assert len(r["source_known_ctor_dword_zero_offsets"])==20
    for base,buffer,witnesses,other in (
        (ORIGINAL_CTOR,fake_ctor,CTOR_WITNESSES,fake_slot7),
        (ORIGINAL_SLOT7,fake_slot7,SLOT7_INSTRUCTIONS,fake_ctor)):
        for va,_ in witnesses:
            damaged=bytearray(buffer)
            damaged[va-base]^=1
            try:
                if base==ORIGINAL_CTOR:audit(damaged,other,vtable,False)
                else:audit(other,damaged,vtable,False)
            except ValueError:pass
            else:raise ValueError("fabricated damaged soundscape method accepted")
    corrupt_vtable=bytearray(vtable)
    corrupt_vtable[7*4]^=1
    try:audit(fake_ctor,fake_slot7,corrupt_vtable,False)
    except ValueError:pass
    else:raise ValueError("fabricated swapped soundscape slot accepted")
    return {
        "source_free_soundscape_constructor_vtable_audit":"PASS",
        "original_free_constructor_selected_opcode_witnesses":len(CTOR_WITNESSES),
        "original_free_slot7_selected_opcode_witnesses":len(SLOT7_INSTRUCTIONS),
        "source_virtual_slot_count":len(ORIGINAL_VIRTUAL_SLOTS),
        "no_original_binary_or_audio_assets_loaded":True,
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
        print("FAIL source original Soundscape ctor/vtable audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
