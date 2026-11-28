#!/usr/bin/env python3
"""Phase1717: authenticate all 42 original 2006 PE32 static temp-event getters.

The original 42 temp-event creator functions consist of exactly
MOV EAX,<original static .data pointer>; RET, six bytes each, with
10 original INT3 padding bytes. This audits the exact original bytes
read-only in private --client mode; public CI checks source metadata
with pinned independent SHA256 and original class registration order.
No original executable bytes or static object pointers are emitted
as callable runtime objects.
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
from audit_sin_original_client_class_registry import verify as original_registry_verify

SOURCE_FUNCTIONS=42
FUNCTION_BYTES=6
SOURCE_DATA_START=0x242F3000
SOURCE_DATA_END=0x2438F320
PUBLIC_CSV_SHA="b40182b4b5257a02bcce50b85da6123665b551b2f6615b3f759e0dccc23f4b50"
WHOLE_42_SOURCE_METHODS_SHA="266efe94cd2323cd097a9ba2af229d5f57ca547889e4dc0fe244a04374a4db30"
CSV_FIELDS=[
    "class_ordinal","network_class","creator_va",
    "original_static_object_va","crt_index"
]

def need(value:bool,message:str)->None:
    if not value:raise ValueError(message)


def verify_public(manifest:Path,classes:Path,header:Path):
    raw=manifest.read_bytes().replace(b"\r\n",b"\n")
    need(hashlib.sha256(raw).hexdigest()==PUBLIC_CSV_SHA,
         "source temp-event getter CSV SHA256 changed")
    with manifest.open(newline="",encoding="utf-8") as f:
        reader=csv.DictReader(f)
        need(reader.fieldnames==CSV_FIELDS,"source getter CSV schema changed")
        rows=list(reader)
    need(len(rows)==SOURCE_FUNCTIONS,
         "original source temp-event getter count differs from 42")
    with classes.open(newline="",encoding="utf-8") as f:
        class_reader=csv.DictReader(f)
        original_classes=list(class_reader)
    need(len(original_classes)==191,"source 191 creator catalog changed")

    source=header.read_text("utf-8")
    match=re.search(r"kSourceStaticGetters\s*\{\{(.*?)\}\};",source,re.S)
    need(match is not None,"native C++ source getter array missing")
    header_items=re.findall(
        r"\{(\d+)u,0x([0-9a-f]{8})u,0x([0-9a-f]{8})u\}",
        match.group(1))
    need(len(header_items)==SOURCE_FUNCTIONS,
         "C++ source temp-event getter array incomplete")
    unique_ord=set()
    unique_creator=set()
    unique_static=set()
    for i,rec in enumerate(rows):
        ordinal=int(rec["class_ordinal"])
        source_creator=int(rec["creator_va"],16)
        source_static=int(rec["original_static_object_va"],16)
        need(0<=ordinal<191 and ordinal not in unique_ord,
             "duplicate/invalid temp-event class ordinal")
        unique_ord.add(ordinal)
        need(source_creator not in unique_creator and
             source_static not in unique_static and
             SOURCE_DATA_START<=source_static<SOURCE_DATA_END,
             "duplicate/invalid source static event object address")
        unique_creator.add(source_creator)
        unique_static.add(source_static)
        native=original_classes[ordinal]
        need(native["kind"]=="T" and
             native["network_class"]==rec["network_class"] and
             int(native["creator_va"],16)==source_creator and
             native["crt_index"]==rec["crt_index"],
             f"original class/event registration mismatch at row {i}")
        row_header=header_items[i]
        need(int(row_header[0])==ordinal and
             int(row_header[1],16)==source_creator and
             int(row_header[2],16)==source_static,
             f"native C++ 42-getter source order drift at row {i}")
        if i:
            need(int(rows[i-1]["class_ordinal"])<ordinal,
                 "source temporary events not in class registry order")
    need(len(unique_ord)==SOURCE_FUNCTIONS and
         sum(c["kind"]=="T" for c in original_classes)==SOURCE_FUNCTIONS,
         "original source 42 temp-event classes not fully mapped")
    return rows


def verify_private(path:Path,rows,original_class_catalog:Path):
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original unchanged owner PE32 client.dll whole hash differs")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    combined=b""
    for rec in rows:
        va=int(rec["creator_va"],16)
        original_target=int(rec["original_static_object_va"],16)
        offset=va-base
        need(0<=offset<=len(code)-16,
             "source getter VA is outside original retail .text")
        original=code[offset:offset+FUNCTION_BYTES]
        need(original==b"\xB8"+struct.pack("<I",original_target)+b"\xC3",
             f"source native 6-byte static event creator changed at {va:#x}")
        need(code[offset+6:offset+16]==b"\xCC"*10,
             f"source native getter INT3 boundary drift at {va:#x}")
        combined+=original
    need(hashlib.sha256(combined).hexdigest()==WHOLE_42_SOURCE_METHODS_SHA,
         "original concatenated 42 complete static getter bodies changed")
    registered=original_registry_verify(binary,
        original_class_catalog.read_text("utf-8"))
    seen=registered["source_records"]
    for row in rows:
        source=seen[int(row["class_ordinal"])]
        need(source["event_creator_va"]==int(row["creator_va"],16) and
             source["entity_creator_va"]==0 and
             source["name"]==row["network_class"],
             "owner-original ClientClass temporary-event creator mismatch")
    print("PASS Phase1717 owner-original 42 complete 6-byte MOV EAX, "
          "static-pointer; RET functions, 10 INT3 each and CRT source registry")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_42_temp_event_static_getters_phase1717.csv"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--original-class-catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_temp_event_static_creators.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    rows=verify_public(args.manifest,args.creator_catalog,args.header)
    if args.self_test:
        need(hashlib.sha256(b"\x90"*(SOURCE_FUNCTIONS*6)).hexdigest()!=
             WHOLE_42_SOURCE_METHODS_SHA,
             "fabricated original static getter code incorrectly authenticated")
        print("PASS Phase1717 original-free all 42 source static event "
              "getters, original class/kind/VA registry and SHA tampering")
    if args.client is not None:
        verify_private(args.client,rows,args.original_class_catalog)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1717: {exc}",file=sys.stderr)
        sys.exit(1)
