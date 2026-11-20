#!/usr/bin/env python3
"""Original SiN 2006 C_SoundscapeSystem vtable slot2 and slot11 audit.

--client/--archive: privately SHA-authenticate the original retail client
DLL and entire 22-byte slot2 wrapper and 76-byte slot11 destructor,
their original vtable entries and original static CALL/JMP targets.
--self-test: fabricate only instruction windows; no proprietary bytes.
This script NEVER invokes original runtime functions.
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

VTABLE=0x24293C34
SLOT2=0x240F3DF0
SLOT2_SIZE=22
SLOT2_SHA="f585618e3c7645a7729ed7b27d96f10ede980d57f439eb6672308c1b1db21331"
SLOT2_TAIL=0x240F37D0
SLOT11=0x240F3E20
SLOT11_SIZE=76
SLOT11_SHA="a46d83727243a32da487b7794d25b7fee70ef1ac8969a3d266dc9042e878e189"
CHILD_B4=0x240F3AD0
CHILD_A0=0x240F3A20
CHILD_ARRAY=0x240F3970
BASE_DTOR=0x24156820
OPTIONAL_FREE=0x2416FD60
SLOT2_WINDOWS=(
    (0x240F3DF0,"568bf1"),
    (0x240F3DF3,"8b06ff5004"), # invoke original *virtual slot1*
    (0x240F3DF8,"8b168bceff12"), # invoke *virtual slot0*
    (0x240F3DFE,"8bce5e"),
    (0x240F3E01,"e9caf9ffff"), # direct JMP at 0x240F37D0
)
SLOT11_WINDOWS=(
    (0x240F3E20,"568bf1"),
    (0x240F3E23,"8d8eb4000000"),
    (0x240F3E29,"c706343c2924"), # restore derived vptr
    (0x240F3E2F,"e89cfcffff"), # child B4
    (0x240F3E34,"8d8ea0000000"),
    (0x240F3E3A,"e8e1fbffff"), # child A0
    (0x240F3E3F,"8d4e1c"),
    (0x240F3E42,"e829fbffff"), # child 1C
    (0x240F3E47,"8d4e08"),
    (0x240F3E4A,"e821fbffff"), # child 08
    (0x240F3E4F,"8bce"),
    (0x240F3E51,"e8ca290600"), # base class destructor
    (0x240F3E56,"f644240801"), # source delete flag bit 0
    (0x240F3E5B,"7409"), # if bit0 clear skip delete
    (0x240F3E5D,"56"),
    (0x240F3E5E,"e8fdbe0700"), # original operator delete
    (0x240F3E63,"83c404"),
    (0x240F3E66,"8bc65e"),
    (0x240F3E69,"c20400"), # thiscall ret 4
)
DIRECT_CALLS=(
    (SLOT2,0x240F3E01,SLOT2_TAIL,0xE9),
    (SLOT11,0x240F3E2F,CHILD_B4,0xE8),
    (SLOT11,0x240F3E3A,CHILD_A0,0xE8),
    (SLOT11,0x240F3E42,CHILD_ARRAY,0xE8),
    (SLOT11,0x240F3E4A,CHILD_ARRAY,0xE8),
    (SLOT11,0x240F3E51,BASE_DTOR,0xE8),
    (SLOT11,0x240F3E5E,OPTIONAL_FREE,0xE8),
)
def inspect_body(data,start,size,expected_sha,witnesses,verify_whole):
    if len(data)!=size:
        raise ValueError("original 2006 soundscape wrapper method length differs")
    if verify_whole and hashlib.sha256(data).hexdigest()!=expected_sha:
        raise ValueError("original soundscape wrapper complete SHA mismatch")
    for va,op in witnesses:
        raw=bytes.fromhex(op)
        at=va-start
        if data[at:at+len(raw)]!=raw:
            raise ValueError("soundscape original wrapper opcode differs at %#x"%va)
    for base,site,target,op in DIRECT_CALLS:
        if base!=start:continue
        code=bytes([op])+struct.pack("<i",target-site-5)
        if data[site-start:site-start+5]!=code:
            raise ValueError("original wrapper CALL/JMP target differs at %#x"%site)
    return len(witnesses)
def audit(slot2,slot11,whole):
    a=inspect_body(slot2,SLOT2,SLOT2_SIZE,SLOT2_SHA,SLOT2_WINDOWS,whole)
    b=inspect_body(slot11,SLOT11,SLOT11_SIZE,SLOT11_SHA,
                   SLOT11_WINDOWS,whole)
    return {
        "original_slot2_whole_22_byte_sha_verified":bool(whole),
        "original_slot11_whole_76_byte_sha_verified":bool(whole),
        "original_slot2_va":hex(SLOT2),
        "original_slot11_va":hex(SLOT11),
        "original_slot2_call_order":["virtual-slot-1","virtual-slot-0",
                                     "tail-0x240F37D0"],
        "original_slot11_child_cleanup_order":[
            "offset-B4","offset-A0","offset-1C","offset-08"],
        "original_slot11_base_destroy_va":hex(BASE_DTOR),
        "original_slot11_optional_delete_va":hex(OPTIONAL_FREE),
        "original_slot11_delete_flag_bit0":True,
        "original_slot11_returns_object_pointer":True,
        "original_slot11_thiscall_ret4":True,
        "original_virtual_slot_calls_not_replaced_with_literal_direct_functions":True,
        "source_opcode_witnesses":a+b,
        "source_direct_call_targets_verified":len(DIRECT_CALLS),
        "original_runtime_or_engine_memory_touched":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("whole original retail client.dll SHA mismatch")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("original retail PE base mismatch")
    for slot,expected in ((2,SLOT2),(11,SLOT11)):
        va=VTABLE+4*slot
        actual=struct.unpack("<I",region_at_va(client,image,va,4))[0]
        if actual!=expected:
            raise ValueError("original source soundscape vtable slot %d mismatch"%slot)
    output=audit(
        region_at_va(client,image,SLOT2,SLOT2_SIZE),
        region_at_va(client,image,SLOT11,SLOT11_SIZE),True)
    output.update(
        original_client_whole_sha_authenticated=True,
        original_system_vtable_slots_2_and_11_authenticated=True,
        original_game_binary_bytes_exported=False)
    return output
def self_test():
    slot2=bytearray(bytes([0xCC])*SLOT2_SIZE)
    slot11=bytearray(bytes([0xCC])*SLOT11_SIZE)
    for base,buf,witnesses in (
        (SLOT2,slot2,SLOT2_WINDOWS),(SLOT11,slot11,SLOT11_WINDOWS)):
        for va,op in witnesses:
            raw=bytes.fromhex(op)
            buf[va-base:va-base+len(raw)]=raw
    out=audit(slot2,slot11,False)
    assert out["source_direct_call_targets_verified"]==7
    for base,buf,witnesses,other in (
        (SLOT2,slot2,SLOT2_WINDOWS,slot11),
        (SLOT11,slot11,SLOT11_WINDOWS,slot2)):
        for va,_ in witnesses:
            bad=bytearray(buf)
            bad[va-base]^=1
            try:
                if base==SLOT2:audit(bad,other,False)
                else:audit(other,bad,False)
            except ValueError:pass
            else:raise ValueError("damaged fabricated source method accepted")
    return {
        "original_free_soundscape_vtable_slot2_slot11_audit":"PASS",
        "fabricated_opcode_witnesses":len(SLOT2_WINDOWS)+len(SLOT11_WINDOWS),
        "fabricated_checked_direct_target_count":len(DIRECT_CALLS),
        "private_original_dll_used":False,
    }
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    args=ap.add_mutually_exclusive_group(required=True)
    args.add_argument("--client",type=Path)
    args.add_argument("--archive",type=Path)
    args.add_argument("--self-test",action="store_true")
    opt=ap.parse_args()
    if opt.self_test:out=self_test()
    elif opt.client:out=private(opt.client.read_bytes())
    else:
        with zipfile.ZipFile(opt.archive) as z:
            out=private(z.read("client.dll"))
    print(json.dumps(out,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (ValueError,AssertionError,FileNotFoundError,KeyError,
            UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL soundscape original slot2/slot11 audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
