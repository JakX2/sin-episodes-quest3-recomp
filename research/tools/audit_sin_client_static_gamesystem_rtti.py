#!/usr/bin/env python3
"""Owner-only SiN 2006 client static AutoGameSystem RTTI and Init audit.

Requires full original client.dll SHA256. Reports names/addresses/digests,
never emits the original code or DLL bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE, CLIENT_SHA256

BASE=0x24000000
# The four registered static objects with initialized raw PE data.
# A few other original global objects are initialized dynamically in
# .bss, and must not be assigned imagined retail vtables.
OWNERS=(
    ("CSoundEmitterSystem",0x242FFC1C,0x242A6BE8,0x242E9B1C,0x242FFBF8,
     0x241BC950,11,"ab2441abbd6dcfa2e834a36621734e3c5b7462edeb623dc55267262be3f5b8fb",True),
    ("CViewportClientSystem",0x2431DD84,0x242AC284,0x242EACF0,0x2431DD60,
     0x241ED310,14,"f144532c23e6873d2d44c134712865ac9b8b6da0b7873a0af8b03f415e0ec473",True),
    ("CPerfVisualBenchmark",0x242FC9DC,0x242A209C,0x242E8634,0x242FC9B8,
     0x24182800,32,"c848bb21b84b40fea3b471f7216dc984a191d5283f951b58af0daccb989980e3",False),
    ("CClientThinkList",0x242FA6B8,0x24297300,0x242E5934,0x242FA694,
     0x24103DF0,11,"97fa672efbb4261fefdcb669fa384ccf07fc158688cc8c81aff670583dddc71e",True)
)
class AuditError(ValueError):pass

def validate_static_owners(pe,owners=OWNERS):
    if pe.base!=BASE:raise AuditError("original image base mismatch")
    out=[]
    for (name,obj,vt,col,td,slot0,body_len,sha,complete) in owners:
        def dword(addr):
            return struct.unpack("<I",pe.read(addr,4))[0]
        if dword(obj)!=vt:raise AuditError(f"{name}: object vtable mismatch")
        if dword(vt-4)!=col:raise AuditError(f"{name}: RTTI COL reference mismatch")
        sig,off,ctor,actual_td,chd=struct.unpack("<IIIII",pe.read(col,20))
        if (sig,off,ctor)!=(0,0,0) or actual_td!=td:
            raise AuditError(f"{name}: malformed x86 MSVC RTTI locator")
        if not chd or dword(chd)!=0:raise AuditError(f"{name}: invalid RTTI hierarchy")
        typename=(".?AV"+name+"@@").encode("ascii")+b"\0"
        if pe.read(td+8,len(typename))!=typename:
            raise AuditError(f"{name}: original RTTI class name mismatch")
        if dword(vt)!=slot0:raise AuditError(f"{name}: virtual Init slot0 mismatch")
        if hashlib.sha256(pe.read(slot0,body_len)).hexdigest()!=sha:
            raise AuditError(f"{name}: virtual Init body/prefix hash mismatch")
        out.append({"name":name,"object_va":hex(obj),"vtable_va":hex(vt),
                    "rtti_type_descriptor_va":hex(td),
                    "init_slot":0,"init_va":hex(slot0),
                    "verified_sha256":sha,
                    "verified_bytes":body_len,
                    "complete_method":complete})
    return {"original_client_sha256":CLIENT_SHA256,"verified_static_systems":out,
            "original_virtual_methods_executed":0,
            "limitations":["other registered BSS objects cannot be assigned raw-file vtables",
                           "benchmark Init entire body not yet reconstructed",
                           "method pointers not invoked in original game process",
                           "normal VClient011::Init not established"]}

def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA256 mismatch")
    return validate_static_owners(OriginalPE(blob))
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("client")
    args=p.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
