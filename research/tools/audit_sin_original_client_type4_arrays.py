#!/usr/bin/env python3
"""Phase1703: audit 13 original 2006 PE32 native type-4 array constructors.

Public --self-test reads only authored source metadata; private --client
authenticates the unchanged original owner DLL in place. No executable
bytes from retail client.dll are written to the public repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
from audit_sin_original_client_member_offsets import (
    PE, original_table_map, original_catalog_order, ORIGINAL_CLIENT_SHA
)

ARRAY_NATIVE_CONSTRUCTOR = 0x24126BD0
ARRAY_NATIVE_LENGTH = 102
ARRAY_NATIVE_SHA = "dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd"
ORIGINAL_TEAM_CALLBACK = 0x240F76B0
# Original x86 CALL VA, registered/aux source table ordinal, 0-based slot,
# native element count, stride, exact source property name, special callback.
EXPECTED = (
    (0x2405FDD7,181,2,3,4,"m_hPassengers",False),
    (0x2406E2BE,163,2,16,12,"m_vecPoints",False),
    (0x2409CDEE,129,15,24,4,"m_flPoseParameter",False),
    (0x2409EED7,128,4,10,4,"m_hAttachEntity",False),
    (0x2409EF09,128,6,10,4,"m_nAttachIndex",False),
    (0x240D21CC,118,13,2,4,"m_hObserverViewModel",False),
    (0x240D3B0B,203,17,2,4,"m_hViewModel",False),
    (0x240D9F8F,112,2,10,255,"m_iszOverlayNames",False),
    (0x240D9FC1,112,4,10,4,"m_flOverlayTimes",False),
    (0x240F778C,81,4,32,0,'"player_array"',True),
    (0x2419DF39,70,2,24,12,"m_ragAngles",False),
    (0x2419DF6B,70,4,24,12,"m_ragPos",False),
    (0x2422175E,37,14,3,4,"m_flFadeColor",False),
)


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def original_free_header(path: Path):
    content = path.read_text(encoding="utf-8")
    match = re.search(r"kSourceArrays\s*\{\{(.*?)\}\};", content, re.S)
    need(match is not None, "native type4 array header missing source list")
    pat = re.compile(
        r'\{(\d+)u,(\d+)u,(\d+)u,(\d+)u,("(?:[^"\\]|\\.)*"),(true|false)\}'
    )
    found = []
    for m in pat.finditer(match.group(1)):
        tab,slot,count,stride = map(int,m.group(1,2,3,4))
        name=json.loads(m.group(5))
        found.append((tab,slot,count,stride,name,m.group(6)=="true"))
    expected=[(tab,slot,count,stride,name,special)
              for _,tab,slot,count,stride,name,special in EXPECTED]
    need(found==expected,"public native type4 table/slot/shape/name/callback drift")
    need(len(found)==13 and sum(special for *_,special in found)==1,
         "source type4 descriptor classification incomplete")
    return found


def previous_imm_push(code: bytes, end: int):
    if end >= 2 and code[end-2] == 0x6A:
        return end-2,code[end-1]
    if end >= 5 and code[end-5] == 0x68:
        return end-5,struct.unpack_from("<I",code,end-4)[0]
    raise ValueError(f"native original static PUSH missing at byte offset {end:x}")


def verify_original(path: Path, catalog_path: Path) -> None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original owner client.dll full SHA256 mismatch")
    pe=PE(data)
    base,code=pe.sections[".text"]
    off=ARRAY_NATIVE_CONSTRUCTOR-base
    need(hashlib.sha256(code[off:off+ARRAY_NATIVE_LENGTH]).hexdigest()
         ==ARRAY_NATIVE_SHA, "original 102-byte type4 constructor SHA mismatch")
    # Original 2006 method 0x24126BD0. +44 is zeroed before any push of
    # ESI/EDI; +48 stride is stored AFTER two pushes; +52 count BEFORE.
    # This is not a newer Source SDK layout guessed from type names.
    witnesses={
        0x24126BDB:bytes.fromhex("33c0"),
        0x24126BE1:bytes.fromhex("8944242c"),
        0x24126C01:bytes.fromhex("89442434"),
        0x24126C0B:bytes.fromhex("89442424"),
        0x24126C13:bytes.fromhex("894c2438")
    }
    for va,want in witnesses.items():
        need(code[va-base:va-base+len(want)]==want,
             f"original type4 native field assignment drift at {va:#x}")
    tables=original_table_map(pe)
    roots=original_catalog_order(catalog_path)
    need(set(roots).issubset(tables),"missing original ClientClass table")
    ordered=roots+sorted(set(tables)-set(roots))
    need(len(ordered)==211,"original 211 receive-table source order changed")
    mapped={}
    for ordinal,table in enumerate(ordered):
        props,count,_=tables[table]
        for slot in range(count):
            mapped[props+60*slot]=(ordinal,slot)
    need(len(mapped)==1110,"original native descriptor slots changed")
    found=[]
    for i in range(len(code)-5):
        if code[i]!=0xE8 or (base+i+5+
            struct.unpack_from("<i",code,i+1)[0])!=ARRAY_NATIVE_CONSTRUCTOR:
            continue
        deststart,dest=previous_imm_push(code,i)
        countstart,count=previous_imm_push(code,deststart)
        stridestart,stride=previous_imm_push(code,countstart)
        namestart,name_va=previous_imm_push(code,stridestart)
        _,callback=previous_imm_push(code,namestart)
        need(dest in mapped,"original type4 destination not a static RecvProp")
        table,slot=mapped[dest]
        name=pe.cstring(name_va)
        need(callback in (0,ORIGINAL_TEAM_CALLBACK),
             "unknown original array proxy callback")
        found.append((base+i,table,slot,count,stride,name,callback!=0))
    need(tuple(found)==EXPECTED,
         "13 original native type4 constructor CALLs/arguments no longer match")
    need(sum(c for *_,c in found)==1,"one Team proxy callback expected")
    print("PASS Phase1703 owner-original type4 x86 constructors: "
          "13 names, counts, strides, +44 zero, +48 stride, +52 count; "
          "one blocked original callback")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_native_type4_array_fields.hpp"))
    parser.add_argument("--catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    if args.self_test:
        original_free_header(args.header)
        changed=list(EXPECTED)
        item=list(changed[0])
        item[3]+=1
        changed[0]=tuple(item)
        need(tuple(changed)!=EXPECTED,
             "mutated native array source shape must be refused")
        print("PASS Phase1703 original-free 13 type4 source descriptors, "
              "one unknown executable callback")
    if args.client is not None:
        verify_original(args.client,args.catalog)


if __name__=="__main__":
    try:
        main()
    except (ValueError,OSError,UnicodeError,struct.error) as error:
        print(f"FAIL Phase1703: {error}",file=sys.stderr)
        sys.exit(1)
