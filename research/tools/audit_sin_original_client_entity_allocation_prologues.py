#!/usr/bin/env python3
"""Phase1718: audit source-original 142 entity creator allocation prologues.

Do not mistake PUSH ESI; PUSH SIZE; CALL source allocator for a
functional entity creator. The private --client path authenticates
only the FIRST ELEVEN native executable bytes at each original
creator function, plus the whole user-owned PE32 SHA. Complete
constructors/vtables/destructors/engine allocator are NOT ported.
The public --self-test checks source manifest, class order, and
C++ constexpr source map, without original game executable bytes.
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

TOTAL_CLASSES=191
ORIGINAL_ENTITIES=149
SHARED_ALLOCATION_CREATORS=142
EXCEPTION_CREATORS=7
ORIGINAL_TEMP_EVENTS=42
DISTINCT_SIZES=69
LOWEST_SIZE=1152
LARGEST_SIZE=10048
PROLOGUE_BYTES=11
ORIGINAL_ALLOCATION_HELPER_VA=0x240C4FF0
CSV_SHA="68fe5b23a33cd1b724fe9aa3761c667ef63333efcbe7c4c3afc32eb13dfa5c0e"
PROLOGUES_SHA="3562d9fedfa0db8a3e570ef023130e519cfcc3c48c2248c5b0c6ce0aa059c1ba"
EXPECTED_HEADER=[
    "class_ordinal","network_class","creator_va","object_size_bytes",
    "original_allocator_va","original_size_push_va",
    "original_allocator_call_va","crt_index"
]
EXCEPTION_CLASSES=(
    (48,"CInfoTeleporterCountdown"),
    (69,"CRagdollPropAttached"),
    (75,"CWorld"),
    (120,"CBaseEntity"),
    (143,"CTEParticleSystem"),
    (170,"CTEBaseBeam"),
    (190,"CBaseTempEntity"),
)


def need(predicate,message):
    if not predicate:
        raise ValueError(message)


def original_free(manifest:Path,classes:Path,header:Path):
    raw=manifest.read_bytes().replace(b"\r\n",b"\n")
    need(hashlib.sha256(raw).hexdigest()==CSV_SHA,
         "public 142 native size/allocator source CSV SHA mismatch")
    with manifest.open(newline="",encoding="utf-8") as inp:
        reader=csv.DictReader(inp)
        need(reader.fieldnames==EXPECTED_HEADER,
             "source allocation prologue CSV fields changed")
        rows=list(reader)
    need(len(rows)==SHARED_ALLOCATION_CREATORS,
         "source allocation record count not 142")
    with classes.open(newline="",encoding="utf-8") as inp:
        cls=list(csv.DictReader(inp))
    need(len(cls)==TOTAL_CLASSES,"source CreatorClass registry changed")
    source_cpp=header.read_text("utf-8")
    match=re.search(r"kOriginalAllocationPrologues\s*\{\{(.*?)\}\};",
                    source_cpp,re.S)
    need(match is not None,"native C++ original source size array missing")
    cpp_entries=re.findall(
        r"\{(\d+)u,(\d+)u,0x([0-9a-f]{8})u\}",
        match.group(1))
    need(len(cpp_entries)==SHARED_ALLOCATION_CREATORS,
         "C++ source allocation list incomplete")
    sizes=set()
    seen=set()
    last_ordinal=-1
    for i,rec in enumerate(rows):
        ordinal=int(rec["class_ordinal"])
        nativeVA=int(rec["creator_va"],16)
        objsize=int(rec["object_size_bytes"])
        allocator=int(rec["original_allocator_va"],16)
        size_push=int(rec["original_size_push_va"],16)
        call=int(rec["original_allocator_call_va"],16)
        need(last_ordinal<ordinal<TOTAL_CLASSES,
             "original constructor list order invalid")
        last_ordinal=ordinal
        need(ordinal not in seen,
             "duplicate source native entity CreatorClass ordinal")
        seen.add(ordinal)
        native=cls[ordinal]
        need(native["kind"]=="E" and
             native["network_class"]==rec["network_class"] and
             int(native["creator_va"],16)==nativeVA and
             native["crt_index"]==rec["crt_index"],
             f"owner source CRT/class/creator mismatch at ordinal {ordinal}")
        need(allocator==ORIGINAL_ALLOCATION_HELPER_VA and
             size_push==nativeVA+1 and call==nativeVA+6,
             "source allocator helper or prologue callsite changed")
        need(LOWEST_SIZE<=objsize<=LARGEST_SIZE and objsize%4==0,
             "source object request size out of original bounds")
        cpp_ordinal,cpp_size,cpp_creator=cpp_entries[i]
        need(int(cpp_ordinal)==ordinal and
             int(cpp_size)==objsize and
             int(cpp_creator,16)==nativeVA,
             f"original C++ source metadata changed at row {i}")
        sizes.add(objsize)
    exceptions=tuple(
        (i,rec["network_class"]) for i,rec in enumerate(cls)
        if rec["kind"]=="E" and i not in seen)
    need(len(seen)==SHARED_ALLOCATION_CREATORS and
         exceptions==EXCEPTION_CLASSES and
         sum(rec["kind"]=="T" for rec in cls)==ORIGINAL_TEMP_EVENTS and
         sum(rec["kind"]=="E" for rec in cls)==ORIGINAL_ENTITIES,
         "source entity allocation/helper exceptions changed")
    need(len(sizes)==DISTINCT_SIZES and min(sizes)==LOWEST_SIZE and
         max(sizes)==LARGEST_SIZE,
         "source entity allocation size distribution changed")
    return rows


def original_private(original:Path,rows):
    binary=original.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "user original 2006 PE32 client.dll whole SHA mismatch")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    prologues=b""
    for row in rows:
        function=int(row["creator_va"],16)
        at=function-base
        need(0<=at<=len(code)-PROLOGUE_BYTES,
             "original entity creator source VA outside native .text")
        original=code[at:at+PROLOGUE_BYTES]
        need(original[:2]==b"\x56\x68" and original[6]==0xE8,
             f"source entity creator PUSH ESI/PUSH SIZE/CALL drift {function:#x}")
        size,=struct.unpack_from("<I",original,2)
        displacement,=struct.unpack_from("<i",original,7)
        target=function+PROLOGUE_BYTES+displacement
        need(size==int(row["object_size_bytes"]) and
             target==ORIGINAL_ALLOCATION_HELPER_VA,
             f"original x86 request bytes or CALL allocator target drift {function:#x}")
        prologues+=original
    need(hashlib.sha256(prologues).hexdigest()==PROLOGUES_SHA,
         "concatenated 142 real 11-byte creator prologue SHA256 mismatch")
    print("PASS Phase1718 owner-original 142 x86 entity creator initial "
          "PUSH ESI/PUSH exact SIZE/CALL 0x240C4FF0 signatures, "
          "69 object request sizes; 7 distinct entities excluded")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    rows=original_free(args.manifest,args.creator_catalog,args.header)
    if args.self_test:
        need(hashlib.sha256(b"\x90"*(PROLOGUE_BYTES*SHARED_ALLOCATION_CREATORS))
             .hexdigest()!=PROLOGUES_SHA,
             "fabricated 142 original executable prologues falsely authenticated")
        print("PASS Phase1718 source-only 142 original entity allocator "
              "prologues, 7 explicit exceptions, original CSV/CPP SHA")
    if args.client is not None:original_private(args.client,rows)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1718: {exc}",file=sys.stderr)
        sys.exit(1)
