#!/usr/bin/env python3
"""Phase1698: original SiN 2006 PE32 20 auxiliary RecvTables/171 properties.

Only the user's SHA-pinned unchanged original client.dll is inspected.
No original code/data segments are committed to GitHub. Produces source
metadata; not runnable Win32 RecvTable memory or network proxy functions.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
from audit_sin_original_client_class_registry import need, registration_records
from audit_sin_original_client_class_recvtable_links import scan_table_constructors
from audit_sin_original_client_recvprop_type_inventory import (
    verify as verify_class_types, DIRECT_HELPERS, WRAPPER_HELPERS,
    TYPE_INTEGER, TYPE_FLOAT, TYPE_VECTOR3, TYPE_ARRAY, TYPE_NESTED_TABLE,
)
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

AUX_TABLES=20
AUX_PROP_SLOTS=171
NORMAL_NATIVE_CALLS=166
SPECIAL_NATIVE_CALLS=5
AUX_ORDERED_FINGERPRINT="0b0ad5e2abb9d0e9833acbf70ab166de70130a12b0a4de866eb3723e35dc4fa8"
AUX_NATIVE_TYPE_COUNTS={TYPE_INTEGER:75,TYPE_FLOAT:62,TYPE_VECTOR3:28,
                        TYPE_ARRAY:1,TYPE_NESTED_TABLE:5}
AUX_SOURCE_ROUTES={"standard":166,"nested-array":3,
                   "special-overlay":1,"array-helper":1}
ADDITIONAL_INTEGER_WRAPPER=0x241A8D80
ADDITIONAL_INTEGER_WRAPPER_SIZE=40
ADDITIONAL_INTEGER_WRAPPER_SHA="dc0a086f0f7bf32f2fe82a62e9dc92feb353d5374adc583b78cf6ef4d1a773a9"
ORIGINAL_OVERLAY_ARRAY_FACTORY=0x241271B0
ORIGINAL_OVERLAY_FACTORY_BYTES=657
ORIGINAL_OVERLAY_FACTORY_SHA="b2e9bf21b248f751855489484f6ac04d4a8809473406e3287701a1ed4188c670"

# Original source debugger-independent noncanonical constructors.
# (source child table name, 0-based property slot, original property name,
#  native helper, native type tag, source output PUSH VA, original name RVA,
#  method route)
SPECIAL_SOURCE_PROPS=(
    ("DT_OverlayVars",0,"m_AnimOverlay",0x241271B0,5,
     0x240C36DA,0x242884D0,"special-overlay"),
    ("DT_BCCLocalPlayerExclusive",1,"m_hMyWeapons",0x24126E90,5,
     0x240C37E3,0x242888A4,"nested-array"),
    ("DT_Local",0,"m_chAreaBits",0x24126E90,5,
     0x240D3346,0x2428A810,"nested-array"),
    ("DT_LocalPlayerExclusive",5,"m_iAmmo",0x24126E90,5,
     0x240D39B6,0x242888FC,"nested-array"),
    ("DT_LocalPlayerExclusive",17,"m_hViewModel",0x24126BD0,4,
     0x240D3B06,0x2428A43C,"array-helper"),
)

def rel32(pe: PEImage, address: int, expected: int) -> None:
    raw=pe.at(address,5)
    need(raw[0]==0xE8 and
         address+5+struct.unpack("<i",raw[1:])[0]==expected,
         "source auxiliary original relative CALL target drift "+hex(address))

def section_bytes(pe: PEImage, name: str):
    sec=next(s for s in pe.sections if s[0]==name)
    _,rva,vs,raw,rs=sec
    return pe.base+rva,pe.data[raw:raw+min(vs,rs)]

def original_named_properties(pe: PEImage, slots: dict) -> dict:
    base,code=section_bytes(pe,".text")
    namebase,chars=section_bytes(pe,".rdata")
    names={namebase+m.start():m.group()[:-1].decode("ascii")
           for m in re.finditer(rb"[A-Za-z_][A-Za-z0-9_\[\].]{0,95}\x00",chars)}
    helpers={**DIRECT_HELPERS,
             **{va:tag for va,(_,tag) in WRAPPER_HELPERS.items()},
             ADDITIONAL_INTEGER_WRAPPER:TYPE_INTEGER}
    properties={}
    for i in range(len(code)-15):
        if code[i]!=0x68 or code[i+5]!=0x68 or code[i+10]!=0xE8:
            continue
        nameptr=struct.unpack_from("<I",code,i+1)[0]
        name=names.get(nameptr)
        dest=struct.unpack_from("<I",code,i+6)[0]
        if name is None or dest not in slots:continue
        helper=base+i+15+struct.unpack_from("<i",code,i+11)[0]
        if helper not in helpers:continue
        need(dest not in properties,
             "duplicate original auxiliary RecvProp normal initializer")
        properties[dest]={
            "name":name,
            "native_type":helpers[helper],
            "source_constructor_va":hex(helper),
            "source_callsite_va":hex(base+i),
            "native_constructor_route":"standard",
        }
    need(len(properties)==NORMAL_NATIVE_CALLS,
         f"expected {NORMAL_NATIVE_CALLS} original ordinary auxiliary properties")
    return properties

def verify_special(pe: PEImage, table_map: dict, found: dict) -> None:
    for name,slot,prop,helper,tag,site,name_va,route in SPECIAL_SOURCE_PROPS:
        tables=[v for v in table_map.values() if v["name"]==name]
        need(len(tables)==1,"source auxiliary RecvTable table name not unique")
        table=tables[0]
        addr=table["props_va"]+slot*60
        need(addr not in found,"noncanonical source overwrote a normal property")
        need(pe.at(site,5)==b"\x68"+struct.pack("<I",addr),
             "source auxiliary noncanonical output descriptor PUSH changed")
        if route in ("nested-array","special-overlay"):
            need(pe.at(site+5,2)==b"\xF3\xA5",
                 "original auxiliary array property rep movsd changed")
            callsite=site+7
        else:callsite=site+5
        rel32(pe,callsite,helper)
        need(pe.section_of(name_va)==".rdata" and
             pe.at(name_va,len(prop)+1)==prop.encode("ascii")+b"\x00",
             "source noncanonical RecvProp exact string literal changed")
        found[addr]={
            "name":prop,
            "native_type":tag,
            "source_constructor_va":hex(helper),
            "source_callsite_va":hex(site),
            "native_constructor_route":route,
        }

def fingerprint(tables: list[dict]) -> str:
    return hashlib.sha256("".join(
        f'{t["table_name"]}|{t["source_table_va"]}|'
        f'{p["slot"]}|{p["name"]}|{p["native_type"]}|'
        f'{p["source_constructor_va"]}|{p["source_callsite_va"]}|'
        f'{p["native_constructor_route"]}\n'
        for t in tables for p in t["properties"]
    ).encode("ascii")).hexdigest()

def verify(client: bytes, *, full_inventory: bool = False) -> dict:
    need(hashlib.sha256(client).hexdigest()==ORIGINALS["client"]["sha256"],
         "whole original private PE32 client DLL does not match")
    pe=PEImage(client)
    # Phase1697 already attested 939 class-attached type DWORDs. This
    # audit extends that identity to all remaining 20 source tables.
    native=verify_class_types(client)
    need(native["original_declared_recprops"]==939,
         "source previous 939 class type inventory drift")
    need(hashlib.sha256(pe.at(ADDITIONAL_INTEGER_WRAPPER,
                              ADDITIONAL_INTEGER_WRAPPER_SIZE)).hexdigest()
         ==ADDITIONAL_INTEGER_WRAPPER_SHA and
         pe.at(ADDITIONAL_INTEGER_WRAPPER+ADDITIONAL_INTEGER_WRAPPER_SIZE,1)
         ==b"\xCC",
         "original 40-byte additional int native factory not authenticated")
    rel32(pe,0x241A8D9C,0x24126DF0)
    need(hashlib.sha256(pe.at(ORIGINAL_OVERLAY_ARRAY_FACTORY,
                              ORIGINAL_OVERLAY_FACTORY_BYTES)).hexdigest()
         ==ORIGINAL_OVERLAY_FACTORY_SHA and
         pe.at(ORIGINAL_OVERLAY_ARRAY_FACTORY+ORIGINAL_OVERLAY_FACTORY_BYTES,8)
         ==b"\xCC"*8,
         "full original 657-byte overlay array constructor digest/boundary changed")
    need(pe.at(ORIGINAL_OVERLAY_ARRAY_FACTORY,6)==b"\x81\xEC\x84\x00\x00\x00"
         and pe.at(0x241271E2,8)==b"\xC7\x44\x24\x24\x05\x00\x00\x00",
         "original special overlay nested RecvProp type5 write altered")
    need(pe.at(0x24127231,2)==b"\x6A\x1C" and
         pe.at(0x241272F1,2)==b"\x6A\x14" and
         pe.at(0x241273B5,2)==b"\x6A\x14",
         "original overlay 28-byte/20-byte auxiliary allocation requests changed")
    rel32(pe,0x24127324,0x24126B50)
    need(pe.at(0x2412734C,8)==
         b"\xC7\x44\x24\x18"+struct.pack("<I",0x242FB110) and
         pe.at(0x24127386,4)==b"\x8B\x09\x89\x0A",
         "original overlay generated numeric name lookup/store changed")
    static=scan_table_constructors(pe)
    table_map={}
    for t in static:
        old=table_map.get(t["table_va"])
        if old is not None:
            need((old["name"],old["count"],old["props_va"]) ==
                 (t["name"],t["count"],t["props_va"]),
                 "repeated original static RecvTable constructor changed")
        table_map[t["table_va"]]=t
    need(len(table_map)==211,"original static RecvTable object count changed")
    class_vas={cl["recvtable_va"] for cl in registration_records(pe)}
    need(len(class_vas)==191,"source class-attached 191 table VAs changed")
    auxiliary=[t for va,t in table_map.items() if va not in class_vas]
    need(len(auxiliary)==AUX_TABLES and
         sum(t["count"] for t in auxiliary)==AUX_PROP_SLOTS,
         "not exactly 20 original auxiliary tables and 171 declared slots")
    destinations={
        t["props_va"]+idx*60:(t,idx)
        for t in auxiliary for idx in range(t["count"])
    }
    need(len(destinations)==AUX_PROP_SLOTS,
         "overlapping original auxiliary RecvProp descriptor VAs")
    found=original_named_properties(pe,destinations)
    verify_special(pe,table_map,found)
    need(len(found)==AUX_PROP_SLOTS,
         "not all 171 original auxiliary RecvProps have authentic constructors")
    result_tables=[]
    type_counts=Counter()
    route_counts=Counter()
    for t in sorted(auxiliary,key=lambda x:x["table_va"]):
        props=[]
        for idx in range(t["count"]):
            address=t["props_va"]+idx*60
            p=found[address]
            type_counts[p["native_type"]]+=1
            route_counts[p["native_constructor_route"]]+=1
            props.append({
                "slot":idx,
                "source_property_descriptor_va":hex(address),
                **p,
            })
        result_tables.append({
            "table_name":t["name"],
            "source_table_va":hex(t["table_va"]),
            "source_props_array_va":hex(t["props_va"]),
            "declared_props":t["count"],
            "properties":props,
        })
    need(dict(type_counts)==AUX_NATIVE_TYPE_COUNTS,
         "source original auxiliary native type0..5 distribution changed")
    need(dict(route_counts)==AUX_SOURCE_ROUTES,
         "source original auxiliary constructor route distribution changed")
    digest=fingerprint(result_tables)
    need(digest==AUX_ORDERED_FINGERPRINT,
         "ordered all 171 native auxiliary RecvProp fingerprint changed")
    result={
       "status":"ORIGINAL_CLIENT_ALL_20_AUX_RECVTABLES_AND_171_PROP_TYPES_AUTHENTICATED",
       "original_client_sha256":ORIGINALS["client"]["sha256"],
       "static_original_recvtable_total":211,
       "class_attached_recvtable_objects":191,
       "auxiliary_recvtable_objects":len(result_tables),
       "class_attached_original_prop_slots":939,
       "auxiliary_original_prop_slots":AUX_PROP_SLOTS,
       "complete_original_static_prop_slots":1110,
       "auxiliary_native_type_counts":{str(k):type_counts[k] for k in sorted(type_counts)},
       "auxiliary_source_constructor_routes":dict(route_counts),
       "full_ordered_aux_slots_sha256":digest,
       "source_complete_overlay_array_method_va":hex(ORIGINAL_OVERLAY_ARRAY_FACTORY),
       "source_complete_overlay_array_method_bytes":ORIGINAL_OVERLAY_FACTORY_BYTES,
       "source_complete_overlay_array_method_sha256":ORIGINAL_OVERLAY_FACTORY_SHA,
       "limitations":("Static RecvTable names and native numeric type metadata "
                      "only; generated nested element contents, callbacks, "
                      "proxies, original engine packet ABI and live slot5 "
                      "are not implemented. Deferred renderer/props/physics "
                      "work remains out of scope."),
    }
    if full_inventory:result["source_auxiliary_20_tables"]=result_tables
    return result

def self_test() -> None:
    fabricated=[{"table_name":"DT_Fake","source_table_va":"0x10000000",
       "properties":[{"slot":0,"name":"fake","native_type":0,
         "source_constructor_va":"0x20000000",
         "source_callsite_va":"0x20000100",
         "native_constructor_route":"standard"}]}]
    sha=fingerprint(fabricated)
    need(len(sha)==64 and sha!=AUX_ORDERED_FINGERPRINT,
         "fabricated metadata unexpectedly matches original SHA")
    for value in ("fake","fake_mutated"):
        fabricated[0]["properties"][0]["name"]=value
        if value=="fake_mutated":
            need(fingerprint(fabricated)!=sha,
                 "synthetic field name mutation accepted")
    need(sum(AUX_NATIVE_TYPE_COUNTS.values())==AUX_PROP_SLOTS,
         "source auxiliary type count does not sum to 171")
    try:verify(b"synthetic retail game client.dll")
    except VerificationError:pass
    else:raise AssertionError("fabricated game client accepted as original")
    print("PASS Phase1698 original 20 auxiliary RecvTables, 171 slots: "
          "166 native ordinary + 5 alternate type paths, type0..5 "
          "mutation checks and wrong-original-SHA refusal")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="owner-private unchanged original 2006 client.dll")
    parser.add_argument("--full-json",type=Path,
                        help="owner-private 20-table/171-property source metadata")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.client is None and not args.self_test:
        parser.error("supply --self-test or --client")
    if args.full_json is not None and args.client is None:
        parser.error("--full-json requires owner-private original client DLL")
    if args.self_test:self_test()
    if args.client is not None:
        report=verify(args.client.read_bytes(),
                      full_inventory=args.full_json is not None)
        if args.full_json is None:
            print(json.dumps(report,indent=2,sort_keys=True))
        else:
            args.full_json.write_text(json.dumps(report,indent=2,sort_keys=True)+"\n",
                                     encoding="utf-8")
            print("PASS all original 20 auxiliary tables, 171 native typed fields "
                  "and source constructor metadata exported without DLL code")

if __name__=="__main__":
    main()
