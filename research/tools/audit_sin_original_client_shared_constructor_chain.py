#!/usr/bin/env python3
"""Phase1723: authenticate three shared original 2006 PE32 constructor bodies.

A(0x24092BD0,206B) -> B(0x240BD7C0,861B) -> C(0x240CCE30,573B)
and C's first CALL -> unresolved 0x24195260.
34 original registered entity creators call A directly; 26 call
C directly. Thus 60 source common creator entrypoints reach C via
these demonstrated native call edges. The others have different
targets; no invented reachability or host code execution is claimed.

Private --client SHA-pins all three COMPLETE 2006 PE32 source method
bodies, native RET and INT3 padding, and original E8 branch targets,
without committing any executable original bytes.
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
from audit_sin_original_client_full_entity_creator_bodies import (
    verify_public as verify_142_complete_source,
)

SOURCE_A=0x24092BD0
SOURCE_B=0x240BD7C0
SOURCE_C=0x240CCE30
SOURCE_NESTED=0x24195260
SOURCES=(
    # original VA, real function bytes, whole source SHA256,
    # original INT3 count, first native CALL offset, CALL target
    (SOURCE_A,206,
     "28e8f086a532a606b78304522eaaeb571f63922feaee80aaaeeacb7e8a03659a",
     2,3,SOURCE_B),
    (SOURCE_B,861,
     "194b3ef35fa66ab726f06f1ce37539c6901ced7a3d22995a228f00aa04e62db0",
     3,7,SOURCE_C),
    (SOURCE_C,573,
     "63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca",
     3,86,SOURCE_NESTED)
)
SOURCE_CONCAT_SHA="3a02d8d7ff3c50af8c1096bcc00be731e02426a0b4f06112138e72cb99ec70ec"
EXPECTED_DIRECT_A=34
EXPECTED_DIRECT_C=26
EXPECTED_OTHER=82
EXPECTED_REACHED=60

def need(ok,message):
    if not ok: raise ValueError(message)


def original_free(manifest:Path,header:Path,previous_header:Path,
                  prologue_manifest:Path,creator_catalog:Path,
                  prologue_header:Path):
    rows=verify_142_complete_source(
        manifest,previous_header,prologue_manifest,
        creator_catalog,prologue_header)
    contents=header.read_text("utf-8")
    match=re.search(r"kOriginalFunctionChain\s*\{\{(.*?)\}\};",
                    contents,re.S)
    need(match is not None,"public original shared constructor chain missing")
    cpp=re.findall(
        r"\{(kNativeSharedConstructor[ABC]),(\d+)u,(\d+)u,"
        r"(\d+)u,((?:kNativeSharedConstructor[BC])|"
        r"(?:kUnreconstructedNestedConstructorVA))\}",
        match.group(1))
    need(len(cpp)==3,"source C++ original three constructor chain incomplete")
    canonical=[
        ("kNativeSharedConstructorA",206,2,3,"kNativeSharedConstructorB"),
        ("kNativeSharedConstructorB",861,3,7,"kNativeSharedConstructorC"),
        ("kNativeSharedConstructorC",573,3,86,"kUnreconstructedNestedConstructorVA")
    ]
    need([tuple((x[0],int(x[1]),int(x[2]),int(x[3]),x[4]))
          for x in cpp]==canonical,
         "C++ original source shared chain identities/callsite widths changed")
    targetCounts={}
    for row in rows:
        t=int(row["original_constructor_target_va"],16)
        targetCounts[t]=targetCounts.get(t,0)+1
    need(targetCounts.get(SOURCE_A)==EXPECTED_DIRECT_A and
         targetCounts.get(SOURCE_C)==EXPECTED_DIRECT_C and
         targetCounts.get(SOURCE_B,0)==0 and
         len(rows)-targetCounts.get(SOURCE_A)-targetCounts.get(SOURCE_C)==EXPECTED_OTHER,
         "source constructor A/C direct-caller graph census drift")
    need(len({r["original_constructor_target_va"] for r in rows})==73,
         "source complete 142 creator target distinct count changed")
    return rows


def original_private(path:Path):
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "unchanged original 2006 PE32 client.dll whole hash differs")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    def at(va,count):
        off=va-base
        need(count>=0 and 0<=off<=len(code)-count,
             "source native class constructor outside original .text")
        return code[off:off+count]
    allOriginal=b""
    for va,size,digest,padding,off,target in SOURCES:
        body=at(va,size)
        need(hashlib.sha256(body).hexdigest()==digest,
             f"whole source constructor target body SHA mismatch at {va:#x}")
        need(body[-1:]==b"\xC3" and
             at(va+size,padding)==b"\xCC"*padding,
             f"original constructor target RET/INT3 boundary changed at {va:#x}")
        need(body[off]==0xE8 and va+off+5+
             struct.unpack_from("<i",body,off+1)[0]==target,
             f"first native source constructor-to-base CALL differs {va:#x}")
        allOriginal+=body
    need(len(allOriginal)==206+861+573 and
         hashlib.sha256(allOriginal).hexdigest()==SOURCE_CONCAT_SHA,
         "1640-byte complete source shared constructor chain digest changed")
    print("PASS Phase1723 three WHOLE source constructor functions: 206B "
          "A -> 861B B -> 573B C, correct x86 E8 calls/RET/INT3; "
          "34 direct A creators and 26 direct C creators source-attested")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_142_full_entity_creator_bodies_phase1722.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_shared_constructor_chain.hpp"))
    parser.add_argument("--previous-header",type=Path,default=Path(
        "include/sin_compat/original_client_full_entity_creator_bodies.hpp"))
    parser.add_argument("--prologue-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--prologue-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    original_free(args.manifest,args.header,args.previous_header,
                  args.prologue_manifest,args.creator_catalog,
                  args.prologue_header)
    if args.self_test:
        for va,size,digest,pad,calloff,target in SOURCES:
            need(hashlib.sha256(b"\x90"*size).hexdigest()!=digest,
                 "fabricated original constructor falsely authenticated")
        need(len(SOURCES)==3 and EXPECTED_DIRECT_A+EXPECTED_DIRECT_C==
             EXPECTED_REACHED,
             "native constructor dependency family source count changed")
        print("PASS Phase1723 original-free three constructor target "
              "chain and 34/26 direct caller counts, SHA mutation refusal")
    if args.client is not None:original_private(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1723: {exc}",file=sys.stderr)
        sys.exit(1)
