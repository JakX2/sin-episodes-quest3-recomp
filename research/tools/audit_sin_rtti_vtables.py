#!/usr/bin/env python3
"""Recover original 2006 SiN MSVC x86 RTTI class hierarchies and vfptr slots.

Metadata only, user-owned DLLs only. PE32 MSVC:
  TypeDescriptor: two pointer fields + .?AVClass@@ name
  CompleteObjectLocator: signature, this-offset, cd-offset, TD ptr, CHD ptr
  ClassHierarchyDescriptor: signature, attrs, base-count, base-array ptr
  BaseClassDescriptor: TD ptr, contained-count, PMD offsets, attrs, CHD ptr
  vfptr: preceding .rdata dword points to COL; subsequent executable pointers

No game original source or method bodies are generated. RTTI ancestry arrays
are flattened, not proof that every listed base is an immediate superclass.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, PEImage, VerificationError,
)

DECORATED = re.compile(rb"\.\?A[VU][A-Za-z0-9_$?<>@]+@@\x00")
MAX_VFUNCS=1024
PAIRS={
 "client":[("C_WeaponMagnum","C_BaseHLCombatWeapon"),
           ("C_WeaponAssaultRifle","C_BaseHLCombatWeapon"),
           ("C_WeaponScatterGun","C_BaseHLCombatWeapon")],
 "server":[("CNPC_SintekGrunt","CNPC_GruntBase"),
           ("CNPC_SintekJetpack","CNPC_JetpackGruntBase"),
           ("CNPC_GruntBase","CAI_BaseHumanoid"),
           ("CWeaponMagnum","CBaseHLCombatWeapon"),
           ("CWeaponAssaultRifle","CHLSelectFireMachineGun"),
           ("CWeaponScatterGun","CBaseHLCombatWeapon"),
           ("CPropPhone","CBaseAnimating"),
           ("CNPC_Quadralex","CAI_BaseNPC")],
}


def section(pe:PEImage,name:str)->tuple[int,bytes]:
    for tag,rva,_,offset,size in pe.sections:
        if tag==name:
            if offset+size>len(pe.data):
                raise VerificationError("truncated "+name)
            return pe.base+rva,pe.data[offset:offset+size]
    raise VerificationError("missing "+name)


def words(pe:PEImage,va:int,count:int,tag:str)->tuple[int,...]|None:
    if pe.section_of(va)!=tag or pe.section_of(va+count*4-1)!=tag:
        return None
    try:
        return struct.unpack("<"+"I"*count,pe.at(va,count*4))
    except (VerificationError,struct.error):
        return None


def scan(blob:bytes,dll:str,*,check_hash:bool=True)->dict:
    if dll not in PAIRS:raise VerificationError("expected client or server")
    digest=hashlib.sha256(blob).hexdigest()
    expected=CLIENT_SHA256 if dll=="client" else SERVER_SHA256
    if check_hash and digest!=expected:
        raise VerificationError("original SiN "+dll+".dll SHA-256 mismatch")
    pe=PEImage(blob)
    if pe.base!=(0x24000000 if dll=="client" else 0x22000000):
        raise VerificationError("wrong original SiN image base")
    dbase,ddata=section(pe,".data")
    rbase,rdata=section(pe,".rdata")
    section(pe,".text")
    types={}
    for hit in DECORATED.finditer(ddata):
        if hit.start()<8:continue
        va=dbase+hit.start()-8
        decorated=hit.group()[:-1].decode("ascii")
        types[va]={
            "class":decorated[4:-2],
            "decorated":decorated,
            "type_descriptor_va":f"0x{va:08X}",
        }
    locators={}
    invalid=Counter()
    for offset in range(0,len(rdata)-19,4):
        signature,displacement,constructor_displacement,td,chd=struct.unpack_from(
            "<5I",rdata,offset)
        if signature!=0 or td not in types:continue
        if displacement>0x100000 or constructor_displacement>0x100000:
            invalid["displacement"]+=1;continue
        hierarchy=words(pe,chd,4,".rdata")
        if hierarchy is None:
            invalid["hierarchy_oob"]+=1;continue
        sig,attributes,count,basearray=hierarchy
        if sig!=0 or count<1 or count>256:
            invalid["hierarchy_bad"]+=1;continue
        pointers=words(pe,basearray,count,".rdata")
        if pointers is None:
            invalid["base_array_oob"]+=1;continue
        bases=[]
        for ptr in pointers:
            entry=words(pe,ptr,7,".rdata")
            if entry is None or entry[0] not in types:
                bases=[];break
            parent,contained,md,pd,vd,flags,parenthierarchy=entry
            to_signed=lambda x: x if x<2**31 else x-2**32
            bases.append({
                "class":types[parent]["class"],
                "contained_bases":contained,
                "mdisp":to_signed(md),"pdisp":to_signed(pd),"vdisp":to_signed(vd),
                "attributes":flags,
            })
        if len(bases)!=count or bases[0]["class"]!=types[td]["class"]:
            invalid["base_description"]+=1;continue
        va=rbase+offset
        locators[va]={
            "class":types[td]["class"],"decorated":types[td]["decorated"],
            "locator_va":f"0x{va:08X}",
            "this_displacement":displacement,
            "construction_displacement":constructor_displacement,
            "hierarchy_va":f"0x{chd:08X}",
            "hierarchy_attributes":attributes,
            "bases":bases,
        }
    vtables=[]
    for offset in range(0,len(rdata)-7,4):
        col=struct.unpack_from("<I",rdata,offset)[0]
        if col not in locators:continue
        vfptr=rbase+offset+4
        slots=[]
        for slot in range(MAX_VFUNCS):
            fn=words(pe,vfptr+slot*4,1,".rdata")
            if fn is None or pe.section_of(fn[0])!=".text":break
            # Verify first byte is present in the actual executable raw section.
            try:pe.at(fn[0],1)
            except VerificationError:break
            slots.append(f"0x{fn[0]:08X}")
        if not slots:continue
        c=locators[col]
        vtables.append({
            "class":c["class"],"decorated":c["decorated"],
            "locator_va":c["locator_va"],"vfptr_va":f"0x{vfptr:08X}",
            "this_displacement":c["this_displacement"],
            "primary":c["this_displacement"]==0,
            "slot_count":len(slots),"functions":slots,
        })
    primary={v["class"]:v for v in vtables if v["primary"]}
    comparisons=[]
    for child,base in PAIRS[dll]:
        if child not in primary or base not in primary:continue
        a=primary[child];b=primary[base]
        changed=[
            {"slot":i,"parent_va":before,"child_va":after}
            for i,(before,after) in enumerate(zip(b["functions"],a["functions"]))
            if before!=after
        ]
        comparisons.append({
            "derived_class":child,"base_class":base,
            "derived_slots":a["slot_count"],"base_slots":b["slot_count"],
            "changed_shared_slots":len(changed),
            "additional_derived_slots":max(0,a["slot_count"]-b["slot_count"]),
            "changed":changed,
        })
    hierarchy_by_class={}
    for col in locators.values():
        name=col["class"]
        if name not in hierarchy_by_class or col["this_displacement"]==0:
            hierarchy_by_class[name]=col["bases"]
    referenced={int(v["locator_va"],16) for v in vtables}
    return {
        "dll":dll+".dll","sha256":digest,
        "counts":{
            "type_descriptors":len(types),
            "qualified_complete_object_locators":len(locators),
            "locator_with_vftable":len(referenced),
            "distinct_vftables":len(vtables),
            "classes_with_hierarchy":len(hierarchy_by_class),
            "total_vftable_slots":sum(v["slot_count"] for v in vtables),
            "distinct_executable_vftable_targets":len({x for v in vtables for x in v["functions"]}),
            "secondary_vftables":sum(not v["primary"] for v in vtables),
            "rejected_potential_locators":dict(invalid),
            "hit_vtable_scan_limit":sum(v["slot_count"]==MAX_VFUNCS for v in vtables),
        },
        "types":list(types.values()),
        "hierarchies":hierarchy_by_class,
        "locators":list(locators.values()),
        "vtables":vtables,
        "focus_override_comparisons":comparisons,
        "limitations":[
            "Only RTTI-registered vftables; nonvirtual/RTTI-stripped functions not enumerated.",
            "Slot address changes include deleting destructors, thunks and overrides; no method names inferred.",
            "Flattened RTTI ancestry is not an immediate base-class graph.",
            "Function bodies are not decompiled or converted to C++.",
            "Vtable slot scans terminate at the first address outside original x86 .text.",
            "No Android/SourceVR ABI or playable engine-hosted SiN code is produced.",
        ],
    }


def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    try:
        report={"client":scan(args.client.read_bytes(),"client"),
                "server":scan(args.server.read_bytes(),"server")}
        report["cross_dll"]={
            "shared_hierarchy_names":sorted(set(report["client"]["hierarchies"])&
                                            set(report["server"]["hierarchies"])),
        }
        args.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,"SIN RTTI VTABLE AUDIT FAILED: "+str(exc)+"\n")
    print(json.dumps({side:report[side]["counts"]
                      for side in ("client","server")},indent=2))
    return 0


if __name__=="__main__":raise SystemExit(main())
