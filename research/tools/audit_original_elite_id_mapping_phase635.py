#!/usr/bin/env python3
"""Original 2006 SiN Elite virtual+0x760 ID-range converter (Phase635).

Authenticates ENTIRE 58-byte retail server.dll function at 0x22073700,
the two original Elite direct-call edges from Phase634, 20 original opcode
witnesses and both retail DLL digests. No original machine code committed.
Use --self-test for invented-byte public regression checks only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256
)

VA=0x22073700
SIZE=58
SHA256="5927384b29bc8c8ce0525ca759560659f83a87d2095bc051d0a1763c97df95c9"
THUNK_VA=0x22355D40
THUNK_SIZE=6
THUNK_SHA256="e80e4e201bee1506627b153171f783c06f9433cfee905f62d08a2dc048dbcc62"
ELITE_VTABLE=0x2248DF04
VIRTUAL760_OFFSET=0x760
MAP_OWNER_VA=0x2259C2F8
FIRST_NODE_VA=MAP_OWNER_VA+0x34
CALL_EDGES=(
    (0x220398C3,VA,"Elite original flag-clear source-ID conversion"),
    (0x2203D2EE,VA,"Elite original state0 ID 0x21 conversion")
)
WITNESSES=(
    (0x22073700,"8b542404"), # original [ESP+4] source ID
    (0x22073704,"83faff"), # source ID == -1 early return
    (0x22073707,"7421"), # immediate skip entire linked list
    (0x22073709,"8da42400000000"), # 7-byte alignment nop
    (0x22073710,"8b4104"), # current entry source low [+4]
    (0x22073713,"3d0f270000"), # sentinel low == 0x270f (9999)
    (0x22073718,"7409"), # sentinel skips current entry
    (0x2207371A,"3bd0"), # source id < low, signed
    (0x2207371C,"7c05"),
    (0x2207371E,"3b5108"), # source id <= high [+8], signed
    (0x22073721,"7e0d"), # inclusive upper bound
    (0x22073723,"8b4910"), # next linked entry [+0x10]
    (0x22073726,"85c9"), # null next terminates
    (0x22073728,"75e6"), # continue loop
    (0x2207372A,"83c8ff"), # not-found -1
    (0x2207372D,"c20400"),
    (0x22073730,"8b01"), # mapped destination base [+0]
    (0x22073732,"2b4104"), # subtract source low
    (0x22073735,"03c2"), # add incoming input (x86 wrap)
    (0x22073737,"c20400")
)
CALLER_WITNESSES=(
    (0x220398C0,"8d4834"), # virtual+0x760 result +0x34
    (0x2203D2E9,"6a21"), # literal state-test ID 0x21
    (0x2203D2EB,"8d4834"), # second caller same +0x34 context
)
def whole_hash(read,va=VA,size=SIZE,digest=SHA256):
    raw=read(va,size)
    if len(raw)!=size or hashlib.sha256(raw).hexdigest()!=digest:
        raise VerificationError(f"original range translator complete {size}-byte SHA256 mismatch at {va:#x}")
def check_opcode(read,va,hexcode):
    target=bytes.fromhex(hexcode)
    if read(va,len(target))!=target:
        raise VerificationError(f"original range translator opcode mismatch {va:#x}")
def check_call(read,site,target):
    raw=read(site,5)
    if len(raw)!=5 or raw[0]!=0xe8 or (
        site+5+struct.unpack_from("<i",raw,1)[0]!=target
    ):
        raise VerificationError(f"original Elite range translator caller changed {site:#x}")
def verify(read):
    whole_hash(read)
    whole_hash(read,THUNK_VA,THUNK_SIZE,THUNK_SHA256)
    if read(THUNK_VA,1)!=b"\xb8" or read(THUNK_VA+5,1)!=b"\xc3":
        raise VerificationError("original Elite vtable +0x760 thunk is not MOV imm32 / RET")
    if struct.unpack("<I",read(THUNK_VA+1,4))[0]!=MAP_OWNER_VA:
        raise VerificationError("original Elite static mapping owner identity changed")
    vslot=ELITE_VTABLE+VIRTUAL760_OFFSET
    if struct.unpack("<I",read(vslot,4))[0]!=THUNK_VA:
        raise VerificationError("original Elite vtable +0x760 entry changed")
    for site,target,_ in CALL_EDGES:
        check_call(read,site,target)
    for site,hexcode in WITNESSES+CALLER_WITNESSES:
        check_opcode(read,site,hexcode)
def verify_original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("both exact original SiN 2006 client/server DLL hashes required")
    pe=PEImage(server)
    if pe.base!=0x22000000 or pe.section_of(VA)!=".text" or (
        pe.section_of(THUNK_VA)!=".text") or (
        pe.section_of(ELITE_VTABLE+VIRTUAL760_OFFSET)!=".rdata") or any(
        pe.section_of(site)!=".text" for site,_,_ in CALL_EDGES
    ):
        raise VerificationError("original SiN PE32 x86 image/section identity mismatch")
    data_section=next((section for section in pe.sections
                       if section[0]==".data"),None)
    if data_section is None:
        raise VerificationError("original PE .data section missing")
    _,data_rva,virtual_size,_,raw_size=data_section
    owner_offset=MAP_OWNER_VA-pe.base-data_rva
    first_node_offset=FIRST_NODE_VA-pe.base-data_rva
    if not (raw_size<=owner_offset<first_node_offset<virtual_size):
        raise VerificationError("original mapping owner must be in .data virtual uninitialized tail")
    verify(pe.at)
    return {
        "status":"ORIGINAL_2006_ELITE_ID_MAPPING_PHASE635_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "original_converter_va":hex(VA),
        "entire_method_bytes":SIZE,
        "entire_method_sha256":SHA256,
        "second_complete_original_method":{
            "role":"Elite primary virtual slot +0x760 static mapping owner getter",
            "vtable_slot_va":hex(ELITE_VTABLE+VIRTUAL760_OFFSET),
            "native_thunk_va":hex(THUNK_VA),
            "full_method_bytes":THUNK_SIZE,
            "full_method_sha256":THUNK_SHA256,
            "static_mapping_owner_va":hex(MAP_OWNER_VA),
            "first_embedded_range_node_va":hex(FIRST_NODE_VA)
        },
        "new_complete_original_functions":2,
        "new_complete_original_executable_bytes":SIZE+THUNK_SIZE,
        "mapping_owner_file_section":".data",
        "mapping_owner_has_raw_file_bytes":False,
        "mapping_owner_proven_uninitialized_virtual_tail":True,
        "verified_original_direct_calls":[
            {"site":hex(site),"target":hex(target),"role":role}
            for site,target,role in CALL_EDGES
        ],
        "source_opcode_witnesses":len(WITNESSES),
        "caller_opcode_witnesses":len(CALLER_WITNESSES),
        "source_range_layout":{
            "destination_first_offset":"0x0",
            "source_low_offset":"0x4",
            "source_high_inclusive_offset":"0x8",
            "next_entry_offset":"0x10",
            "sentinel_low":9999,
            "signed_range_comparisons":True,
            "arithmetic_wrap":"modulo 2^32",
            "no_match":-1
        },
        "vtable760_result_node_offset":"0x34",
        "owner_original_game_executed":False,
        "original_mapping_table_content_verified":False,
        "replacement_engine_callback_observed":False
    }
def self_test():
    fabricated=bytes(((i*47+11)&255) for i in range(131))
    sha=hashlib.sha256(fabricated).hexdigest()
    whole_hash(lambda _va,n:fabricated[:n],0x71001000,len(fabricated),sha)
    invented_thunk=b"\xb8"+struct.pack("<I",0x71234000)+b"\xc3"
    invented_sha=hashlib.sha256(invented_thunk).hexdigest()
    whole_hash(lambda _va,n:invented_thunk[:n],
               0x71001100,len(invented_thunk),invented_sha)
    for byte_index in range(len(invented_thunk)):
        changed=bytearray(invented_thunk)
        changed[byte_index]^=1
        try:whole_hash(lambda _va,n:bytes(changed[:n]),
                       0x71001100,len(invented_thunk),invented_sha)
        except VerificationError:pass
        else:raise AssertionError("accepted mutated invented original mapping owner thunk")
    for i in range(len(fabricated)):
        changed=bytearray(fabricated);changed[i]^=1
        try:
            whole_hash(lambda _va,n:bytes(changed[:n]),
                       0x71001000,len(fabricated),sha)
        except VerificationError:pass
        else:raise AssertionError("accepted mutated invented complete native method")
    site=0x71002000;dest=0x71002400
    call=b"\xe8"+struct.pack("<i",dest-site-5)
    check_call(lambda _va,n:call[:n],site,dest)
    for i in range(len(call)):
        changed=bytearray(call);changed[i]^=1
        try:check_call(lambda _va,n:bytes(changed[:n]),site,dest)
        except VerificationError:pass
        else:raise AssertionError("accepted corrupt invented direct call")
    check_opcode(lambda _va,n:b"\x3d\x0f\x27\x00\x00"[:n],
                 0x71001000,"3d0f270000")
    try:
        check_opcode(lambda _va,n:b"\x3d\x00\x27\x00\x00"[:n],
                     0x71001000,"3d0f270000")
    except VerificationError:pass
    else:raise AssertionError("accepted modified 9999 sentinel")
    try:verify_original(b"wrong server",b"wrong client")
    except VerificationError:pass
    else:raise AssertionError("accepted unverified original binaries")
    print("PASS Phase635 original-free two complete method hashes, mapping-owner thunk, sentinel and direct-call mutation refusal")
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    if args.self_test:
        self_test();return 0
    if args.server is None or args.client is None:
        parser.error("provide exact original --server/--client DLLs or --self-test")
    try:
        report=verify_original(args.server.read_bytes(),args.client.read_bytes())
    except (VerificationError,OSError,ValueError,struct.error) as error:
        parser.exit(1,"PHASE635 ORIGINAL DLL AUDIT REFUSED: "+str(error)+"\n")
    payload=json.dumps(report,indent=2)+"\n"
    if args.output:args.output.write_text(payload,encoding="utf-8")
    else:print(payload,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
