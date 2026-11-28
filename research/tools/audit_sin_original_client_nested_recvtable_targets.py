#!/usr/bin/env python3
"""Phase1697: original SiN 2006 PE32 static nested RecvTable edge audit.

Original client is PRIVATE and SHA-pinned. Each of the 191 ordinary type5
RecvProps must lead to the REAL child RecvTable object through either an
original MOV EAX, [pointer-global] or a literal PUSH table address. This
recovers SOURCE GRAPH EDGES only, never executable engine-owned tables.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_class_registry import need
from audit_sin_original_client_class_recvtable_links import scan_table_constructors
from audit_sin_original_client_recvprop_type_inventory import (
    verify as verify_native_types, TYPE_NESTED_TABLE,
)
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

TOTAL_STATIC_EDGES = 191
GLOBAL_POINTER_EDGES = 171
DIRECT_POINTER_EDGES = 20
UNIQUE_TARGET_TABLES = 46
CLASS_ATTACHED_TARGETS = 28
AUXILIARY_TARGETS = 18
MAX_CLASS_LINK_DEPTH = 9
ALL_STATIC_EDGES_SHA = "1ae2fc278381df74ab9d08179168169d87756f6e3f59dc86956c2ea6d2689242"
SOURCE_NESTED_FACTORY = 0x24126B50
SOURCE_NESTED_DEFAULT_PROXY = 0x24126CE0


def source_rel32_call(pe: PEImage, site: int, expected: int) -> None:
    raw = pe.at(site, 5)
    need(raw[0] == 0xE8 and
         site + 5 + struct.unpack("<i",raw[1:])[0] == expected,
         f"original nested type5 constructor CALL changed {site:#x}")


def static_edge(pe: PEImage, source_site: int, tables: dict) -> dict:
    # The original normal RecvProp source caller directly PUSHes its
    # name, destination pointer and calls original 0x24126B50.
    need(pe.at(source_site,1)==b"\x68" and
         pe.at(source_site+5,1)==b"\x68",
         "original nested property name/output PUSH sequence changed")
    source_rel32_call(pe,source_site+10,SOURCE_NESTED_FACTORY)
    # The source type5 factory takes a REAL RecvTable pointer through
    # a preceding MOV EAX,[g]; PUSH proxy; PUSH EAX or direct PUSH va.
    before = pe.at(source_site-32,32)
    candidates = []
    for i in range(len(before)-4):
        operand = struct.unpack_from("<I",before,i+1)[0]
        if before[i]==0xA1 and pe.section_of(operand)==".data":
            try:
                target = struct.unpack("<I",pe.at(operand,4))[0]
            except VerificationError:
                continue
            if target in tables:
                candidates.append(("global",source_site-32+i,target,operand))
        elif before[i]==0x68 and operand in tables:
            candidates.append(("direct",source_site-32+i,operand,None))
    need(len(candidates)==1,
         f"original nested child pointer is not unique {source_site:#x}")
    kind,operand_site,target,global_va = candidates[0]
    if kind=="global":
        need(operand_site==source_site-15,
             "original indirect nested pointer not expected source position")
        need(pe.at(operand_site,15) ==
             b"\xA1"+struct.pack("<I",global_va)+
             b"\x68"+struct.pack("<I",SOURCE_NESTED_DEFAULT_PROXY)+
             b"\x50\x6A\x00\x6A\x00",
             "original indirect RecvTable+proxy argument order changed")
    else:
        need(operand_site in (source_site-9,source_site-12),
             "original direct nested pointer not at source stack position")
        tail = pe.at(operand_site,source_site-operand_site)
        need(tail[:5]==b"\x68"+struct.pack("<I",target) and
             tail[5:7]==b"\x6A\x00",
             "original direct RecvTable literal/offset argument order changed")
        if operand_site==source_site-9:
            need(tail[7]==0x6A, "original 8-bit direct table offset PUSH changed")
        else:
            need(tail[7]==0x68, "original 32-bit direct table offset PUSH changed")
    return {
        "original_pointer_access_kind":kind,
        "source_operand_va":hex(operand_site),
        "source_pointer_global_va":hex(global_va) if global_va else None,
        "source_nested_table_va":hex(target),
        "source_nested_table_name":tables[target]["name"],
        "source_nested_table_declared_properties":tables[target]["count"],
    }


def ordered_fingerprint(edges: list[dict]) -> str:
    plain="".join(
        f'{r["source_class"]}|{r["source_class_recvtable"]}|'
        f'{r["source_property_slot"]}|{r["source_property_name"]}|'
        f'{r["source_property_init_va"]}|'
        f'{r["source_nested_table_name"]}|{r["source_nested_table_va"]}|'
        f'{r["original_pointer_access_kind"]}|{r["source_operand_va"]}|'
        f'{r["source_pointer_global_va"] or "-"}\n'
        for r in edges
    )
    return hashlib.sha256(plain.encode("ascii")).hexdigest()


def class_graph_max_depth(edges: list[dict], classes: list[dict]) -> int:
    # Restrict to source class-attached tables. The auxiliary nested
    # table property graph remains outside this phase.
    sources={cl["client_class_name"]:int(cl["source_recvtable_va"],16)
             for cl in classes}
    graph=defaultdict(list)
    for edge in edges:
        graph[sources[edge["source_class"]]].append(
            int(edge["source_nested_table_va"],16))
    memo={}
    active=set()
    def depth(source: int) -> int:
        if source in active:
            raise VerificationError("unexpected cycle in source class-attached graph")
        if source in memo:return memo[source]
        active.add(source)
        d=1+max((depth(t) for t in graph.get(source,())),default=0)
        active.remove(source)
        memo[source]=d
        return d
    return max((depth(source) for source in sources.values()),default=0)


def verify(client: bytes, *, full_inventory: bool = False) -> dict:
    need(hashlib.sha256(client).hexdigest()==ORIGINALS["client"]["sha256"],
         "whole original private client DLL SHA mismatch")
    pe=PEImage(client)
    native=verify_native_types(client,full_inventory=True)
    classes=native["source_class_and_prop_inventory"]
    need(len(classes)==191,"source 191 class names must be fully authenticated")
    tables={}
    for table in scan_table_constructors(pe):
        if table["table_va"] in tables:
            old=tables[table["table_va"]]
            need((old["name"],old["count"],old["props_va"]) ==
                 (table["name"],table["count"],table["props_va"]),
                 "repeated original child table constructor differs")
        tables[table["table_va"]]=table
    need(len(tables)==211,"expected 211 source static RecvTable objects")
    edges=[]
    for cl in classes:
        for prop in cl["properties"]:
            if prop["original_native_property_type"]!=TYPE_NESTED_TABLE or \
               prop["native_constructor_route"]!="standard":
                continue
            init_site=int(prop["source_callsite_va"],16)
            link=static_edge(pe,init_site,tables)
            edges.append({
                "source_class":cl["client_class_name"],
                "source_class_recvtable":cl["source_table_name"],
                "source_property_name":prop["source_property_name"],
                "source_property_slot":prop["slot"],
                "source_property_init_va":hex(init_site),
                **link,
            })
    need(len(edges)==TOTAL_STATIC_EDGES,
         "source ordinary type5 RecvTable link count not 191")
    kinds=Counter(e["original_pointer_access_kind"] for e in edges)
    need(kinds=={"global":GLOBAL_POINTER_EDGES,"direct":DIRECT_POINTER_EDGES},
         "source direct vs indirect nested RecvTable pointer balance changed")
    targets={int(e["source_nested_table_va"],16) for e in edges}
    classes_table_vas={int(cl["source_recvtable_va"],16) for cl in classes}
    need(len(targets)==UNIQUE_TARGET_TABLES and
         len(targets&classes_table_vas)==CLASS_ATTACHED_TARGETS and
         len(targets-classes_table_vas)==AUXILIARY_TARGETS,
         "source nested RecvTable target sets differ")
    need(class_graph_max_depth(edges,classes)==MAX_CLASS_LINK_DEPTH,
         "source class-attached nested table graph depth changed")
    signature=ordered_fingerprint(edges)
    need(signature==ALL_STATIC_EDGES_SHA,
         "full original static RecvTable edge fingerprint drift")
    result={
       "status":"ORIGINAL_CLIENT_ALL_191_STATIC_NESTED_RECVTABLE_EDGES_AUTHENTICATED",
       "original_client_sha256":ORIGINALS["client"]["sha256"],
       "static_type5_links":len(edges),
       "original_indirect_global_links":kinds["global"],
       "original_direct_literal_links":kinds["direct"],
       "unique_static_nested_target_tables":len(targets),
       "attached_class_table_targets":len(targets&classes_table_vas),
       "auxiliary_recvtable_targets":len(targets-classes_table_vas),
       "class_attached_subgraph_max_depth":MAX_CLASS_LINK_DEPTH,
       "class_attached_subgraph_cycles":0,
       "source_static_edge_manifest_sha256":signature,
       "dynamic_type5_array_wrappers_covered_by_separate_audit":11,
       "limitations":("Only source table linkage metadata. The 18 auxiliary "
                      "table RecvProp contents, dynamic child allocation, "
                      "proxy ABI, runtime class factories and game engine "
                      "slot5 execution remain unimplemented."),
    }
    if full_inventory:result["source_static_nested_edges"]=edges
    return result


def self_test() -> None:
    # Original-free metadata mutation test, never a fake retail PE32.
    fabricated=[{
        "source_class":"CFake","source_class_recvtable":"DT_Fake",
        "source_property_slot":0,"source_property_name":"baseclass",
        "source_property_init_va":"0x10000000","source_nested_table_name":"DT_Base",
        "source_nested_table_va":"0x20000000",
        "original_pointer_access_kind":"direct",
        "source_operand_va":"0x0ffffff7","source_pointer_global_va":None,
    }]
    base=ordered_fingerprint(fabricated)
    need(len(base)==64 and base!=ALL_STATIC_EDGES_SHA,
         "fabricated source digest unexpectedly equals genuine original")
    for field in fabricated[0]:
        rows=[dict(fabricated[0])]
        rows[0][field]=str(rows[0][field])+"x"
        need(ordered_fingerprint(rows)!=base,
             "synthetic nested link metadata mutation accepted: "+field)
    try:verify(b"fabricated original retail client")
    except VerificationError:pass
    else:raise AssertionError("fabricated retail client accepted as original")
    print("PASS Phase1697 191 original static type5 RecvTable edges: "
          "171 global / 20 direct, 46 targets, original-free metadata "
          "mutation and wrong whole-image SHA refusal")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="owner-private unchanged original client.dll")
    parser.add_argument("--full-json",type=Path,
                        help="owner-private original 191 nested link metadata export")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.client is None and not args.self_test:
        parser.error("supply --self-test or --client")
    if args.full_json is not None and args.client is None:
        parser.error("--full-json requires owner-private original client.dll")
    if args.self_test:self_test()
    if args.client is not None:
        result=verify(args.client.read_bytes(),
                      full_inventory=args.full_json is not None)
        if args.full_json is None:
            print(json.dumps(result,indent=2,sort_keys=True))
        else:
            args.full_json.write_text(
                json.dumps(result,indent=2,sort_keys=True)+"\n",
                encoding="utf-8")
            print("PASS 191 original ordinary type5 static table links "
                  "and source target name/VA fingerprint exported; "
                  "no native executable pointers or original DLL code")

if __name__=="__main__":
    main()
