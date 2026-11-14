#!/usr/bin/env python3
"""Phase218: owner-original 2006 SiN client snapshot type-7 short-field helpers.

Read-only, dual-entire-DLL SHA256-gated binary verification. Never publishes
original DLLs, complete native function bodies, or proprietary assets.
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
from audit_sin_original_client_snapshot_record_router import (
    CALL_EDGES, CASE_DESTINATIONS, TYPE_CASES,
)

# Entire original function VA, byte length, entire native SHA256,
# exact following INT3 count (including zero), router CALL VA,
# native RET stack bytes, independent sparse original x86 opcode witnesses.
METHODS = {
    "type7_short_word_comparison": (
        0x24197D50, 84,
        "451e236f93e76e4bbb98c8403fbfe966f5902fb8d7de339ba19999010d96ca59",
        12, 0x241999F4, 12, {
            0x00:"80791400",0x04:"7505",0x06:"33c0",
            0x08:"c20c00",0x0B:"8b4118",0x0E:"f6401304",
            0x16:"8b542414",0x20:"8b442410",0x24:"8b74240c",
            0x30:"668b3c06",0x34:"663b38",0x37:"7514",
            0x44:"b801000000",0x4A:"c20c00",0x51:"c20c00",
        }
    ),
    "type7_short_word_copy": (
        0x241989E0, 48,
        "e2ddec19682c14da96bb43b3ad18312d1b6789e38f25dd3c629f8d412f209bd6",
        0, 0x24199A05, 16, {
            0x00:"80792c00",0x04:"7427",0x06:"837c240401",
            0x0D:"8b4c2410",0x11:"03c9",0x14:"8b742410",
            0x1B:"8b7c2410",0x1F:"c1e902",0x22:"f3a5",
            0x26:"83e103",0x29:"f3a4",0x2D:"c21000",
        }
    ),
    "type7_short_signed_diagnostics": (
        0x24197B10, 79,
        "a82821988ab380a78ec609508f6f43502daa19b9ff5d63ad211de7f9c0362fbf",
        1, 0x24199A10, 16, {
            0x00:"56",0x03:"807e1400",0x0A:"8b5c240c",
            0x11:"8b7c2414",0x17:"0fbf07",0x1A:"8b4c2418",
            0x1E:"0fbf09",0x23:"2bd1",0x28:"68803e2a24",
            0x2E:"e8cdfdffff",0x36:"0fbf07",0x3A:"68743e2a24",
            0x41:"e86afeffff",0x4C:"c21000",
        }
    ),
    "type7_short_signed_preview": (
        0x24198380, 33,
        "80ed762c28807f23273f2449855fd6063b3b3d37ee4068a007b77e3b2783cb2f",
        15, 0x24199A1F, 16, {
            0x00:"8b4134",0x03:"3b4118",0x06:"7516",
            0x08:"8b542408",0x0C:"0fbf02",
            0x10:"6804402a24",0x16:"e845fcffff",
            0x1B:"83c40c",0x1E:"c21000",
        }
    ),
}
FORMAT_STRINGS = {
    0x242A3E80: "short differs (net %i pred %i) diff(%i)\n",
    0x242A3E74: "short (%i)\n",
    0x242A4004: "short (%i)",
}
NESTED_CALLS = (
    (0x24197B3E, 0x24197910),
    (0x24197B51, 0x241979C0),
    (0x24198396, 0x24197FE0),
)
# No INT3 between original type7 word-copy function and Phase213 type5
# DWORD-copy function. Do not mistake their adjacent bodies as one method.
FOLLOWING_FUNCTION_VA = 0x24198A10
FOLLOWING_ENTRY_WITNESS = bytes.fromhex("80792c00")


def rel_call(site: int, destination: int) -> bytes:
    return b"\xe8" + struct.pack("<i", destination - site - 5)


def verify_native(client: PEImage, *, check_hash: bool = True) -> dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase218 wrong original client PE32 image base")
    if TYPE_CASES[7] != 7 or CASE_DESTINATIONS[7] != 0x241999EF:
        raise VerificationError("Phase218 original type7 case metadata drift")
    result = {}
    witnessed = 0
    for name,(va,n,digest,padding,router_site,stack_pop,anchors) in METHODS.items():
        if client.section_of(va) != ".text" or client.section_of(va+n-1) != ".text":
            raise VerificationError("Phase218 method outside original .text: "+name)
        body = client.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest() != digest:
            raise VerificationError("Phase218 original whole-function SHA drift: "+name)
        if body[-3:] != b"\xc2" + struct.pack("<H",stack_pop):
            raise VerificationError("Phase218 original RET immediate drift: "+name)
        if client.at(va+n,padding) != b"\xcc"*padding:
            raise VerificationError("Phase218 original RET/INT3 boundary drift: "+name)
        for off,hx in anchors.items():
            needle=bytes.fromhex(hx)
            if body[off:off+len(needle)] != needle:
                raise VerificationError("Phase218 native x86 anchor drift: "+name+" "+hex(off))
            witnessed += 1
        if (router_site,va) not in CALL_EDGES or (
                client.section_of(router_site) != ".text" or
                client.at(router_site,5) != rel_call(router_site,va)):
            raise VerificationError("Phase218 original type7 CALL rel32 drift: "+name)
        result[name] = {
            "original_va":f"0x{va:08X}",
            "original_native_bytes":n,
            "whole_original_native_sha256":digest if check_hash else None,
            "original_router_callsite":f"0x{router_site:08X}",
            "original_ret_stack_bytes":stack_pop,
            "native_padding_int3_bytes":padding,
        }
    copy_va,copy_len,*_ = METHODS["type7_short_word_copy"]
    if copy_va+copy_len != FOLLOWING_FUNCTION_VA or (
            client.section_of(FOLLOWING_FUNCTION_VA) != ".text" or
            client.at(FOLLOWING_FUNCTION_VA,len(FOLLOWING_ENTRY_WITNESS)) !=
            FOLLOWING_ENTRY_WITNESS):
        raise VerificationError("Phase218 adjacent original method boundary drift")
    for site,target in NESTED_CALLS:
        if (client.section_of(site) != ".text" or
                client.section_of(target) != ".text" or
                client.at(site,5) != rel_call(site,target)):
            raise VerificationError("Phase218 original nested formatter CALL drift")
    for va,value in FORMAT_STRINGS.items():
        if client.section_of(va) != ".rdata" or (
                client.at(va,len(value)+1) != value.encode("ascii")+b"\0"):
            raise VerificationError("Phase218 original signed-short format literal drift")
    return {
        "new_sha_verified_original_client_functions":len(METHODS) if check_hash else 0,
        "new_complete_original_x86_bytes":sum(row[1] for row in METHODS.values()) if check_hash else 0,
        "method_evidence":result,
        "sparse_native_instruction_witnesses":witnessed,
        "exact_type7_router_call_edges":len(METHODS),
        "exact_native_nested_formatter_call_edges":len(NESTED_CALLS),
        "original_literal_count":len(FORMAT_STRINGS),
        "verified_adjacent_no_padding_boundary": True,
        "limits":[
            "Complete type7 native callees reviewed, but not original Source engine object ownership.",
            "Original short comparator uses signed word count and bitwise exact 16-bit equality.",
            "Original word-copy uses unsigned DWORD count times two with 32-bit x86 wrap; portable host rejects overflow.",
            "Native invalid-pointer faults, DF/CPU state and original formatter implementation not emulated.",
            "No playable Windows replacement DLL, Android game or Quest 3 game demonstrated.",
        ],
    }


def audit(server_data: bytes, client_data: bytes) -> dict:
    if (hashlib.sha256(server_data).hexdigest() != FULL_SHA["server"] or
            hashlib.sha256(client_data).hexdigest() != FULL_SHA["client"]):
        raise VerificationError("Phase218 requires both complete owner-original DLL SHA256s")
    return verify_native(PEImage(client_data))


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    files=p.add_mutually_exclusive_group(required=True)
    files.add_argument("--archive",type=Path,help="owner-private bin.zip")
    files.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:
            p.error("--client cannot accompany --archive")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner archive must contain one root DLL pair")
            server,client=z.read("server.dll"),z.read("client.dll")
    else:
        if not args.client:
            p.error("--server requires --client")
        server,client=args.server.read_bytes(),args.client.read_bytes()
    report=json.dumps(audit(server,client),indent=2)+"\n"
    if args.output:
        args.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")


if __name__ == "__main__":
    main()
