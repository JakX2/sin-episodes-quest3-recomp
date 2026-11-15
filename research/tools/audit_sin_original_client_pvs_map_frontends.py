#!/usr/bin/env python3
"""Phase236: SHA-authenticate original SiN 2006 client PVS map frontends.

Original retail PE32 DLL bytes and complete function disassemblies remain
owner-local. Only whole-DLL provenance and whole-code SHA fingerprints can
award reviewed-method credit. Synthetic fixtures never count as originals.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Name, original entry VA, entire original code length, whole-code SHA256,
# trailing INT3 count, native RET 8 offsets, native RET 4 offsets,
# small independent sparse opcode witnesses (not full original code bodies).
METHODS = (
    ("map_insert_frontend", 0x24108750, 100,
     "3900e2d517db2f4b51ec4166f966baadda6bf68135f01e797df1eb712cf09342",
     12, (0x61,), (), {
         0x00: "8b442408",
         0x04: "538b5c24",
         0x0A: "578bf150",
         0x14: "54241852",
         0x2B: "8bf88b44",
         0x3B: "8b460466",
         0x40: "4612010f",
         0x45: "d7c1e204",
         0x4C: "0885c074",
         0x53: "89088b53",
         0x59: "5004668b",
     }),
    ("map_comparator_tree_lookup", 0x24106B20, 251,
     "ad3064ab01086b83eaff75118e1769da0beeabe3439df7f677d7611eb26e59c4",
     5, (0xEF, 0xF8), (), {
         0x00: "5355578b",
         0x03: "8bf9668b",
         0x05: "668b6f10",
         0x0E: "663beb0f",
         0x20: "8b47048b",
         0x29: "f5c1e604",
         0x38: "84c07449",
         0x3C: "f605c4fd",
         0x6B: "663beb74",
         0x85: "8b57048b",
         0x92: "ff1783c4",
         0xCB: "3beb7407",
         0xE1: "eb0f8538",
         0xE8: "5e5f668b",
     }),
    ("map_index_release_variant_a", 0x241087C0, 61,
     "6de2e1aacee40a88f1d14f69387c17d79b44493ee3c0e8ea2dc67c7c1b28ec77",
     3, (), (0x3A,), {
         0x00: "56578b7c",
         0x02: "8b7c240c",
         0x06: "6681ffff",
         0x15: "8b4e040f",
         0x1B: "c1e00466",
         0x25: "148b5604",
         0x2E: "66814612",
         0x38: "5f5ec204",
     }),
    ("map_index_release_variant_b", 0x24108840, 61,
     "46609fee9369fed12e4ee0765eacd3b31a98855929d5a94dc7b03bb1bd9d4a01",
     3, (), (0x3A,), {
         0x00: "56578b7c",
         0x02: "8b7c240c",
         0x06: "6681ffff",
         0x15: "8b4e040f",
         0x1B: "c1e00466",
         0x25: "148b5604",
         0x2E: "66814612",
         0x38: "5f5ec204",
     }),
)

# Direct E8 rel32 CALL sites, including real callers outside the new bodies.
CALLS = (
    (0x2410876B, 0x24106440), # map find insertion location, body opaque
    (0x24108772, 0x24106700), # map index allocator, body opaque
    (0x24108786, 0x241076E0), # tree attach/balance, body opaque
    (0x241087D0, 0x241083C0), # variant A tree removal, body opaque
    (0x24108850, 0x241083C0), # variant B tree removal, body opaque
    (0x2410882B, 0x24108750), # PVS map insertion wrapper
    (0x241088F6, 0x24108750), # PVS notifier registration
    (0x2410895C, 0x24106B20), # PVS notifier unregistration search
    (0x2410756F, 0x24106B20), # other map lookup wrapper
    (0x241089BF, 0x24108840), # PVS notifier unregistration release
)
# Original x86 CALL DWORD PTR [EDI] comparator, invoked in both operand orders.
INDIRECT_COMPARATOR_CALLS = (0x24106B53, 0x24106BB2)

def rel_call(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase236 original client PE base mismatch")
    rows = []
    witnesses = 0
    for name, va, length, digest, pad, ret8, ret4, anchors in METHODS:
        if client.section_of(va) != ".text" or \
                client.section_of(va + length - 1) != ".text":
            raise VerificationError("Phase236 method outside .text: " + name)
        code = client.at(va, length)
        if check_hash and hashlib.sha256(code).hexdigest() != digest:
            raise VerificationError("Phase236 whole original method SHA drift: " + name)
        if client.at(va + length, pad) != b"\xCC" * pad:
            raise VerificationError("Phase236 native INT3 function boundary drift: " + name)
        for off in ret8:
            if code[off:off + 3] != b"\xC2\x08\x00":
                raise VerificationError("Phase236 native RET8 ABI drift: " + name)
        for off in ret4:
            if code[off:off + 3] != b"\xC2\x04\x00":
                raise VerificationError("Phase236 native RET4 ABI drift: " + name)
        for off, hx in anchors.items():
            raw = bytes.fromhex(hx)
            if code[off:off + len(raw)] != raw:
                raise VerificationError(f"Phase236 sparse native witness drift: {name} +{off:#x}")
            witnesses += 1
        rows.append({
            "name": name, "native_entry": f"0x{va:08X}",
            "whole_original_native_bytes": length,
            "complete_original_code_sha256": digest if check_hash else None,
            "native_RET8_paths": len(ret8),
            "native_RET4_paths": len(ret4),
            "native_trailing_INT3": pad,
        })
    for site, target in CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site, 5) != rel_call(site, target):
            raise VerificationError(f"Phase236 original E8 rel32 drift: {site:#x}")
    for site in INDIRECT_COMPARATOR_CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site, 2) != b"\xFF\x17":
            raise VerificationError(f"Phase236 original indirect comparator call drift: {site:#x}")
    return {
        "phase": 236,
        "new_original_complete_SHA_methods": (
            len(rows) if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes": (
            sum(m[2] for m in METHODS) if check_hash and authentic_provenance else 0
        ),
        "reviewed_original_methods": rows,
        "checked_sparse_opcode_witnesses": witnesses,
        "checked_original_direct_CALL_edges": len(CALLS),
        "checked_original_indirect_comparator_CALLs": len(INDIRECT_COMPARATOR_CALLS),
        "map_record_stride": 16,
        "map_record_left_WORD_offset": 0,
        "map_record_right_WORD_offset": 2,
        "map_record_payload_offset": 8,
        "map_record_payload_bytes": 8,
        "map_root_WORD_offset": 0x10,
        "map_live_count_WORD_offset": 0x12,
        "map_free_head_WORD_offset": 0x14,
        "map_absent_index": 0xFFFF,
        "scope_limits": [
            "Map insertion-location helper 0x24106440 is not reconstructed.",
            "Map index allocator 0x24106700 is not reconstructed.",
            "Tree attach helper 0x241076E0 and tree erase helper 0x241083C0 are opaque.",
            "Comparator callback semantics are injected, not inferred for arbitrary game objects.",
            "No original engine-owned C++ map or runnable Windows game DLL is constructed.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for name, raw in (("server", server), ("client", client)):
        if hashlib.sha256(raw).hexdigest() != ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase236 requires BOTH owner-original full DLL SHA256s: " + name)
    result = verify_native(PEImage(client), authentic_provenance=True)
    if result["new_original_complete_SHA_methods"] != 4 or \
            result["new_original_native_code_bytes"] != 473:
        raise VerificationError("Phase236 original method inventory drift")
    return result

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    group = p.add_mutually_exclusive_group(required=True)
    group.add_argument("--archive", type=Path)
    group.add_argument("--server", type=Path)
    p.add_argument("--client", type=Path)
    a = p.parse_args()
    if a.archive:
        if a.client:
            p.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll") != 1 or \
                    z.namelist().count("client.dll") != 1:
                raise VerificationError("Phase236 requires root original DLL pair")
            result = audit(z.read("server.dll"), z.read("client.dll"))
    else:
        if not a.client:
            p.error("--server requires --client")
        result = audit(a.server.read_bytes(), a.client.read_bytes())
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
