#!/usr/bin/env python3
"""Phase1722: original 118 complete canonical native entity creator methods.

Exactly 68 original ClientClass factory methods are 60 executable bytes,
and 50 are 86 bytes. Both call the SHA-backed original 0x240C4FF0
zeroing allocator, call their original class-specific or shared
constructor, invoke native vtable byte offset +0x94, and return a
networkable subobject at original entity+8 on success (NULL on failure).
The 86B variant also writes four native 32-bit vtable pointer labels.

--self-test is original-free and authenticates published source identity
metadata against previous original-class and allocator-size catalogs.
--client reads the unchanged user-owned original DLL, checking entire
original executable method bodies via original aggregate SHA256, sparse
opcodes, normalized template SHA, precise E8 targets and RET/INT3
boundaries. Original executable code bytes are NOT committed to GitHub.
No actual constructors or native function pointers are published.
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
from audit_sin_original_client_entity_allocation_prologues import (
    original_free as verify_142_source_allocations,
)

COUNTS={60:68,86:50}
PAD={60:4,86:10}
BODY_SHA={
    60:"401286913c256886a575d8d55985c7b3579143e9623b897fecb13a6cc548d10a",
    86:"ee7e1deea1a0608ca47aefc56bf8b2c6ca389ca19f35d86e641c1435cf27d84b",
}
ALL_BODY_SHA="a0fe29c6f806dd85c7f1e46ae18c5d167a5e50493faf48640425245ca2131cfa"
NORMALIZED_SHA={
    60:"7b4cd8eb8b67ef453cf3e8c586708b78fb34e6fff0bf5e4c5b02c36b25e1ca3d",
    86:"f72e0856a850946a80e7332dd14ed37c15150062e8b154c940a81a9b4e8e0a06",
}
NORMALIZED_VARIABLES={
    60:tuple(range(2,6))+tuple(range(7,11))+tuple(range(21,25)),
    86:tuple(range(2,6))+tuple(range(7,11))+tuple(range(24,28))+
       tuple(range(42,46))+tuple(range(49,53))+
       tuple(range(55,59))+tuple(range(62,66)),
}
EXPECTED_60_86_CTOR_CALL_OFFSETS={60:20,86:23}
SOURCE_COMMON_ALLOCATOR=0x240C4FF0
SOURCE_CTOR_RETURN_SUBOBJECT=8
SOURCE_NETWORKABLE_VIRTUAL_OFFSET=0x94

def need(valid:bool,message:str)->None:
    if not valid:raise ValueError(message)


def parse_cpp_source(header:Path):
    txt=header.read_text("utf-8")
    result={}
    for length,name in ((60,"k60ByteConstructors"),
                        (86,"k86ByteConstructors")):
        m=re.search(r"\b"+name+r"\s*\{\{(.*?)\}\};",txt,re.S)
        need(m is not None,f"missing C++ source constructor array {name}")
        records=re.findall(r"\{(\d+)u,\s*0x([0-9a-f]{8})u\}",m.group(1))
        need(len(records)==COUNTS[length],
             f"original complete creator family {length} count drift")
        result[length]=[(int(n),int(v,16)) for n,v in records]
    return result


def verify_public(header:Path,classes:Path,common_manifest:Path,
                  common_header:Path):
    full=parse_cpp_source(header)
    with classes.open(newline="",encoding="utf-8") as f:
        names=list(csv.DictReader(f))
    need(len(names)==191,"original 191 client-class source catalog changed")
    common=verify_142_source_allocations(
        common_manifest,classes,common_header)
    allocations={int(rec["class_ordinal"]):rec for rec in common}
    need(len(allocations)==142,"source 142 common allocation routes changed")
    chosen=set()
    for length,rows in full.items():
        last=-1
        for ordinal,ctor in rows:
            need(last<ordinal<191 and ordinal not in chosen,
                 "source canonical body order/identity duplicated")
            last=ordinal
            chosen.add(ordinal)
            original=names[ordinal]
            need(original["kind"]=="E" and ordinal in allocations,
                 "full creator body not in authenticated original 142 entities")
            need(0x24001000<=ctor<0x24275fba,
                 "original 2006 PE32 constructor target outside .text")
    need(len(chosen)==118 and
         len(allocations.keys()-chosen)==24,
         "full canonical method / uncommon source class split changed")
    return full,allocations,names


def verify_original(path:Path,full,allocations):
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "owner-original 2006 client.dll entire SHA256 mismatch")
    pe=PE(binary)
    codebase,code=pe.sections[".text"]
    database,rd=pe.sections[".rdata"]
    def read(va,n):
        offset=va-codebase
        need(n>=0 and 0<=offset<=len(code)-n,
             f"original PE32 executable VA outside .text: {va:#x}")
        return code[offset:offset+n]
    combined=b""
    for length,records in full.items():
        concatenated=b""
        template=None
        for ordinal,constructor in records:
            identity=allocations[ordinal]
            va=int(identity["creator_va"],16)
            size=int(identity["object_size_bytes"])
            body=read(va,length)
            need(read(va+length,PAD[length])==b"\xCC"*PAD[length],
                 f"source {length} byte function native CC boundary drift at {va:#x}")
            need(read(va+length+PAD[length],1)!=b"\xCC",
                 f"source {length} byte original function padding length drift at {va:#x}")
            need(body[:2]==b"\x56\x68" and body[6]==0xE8 and
                 struct.unpack_from("<I",body,2)[0]==size,
                 f"source {length} original allocation request prologue drift at {va:#x}")
            alloc_displacement,=struct.unpack_from("<i",body,7)
            need(va+11+alloc_displacement==SOURCE_COMMON_ALLOCATOR,
                 f"original source zeroing allocator route drift {va:#x}")
            call_at=EXPECTED_60_86_CTOR_CALL_OFFSETS[length]
            need(body[call_at]==0xE8,
                 f"source {length} original constructor CALL changed {va:#x}")
            displacement,=struct.unpack_from("<i",body,call_at+1)
            need(va+call_at+5+displacement==constructor,
                 f"source {length} original entity constructor target changed {va:#x}")
            if length==60:
                # The ctor returns an object pointer in EAX. If null
                # the source returns null, otherwise it forwards two
                # network arguments through virtual method +0x94.
                need(body[31:35]==b"\x33\xC0\x5E\xC3" and
                     body[49:55]==bytes.fromhex("ff9094000000") and
                     body[55:60]==bytes.fromhex("8d46085ec3"),
                     "original 60B ctor success/failure/virtual ABI changed")
            else:
                # Inline vtable quartet at source base +0,+4,+8,+12.
                need(body[71:77]==bytes.fromhex("ff9094000000") and
                     body[77:86]==bytes.fromhex("8bc75f5ec333c05ec3"),
                     "original 86B ctor success/failure/virtual ABI changed")
                for off in (42,49,55,62):
                    vtable,=struct.unpack_from("<I",body,off)
                    need(database<=vtable<database+len(rd),
                         f"original native inline vtable label outside .rdata {va:#x}")
            normalized=bytearray(body)
            for off in NORMALIZED_VARIABLES[length]: normalized[off]=0
            if template is None:template=bytes(normalized)
            need(bytes(normalized)==template,
                 f"original {length} byte factory not the pinned opcode template {va:#x}")
            concatenated+=body
        need(hashlib.sha256(template).hexdigest()==NORMALIZED_SHA[length],
             f"source {length} byte normalized x86 complete opcode template drift")
        need(hashlib.sha256(concatenated).hexdigest()==BODY_SHA[length],
             f"all original {length} byte complete native methods SHA changed")
        combined+=concatenated
    need(hashlib.sha256(combined).hexdigest()==ALL_BODY_SHA,
         "full original 118 native entity constructor source SHA changed")
    print("PASS Phase1722 owner original 118 complete entity creator functions "
          "(68x60B +50x86B), exact constructor targets, four inline "
          "native vtables, virtual+0x94, return+8, and full SHA256")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
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
    full,allocations,_=verify_public(args.header,args.creator_catalog,
                                     args.common_manifest,args.common_header)
    if args.self_test:
        for length,count in COUNTS.items():
            fake=b"\x90"*(length*count)
            need(hashlib.sha256(fake).hexdigest()!=BODY_SHA[length],
                 "fabricated complete original entity function bodies accepted")
        print("PASS Phase1722 original-free 68x60B +50x86B source immutable "
              "constructor labels, 118/142 split and 24 deferred creators")
    if args.client is not None:
        verify_original(args.client,full,allocations)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1722: {exc}",file=sys.stderr)
        sys.exit(1)
