#!/usr/bin/env python3
"""Phase240: authenticate original 2006 SiN client PVS red/black balancing.

Both complete owner-original PE32 DLL SHA256s are required before awarding
whole-native-method credit. No commercial original binaries, whole-method
bytes or complete disassemblies are stored in this file or the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Native method, original entry, entire x86 body size, complete original
# SHA256, trailing INT3 count, RET4 offsets, independent sparse witnesses.
METHODS = (
    ("tree_rotate_left", 0x241067C0, 231,
     "a32bf577a10663ba6734c3a35a5324e80b3b66141b75f5e8d1a9fefd9a0789db",
     9, (0xE4,), {
         0x000: "53555657",
         0x004: "8b7c2414",
         0x00B: "e870f7ffff",
         0x017: "e864f7ffff",
         0x0C0: "66895e10",
         0x0E4: "c20400",
     }),
    ("tree_rotate_right", 0x24106D40, 234,
     "9aee654ed5490c3e25fea44e6961666620d7e28493f8cbff80de9dff293981b8",
     6, (0xE7,), {
         0x000: "538b5c2408",
         0x00B: "e8f0f1ffff",
         0x016: "e8e5f1ffff",
         0x0C2: "66897e10",
         0x0E7: "c20400",
     }),
    ("tree_insert_red_black_fixup", 0x24106E30, 1482,
     "f778c9c8b49e5a2f40cb79694d3c0a7191da19137edea216d456c31705775e81",
     6, (0x5C7,), {
         0x000: "56578b7c240c8bf1",
         0x008: "663b7e10",
         0x011: "0f849f050000",
         0x307: "e884f6ffff",
         0x3BF: "e84cfbffff",
         0x4E3: "e828faffff",
         0x59B: "e8f0f3ffff",
         0x5C7: "c20400",
     }),
)

# All 4 original rebalancer->rotation E8 calls, all 3 Phase239
# attach->balancer E8 calls, and 4 rotation->original node resolver E8s.
CALLS = (
    (0x24107137, 0x241067C0),
    (0x241071EF, 0x24106D40),
    (0x24107313, 0x24106D40),
    (0x241073CB, 0x241067C0),
    (0x24107724, 0x24106E30),
    (0x24107733, 0x24106E30),
    (0x24107741, 0x24106E30),
    (0x241067CB, 0x24105F40),
    (0x241067D7, 0x24105F40),
    (0x24106D4B, 0x24105F40),
    (0x24106D56, 0x24105F40),
)

def rel32_call(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase240 original client PE base drift")
    rows = []
    witnesses = 0
    for name, va, length, digest, pad, ret_offsets, anchors in METHODS:
        if (client.section_of(va) != ".text" or
                client.section_of(va+length-1) != ".text"):
            raise VerificationError("Phase240 original native code outside .text: " + name)
        code = client.at(va, length)
        if check_hash and hashlib.sha256(code).hexdigest() != digest:
            raise VerificationError("Phase240 complete native function SHA256 drift: " + name)
        if client.at(va+length,pad) != b"\xCC"*pad:
            raise VerificationError("Phase240 original method padding drift: " + name)
        for off in ret_offsets:
            if code[off:off+3] != b"\xC2\x04\x00":
                raise VerificationError(f"Phase240 native RET4 drift: {name}+{off:#x}")
        for off,encoded in anchors.items():
            expected=bytes.fromhex(encoded)
            if code[off:off+len(expected)] != expected:
                raise VerificationError(
                    f"Phase240 native opcode witness drift: {name}+{off:#x}"
                )
            witnesses += 1
        rows.append({
            "name":name, "native_entry":f"0x{va:08X}",
            "original_entire_native_bytes":length,
            "whole_original_SHA256":digest if check_hash else None,
            "original_RET4_paths":len(ret_offsets),
            "original_trailing_INT3":pad,
        })
    for site, target in CALLS:
        if (client.section_of(site) != ".text" or
                client.at(site,5) != rel32_call(site,target)):
            raise VerificationError(f"Phase240 native CALL edge drift: {site:#x}")
    return {
        "phase":240,
        "new_original_complete_SHA_methods": (
            len(rows) if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes": (
            sum(m[2] for m in METHODS) if check_hash and authentic_provenance else 0
        ),
        "reviewed_original_methods":rows,
        "original_sparse_opcode_witnesses":witnesses,
        "original_E8_rel32_CALL_edges":len(CALLS),
        "original_native_record_stride":16,
        "node_left_WORD_offset":0,
        "node_right_WORD_offset":2,
        "node_parent_WORD_offset":4,
        "node_color_WORD_offset":6,
        "tree_root_WORD_offset":0x10,
        "nil_index":0xFFFF,
        "native_red_state_WORD":0,
        "native_black_state_WORD":1,
        "original_tree_rotation_targets": ["0x241067C0","0x24106D40"],
        "scope_limits":[
            "Full original code SHA review is not line-by-line source equivalence.",
            "Host-safe red/black insertion fixup is tested as an independent C++ model.",
            "Original map deletion/rebalance method 0x241083C0 is not recovered.",
            "Original global nil-object CRT lifecycle is not rebuilt.",
            "Original game still does not boot reconstructed client/server gameplay DLLs.",
        ],
    }

def audit(server:bytes, client:bytes) -> dict:
    for dll, raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest() != ORIGINALS[dll]["sha256"]:
            raise VerificationError(
                "Phase240 requires BOTH complete original retail DLL SHA256s: "+dll
            )
    result=verify_native(PEImage(client),authentic_provenance=True)
    if (result["new_original_complete_SHA_methods"] != 3 or
            result["new_original_native_code_bytes"] != 1947):
        raise VerificationError("Phase240 original-method inventory drift")
    return result

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    sources=parser.add_mutually_exclusive_group(required=True)
    sources.add_argument("--archive",type=Path)
    sources.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    if args.archive:
        if args.client:
            parser.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as archive:
            if (archive.namelist().count("server.dll") != 1 or
                    archive.namelist().count("client.dll") != 1):
                raise VerificationError("Phase240 requires one original root DLL pair")
            report=audit(archive.read("server.dll"),archive.read("client.dll"))
    else:
        if not args.client:
            parser.error("--server also requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
