#!/usr/bin/env python3
"""Phase221 original 2006 client.dll type-3/type-4 fixed-stride copy audit.

Requires both owner-local complete retail DLL SHA256s, then independently
checks two complete original native function SHA256s, sparse original
instruction witnesses, caller instructions and original code boundaries.
Does not publish original binary bodies or game assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA, BASE
from audit_sin_original_client_snapshot_record_router import CALL_EDGES, TYPE_CASES

# entry, native length, complete original SHA256, following INT3 bytes,
# original type-specific Phase212 router CALL site, RET cleanup,
# sparse exact original x86 complete-instruction witnesses.
METHODS = {
 "type3_stride12_forward_copy":(
  0x24198DA0,53,
  "4a10b9e0f382e4732ecd64991a5a6ea8fcd67caf8a8ac8521c9281f7dd36eac0",
  11,0x241998C3,16,{
    0x00:"80792c00",0x04:"742c",0x06:"837c240401",
    0x0B:"7425",0x0D:"8b442410",0x11:"8d0c40",
    0x14:"03c9",0x16:"03c9",0x19:"8b742410",
    0x1D:"8bc1",0x20:"8b7c2410",0x24:"c1e902",
    0x27:"f3a5",0x29:"8bc8",0x2B:"83e103",
    0x2E:"f3a4",0x30:"5f",0x32:"c21000",
  }),
 "type4_stride16_forward_copy":(
  0x24198DE0,49,
  "4fe077835ba1cc25f17844afb49b481eabcf130515231479bc3fe22e45ccc33b",
  15,0x241998FD,16,{
    0x00:"80792c00",0x04:"7428",0x06:"837c240401",
    0x0B:"7421",0x0D:"8b4c2410",0x11:"c1e104",
    0x15:"8b742410",0x19:"8bc1",0x1C:"8b7c2410",
    0x20:"c1e902",0x23:"f3a5",0x25:"8bc8",
    0x27:"83e103",0x2A:"f3a4",0x2C:"5f",
    0x2E:"c21000",
  }),
}
NEIGHBOR_ENTRIES={
 0x24198DE0:bytes.fromhex("80792c00"), # type4 copy
 0x24198E20:bytes.fromhex("83ec1c"), # original type3 float comparator
}


def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)


def verify_native(client:PEImage,*,check_hash:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase221 original client PE32 base drift")
    if TYPE_CASES[3]!=3 or TYPE_CASES[4]!=4:
        raise VerificationError("Phase221 original type-to-case mapping drift")
    result={}
    anchors=0
    for name,(va,size,digest,pad,caller,stack,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase221 original copy not entirely .text: "+name)
        code=client.at(va,size)
        if check_hash and hashlib.sha256(code).hexdigest()!=digest:
            raise VerificationError("Phase221 complete original native function SHA drift: "+name)
        if code[-3:]!=b"\xc2"+struct.pack("<H",stack):
            raise VerificationError("Phase221 original RET stack cleanup drift: "+name)
        if client.at(va+size,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase221 original INT3 code boundary drift: "+name)
        for off,hx in witnesses.items():
            expected=bytes.fromhex(hx)
            if code[off:off+len(expected)]!=expected:
                raise VerificationError("Phase221 sparse original opcode mismatch: "+name+" "+hex(off))
            anchors+=1
        if (caller,va) not in CALL_EDGES or (
                client.section_of(caller)!=".text" or
                client.at(caller,5)!=rel_call(caller,va)):
            raise VerificationError("Phase221 original router CALL drift: "+name)
        result[name]={
            "entry_va":f"0x{va:08X}","whole_native_bytes":size,
            "original_native_sha256":digest if check_hash else None,
            "router_call":f"0x{caller:08X}","ret_stack_bytes":stack,
            "int3_padding":pad,
        }
    for va,head in NEIGHBOR_ENTRIES.items():
        if client.section_of(va)!=".text" or client.at(va,len(head))!=head:
            raise VerificationError("Phase221 original following method entry drift")
    if METHODS["type3_stride12_forward_copy"][0]+53+11!=0x24198DE0 or (
            METHODS["type4_stride16_forward_copy"][0]+49+15!=0x24198E20):
        raise VerificationError("Phase221 exact original native method adjacency drift")
    return {
        "new_sha_verified_original_client_functions":len(METHODS) if check_hash else 0,
        "new_whole_original_native_bytes":102 if check_hash else 0,
        "method_evidence":result,
        "sparse_native_opcode_witnesses":anchors,
        "original_exact_router_CALLs":2,
        "original_stride_bytes":{"type3":12,"type4":16},
        "limits":[
            "The two native copy helpers are complete; type3/type4 comparisons and log/preview remain opaque.",
            "Original multiplier is 32-bit wrapping i386 arithmetic; host refuses unsafe overflow.",
            "Original REP MOVSD then REP MOVSB advances forwards and is not memmove.",
            "No retail Windows engine has loaded replacement gameplay DLLs.",
        ],
    }


def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or (
            hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase221 requires both complete owner-original DLL SHA256s")
    return verify_native(PEImage(client))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive must not be combined with --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("original owner zip needs one root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        report=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(report,indent=2))


if __name__=="__main__":
    main()
