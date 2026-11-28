#!/usr/bin/env python3
"""Phase1721: authenticate seven exceptional original entity creator methods.

Completes the ORIGINAL source entrypoint classification of all 149
registered entity creators: 142 shared zeroing prologues; TWO
nonstandard calls of the same original zero wrapper; ONE direct
original allocator+manual zero; THREE raw allocations through
0x2416FCF0; and ONE CWorld global object singleton path.
Source-only data and tests do NOT create executable 2006 PE32
entities or reconstruct object constructors/vtables/ownership.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path
import re
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_entity_allocation_prologues import (
    original_free as source_common_original_free,
)

SOURCE_EXCEPTIONS=7
ENTITY_CLASSES=149
COMMON_ALLOCATION_CALLS=142
REQUESTED_ALLOCATION_COUNT=148
SOURCE_SHA="24b00c39fbf90a0af79448496cd4f1d91221dc752dc0021ae6a755c7f77f38e7"
CSV_COLUMNS=[
    "class_ordinal","network_class","creator_va","path_kind",
    "allocation_request_bytes","original_allocator_or_global_va",
    "returned_subobject_offset","original_virtual_call_offset",
    "complete_native_function_bytes","complete_native_function_sha256",
    "native_int3_padding_bytes"
]
ENUMS={
    "shared_zero_wrapper":"SharedZeroingHelper",
    "direct_allocator_manual_zero":"DirectAllocatorManualZero",
    "raw_allocator":"RawAllocator",
    "global_singleton":"ExistingGlobalSingleton"
}
# Exact original linked-list ordinal/name, complete-function length, full
# SHA256 and original native RET/INT3 boundary for these seven functions.
EXACT=(
    (48,"CInfoTeleporterCountdown",110,
     "31f3c9ce26de347771a36aa612f05c6bcc906d1b333bc7daa110e080c13e6af7",2),
    (69,"CRagdollPropAttached",116,
     "17e5a312ac4a62a1a95cb860f30d2627827ca0dc5701603c81ddc9e5eae8d7f0",12),
    (75,"CWorld",40,
     "8b06decbe2712135eef0780b69446b5147fbf52c618fa78e7c46aa3fcffde274",8),
    (120,"CBaseEntity",76,
     "39fdcf99ddac56ecd5122ce124fbf2926770133608afafc409fb660f3f7d2cd6",4),
    (143,"CTEParticleSystem",88,
     "810446d4eb05b2cd19933a330b752d64a23bb254ef8a8442162d79bbdbb439a1",8),
    (170,"CTEBaseBeam",54,
     "2cc291214fa7a1202ad321c0595bc1618d828524278bb343342b16ef22ab5b53",10),
    (190,"CBaseTempEntity",82,
     "5dbce3a6852f0b166bf734c76a594ff6f08644c2765e201e1431ab58bbe66ed7",14)
)

# Original source bytes indicating return of an *interior networkable*
# subobject, not the original allocation base or a full host pointer.
RETURN_WITNESSES={
    48:"8d7e08", 69:"8d7e08",75:"83c008",120:"8d4608",
    143:"8d7e04",170:"8d4604",190:"8d7004"
}

def need(yes:bool,message:str)->None:
    if not yes:raise ValueError(message)


def verify_public(manifest:Path,cpp_header:Path,creator_catalog:Path,
                  common_manifest:Path,common_header:Path):
    raw=manifest.read_bytes().replace(b"\r\n",b"\n")
    need(hashlib.sha256(raw).hexdigest()==SOURCE_SHA,
         "source seven-creator CSV SHA256 mismatch")
    with manifest.open(newline="",encoding="utf-8") as f:
        reader=csv.DictReader(f)
        need(reader.fieldnames==CSV_COLUMNS,
             "source seven exceptional creator schema changed")
        rows=list(reader)
    need(len(rows)==SOURCE_EXCEPTIONS,
         "seven original creator exceptions incomplete")
    with creator_catalog.open(newline="",encoding="utf-8") as f:
        cls=list(csv.DictReader(f))
    need(len(cls)==191,"source 191 original ClientClass catalog changed")
    common=source_common_original_free(
        common_manifest,creator_catalog,common_header)
    known={int(r["class_ordinal"]) for r in common}
    expected=[i for i,x in enumerate(cls)
              if x["kind"]=="E" and i not in known]
    need(len(expected)==SOURCE_EXCEPTIONS and
         expected==[r[0] for r in EXACT],
         "source seven exceptions do not complement 142 common creators")

    contents=cpp_header.read_text("utf-8")
    match=re.search(r"kSevenNativeSourceExceptions\s*\{\{(.*?)\}\};",
                    contents,re.S)
    need(match is not None,"native C++ exceptional source array absent")
    cpp_items=re.findall(
        r"\{(\d+)u,0x([0-9a-f]{8})u,CreatorPath::(\w+),"
        r"(\d+)u,0x([0-9a-f]{8})u,(\d+)u,(\d+)u,(\d+)u\}",
        match.group(1))
    need(len(cpp_items)==SOURCE_EXCEPTIONS,
         "native C++ exceptional source array incomplete")
    sizes=0
    singleton=0
    for i,(r,expected_source,cpp) in enumerate(zip(rows,EXACT,cpp_items)):
        ordinal=int(r["class_ordinal"])
        source_va=int(r["creator_va"],16)
        n=int(r["allocation_request_bytes"])
        allocator=int(r["original_allocator_or_global_va"],16)
        return_offset=int(r["returned_subobject_offset"])
        virtual_offset=int(r["original_virtual_call_offset"])
        byte_length=int(r["complete_native_function_bytes"])
        padding=int(r["native_int3_padding_bytes"])
        kind=r["path_kind"]
        need((ordinal,r["network_class"],byte_length,
              r["complete_native_function_sha256"],padding)==expected_source,
             f"whole source SHA/creator identity differs at row {i}")
        need(cls[ordinal]["kind"]=="E" and
             cls[ordinal]["network_class"]==r["network_class"] and
             int(cls[ordinal]["creator_va"],16)==source_va,
             f"original CRT entity identity differs at {ordinal}")
        need(kind in ENUMS and
             (ordinal,source_va,ENUMS[kind],n,allocator,return_offset,
              virtual_offset,byte_length)==
             (int(cpp[0]),int(cpp[1],16),cpp[2],int(cpp[3]),
              int(cpp[4],16),int(cpp[5]),int(cpp[6]),int(cpp[7])),
             f"source C++ seven-variant ABI metadata drift at {ordinal}")
        if kind=="global_singleton":
            singleton+=1
            need(ordinal==75 and n==0 and allocator==0x2433F3F4
                 and return_offset==8 and virtual_offset==0x94,
                 "original CWorld source global path changed")
        else:
            sizes+=1
            need(n in (16,28,76,1152,1168,3408),
                 "unknown alternative original object size")
            target={
                "shared_zero_wrapper":0x240C4FF0,
                "direct_allocator_manual_zero":0x2416F8F0,
                "raw_allocator":0x2416FCF0
            }[kind]
            need(allocator==target,
                 "original exceptional creator allocation delegate changed")
    need(sizes==6 and singleton==1 and
         sizes+len(common)==REQUESTED_ALLOCATION_COUNT and
         sum(x["kind"]=="E" for x in cls)==ENTITY_CLASSES,
         "source complete 149-entity family coverage wrong")
    return rows


def verify_private(path:Path,rows):
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original unchanged 2006 owner PE32 client.dll whole hash differs")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def read(va,n):
        at=va-base
        need(n>=0 and 0<=at<=len(code)-n,
             "original exceptional creator outside original .text")
        return code[at:at+n]
    for r in rows:
        ord=int(r["class_ordinal"])
        va=int(r["creator_va"],16)
        length=int(r["complete_native_function_bytes"])
        digest=r["complete_native_function_sha256"]
        padding=int(r["native_int3_padding_bytes"])
        source=read(va,length)
        need(hashlib.sha256(source).hexdigest()==digest,
             f"whole original exceptional creator method SHA changed {va:#x}")
        need(source[-1]==0xC3 and read(va+length,padding)==b"\xCC"*padding,
             f"original function RET/native alignment boundary differs {va:#x}")
        need(bytes.fromhex(RETURN_WITNESSES[ord]) in source,
             f"original networkable subobject return adjustment changed {va:#x}")
        virtual=int(r["original_virtual_call_offset"])
        if virtual==0x94:
            need((b"\xFF\x90\x94\x00\x00\x00" in source or
                  b"\xFF\x92\x94\x00\x00\x00" in source),
                 f"source entity creator virtual+0x94 dispatch changed {va:#x}")
        else:
            need((b"\xFF\x50\x30" in source or b"\xFF\x52\x30" in source),
                 f"source event creator virtual+0x30 dispatch changed {va:#x}")
        kind=r["path_kind"]
        if kind=="global_singleton":
            need(source[:10]==bytes.fromhex("8b5424088b0df4f33324") and
                 b"\x83\xC0\x08" in source,
                 "source original CWorld global singleton path changed")
            continue
        if ord in (48,69,120):
            offset=7
            need(source[2]==0x68 and source[7]==0xE8,
                 "original exceptional PUSH DWORD SIZE/E8 callsite changed")
            size,=struct.unpack_from("<I",source,3)
        elif ord in (143,170):
            offset=3
            need(source[1]==0x6A and source[3]==0xE8,
                 "original temp-event PUSH BYTE SIZE/E8 changed")
            size=source[2]
        elif ord==190:
            offset=2
            need(source[0]==0x6A and source[2]==0xE8,
                 "original CBaseTempEntity PUSH BYTE SIZE/E8 changed")
            size=source[1]
        else:
            raise ValueError(f"unrecognized special creator source ordinal {ord}")
        displacement,=struct.unpack_from("<i",source,offset+1)
        destination=va+offset+5+displacement
        need(size==int(r["allocation_request_bytes"]) and
             destination==int(r["original_allocator_or_global_va"],16),
             f"original exceptional allocator size/target changed {va:#x}")
        if ord==120:
            need(b"\xF3\xAB" in source,
                 "source CBaseEntity direct allocation zero DWORDs missing")
    print("PASS Phase1721 owner-original all 7 complete exceptional creator "
          "functions SHA/RET/INT3, 6 allocations + CWorld singleton, "
          "native return-subobject offsets and virtual call instructions")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_seven_creator_exceptions_phase1721.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_seven_creator_exceptions.hpp"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--common-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--common-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    rows=verify_public(args.manifest,args.header,args.creator_catalog,
                       args.common_manifest,args.common_header)
    if args.self_test:
        for rec in rows:
            length=int(rec["complete_native_function_bytes"])
            sha=rec["complete_native_function_sha256"]
            need(hashlib.sha256(b"\x90"*length).hexdigest()!=sha,
                 "fabricated x86 entity creator method falsely authenticated")
        print("PASS Phase1721 original-free all 149 source entity creator "
              "entrypoint strategies: 142 common + 6 custom allocations "
              "+ one CWorld global singleton; source CSV/CPP integrity")
    if args.client is not None:verify_private(args.client,rows)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1721: {exc}",file=sys.stderr)
        sys.exit(1)
