#!/usr/bin/env python3
"""Phase1722: audit all 142 COMPLETE original 2006 PE32 entity creator bodies.

This extends the prior 142 first 11-byte original allocator prologues
to the FULL x86 executable method of each one, including method-end
RET, real original constructor CALL, virtual+0x94 interface dispatch,
returned networkable subobject+8 and original INT3 padding (or the
exact 12 original bodies that have no padding at all).

Private --client compares each complete original method to the
immutable per-class original whole SHA and recomputes 10,770 bytes
concatenated source SHA, using the user-owned unchanged client.dll.
No executable x86 bytes or engine function pointers are committed.
This does NOT decompile or execute the 73 distinct constructor TARGET
methods, nor implement actual native object layouts, creator lifetimes
or a valid replacement VClient011.slot5.
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
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_entity_allocation_prologues import (
    original_free as verify_142_prologues,
)

SOURCE_CLASSES=142
SOURCE_CSV_SHA="3393a7ef2ddaefa2ff0c4a3d0cb6539ae165d764f3a97a42f4b177e5483f2530"
CONCAT_METHOD_SHA="0badb693b0f8c3977a74b182c1fa696bbd6f620e40c0acbbd33746933a9ac902"
CONCAT_EXECUTABLE_BYTES=10770
CTOR_FIRST=68
VTABLE_STAMPED=74
UNPADDED=12
CTOR_TARGETS=73
ORIGINAL_SOURCE_VIRTUAL_SLOT=0x94
NETWORKABLE_RETURN_ADJUSTMENT=8
CSV_HEADER=[
    "class_ordinal","network_class","creator_va",
    "allocation_request_bytes","complete_method_bytes",
    "original_constructor_call_va","original_constructor_target_va",
    "original_virtual_call_byte_offset","original_return_subobject_offset",
    "original_creator_family","native_int3_padding_bytes",
    "complete_function_sha256"
]

def need(yes:bool,message:str):
    if not yes:raise ValueError(message)


def verify_public(manifest:Path,cpp_header:Path,previous_manifest:Path,
                  creator_catalog:Path,prologue_header:Path):
    data=manifest.read_bytes().replace(b"\r\n",b"\n")
    need(hashlib.sha256(data).hexdigest()==SOURCE_CSV_SHA,
         "full original 142-source creator-body CSV SHA changed")
    with manifest.open(newline="",encoding="utf-8") as source:
        reader=csv.DictReader(source)
        need(reader.fieldnames==CSV_HEADER,"original full body source fields drift")
        rows=list(reader)
    previous=verify_142_prologues(previous_manifest,creator_catalog,
                                  prologue_header)
    need(len(rows)==SOURCE_CLASSES and len(previous)==SOURCE_CLASSES,
         "original creator body source count incomplete")
    contents=cpp_header.read_text("utf-8")
    match=re.search(r"kSourceCompleteCreatorBodies\s*\{\{(.*?)\}\};",
                    contents,re.S)
    need(match is not None,"native C++ complete method inventory missing")
    cpp=re.findall(
        r"\{(\d+)u,0x([0-9a-f]{8})u,(\d+)u,(\d+)u,"
        r"0x([0-9a-f]{8})u,(\d+)u,"
        r"CreatorBodyFamily::(\w+)\}",
        match.group(1))
    need(len(cpp)==SOURCE_CLASSES,
         "C++ source 142 complete creator-body records missing")
    kinds=Counter()
    lengths=Counter()
    nopadding=0
    targets=set()
    total_bytes=0
    for i,(r,p,c) in enumerate(zip(rows,previous,cpp)):
        ordinal=int(r["class_ordinal"])
        va=int(r["creator_va"],16)
        requested=int(r["allocation_request_bytes"])
        length=int(r["complete_method_bytes"])
        call=int(r["original_constructor_call_va"],16)
        target=int(r["original_constructor_target_va"],16)
        virtual=int(r["original_virtual_call_byte_offset"],16)
        returned=int(r["original_return_subobject_offset"])
        padding=int(r["native_int3_padding_bytes"])
        family=r["original_creator_family"]
        expected_family={
            "ctor_then_virtual":"ConstructThenVirtual",
            "vtable_stamped_then_virtual":"VtableStampThenVirtual"
        }.get(family)
        need(expected_family is not None,"unrecognized original creator method family")
        need(ordinal==int(p["class_ordinal"]) and
             r["network_class"]==p["network_class"] and
             va==int(p["creator_va"],16) and
             requested==int(p["object_size_bytes"]),
             f"original first-11-byte and whole-function creator mismatch at {i}")
        need(length>=60 and length<=117 and
             virtual==ORIGINAL_SOURCE_VIRTUAL_SLOT and
             returned==NETWORKABLE_RETURN_ADJUSTMENT and
             padding<=15 and
             0x24001000<=target<0x24275FBA,
             "invalid source native creator complete function fields")
        need(call==va+(20 if family=="ctor_then_virtual" else 23),
             "original constructor callback callsite no longer source-exact")
        need(len(r["complete_function_sha256"])==64 and
             all(ch in "0123456789abcdef" for ch in r["complete_function_sha256"]),
             "missing original complete-function SHA evidence")
        if family=="ctor_then_virtual":
            need(length==60 and padding==4,
                 "original 60-byte constructed-first body boundary drift")
        if padding==0:
            need(length==96 and family=="vtable_stamped_then_virtual",
                 "un-padded original methods are not authenticated")
            nopadding+=1
        need(
            (int(c[0]),int(c[1],16),int(c[2]),int(c[3]),
             int(c[4],16),int(c[5]),c[6])==
            (ordinal,va,requested,length,target,padding,expected_family),
            "portable C++ full creator-body metadata changed")
        total_bytes+=length
        kinds[family]+=1
        lengths[length]+=1
        targets.add(target)
    need(kinds=={"ctor_then_virtual":CTOR_FIRST,
                 "vtable_stamped_then_virtual":VTABLE_STAMPED},
         "complete source constructor body family classification changed")
    need(nopadding==UNPADDED and len(targets)==CTOR_TARGETS and
         total_bytes==CONCAT_EXECUTABLE_BYTES,
         "original constructor target/padding/whole executable byte census changed")
    return rows


def verify_original(path:Path,rows):
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original user 2006 PE32 client.dll whole hash changed")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def at(va,count):
        off=va-base
        need(count>=0 and 0<=off<=len(code)-count,
             "source native creator method outside original .text")
        return code[off:off+count]
    full_concatenated=b""
    for r in rows:
        va=int(r["creator_va"],16)
        length=int(r["complete_method_bytes"])
        ctorcall=int(r["original_constructor_call_va"],16)
        ctortarget=int(r["original_constructor_target_va"],16)
        original_pad=int(r["native_int3_padding_bytes"])
        method=at(va,length)
        need(hashlib.sha256(method).hexdigest()==r["complete_function_sha256"],
             f"whole original executable creator body SHA drift at {va:#x}")
        need(method[0:2]==b"\x56\x68" and method[6]==0xE8 and
             method[11:13] in (b"\x83\xC4",b"\x8B\xF0"),
             "original source common allocation prologue/body handoff drift")
        size,=struct.unpack_from("<I",method,2)
        need(size==int(r["allocation_request_bytes"]),
             "original whole creator request allocation size changed")
        call_offset=ctorcall-va
        need(method[call_offset]==0xE8 and
             ctorcall+5+
             struct.unpack_from("<i",method,call_offset+1)[0]==ctortarget,
             "original source creator internal constructor CALL changed")
        need(method.count(b"\xFF\x90\x94\x00\x00\x00")==1,
             "original creator virtual slot +0x94 call count differs")
        if r["original_creator_family"]=="ctor_then_virtual":
            # 60-byte family: allocation failure or null constructed
            # pointer returns an early zero; success calls original
            # constructor then virtual+94 and returns owner+8.
            need(method[31:35]==b"\x33\xC0\x5E\xC3" and
                 method[-5:]==b"\x8D\x46\x08\x5E\xC3",
                 "original 60B constructor-first failure/success return changed")
        else:
            # 74 vtable-stamped originals store native class vtables,
            # invoke original virtual +0x94, return address of
            # networkable subobject +8, and use final null return.
            need(method[-9:]==
                 b"\x8B\xC7\x5F\x5E\xC3\x33\xC0\x5E\xC3" and
                 method.count(b"\x8D\x7E\x08")==1,
                 "original vtable-stamped creator return and +8 adjustment drift")
        need(method[-1]==0xC3,
             "original native complete entity creator terminal RET missing")
        if original_pad:
            need(at(va+length,original_pad)==b"\xCC"*original_pad,
                 "original entity creator function INT3 padding differs")
        else:
            need(length==96 and at(va+length,1)!=b"\xCC",
                 "original 12 densely adjacent source creators no-padding drift")
        full_concatenated+=method
    need(len(full_concatenated)==CONCAT_EXECUTABLE_BYTES and
         hashlib.sha256(full_concatenated).hexdigest()==CONCAT_METHOD_SHA,
         "entire 10,770-byte original source creator method corpus changed")
    print("PASS Phase1722 owner-original all 142 COMPLETE x86 entity creator "
          "bodies/10,770B: 68 constructor-first +74 vtable-stamped, "
          "73 distinct constructor targets, native +0x94 virtual and +8 "
          "networkable return; no original functions executed")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_142_full_entity_creator_bodies_phase1722.csv"))
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_full_entity_creator_bodies.hpp"))
    parser.add_argument("--previous-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--prologue-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    rows=verify_public(args.manifest,args.header,args.previous_manifest,
                       args.creator_catalog,args.prologue_header)
    if args.self_test:
        need(hashlib.sha256(b"\x90"*CONCAT_EXECUTABLE_BYTES).hexdigest() !=
             CONCAT_METHOD_SHA,
             "fabricated complete original native entity creators authenticated")
        print("PASS Phase1722 original-free whole-method SHA/metadata "
              "integrity, 142 real creator bodies and 73 constructor targets")
    if args.client is not None:verify_original(args.client,rows)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1722: {exc}",file=sys.stderr)
        sys.exit(1)
