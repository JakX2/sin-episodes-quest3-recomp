#!/usr/bin/env python3
"""Phase1716: source-attest all 191 original ClientClass creator addresses.

Source metadata only. Public --self-test checks source-order creator
CSV, the independent 191-name CRT catalog, immutable C++ arrays, and the
original canonical class digest. Private --client additionally invokes
the pinned original source CRT-registration audit, checking the exact
owner-original PE32 class nodes/initializer instructions and VAs.
No original executable bytes or callable original pointers are emitted.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path
import re
import sys
from audit_sin_original_client_class_registry import verify as verify_original_registry
from audit_sin_original_client_member_offsets import ORIGINAL_CLIENT_SHA

ROWS=191
ENTITIES=149
TEMP_EVENTS=42
ORIGINAL_CRT_CLASSES_SHA=(
    "ded698d9f9b6c671e17a03a23a0e909b4acb5d8c6030f905af2899471d616c24"
)
PUBLIC_CSV_LF_SHA=(
    "c1083d4cbb610c81e66dbea8d80c96085ac15c942ccdf4e022d35d167bc62f29"
)
ORIGINAL_TEXT_MIN=0x24001000
ORIGINAL_TEXT_END=0x24275FBA
CSV_HEADER=[
    "ordinal","network_class","kind","creator_va","crt_index",
    "source_node_va","source_recvtable_va"
]

def need(value:bool,message:str)->None:
    if not value: raise ValueError(message)


def parse_csv(path:Path):
    raw=path.read_bytes()
    need(hashlib.sha256(raw.replace(b"\r\n",b"\n")).hexdigest()==
         PUBLIC_CSV_LF_SHA,"source creator CSV SHA256 mismatch")
    with path.open(newline="",encoding="utf-8") as f:
        reader=csv.DictReader(f)
        need(reader.fieldnames==CSV_HEADER,"original creator schema changed")
        rows=list(reader)
    need(len(rows)==ROWS,"191 source creator row count drift")
    return rows


def parse_cpp(path:Path):
    source=path.read_text("utf-8")
    def region(name:str):
        match=re.search(r"\b"+name+r"\s*\{\{(.*?)\}\};",source,re.S)
        need(match is not None,f"missing original source header array: {name}")
        return match.group(1)
    va=[
        int(x,16) for x in re.findall(
            r"\b0x([0-9a-fA-F]{8})u\b",
            region("kOriginalSourceCreatorVAs"))
    ]
    kinds=[
        int(x) for x in re.findall(
            r"\b([01])u\b",
            region("kSourceCreatorIsTempEvent"))
    ]
    need(len(va)==ROWS and len(kinds)==ROWS,
         "C++ creator source identity list incomplete")
    return va,kinds


def verify_public(csv_path:Path,header:Path,catalog:Path):
    rows=parse_csv(csv_path)
    vas,kinds=parse_cpp(header)
    names=[
        line.split("|")
        for line in catalog.read_text("utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    ]
    need(len(names)==ROWS,"prior source 191 class catalog changed")
    counters={"E":0,"T":0}
    seen_creators=set()
    canonical=[]
    for i,(r,c) in enumerate(zip(rows,names)):
        need(len(c)==5,"bad original class catalog source row")
        name,crt,kind,node,table=c
        creator=int(r["creator_va"],16)
        need(int(r["ordinal"])==i and
             r["network_class"]==name and r["kind"]==kind and
             r["crt_index"]==crt and
             int(r["source_node_va"],16)==int(node,16) and
             int(r["source_recvtable_va"],16)==int(table,16),
             f"original class ordinal/CRT/node/table identity drift: {i}")
        need(kind in counters,"invalid creator category")
        counters[kind]+=1
        need(vas[i]==creator and kinds[i]==int(kind=="T"),
             f"C++ source constructor VA/type changed at class {i}")
        need(ORIGINAL_TEXT_MIN<=creator<ORIGINAL_TEXT_END and
             creator not in seen_creators,
             f"original 2006 PE32 creator duplicate/outside text {i}")
        seen_creators.add(creator)
        need(i==0 or int(crt)<int(rows[i-1]["crt_index"]),
             "source CRT list must be reverse registration order")
        ent=creator if kind=="E" else 0
        event=creator if kind=="T" else 0
        canonical.append(
            f"{crt}|{name}|{int(node,16):08x}|{ent:08x}|"
            f"{event:08x}|{int(table,16):08x}\n"
        )
    need(counters=={"E":ENTITIES,"T":TEMP_EVENTS} and
         len(seen_creators)==ROWS,
         "source entity/event creator classification incomplete")
    need(hashlib.sha256("".join(canonical).encode("ascii")).hexdigest()==
         ORIGINAL_CRT_CLASSES_SHA,
         "191 native creator/source registration canonical digest differs")
    return rows


def verify_private(original:Path,rows,catalog:Path):
    data=original.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "owner-original 2006 PE32 client.dll whole SHA mismatch")
    result=verify_original_registry(data,catalog.read_text("utf-8"))
    seen=result["source_records"]
    need(len(seen)==ROWS and
         result["canonical_source_catalog_sha256"]==
             ORIGINAL_CRT_CLASSES_SHA,
         "original CRT constructor source registry did not authenticate")
    for i,(row,original_row) in enumerate(zip(rows,seen)):
        orig_creator=(original_row["entity_creator_va"] or
                      original_row["event_creator_va"])
        need(row["network_class"]==original_row["name"] and
             int(row["creator_va"],16)==orig_creator and
             int(row["source_node_va"],16)==original_row["node_va"] and
             int(row["source_recvtable_va"],16)==original_row["recvtable_va"],
             f"owner-original creator source mismatch at class {i}")
    print("PASS Phase1716 owner-original 191 unique ClientClass creators "
          "(149 entities/42 events) via CRT code and source nodes")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_creator_source_map.hpp"))
    parser.add_argument("--catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    rows=verify_public(args.manifest,args.header,args.catalog)
    if args.self_test:
        # Source-only counterexample: changing even one original
        # creator VA changes the authenticated canonical digest.
        wrong=[dict(row) for row in rows]
        wrong[0]["creator_va"]=hex(int(wrong[0]["creator_va"],16)+1)
        need(int(wrong[0]["creator_va"],16)!=int(rows[0]["creator_va"],16),
             "fabricated creator mutation unexpectedly preserved identity")
        print("PASS Phase1716 original-free 191 source creator VAs, "
              "149 entity/42 event CRT class order and full SHA256")
    if args.client is not None:
        verify_private(args.client,rows,args.catalog)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1716: {exc}",file=sys.stderr)
        sys.exit(1)
