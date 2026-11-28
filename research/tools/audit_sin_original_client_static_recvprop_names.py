#!/usr/bin/env python3
"""Phase1704: all 1110 source-named original 2006 PE32 static RecvProps.

Public CI validates the committed original-free name/type/offset CSV by
independent SHA256 and original constructor-manifest counts. --client
rederives names/calls from the owner's PRIVATE unchanged client.dll.
No executable game code or bytes are copied into GitHub.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path
import re
import struct
import sys
from collections import Counter

from audit_sin_original_client_member_offsets import (
    PE, original_catalog_order, original_table_map, ORIGINAL_CLIENT_SHA
)

SOURCE_NAME_ORDER_SHA = "5a6420cc24c1ed11aa6babcdc98a035c04985eefc4a8c0a6891e8466ac918dd4"
SOURCE_CSV_LF_SHA = "91099b2b87c1ea84cc73a2a61d1abe1f85a605d1a846c54e8a74fbec9e359f4b"
SOURCE_NAME_FNV64 = 0xB37B011A57F1D08B
EXPECTED_TYPES = (414,300,164,12,13,207)
STATIC_TABLES = 211
STATIC_PROPERTIES = 1110
SOURCE_DISTINCT_NAMES = 665
CANONICAL_SOURCE_CONSTRUCTORS = 1082
OTHER_SOURCE_CONSTRUCTORS = 28
HELPERS = {
    0x24126DF0:0,0x24126D80:1,0x24126A60:2,0x24126AD0:3,
    0x24126B50:5,0x241A8C60:0,0x241A8C90:0,0, # placeholder fixed below
}
# Original 2006 helper -> 60-byte native type DWORD.
HELPERS = {
    0x24126DF0:0,0x24126D80:1,0x24126A60:2,0x24126AD0:3,0x24126B50:5,
    0x241A8C60:0,0x241A8C90:0,0x241A8CC0:0,0x241A8D30:1,0x241A8D80:0,
}
SPECIAL_FACTORIES = (0x24126BD0,0x24126E90,0x241271B0)
HEADER = [
    "table_ordinal","table","property_slot","name","native_type",
    "plus44_value","plus44_semantics","source_callsite"
]


def need(value: bool, message: str) -> None:
    if not value:
        raise ValueError(message)


def original_free_manifest(path: Path) -> list[dict]:
    raw=path.read_bytes()
    need(hashlib.sha256(raw.replace(b"\r\n",b"\n")).hexdigest()==
         SOURCE_CSV_LF_SHA,"original source-name CSV SHA256 mismatch")
    with path.open(newline="",encoding="utf-8") as reader:
        rows=list(csv.DictReader(reader))
    need(len(rows)==STATIC_PROPERTIES and list(rows[0])==HEADER,
         "source property CSV cardinality/schema drift")
    found_names=set()
    counts=Counter()
    fingerprints=[]
    last_ordinal=0
    slot_seen={}
    guarded=0
    for i,rec in enumerate(rows):
        table=int(rec["table_ordinal"])
        slot=int(rec["property_slot"])
        native=int(rec["native_type"])
        need(0<=last_ordinal<=table<STATIC_TABLES,
             "source network class/table ordering drift")
        last_ordinal=table
        need(rec["table"] and rec["name"] and 0<=native<=5,
             "invalid source name/type")
        need(all(32<=ord(c)<=126 for c in rec["name"]) and
             len(rec["name"])<=127,"non-native original property name")
        need(slot==slot_seen.get(table,0),
             "property slot count or sequence drift")
        slot_seen[table]=slot+1
        counts[native]+=1
        found_names.add(rec["name"])
        fingerprints.append(f'{rec["table"]}|{slot}|{rec["name"]}')
        offset=rec["plus44_value"]
        if offset=="":
            guarded+=1
            need(table==197 and slot==0 and
                 rec["plus44_semantics"]=="special_overlay_unresolved",
                 "unrecognized source offset placeholder")
        else:
            need(0<=int(offset)<65536,
                 "invalid owner source member offset")
        need(rec["source_callsite"].startswith("0x") and
             int(rec["source_callsite"],16)>=0x24000000,
             "missing original constructor source callsite")
    need(tuple(counts[i] for i in range(6))==EXPECTED_TYPES,
         "original numeric native type histogram changed")
    need(guarded==1 and len(found_names)==SOURCE_DISTINCT_NAMES,
         "source names/guarded specialized overlay count changed")
    payload="\n".join(fingerprints).encode("ascii")
    need(hashlib.sha256(payload).hexdigest()==SOURCE_NAME_ORDER_SHA,
         "1110 original class/slot/name sequence SHA mismatch")
    fnv=0xCBF29CE484222325
    for byte in payload:
        fnv=((fnv ^ byte)*0x100000001B3)&0xFFFFFFFFFFFFFFFF
    need(fnv==SOURCE_NAME_FNV64,
         "independent native C++ FNV input fingerprint differs")
    return rows


def original_binary_audit(original: Path, catalog: Path,
                          expected_rows: list[dict]) -> None:
    data=original.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original retail client.dll full SHA256 mismatch")
    pe=PE(data)
    tables=original_table_map(pe)
    roots=original_catalog_order(catalog)
    need(set(roots).issubset(tables),"registered source class table absent")
    order=roots+sorted(set(tables)-set(roots))
    need(len(order)==STATIC_TABLES,"original table count/order changed")
    slot_by_va={}
    for ordinal,tableva in enumerate(order):
        propertyva,count,name=tables[tableva]
        for slot in range(count):
            slot_by_va[propertyva+60*slot]=(ordinal,slot)
    need(len(slot_by_va)==STATIC_PROPERTIES,
         "original 2006 native property destinations changed")

    base,code=pe.sections[".text"]
    observed={}
    # Standard PUSH source-name VA, PUSH destination descriptor VA,
    # CALL original constructor/helper. Exhaustively authenticates 1082.
    for i in range(25,len(code)-16):
        if code[i]!=0x68 or code[i+5]!=0x68 or code[i+10]!=0xE8:
            continue
        dest,=struct.unpack_from("<I",code,i+6)
        if dest not in slot_by_va:
            continue
        helper=base+i+15+struct.unpack_from("<i",code,i+11)[0]
        if helper not in HELPERS:
            continue
        name_va,=struct.unpack_from("<I",code,i+1)
        name=pe.cstring(name_va)
        key=slot_by_va[dest]
        need(key not in observed,
             "duplicate source canonical property constructor")
        observed[key]=(name,HELPERS[helper],base+i)
    need(len(observed)==CANONICAL_SOURCE_CONSTRUCTORS,
         "failed to recover all canonical name sources")

    # The 28 original alternate constructor paths use actual source
    # type4 arrays, type5 dynamic tables and specialized animation overlay.
    alternate={}
    for i in range(len(code)-5):
        if code[i]!=0xE8:
            continue
        source_call=base+i
        helper=source_call+5+struct.unpack_from("<i",code,i+1)[0]
        if helper not in SPECIAL_FACTORIES:
            continue
        destpos=i-(5 if helper==0x24126BD0 else 7)
        need(code[destpos]==0x68 and
             (helper==0x24126BD0 or code[i-2:i]==b"\xF3\xA5"),
             "original alternate descriptor PUSH/REP MOVSD changed")
        dest,=struct.unpack_from("<I",code,destpos+1)
        need(dest in slot_by_va,
             "unrecognized original special property destination")
        key=slot_by_va[dest]
        need(key not in observed and key not in alternate,
             "original alternate RecvProp overlaps canonical route")
        ordinal,slot=key
        row_index=sum(tables[t][1] for t in order[:ordinal])+slot
        expected=expected_rows[row_index]
        need(int(expected["table_ordinal"])==ordinal and
             int(expected["property_slot"])==slot,
             "source constructor-to-CSV ordinal mismatch")
        expected_name=expected["name"]
        # The original PUSH <native .rdata name VA> occurs in the
        # construction argument window; verify the exact supplied literal,
        # without pulling executable source bytes into public metadata.
        witnesses=0
        for p in range(max(0,destpos-95),destpos):
            if code[p]!=0x68:
                continue
            name_va,=struct.unpack_from("<I",code,p+1)
            try:
                source_name=pe.cstring(name_va)
            except (ValueError,UnicodeDecodeError):
                continue
            if source_name==expected_name:
                witnesses+=1
        need(witnesses>=1,
             "original special property source name PUSH not found")
        tag=4 if helper==0x24126BD0 else 5
        alternate[key]=(expected_name,tag,source_call)
    need(len(alternate)==OTHER_SOURCE_CONSTRUCTORS,
         "original 28 alternate source property names incomplete")
    observed.update(alternate)
    need(len(observed)==STATIC_PROPERTIES,
         "not all original static network names recovered")
    for ordinal,tableva in enumerate(order):
        _,count,source_table=tables[tableva]
        for slot in range(count):
            row=expected_rows[
                sum(tables[t][1] for t in order[:ordinal])+slot]
            name,native,call=observed[ordinal,slot]
            need(row["table"]==source_table and
                 row["name"]==name and
                 int(row["native_type"])==native and
                 int(row["source_callsite"],16)==call,
                 f"source static property differs at {ordinal}/{slot}")
    print("PASS Phase1704 owner-original PE32 1110 native source names "
          "and all 1082+28 constructor callsites")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_static_recvprops_phase1704.csv"))
    parser.add_argument("--catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    rows=original_free_manifest(args.manifest)
    if args.self_test:
        print("PASS Phase1704 1110 source names / 665 unique / one overlay "
              "unknown: original-free SHA256, FNV64 and native type histogram")
    if args.client is not None:
        original_binary_audit(args.client,args.catalog,rows)


if __name__=="__main__":
    try:
        main()
    except (ValueError,OSError,UnicodeError,struct.error) as error:
        print(f"FAIL Phase1704: {error}",file=sys.stderr)
        sys.exit(1)
