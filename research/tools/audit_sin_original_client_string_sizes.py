#!/usr/bin/env python3
"""Phase1708 authenticates twelve original 2006 PE32 RecvProp string sizes.

Original source native type3 factory at 0x24126AD0 stores its source
capacity into descriptor +12; no other types' +12 values are guessed.
Owner-private --client compares source argument PUSHes against the committed
12-item original-free C++ manifest. No retail executable bytes committed.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
from pathlib import Path
import re
import struct
import sys
from audit_sin_original_client_member_offsets import (
    PE,original_catalog_order,original_table_map,ORIGINAL_CLIENT_SHA
)
from audit_sin_original_client_source_proxy_catalog import prior_push

EXPECTED_SHA="62b1a1a412724fbd51a2556545af0c28b8819ee1427ece1b6f7d1e4d5e5b5292"
EXPECTED_COUNT=12
ORIGINAL_TYPE3_FACTORY=0x24126AD0
ORIGINAL_TYPE3_BODY_SIZE=117
ORIGINAL_TYPE3_BODY_SHA="0c29e142f7c064d5170e27d1a2759c4ff059c962a576febddedabbf8e4724300"
ORIGINAL_STRING_PROXY=0x24126CA0

def need(ok,message):
    if not ok:raise ValueError(message)

def public_sizes(header: Path):
    content=header.read_text(encoding="utf-8")
    match=re.search(r"kOriginalStrings\s*\{\{(.*?)\}\};",content,re.S)
    need(match is not None,"source type3 string size array absent")
    tuples=[tuple(int(z) for z in m.groups()) for m in re.finditer(
        r"\{(\d+)u,(\d+)u,(\d+)u\}",match.group(1))]
    need(len(tuples)==EXPECTED_COUNT,"unexpected original string property count")
    need(len({(t,s) for t,s,n in tuples})==EXPECTED_COUNT,
         "duplicate original native type3 string slot")
    need(all(0<=t<211 and 0<=s<256 and 0<n<=256 for t,s,n in tuples),
         "unproven native string buffer dimensions")
    raw=b"".join(struct.pack("<BBH",*x) for x in tuples)
    need(hashlib.sha256(raw).hexdigest()==EXPECTED_SHA,
         "original string descriptor capacity fingerprint mismatch")
    mutant=bytearray(raw)
    mutant[-1]^=1
    need(hashlib.sha256(mutant).hexdigest()!=EXPECTED_SHA,
         "mutated native string capacity must fail audit")
    return tuples

def verify_original(client: Path,manifest: Path,catalog: Path,tuples):
    original=client.read_bytes()
    need(hashlib.sha256(original).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original owner PE32 client.dll whole hash changed")
    pe=PE(original)
    table_map=original_table_map(pe)
    roots=original_catalog_order(catalog)
    order=roots+sorted(set(table_map)-set(roots))
    need(len(order)==211,"source 211-table exact order changed")
    with manifest.open(newline="",encoding="utf-8") as file:
        rows=list(csv.DictReader(file))
    source={(int(x["table_ordinal"]),int(x["property_slot"])):x for x in rows}
    need(len(source)==1110,"original 1110 property manifest incomplete")
    base,code=pe.sections[".text"]
    at=ORIGINAL_TYPE3_FACTORY-base
    need(hashlib.sha256(code[at:at+ORIGINAL_TYPE3_BODY_SIZE]).hexdigest()
         ==ORIGINAL_TYPE3_BODY_SHA,
         "native 117B type3 constructor function digest drift")
    need(code[0x24126B0A-base:0x24126B0E-base]==
         bytes.fromhex("89442414"),
         "original string size native destination +12 write changed")
    for table,slot,expected_size in tuples:
        rec=source[table,slot]
        need(int(rec["native_type"])==3 and
             rec["plus44_semantics"]=="member_offset",
             "source native string descriptor type/offset route drift")
        va=int(rec["source_callsite"],16)
        p=va-base
        need(code[p]==0x68 and code[p+5]==0x68 and code[p+10]==0xE8,
             "native original string constructor callsite changed")
        name,=struct.unpack_from("<I",code,p+1)
        destination,=struct.unpack_from("<I",code,p+6)
        method=va+15+struct.unpack_from("<i",code,p+11)[0]
        need(method==ORIGINAL_TYPE3_FACTORY and
             pe.cstring(name)==rec["name"],
             "original string source name/helper drift")
        props,count,_=table_map[order[table]]
        need(slot<count and destination==props+60*slot,
             "original string source slot destination mismatch")
        start,offset=prior_push(pe,va)
        start,capacity=prior_push(pe,start)
        start,flags=prior_push(pe,start)
        start,proxy=prior_push(pe,start)
        need(offset==int(rec["plus44_value"]) and
             capacity==expected_size and flags==0 and
             proxy==ORIGINAL_STRING_PROXY,
             "original source string size/flags/receiver callback differs")
    print("PASS Phase1708 original-client PE32 all 12 STRING RecvProp +12 "
          "capacities from native x86 constructor callsites")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_source_string_sizes.hpp"))
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_static_recvprops_phase1704.csv"))
    parser.add_argument("--catalog",type=Path,default=Path(
        "research/manifests/original_client_class_catalog_phase1696.tsv"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    sizes=public_sizes(args.header)
    if args.self_test:
        print("PASS Phase1708 original-free 12 native string sizes, "
              "48-byte stream SHA256 and mutation rejection")
    if args.client is not None:
        verify_original(args.client,args.manifest,args.catalog,sizes)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError,UnicodeError,struct.error) as error:
        print("FAIL Phase1708:",error,file=sys.stderr)
        sys.exit(1)
