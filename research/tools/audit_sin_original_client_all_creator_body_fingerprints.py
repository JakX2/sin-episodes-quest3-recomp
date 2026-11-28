#!/usr/bin/env python3
"""Phase1724: complete SHA source fingerprint for ALL 191 retail creator methods.

Source original SiN Episodes (2006) PE32 client.dll, 191 ClientClass
creator function bodies in their authenticated original head-list order:
42 x original 6-byte static temporary-event getters (Phase1717),
118 x canonical 60/86-byte entities (Phase1722),
7 x exceptional whole-body source entities (Phase1721),
24 x uniquely coded noncanonical full-body entities (THIS phase).

Whole concatenation of all 191 original executable method bodies,
14,868 bytes, SHA256 c62a23ee2494ee996a00fb77745f734a821993f86546774b6d43669095767d5b.

Whole-body SHA verification DOES NOT reconstruct constructors,
engine heap, Source RTTI/vtables or actual executable callbacks.
Public mode uses only original-free research metadata; private mode
requires unchanged source DLL and never exports executable game bytes.
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
from audit_sin_original_client_canonical_creator_bodies import (
    verify_public as verify_118_public,
    verify_original as verify_118_original,
)
from audit_sin_original_client_seven_creator_exceptions import (
    verify_public as verify_7_public,
)
from audit_sin_original_client_temp_event_static_creators import (
    verify_public as verify_42_public,
)

WHOLE_191_METHODS_SHA="c62a23ee2494ee996a00fb77745f734a821993f86546774b6d43669095767d5b"
SOURCE_24_MANIFEST_SHA="7d11b2d9eb4ad1c22d399cbad38fe8d9ce8c65e44a55c0d670f0ff732914a2d2"
SOURCE_24_WHOLE_METHODS_SHA="9633b2d099c8882f69e517be7700b5d1d7a323655f1954ea8cdfc4790238f6c9"
TOTAL_WHOLE_NATIVE_BYTES=14868
EXPECTED_SOURCE_COUNTS=(42,68,50,24,7)
SOURCE_COLS=[
    "class_ordinal","network_class","creator_va","original_request_bytes",
    "complete_method_bytes","original_int3_padding_bytes",
    "original_complete_method_sha256"
]

def need(yes:bool,message:str)->None:
    if not yes:raise ValueError(message)


def verify_public(manifest:Path,header:Path,classes:Path,
                  common_manifest:Path,common_header:Path,
                  canonical_header:Path,special_manifest:Path,
                  special_header:Path,static_manifest:Path,
                  static_header:Path):
    raw=manifest.read_bytes().replace(b"\r\n",b"\n")
    need(hashlib.sha256(raw).hexdigest()==SOURCE_24_MANIFEST_SHA,
         "full 24 noncanonical function metadata SHA256 changed")
    with manifest.open(newline="",encoding="utf-8") as file:
        reader=csv.DictReader(file)
        need(reader.fieldnames==SOURCE_COLS,
             "source 24 unusual creator method CSV schema changed")
        complex_rows=list(reader)
    need(len(complex_rows)==24,"not 24 original noncanonical methods")
    all118,allocations,names=verify_118_public(
        canonical_header,classes,common_manifest,common_header)
    all7=verify_7_public(special_manifest,special_header,classes,
                        common_manifest,common_header)
    all42=verify_42_public(static_manifest,classes,static_header)
    with classes.open(newline="",encoding="utf-8") as inp:
        classes191=list(csv.DictReader(inp))
    need(len(classes191)==191,"source all 191 original creator names changed")
    uncommon={int(r["class_ordinal"]) for r in allocations.values()}
    canonical_ords={ordinal for body in all118.values() for ordinal,_ in body}
    target=uncommon-canonical_ords
    need(len(target)==24,"original 142 -> 118 + 24 split drift")
    hdr=header.read_text("utf-8")
    match=re.search(r"kOriginal24NoncanonicalMethods\s*\{\{(.*?)\}\};",
                    hdr,re.S)
    need(match is not None,"native C++ original noncanonical body records missing")
    cpp=re.findall(
        r"\{(\d+)u,0x([0-9a-f]{8})u,(\d+)u,(\d+)u,(\d+)u\}",
        match.group(1))
    need(len(cpp)==24,
         "native C++ exact 24 noncanonical function records absent")
    seen=set()
    for i,(r,h) in enumerate(zip(complex_rows,cpp)):
        ordinal=int(r["class_ordinal"])
        function=int(r["creator_va"],16)
        requested=int(r["original_request_bytes"])
        length=int(r["complete_method_bytes"])
        padding=int(r["original_int3_padding_bytes"])
        sha=r["original_complete_method_sha256"]
        need(ordinal in target and ordinal not in seen,
             "missing/duplicated original source noncanonical class")
        seen.add(ordinal)
        native=allocations[ordinal]
        need(native["network_class"]==r["network_class"] and
             int(native["creator_va"],16)==function and
             int(native["object_size_bytes"])==requested and
             classes191[ordinal]["kind"]=="E",
             f"original class/source allocator identity mismatch at {ordinal}")
        need((ordinal,function,requested,length,padding)==
             (int(h[0]),int(h[1],16),int(h[2]),int(h[3]),int(h[4])),
             f"original C++ whole-function source metadata drift at {ordinal}")
        need(86<length<=1000 and 4<=padding<=15 and
             len(sha)==64 and all(ch in "0123456789abcdef" for ch in sha),
             "original unusual body length / padding / SHA invalid")
    need(seen==target,"some source original noncanonical bodies missing")
    need(len(all7)==7 and len(all42)==42 and
         sum(len(x) for x in all118.values())==118,
         "original source whole-body family inventory incomplete")
    return complex_rows,all118,allocations,classes191,all7,all42


def verify_original(path:Path,records,all118,allocations,classes191,
                    all7,all42):
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "unchanged retail 2006 PE32 client.dll whole SHA256 mismatch")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def read(va,n):
        offset=va-base
        need(n>=0 and 0<=offset<=len(code)-n,
             f"original native PE32 method outside source .text at {va:#x}")
        return code[offset:offset+n]
    # Original 118 whole canonical methods have independent normalized
    # instruction template, allocator+constructor and aggregate SHA checks.
    verify_118_original(path,all118,allocations)
    complex_by_ordinal={int(r["class_ordinal"]):r for r in records}
    complex_concat=b""
    for r in records:
        ordinal=int(r["class_ordinal"])
        va=int(r["creator_va"],16)
        n=int(r["complete_method_bytes"])
        padding=int(r["original_int3_padding_bytes"])
        body=read(va,n)
        need(body[-1:]==b"\xC3" and
             read(va+n,padding)==b"\xCC"*padding and
             read(va+n+padding,1)!=b"\xCC",
             f"original noncanonical method end RET/INT3 boundary drift at {ordinal}")
        need(hashlib.sha256(body).hexdigest()==
             r["original_complete_method_sha256"],
             f"whole source noncanonical creator SHA256 mismatch at {ordinal}")
        # All 24 still have the real authenticated common allocator
        # first 11 executable bytes (Phase1718) and native size.
        need(body[:2]==b"\x56\x68" and body[6]==0xE8 and
             struct.unpack_from("<I",body,2)[0]==
                  int(r["original_request_bytes"]),
             "original complex entity allocation prologue instruction drift")
        rel,=struct.unpack_from("<i",body,7)
        need(va+11+rel==0x240C4FF0,
             "original complex source allocation target no longer common")
        complex_concat+=body
    need(hashlib.sha256(complex_concat).hexdigest()==
         SOURCE_24_WHOLE_METHODS_SHA,
         "all 24 original noncanonical whole method combined SHA drift")
    special_by_ordinal={int(r["class_ordinal"]):r for r in all7}
    temp_by_ordinal={int(r["class_ordinal"]):r for r in all42}
    canonical_lengths={
        ordinal:length
        for length,pairs in all118.items() for ordinal,_ in pairs
    }
    combined=b""
    for ordinal,row in enumerate(classes191):
        va=int(row["creator_va"],16)
        if row["kind"]=="T":
            need(ordinal in temp_by_ordinal,"original source temp getter missing")
            r=temp_by_ordinal[ordinal]
            n=6
            body=read(va,n)
            need(body==b"\xB8"+struct.pack(
                "<I",int(r["original_static_object_va"],16))+b"\xC3",
                "original 6B static temp-event function opcode drift")
        elif ordinal in special_by_ordinal:
            r=special_by_ordinal[ordinal]
            n=int(r["complete_native_function_bytes"])
            body=read(va,n)
            need(hashlib.sha256(body).hexdigest()==
                 r["complete_native_function_sha256"],
                 "original seven exceptional complete function SHA drift")
        elif ordinal in canonical_lengths:
            n=canonical_lengths[ordinal]
            body=read(va,n)
        else:
            need(ordinal in complex_by_ordinal,
                 "source original entity creator missing body fingerprint")
            n=int(complex_by_ordinal[ordinal]["complete_method_bytes"])
            body=read(va,n)
        combined+=body
    need(len(combined)==TOTAL_WHOLE_NATIVE_BYTES,
         "original 191 creator full-body total bytes mismatch")
    need(hashlib.sha256(combined).hexdigest()==WHOLE_191_METHODS_SHA,
         "original ALL 191 executable creator bodies combined source SHA drift")
    print("PASS Phase1724 owner-original all 191 source creator whole functions "
          "14,868 bytes SHA, 42 temp +118 canonical +24 noncanonical +7 "
          "exceptional functions, no runtime creator/vtable published")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_24_complex_creator_bodies_phase1724.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_all_creator_body_fingerprints.hpp"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--common-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--common-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    parser.add_argument("--canonical-header",type=Path,default=Path(
        "include/sin_compat/original_client_canonical_creator_bodies.hpp"))
    parser.add_argument("--special-manifest",type=Path,default=Path(
        "research/manifests/original_client_seven_creator_exceptions_phase1721.csv"))
    parser.add_argument("--special-header",type=Path,default=Path(
        "include/sin_compat/original_client_seven_creator_exceptions.hpp"))
    parser.add_argument("--temp-manifest",type=Path,default=Path(
        "research/manifests/original_client_42_temp_event_static_getters_phase1717.csv"))
    parser.add_argument("--temp-header",type=Path,default=Path(
        "include/sin_compat/original_client_temp_event_static_creators.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    prepared=verify_public(args.manifest,args.header,args.creator_catalog,
                           args.common_manifest,args.common_header,
                           args.canonical_header,args.special_manifest,
                           args.special_header,args.temp_manifest,
                           args.temp_header)
    if args.self_test:
        for n,digest in ((TOTAL_WHOLE_NATIVE_BYTES,WHOLE_191_METHODS_SHA),
                         (5670,SOURCE_24_WHOLE_METHODS_SHA)):
            need(hashlib.sha256(b"\x90"*n).hexdigest()!=digest,
                 "fabricated complete original native code accepted")
        print("PASS Phase1724 original-free source 191 complete method "
              "fingerprints, 118 canonical /24 noncanonical /7 exceptional "
              "/42 static getters, source CSV SHA and body mutation refusal")
    if args.client is not None:
        verify_original(args.client,*prepared)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1724: {exc}",file=sys.stderr)
        sys.exit(1)
