#!/usr/bin/env python3
"""Phase1723: verify original 50 inline factory virtual-method dependencies.

50 original PE32 ClientClass creators each write four native RTTI vtable
pointer labels at owner +0,+4,+8,+12. Every primary vtable byte+0x94
points to the SAME original 75-byte 0x240C8FE0 network-init function;
the second original vtable's byte+0 points to the SAME 4-byte
0x24054EC0 networkable-adjustor thunk (return this-4).
Primary vtable byte+0x44 resolves to three source target groups
of 34/13/3 original methods. Original source code is NOT committed,
no original function or engine callback is invoked.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_canonical_creator_bodies import (
    verify_public as verify_118_public,
)

PRIMARY_NETWORK_INIT_VA=0x240C8FE0
PRIMARY_NETWORK_INIT_BYTES=75
PRIMARY_NETWORK_INIT_SHA="fc701a745f638b46ebb9bacae97369d2a46545dd9bef4351e500ecc8d20bbe92"
PRIMARY_NETWORK_INIT_PAD=5
SECONDARY_ADJUSTOR_VA=0x24054EC0
SECONDARY_ADJUSTOR_BYTES=4
SECONDARY_ADJUSTOR_SHA="b8bcfc02fb79817e275fdfb8a7c0c73dc6dff41e2b950288a2668b7d0ff6e92d"
SECONDARY_ADJUSTOR_PAD=12
FIFTY_VTABLE_PAIRS_SHA="42db228cc0ea0cb4c213d46e1140656d0dea885e0cd8d56994ff472dbc4dc02e"
FIFTY_VIRTUAL_TRIPLES_SHA="ccfbb162a58b55ea5be781e55cef7b386a06327ab06b8b4950707a9d4b725413"
VIRTUAL44_COUNTS={
    0x24062070:34,
    0x24054E80:13,
    0x240982D0:3,
}
VIRTUAL_METHOD_IMMEDIATES=(
    (0,"8b442408"),             # load source network argument +8
    (12,"8b5604"),              # original object +4 interface vtable
    (15,"8d4e04"),              # this points to object+4 interface
    (20,"ff12"),                # secondary interface vtable byte+0
    (22,"8b0d38a72f24"),        # original engine global 0x242FA738
    (34,"e899f00500"),          # original helper 0x241280A0
    (39,"8d8e6c010000"),        # owner subobject +0x16C
    (45,"e85e050500"),          # original helper 0x24119570
    (54,"ff5244"),              # primary vtable byte+0x44
    (60,"e86ffeffff"),          # original helper 0x240C8E90
    (65,"897e50"),              # MOV owner +0x50, EDI
    (69,"b001"),                # return boolean TRUE in AL
    (72,"c20800"),              # thiscall stdcall-like RET 8
)

def need(valid:bool,message:str)->None:
    if not valid:raise ValueError(message)


def verify_public(source_header:Path,canonical_header:Path,
                  classes:Path,common_manifest:Path,common_header:Path):
    full,allocations,names=verify_118_public(
        canonical_header,classes,common_manifest,common_header)
    source=source_header.read_text("utf-8")
    match=re.search(r"kOriginalSourceVirtual44Routes\s*\{\{(.*?)\}\};",
                    source,re.S)
    need(match is not None,"source 50 inline primary vtable+0x44 routes absent")
    entries=re.findall(r"\{(\d+)u,\s*0x([0-9a-f]{8})u\}",match.group(1))
    need(len(entries)==50,"source 50 inline VTable routes incomplete")
    original_class_ordinals={o for o,_ in full[86]}
    found=set()
    counts={target:0 for target in VIRTUAL44_COUNTS}
    by_ordinal={}
    for ordtext,vahex in entries:
        ordinal=int(ordtext)
        target=int(vahex,16)
        need(ordinal in original_class_ordinals and ordinal not in found,
             "duplicate/unknown 86-byte original factory VTable route")
        found.add(ordinal)
        need(target in VIRTUAL44_COUNTS and
             0x24001000<=target<0x24275fba,
             "unknown original primary VTable +0x44 method")
        counts[target]+=1
        by_ordinal[ordinal]=target
    need(counts==VIRTUAL44_COUNTS and found==original_class_ordinals,
         "source original VTable+44 34/13/3 group count drift")
    return full,allocations,by_ordinal


def verify_original(path:Path,full,allocations,by_ordinal):
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "owner-original 2006 PE32 client.dll entire SHA256 mismatch")
    pe=PE(binary)
    codebase,code=pe.sections[".text"]
    database,rd=pe.sections[".rdata"]
    def read(va,n):
        offset=va-codebase
        need(n>=0 and 0<=offset<=len(code)-n,
             f"native source VA outside original .text: {va:#x}")
        return code[offset:offset+n]
    def dword_rdata(address):
        offset=address-database
        need(0<=offset<=len(rd)-4,
             f"original RTTI vtable outside .rdata at {address:#x}")
        return struct.unpack_from("<I",rd,offset)[0]
    whole=read(PRIMARY_NETWORK_INIT_VA,PRIMARY_NETWORK_INIT_BYTES)
    need(hashlib.sha256(whole).hexdigest()==PRIMARY_NETWORK_INIT_SHA,
         "whole original 75-byte network initializer SHA mismatch")
    need(read(PRIMARY_NETWORK_INIT_VA+PRIMARY_NETWORK_INIT_BYTES,
              PRIMARY_NETWORK_INIT_PAD)==b"\xCC"*PRIMARY_NETWORK_INIT_PAD,
         "source 75-byte initializer RET/INT3 boundary changed")
    for offset,opcode in VIRTUAL_METHOD_IMMEDIATES:
        witness=bytes.fromhex(opcode)
        need(whole[offset:offset+len(witness)]==witness,
             f"original network init virtual call/owner field drift at {offset}")
    for off,target in ((34,0x241280A0),(45,0x24119570),
                       (60,0x240C8E90)):
        displ,=struct.unpack_from("<i",whole,off+1)
        need(PRIMARY_NETWORK_INIT_VA+off+5+displ==target,
             "original network init downstream helper E8 target changed")
    thunk=read(SECONDARY_ADJUSTOR_VA,SECONDARY_ADJUSTOR_BYTES)
    need(thunk==bytes.fromhex("8d41fcc3") and
         hashlib.sha256(thunk).hexdigest()==SECONDARY_ADJUSTOR_SHA,
         "complete 4-byte native this-minus-four interface thunk drift")
    need(read(SECONDARY_ADJUSTOR_VA+SECONDARY_ADJUSTOR_BYTES,
              SECONDARY_ADJUSTOR_PAD)==b"\xCC"*SECONDARY_ADJUSTOR_PAD,
         "source secondary adjustor boundary changed")
    pair_bytes=b""
    target_bytes=b""
    for ordinal,_constructor in full[86]:
        class_va=int(allocations[ordinal]["creator_va"],16)
        body=read(class_va,86)
        primary,=struct.unpack_from("<I",body,42)
        secondary,=struct.unpack_from("<I",body,49)
        need(database<=primary<database+len(rd) and
             database<=secondary<database+len(rd),
             "original 2006 primary/secondary vtables outside .rdata")
        init=dword_rdata(primary+0x94)
        secondary_slot0=dword_rdata(secondary)
        virtual44=dword_rdata(primary+0x44)
        need(init==PRIMARY_NETWORK_INIT_VA and
             secondary_slot0==SECONDARY_ADJUSTOR_VA and
             virtual44==by_ordinal[ordinal],
             f"source 86B class primary/secondary vtable ABI changed at {ordinal}")
        pair_bytes+=struct.pack("<II",primary,secondary)
        target_bytes+=struct.pack("<III",init,secondary_slot0,virtual44)
    need(hashlib.sha256(pair_bytes).hexdigest()==FIFTY_VTABLE_PAIRS_SHA,
         "source 50 original primary/secondary vtable pointer pairs SHA drift")
    need(hashlib.sha256(target_bytes).hexdigest()==FIFTY_VIRTUAL_TRIPLES_SHA,
         "source 50 original virtual+0x94/+0x44/secondary+0x0 targets SHA drift")
    print("PASS Phase1723 owner-original all 50 inline creator primary+0x94 "
          "target 75B virtual init, secondary+0 target 4B this-4 adjustor, "
          "34/13/3 primary+0x44 targets and original VTable pair SHA")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_inline_factory_vtable_init.hpp"))
    parser.add_argument("--canonical-header",type=Path,default=Path(
        "include/sin_compat/original_client_canonical_creator_bodies.hpp"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--common-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--common-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    full,allocations,by_ordinal=verify_public(
        args.header,args.canonical_header,args.creator_catalog,
        args.common_manifest,args.common_header)
    if args.self_test:
        for n,digest in ((PRIMARY_NETWORK_INIT_BYTES,PRIMARY_NETWORK_INIT_SHA),
                         (SECONDARY_ADJUSTOR_BYTES,SECONDARY_ADJUSTOR_SHA)):
            need(hashlib.sha256(b"\x90"*n).hexdigest()!=digest,
                 "fabricated original native virtual method falsely authentic")
        print("PASS Phase1723 original-free 50 inline factory VTable "
              "dependencies 34/13/3, and whole 75B/4B source method SHA refusal")
    if args.client is not None:
        verify_original(args.client,full,allocations,by_ordinal)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1723: {exc}",file=sys.stderr)
        sys.exit(1)
