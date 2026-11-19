#!/usr/bin/env python3
"""Authenticated original SiN 2006 static-constructor vtables for BSS systems.

Each constructor's exact vptr-write instruction, RTTI COL class name
and virtual Init entry are checked against the owner's original PE32.
Never exports code bytes, just addresses and evidence hashes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE, CLIENT_SHA256
class AuditError(ValueError):pass
# Source order matches original VClient011::Init's twelve registrations.
# Fields: index, class name, object base VA, secondary base offset,
# vtable VA, COL VA, type descriptor VA, Init slot0 VA,
# constructor vtable-store VA, instruction kind, Init 16-byte SHA256.
CONSTRUCTED=(
 (1,"CGameStringPool",0x2436212C,0,0x2429AD94,0x242E709C,0x242FBC6C,0x240566C0,0x2426950F,"absolute",
  "28cf35ca20ab81145888939f458aa7ea20071702d666a8c9049eef19ce7dccc3"),
 (3,"CClientLeafSystem",0x24357F30,4,0x24297534,0x242E5BC0,0x242FA830,0x2410CA00,0x2410C7A6,"secondary",
  "28cf35ca20ab81145888939f458aa7ea20071702d666a8c9049eef19ce7dccc3"),
 (4,"CDetailObjectSystem",0x243589A8,0,0x24298694,0x242E644C,0x242FB0E8,0x24126300,0x241261FB,"this",
  "f43298ae6670d12c30ddb2a08d1374aa91873368cdcdd2670f7ee61aae8f968a"),
 (6,"CClientEffectPrecacheSystem",0x2433FD9C,0,0x24297334,0x242E597C,0x242FA708,0x241055A0,0x2410567A,"absolute",
  "28cf35ca20ab81145888939f458aa7ea20071702d666a8c9049eef19ce7dccc3"),
 (7,"CClientShadowMgr",0x24358280,0,0x24297C4C,0x242E5E98,0x242FA938,0x24118880,0x241179CC,"this",
  "5f53e5aa9cc03ff024407b8218a4d8ea6cb11c328f1389c8aed32454856aff8a"),
 (8,"C_StatsSystem",0x24378DE8,0,0x242AE124,0x242EB764,0x2431E2FC,0x241FCAA0,0x241FCCC4,"this",
  "25f4610d84e10b0d990ad358b88d9a8ac5d125f30e0f28ab3b9d5f1a576677ec"),
 (9,"C_DynamicDifficulty",0x24378D18,0,0x242ADF34,0x242EB648,0x2431E270,0x241FAC90,0x241FA4BB,"this",
  "e64a2533dfc4aab4755b47117343bc8947f20d4f1a9f79fcb250b9a43f2790f9"),
 (11,"C_SoundscapeSystem",0x2433E4F8,0,0x24293C34,0x242E4BD8,0x242FA038,0x240F4160,0x240F3BC2,"eax",
  "dc786f99f772336c10e6a531ee10294d5737ad13b6e1537b1b0c2153711a8d85")
)
BASE=0x24000000
DETAIL_INIT=(0x24126300,31,
    "0deaa3657822d8b54852e1a66ef806b10667f3217e80a63f51bea3a3b8a53939")
def verify_detail_full(pe,digest=DETAIL_INIT[2]):
    va,length,_=DETAIL_INIT
    if hashlib.sha256(pe.read(va,length)).hexdigest()!=digest:
        raise AuditError("original complete DetailObjectSystem Init SHA mismatch")
    # Original copies provider+0x28 twice to this+0x806C and +0x8070.
    # Distinct offsets/call order are backed by the complete 31-byte SHA.
    return {"original_va":hex(va),"complete_bytes":length,
            "original_sha256":DETAIL_INIT[2],
            "source_globals":["0x24358930","0x243588E8"],
            "source_offsets":[40,40],
            "destination_offsets":[0x806c,0x8070],
            "returns_true":True}

def store_bytes(obj,vt,kind):
    u32=lambda v:struct.pack("<I",v)
    if kind=="absolute":return b"\xc7\x05"+u32(obj)+u32(vt)
    if kind=="secondary":return b"\xc7\x40\x04"+u32(vt)
    if kind=="this":return b"\xc7\x06"+u32(vt)
    if kind=="eax":return b"\xc7\x00"+u32(vt)
    raise AuditError("unsupported constructor store encoding")
def verify_constructed(pe,specs=CONSTRUCTED):
    if pe.base!=BASE:raise AuditError("original image base mismatch")
    out=[]
    for (index,name,obj,delta,vt,col,td,init,store,kind,sha) in specs:
        instruction=store_bytes(obj,vt,kind)
        if pe.read(store,len(instruction))!=instruction:
            raise AuditError(f"{name}: original constructor vptr write mismatch")
        get=lambda addr:struct.unpack("<I",pe.read(addr,4))[0]
        if get(vt-4)!=col:
            raise AuditError(f"{name}: vtable RTTI locator mismatch")
        sig,actual_offset,cd,actual_td,chd=struct.unpack("<IIIII",pe.read(col,20))
        if (sig,actual_offset,cd)!=(0,delta,0) or actual_td!=td:
            raise AuditError(f"{name}: invalid original RTTI secondary-base offset")
        if not chd or get(chd)!=0:
            raise AuditError(f"{name}: invalid original RTTI hierarchy descriptor")
        typename=(".?AV"+name+"@@").encode("ascii")+b"\0"
        if pe.read(td+8,len(typename))!=typename:
            raise AuditError(f"{name}: original RTTI type name mismatch")
        if get(vt)!=init:raise AuditError(f"{name}: Init slot0 address mismatch")
        if hashlib.sha256(pe.read(init,16)).hexdigest()!=sha:
            raise AuditError(f"{name}: Init entry SHA256 mismatch")
        # Three original methods are exactly mov al,1 / ret;
        # all other entries are merely 16-byte prefixes.
        is_trivial=index in (1,3,6)
        if is_trivial and pe.read(init,3)!=bytes((0xb0,1,0xc3)):
            raise AuditError(f"{name}: trivial true-return method mismatch")
        out.append({"index":index,"class":name,"object_va":hex(obj),
            "registered_subobject_va":hex(obj+delta),
            "rtti_subobject_offset":delta,
            "vtable_va":hex(vt),"init_slot":0,"init_va":hex(init),
            "init_prefix_sha256":sha,
            "init_complete_three_byte_true":is_trivial})
    return {"owner_original_client_sha256":CLIENT_SHA256,
        "constructor_rtti_owners":out,
        "constructor_to_vtable_links_authenticated":len(out),
        "limitations":["original BSS contents absent from raw PE; vptrs established by constructors",
                       "nontrivial method bodies have only a 16-byte entry prefix audited",
                       "no VClient011::Init normal return or gameplay proven"]}
def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA256 mismatch")
    pe=OriginalPE(blob)
    result=verify_constructed(pe)
    result["complete_detail_object_init"]=verify_detail_full(pe)
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client")
    args=parser.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
