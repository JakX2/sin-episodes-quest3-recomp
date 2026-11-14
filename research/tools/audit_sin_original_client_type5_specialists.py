#!/usr/bin/env python3
"""Phase220 SHA-gated original 2006 SiN client type-5 DWORD specialists.

Authenticates two original native method bodies in the owner's retail DLL
pair, exact Phase212 call sites, formatter calls, names and original PE IAT.
No commercial binaries, complete native bodies or disassembly are published.
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
from audit_sin_original_client_snapshot_field_copy import METHODS as COPY_METHODS
from audit_sin_original_client_binary_integer_specialists import (
    METHODS as INTEGER_METHODS,
    SYMBOL_RESOLVER_GLOBAL, Q_FILEBASE_IAT,
)
from audit_sin_original_client_boolean_specialists import METHODS as BOOLEAN_METHODS
from audit_sin_original_allocator_game_callers import named_iat

# VA, complete native length, complete original function SHA256, INT3 pad,
# original exact router CALL site, original RET cleanup bytes,
# sparse independent whole-instruction byte witnesses.
METHODS = {
    "type5_dword_compare_with_immediate_first_mismatch_report": (
        0x24197DB0, 125,
        "256748201d4c29fb46076705316ea2345bbeb24a885a978e21d7b6bb9824cdbf",
        3, 0x241999BC, 12, {
            0x00:"51", 0x01:"80791400", 0x05:"890c24",
            0x08:"7506", 0x0D:"c20c00", 0x10:"8b4118",
            0x13:"f6401304", 0x1D:"8b742420", 0x23:"85f6",
            0x25:"7e1f", 0x27:"8b6c2418", 0x2B:"8b7c241c",
            0x35:"8b0c13", 0x38:"3b0a", 0x3A:"7517",
            0x49:"b801000000", 0x50:"c20c00",
            0x53:"8b548500", 0x57:"8b0487", 0x5C:"2bc8",
            0x65:"68d03e2a24", 0x6B:"e8f0faffff", 0x7A:"c20c00",
        }
    ),
    "type5_integer_symbol_aware_preview": (
        0x241983B0, 144,
        "88eb84ee0d78cae0b3754d272b8b462e89e662e52f2735a8c547018b48fad874",
        0, 0x241999E5, 16, {
            0x00:"81ec00020000",0x06:"57",0x09:"8b4718",
            0x0C:"394734",0x0F:"7575",0x11:"f6401308",
            0x16:"8bb42410020000",0x1D:"7455",
            0x21:"8b0dacf63324",0x2A:"ff5204",
            0x2F:"7443",0x31:"6800020000",
            0x3B:"8b0dacf63324",0x49:"ff520c",
            0x4D:"ff1554622724",0x5B:"6820402a24",
            0x61:"e8cafbffff",0x71:"c21000",
            0x77:"6810402a24",0x7D:"e8aefbffff",0x8D:"c21000",
        }
    ),
}

NESTED_DIRECT_CALLS=(
    (0x24197E1B,0x24197910), # first mismatching DWORD (pre-copy)
    (0x24198411,0x24197FE0), # resolved integer display
    (0x2419842D,0x24197FE0), # fallback numeric integer display
)
FORMAT_STRINGS={
    0x242A3ED0:"int differs (net %i pred %i) diff(%i)\n",
    0x242A4020:"integer (%i->%s)",
    0x242A4010:"integer (%i)",
}
FOLLOWING_VA=0x24198440
FOLLOWING_ENTRY=bytes.fromhex("8b41343b4118")


def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)


def verify_native(client:PEImage,*,check_hash:bool=True,
                  check_imports:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase220 original client PE32 image base drift")
    if TYPE_CASES[5]!=5 or (
        COPY_METHODS["type5_dword_copy"][0]!=0x24198A10
    ) or INTEGER_METHODS["integer_symbol_aware_diagnostics"][0]!=0x24197B60:
        raise VerificationError("Phase220 reviewed type5 dependency metadata drift")
    if BOOLEAN_METHODS["type6_boolean_preview"][0]!=FOLLOWING_VA:
        raise VerificationError("Phase220 adjacent Phase219 entry metadata drift")

    evidence={}
    total_witnesses=0
    for name,(va,size,digest,pad,caller,ret_pop,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase220 original native method outside .text: "+name)
        body=client.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase220 whole original native method SHA drift: "+name)
        if body[-3:]!=b"\xc2"+struct.pack("<H",ret_pop):
            raise VerificationError("Phase220 original RET cleanup drift: "+name)
        if client.at(va+size,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase220 original INT3/method boundary drift: "+name)
        for offset,hx in witnesses.items():
            insn=bytes.fromhex(hx)
            if body[offset:offset+len(insn)]!=insn:
                raise VerificationError("Phase220 sparse x86 opcode drift: "+name+" "+hex(offset))
            total_witnesses+=1
        if (caller,va) not in CALL_EDGES or (
            client.section_of(caller)!=".text" or
            client.at(caller,5)!=rel_call(caller,va)
        ):
            raise VerificationError("Phase220 original type5 router CALL drift: "+name)
        evidence[name]={
            "entry_va":f"0x{va:08X}","entire_original_native_bytes":size,
            "whole_native_sha256":digest if check_hash else None,
            "router_callsite":f"0x{caller:08X}",
            "native_ret_stack_bytes":ret_pop,
            "native_int3_padding_bytes":pad,
        }

    preview,size,*_=METHODS["type5_integer_symbol_aware_preview"]
    if preview+size!=FOLLOWING_VA or (
            client.section_of(FOLLOWING_VA)!=".text" or
            client.at(FOLLOWING_VA,len(FOLLOWING_ENTRY))!=FOLLOWING_ENTRY):
        raise VerificationError("Phase220 adjacent no-INT3 preview/next method boundary drift")

    for caller,target in NESTED_DIRECT_CALLS:
        if client.section_of(caller)!=".text" or (
                client.section_of(target)!=".text" or
                client.at(caller,5)!=rel_call(caller,target)):
            raise VerificationError("Phase220 original nested formatter CALL drift")
    for va,s in FORMAT_STRINGS.items():
        if client.section_of(va)!=".rdata" or (
                client.at(va,len(s)+1)!=s.encode("ascii")+b"\0"):
            raise VerificationError("Phase220 original string literal drift "+hex(va))
    if client.section_of(SYMBOL_RESOLVER_GLOBAL)!=".data":
        raise VerificationError("Phase220 original symbol registry global section drift")
    if check_imports and named_iat(client,"vstdlib.dll","Q_FileBase")!=[Q_FILEBASE_IAT]:
        raise VerificationError("Phase220 original Q_FileBase import/IAT drift")
    # All four direct type5 native callees witnessed. Two were previously
    # reviewed; neither is counted as new.
    if ((0x241999CB,0x24198A10) not in CALL_EDGES or
        (0x241999D6,0x24197B60) not in CALL_EDGES or
        client.at(0x241999CB,5)!=rel_call(0x241999CB,0x24198A10) or
        client.at(0x241999D6,5)!=rel_call(0x241999D6,0x24197B60)):
        raise VerificationError("Phase220 earlier type5 copy/diagnostic caller drift")

    return {
        "new_SHA_verified_original_client_functions":len(METHODS) if check_hash else 0,
        "new_complete_native_x86_bytes":sum(x[1] for x in METHODS.values()) if check_hash else 0,
        "methods":evidence,
        "sparse_original_instruction_witnesses":total_witnesses,
        "original_direct_type5_router_calls":4,
        "nested_original_native_formatter_calls":len(NESTED_DIRECT_CALLS),
        "original_format_literals":len(FORMAT_STRINGS),
        "original_named_Q_FileBase_IAT":f"0x{Q_FILEBASE_IAT:08X}" if check_imports else None,
        "original_registry_global":f"0x{SYMBOL_RESOLVER_GLOBAL:08X}",
        "no_padding_between_preview_and_type6_preview":True,
        "limits":[
            "Both complete native helper bodies verified, not full Source engine record ownership.",
            "First differing DWORD reports BEFORE copy; later integer diagnostic runs AFTER copy.",
            "Original symbol registry virtual methods and formatter bodies remain opaque.",
            "Host vector bounds/count guards do not reproduce x86 invalid-pointer behavior.",
            "Native retail engine has not executed rebuilt gameplay DLLs.",
        ],
    }


def audit(server_data:bytes,client_data:bytes)->dict:
    if (hashlib.sha256(server_data).hexdigest()!=FULL_SHA["server"] or
            hashlib.sha256(client_data).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase220 requires both original full DLL SHA256 fingerprints")
    return verify_native(PEImage(client_data))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path,help="owner-local original bin.zip")
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive cannot be combined with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner original archive needs exactly one root DLL pair")
            server,client=z.read("server.dll"),z.read("client.dll")
    else:
        if not args.client:p.error("--server requires --client")
        server,client=args.server.read_bytes(),args.client.read_bytes()
    report=json.dumps(audit(server,client),indent=2)+"\n"
    if args.output:args.output.write_text(report,encoding="utf-8")
    else:print(report,end="")


if __name__=="__main__":
    main()
