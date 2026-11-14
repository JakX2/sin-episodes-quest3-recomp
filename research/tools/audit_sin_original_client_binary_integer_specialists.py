#!/usr/bin/env python3
"""Phase217: SHA-gated 2006 SiN client binary/integer serializer specialists.

Read-only audit of owner-local original complete server.dll and client.dll.
Only sparse short instruction witnesses and complete function SHA256 digests are
published. Neither retail DLLs nor complete proprietary function bodies ship.
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
from audit_sin_original_client_snapshot_record_router import CALL_EDGES
from audit_sin_original_allocator_game_callers import named_iat

# (VA, full function size, authentic whole-native SHA256, INT3 pad length,
#  original Phase212 router callsite(s), RET immediate stack pop, sparse opcodes)
METHODS = {
    "shared_binary_byte_compare": (
        0x24197A90,56,
        "0fb74121fd315127c05255395992aae9e6737a74cee4133f2d1c3685947f0276",
        8,(0x2419992E,0x24199A2E),12,{
            0x00:"80791400",0x04:"7505",0x0B:"8b4118",
            0x0E:"f6401304",0x14:"8b4c2404",
            0x19:"8b74240c",0x1E:"8b7c2414",
            0x22:"33d2",0x24:"f3a6",0x28:"5f",
            0x29:"0f94c0",0x2C:"5e",0x30:"b801000000",0x35:"c20c00",
        }),
    "binary_byte_count_diagnostics": (
        0x24197AD0,60,
        "fc0c439370b8fca8033459b1433ead82be379a2c135b394fb8e59b8427281441",
        4,(0x24199964,),16,{
            0x00:"56",0x03:"807e1400",0x0A:"8b5c2410",
            0x0F:"8b7c2410",0x13:"85ff",
            0x18:"68543e2a24",0x1E:"e81dfeffff",
            0x27:"68403e2a24",0x2E:"e8bdfeffff",
            0x39:"c21000",
        }),
    "integer_symbol_aware_diagnostics": (
        0x24197B60,197,
        "37d16baa4f88742042943342958e62964451d66c7eab39b12c539bffa96d49f2",
        11,(0x24199A78,),16,{
            0x00:"81ec00020000",0x09:"807f1400",
            0x14:"8b9c240c020000",0x1E:"8bb42414020000",
            0x39:"68d03e2a24",0x3F:"e86cfdffff",
            0x4A:"f6401308",0x52:"8b0dacf63324",
            0x5B:"ff5204",0x62:"6800020000",
            0x6C:"8b0dacf63324",0x7A:"ff520c",
            0x7E:"ff1554622724",0x91:"53",
            0x93:"e8c8fdffff",0xA4:"c21000",
            0xAA:"68ac3e2a24",0xB1:"e8aafdffff",0xC2:"c21000",
        }),
    "shared_binary_byte_count_preview": (
        0x24198360,30,
        "c654d2ca175b5fad2fd98a18efcbc4709f8feb15190c9ee733e578d63cf6279d",
        2,(0x24199973,0x24199A87),16,{
            0x00:"8b4134",0x03:"3b4118",0x06:"7513",
            0x08:"8b542408",0x0D:"68f03f2a24",
            0x13:"e868fcffff",0x18:"83c40c",0x1B:"c21000",
        }),
}
NESTED_DIRECT_CALLS=(
    (0x24197AEE,0x24197910),
    (0x24197AFE,0x241979C0),
    (0x24197B9F,0x24197910),
    (0x24197BF3,0x241979C0),
    (0x24197C11,0x241979C0),
    (0x24198373,0x24197FE0),
)
FORMATS={
    0x242A3E54:"binary data differs (%i bytes)\n",
    0x242A3E40:"binary (%i bytes)\n",
    0x242A3ED0:"int differs (net %i pred %i) diff(%i)\n",
    0x242A3EBC:"integer (%i->%s)\n",
    0x242A3EAC:"integer (%i)\n",
    0x242A3FF0:"binary (%i bytes)",
}
Q_FILEBASE_IAT=0x24276254
SYMBOL_RESOLVER_GLOBAL=0x2433F6AC


def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)


def verify_native(client:PEImage,*,check_hash:bool=True,
                  check_imports:bool=True)->dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase217 original client 2006 PE32 base mismatch")
    evidence={}
    witness_count=0
    caller_count=0
    for name,(va,n,digest,pad,callers,stack_pop,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+n-1)!=".text":
            raise VerificationError("Phase217 method outside native client .text "+name)
        body=client.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase217 complete native SHA256 mismatch "+name)
        if body[-3:]!=b"\xc2"+struct.pack("<H",stack_pop):
            raise VerificationError("Phase217 original native RET cleanup drift "+name)
        if client.at(va+n,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase217 original INT3 method boundary drift "+name)
        for off,hx in witnesses.items():
            raw=bytes.fromhex(hx)
            if body[off:off+len(raw)]!=raw:
                raise VerificationError("Phase217 original sparse opcode drift "+
                                        name+" "+hex(off))
            witness_count+=1
        for caller in callers:
            if (caller,va) not in CALL_EDGES or client.section_of(caller)!=".text" or (
                    client.at(caller,5)!=rel_call(caller,va)):
                raise VerificationError("Phase217 original router CALL drift "+name)
            caller_count+=1
        evidence[name]={
            "native_va":f"0x{va:08X}","whole_native_bytes":n,
            "native_sha256":digest if check_hash else None,
            "native_ret_stack_bytes":stack_pop,
            "router_call_sites":[f"0x{site:08X}" for site in callers],
        }
    for site,dest in NESTED_DIRECT_CALLS:
        if (client.section_of(site)!=".text"
                or client.section_of(dest)!=".text"
                or client.at(site,5)!=rel_call(site,dest)):
            raise VerificationError("Phase217 nested native format CALL drift "+
                                    hex(site))
    for va,literal in FORMATS.items():
        if client.section_of(va)!=".rdata" or (
                client.at(va,len(literal)+1)!=literal.encode("ascii")+b"\0"):
            raise VerificationError("Phase217 original format literal drift "+hex(va))
    if client.section_of(SYMBOL_RESOLVER_GLOBAL)!=".data":
        raise VerificationError("Phase217 original symbol resolver global outside .data")
    if check_imports and named_iat(client,"vstdlib.dll","Q_FileBase")!=[Q_FILEBASE_IAT]:
        raise VerificationError("Phase217 original Q_FileBase named import/IAT drift")
    return {
        "new_sha_verified_native_client_functions":len(METHODS) if check_hash else 0,
        "complete_original_x86_bytes":sum(row[1] for row in METHODS.values()) if check_hash else 0,
        "methods":evidence,
        "sparse_instruction_witnesses":witness_count,
        "verified_original_router_call_sites":caller_count,
        "verified_nested_original_direct_calls":len(NESTED_DIRECT_CALLS),
        "original_string_literals":len(FORMATS),
        "original_symbol_resolver_global":f"0x{SYMBOL_RESOLVER_GLOBAL:08X}",
        "original_Q_FileBase_IAT":f"0x{Q_FILEBASE_IAT:08X}" if check_imports else None,
        "limits":[
            "Native comparison/format branches are recovered, not the complete record serializer.",
            "Original dynamic symbol registry and formatter method implementations remain opaque.",
            "Synthetic portable host callbacks and bounds checks are not original x86 exception behavior.",
            "No native Windows retail game-engine load of rebuilt client/server DLLs is proven.",
        ],
    }


def audit(server:bytes,client:bytes)->dict:
    if (hashlib.sha256(server).hexdigest()!=FULL_SHA["server"]
            or hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase217 requires both SHA256-matched original 2006 game DLLs")
    return verify_native(PEImage(client))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    sources=p.add_mutually_exclusive_group(required=True)
    sources.add_argument("--archive",type=Path,help="owner-only original bin.zip")
    sources.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:
            p.error("--client must not accompany --archive")
        with zipfile.ZipFile(args.archive) as archive:
            if archive.namelist().count("server.dll")!=1 or (
                    archive.namelist().count("client.dll")!=1):
                raise VerificationError("owner archive requires exactly one root game DLL pair")
            server,client=archive.read("server.dll"),archive.read("client.dll")
    else:
        if not args.client:
            p.error("--client required with --server")
        server,client=args.server.read_bytes(),args.client.read_bytes()
    document=json.dumps(audit(server,client),indent=2)+"\n"
    if args.output:
        args.output.write_text(document,encoding="utf-8")
    else:
        print(document,end="")


if __name__=="__main__":
    main()
