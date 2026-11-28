#!/usr/bin/env python3
"""Phase1706 source proxy catalog: 1082 original receiver constructor routes.

--self-test authenticates committed original-free source proxy ID bytes;
--client independently rederives every native callback VA from owner-original
SHA-pinned 2006 PE32 client.dll. Native VAs are labels, NEVER executable
pointers for the independent Win32/Android runtime.
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
from audit_sin_original_client_recvprop_type_inventory import (
    SOURCE_FACTORY_BODIES, SOURCE_WRAPPER_BODIES
)

EXPECTED_INDEX_SHA="e74fe1d930d61ba70651fcb0a22dd6d13df222633d83a3812e2ddaffbff7b4f3"
EXPECTED_VA_SHA="eb7f8427a064fe266cde69b8d587bc3b7835653da8c7b9f11f8fc9fa8ae9fc1f"
N_STATIC=1110
N_CANONICAL=1082
N_STANDARD=999
N_CUSTOM=83
N_SPECIAL=28
TYPE_HISTOGRAM=(414,300,164,12,13,207)
BUILTIN={0x24126C40,0x24126C50,0x24126C70,0x24126C80,
         0x24126C90,0x24126CA0,0x24126CE0}
DIRECT={
    0x24126DF0:0,0x24126D80:1,0x24126A60:2,0x24126AD0:3,
    0x24126B50:5,
}
WRAPPERS={
    0x241A8C60:0,0x241A8C90:0,0x241A8CC0:0,
    0x241A8D30:1,0x241A8D80:0,
}
ADDITIONAL_INT_WRAPPER=(0x241A8D80,40,
    "dc0a086f0f7bf32f2fe82a62e9dc92feb353d5374adc583b78cf6ef4d1a773a9")


def need(condition,message):
    if not condition:raise ValueError(message)


def public_header(path: Path):
    text=path.read_text(encoding="utf-8")
    keys=re.search(r"kSourceIndexHex\[\]\s*=\s*(.*?);",text,re.S)
    need(keys is not None,"source callback index encoding missing")
    source="".join(re.findall(r'"([0-9a-f]+)"',keys.group(1)))
    need(len(source)==N_STATIC*2,"source callback index count mismatch")
    raw=bytes.fromhex(source)
    need(hashlib.sha256(raw).hexdigest()==EXPECTED_INDEX_SHA,
         "source callback index SHA256 mismatch")
    vas=re.search(r"kOriginalSourceCallbackVAs\s*\{\{(.*?)\}\};",
                  text,re.S)
    need(vas is not None,"native original source callback VA dictionary missing")
    source_vas=[int(x,16) for x in
                re.findall(r"0x([0-9a-f]{8})u",vas.group(1))]
    need(len(source_vas)==45 and source_vas[0]==0 and
         len(set(source_vas))==45,"original 44 callback VAs + unknown drift")
    need(all(i<len(source_vas) for i in raw),"original source proxy ID outside map")
    expanded=[source_vas[i] for i in raw]
    native_dwords=struct.pack("<"+"I"*N_STATIC,*expanded)
    need(hashlib.sha256(native_dwords).hexdigest()==EXPECTED_VA_SHA,
         "original source callback VA stream SHA256 mismatch")
    need(sum(bool(v) for v in expanded)==N_CANONICAL and
         sum(v in BUILTIN for v in expanded)==N_STANDARD and
         sum(v not in BUILTIN and v!=0 for v in expanded)==N_CUSTOM and
         expanded.count(0)==N_SPECIAL,
         "source receiver proxy canonical/custom/unresolved count drift")
    # Verify a mutation of valid-looking original callbacks is refused.
    wrong=bytearray(raw)
    wrong[0]=1 if wrong[0]!=1 else 2
    need(hashlib.sha256(wrong).hexdigest()!=EXPECTED_INDEX_SHA,
         "mutated original callback source IDs unexpectedly authenticate")
    return expanded


def prior_push(pe: PE, end: int):
    base,code=pe.sections[".text"]
    pos=end-base
    # Prefer the short PUSH when both raw byte patterns accidentally
    # overlap. Original complete source callsites are SHA-pinned.
    if pos>=2 and code[pos-2]==0x6A:
        return end-2,code[pos-1]
    if pos>=5 and code[pos-5]==0x68:
        return end-5,struct.unpack_from("<I",code,pos-4)[0]
    if pos>=1 and 0x50<=code[pos-1]<=0x57:
        return end-1,None  # register-sourced, not a literal VA
    raise ValueError(f"unclassified source PUSH operand ending {end:#x}")


def verify_original(path: Path, catalog: Path, manifest: Path,
                    expected: list[int]) -> None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original owner PE32 client.dll full SHA changed")
    pe=PE(data)
    original_tables=original_table_map(pe)
    roots=original_catalog_order(catalog)
    order=roots+sorted(set(original_tables)-set(roots))
    need(len(order)==211,"original source 211-table ordering drift")
    with manifest.open(newline="",encoding="utf-8") as f:
        rows=list(csv.DictReader(f))
    need(len(rows)==N_STATIC,"original property manifest incomplete")
    base,code=pe.sections[".text"]
    props_by_va={}
    for ordinal,va in enumerate(order):
        props,count,_=original_tables[va]
        for slot in range(count):
            props_by_va[props+60*slot]=(ordinal,slot)
    need(len(props_by_va)==N_STATIC,"source RecvProp descriptor overlap")
    canonical=standard=custom=special=0
    types=Counter()
    for pos,row in enumerate(rows):
        table=int(row["table_ordinal"])
        slot=int(row["property_slot"])
        native=int(row["native_type"])
        need((table,slot)==props_by_va.get(
            original_tables[order[table]][0]+60*slot),
            "source property destination ordinal altered")
        need(row["table"]==original_tables[order[table]][2],
             "original RecvTable source name drift")
        types[native]+=1
        if row["plus44_semantics"]!="member_offset":
            special+=1
            need(expected[pos]==0 and native in (4,5),
                 "alternate source constructor callback incorrectly invented")
            continue
        va=int(row["source_callsite"],16)
        offset=va-base
        need(code[offset]==0x68 and code[offset+5]==0x68 and
             code[offset+10]==0xE8,
             "original canonical constructor source callsite changed")
        name=struct.unpack_from("<I",code,offset+1)[0]
        dest=struct.unpack_from("<I",code,offset+6)[0]
        need(pe.cstring(name)==row["name"] and
             props_by_va[dest]==(table,slot),
             "source constructor name/destination mismatch")
        helper=va+15+struct.unpack_from("<i",code,offset+11)[0]
        need((DIRECT|WRAPPERS).get(helper)==native,
             "original helper native property type mismatch")
        pushes=[]
        cur=va
        for j in range(4):
            try:start,val=prior_push(pe,cur)
            except ValueError:break
            pushes.append(val)
            cur=start
        need(len(pushes)>=2 and pushes[0]==int(row["plus44_value"]),
             "original source canonical member offset operand drift")
        if helper in DIRECT:
            need(len(pushes)>=4,"original direct proxy PUSH chain incomplete")
            if native==5:
                flags=pushes[1]
                width=0
                proxy=pushes[3]
            else:
                width,flags,proxy=pushes[1:4]
        else:
            width=pushes[1]
            flags=0
            if helper==0x241A8CC0:proxy=0
            elif helper==0x241A8D30:proxy=0x24126C40
            elif helper==0x241A8D80:proxy=0x241A8D60
            else:
                need(len(pushes)>=3,
                     "original wrapper-supplied proxy operand unknown")
                proxy=pushes[2]
        need(flags==0,"original native descriptor +8 constructor flags changed")
        need(isinstance(proxy,int),"original callback VA not source literal")
        if native==0 and proxy==0:
            proxy=(0x24126C80 if width==2 else
                   0x24126C90 if width==4 else 0x24126C70)
        need(proxy==expected[pos],
             f"original native receiver callback mismatch {table}/{slot}")
        canonical+=1
        standard+=(proxy in BUILTIN)
        custom+=(proxy not in BUILTIN)
    need(tuple(types[i] for i in range(6))==TYPE_HISTOGRAM,
         "original native property type histogram changed")
    need((canonical,standard,custom,special)==
         (N_CANONICAL,N_STANDARD,N_CUSTOM,N_SPECIAL),
         "original source callback classification incomplete")
    for va,size,sha in SOURCE_FACTORY_BODIES+SOURCE_WRAPPER_BODIES+(
            ADDITIONAL_INT_WRAPPER,):
        at=va-base
        need(hashlib.sha256(code[at:at+size]).hexdigest()==sha,
             f"native 2006 receiver helper complete-body hash mismatch {va:#x}")
    print("PASS Phase1706 original client native receiver source audit: "
          "1082 canonical, 999 standard, 83 custom, 28 special; "
          "all 1082 constructor flags +8 zero")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_source_proxy_catalog.hpp"))
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_static_recvprops_phase1704.csv"))
    parser.add_argument("--catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    original_vas=public_header(args.header)
    if args.self_test:
        print("PASS Phase1706 original-free proxy dictionary and source "
              "SHA256: 999 standard / 83 custom / 28 guarded")
    if args.client is not None:
        verify_original(args.client,args.catalog,args.manifest,original_vas)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError,UnicodeError,struct.error) as error:
        print("FAIL Phase1706:",error,file=sys.stderr)
        sys.exit(1)
