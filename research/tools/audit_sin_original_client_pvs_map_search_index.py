#!/usr/bin/env python3
"""Phase237: audit original SiN 2006 client PVS map placement and index reuse.

Never counts artificial PE fixtures as recovered retail code. Both full
owner-original DLL SHA256s are required before awarding whole-method credit.
Does not export proprietary native function bodies or complete disassembly.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Name, native VA, complete original code size, whole code SHA256,
# original INT3 padding, original RET16 / RET0 positions, sparse original
# instruction witnesses. Sparse checks do NOT substitute for complete hashes.
METHODS = (
    ("map_insertion_placement_search", 0x24106440, 255,
     "23e29d786cbdb4acf862213297595c65259484779730a3c7788ea6325046006a",
     1, (0xFC,), (), {
         0x00: "8b442408",
         0x1C: "c601000f84",
         0x25: "56eb088da4",
         0x3B: "66893a8b45",
         0x47: "085051ff55",
         0x4A: "ff5500",
         0x50: "84c074508b",
         0x5B: "f605c4fd3324",
         0x6F: "66891dbcfd33",
         0x8F: "8b4504668b",
         0xB0: "24017526b801",
         0xD0: "c0fd332466",
         0xF1: "fb0f8538ffff",
         0xFC: "c21000",
     }),
    ("map_free_or_virgin_index_allocator", 0x24106700, 86,
     "15307e759a5ad044b5f781b7eb42914c95b1be056748356d53d664f68fd3344c",
     10, (), (0x37, 0x55), {
         0x00: "568bf1668b",
         0x03: "668b461466",
         0x09: "ffff5775",
         0x0E: "0fb7461639",
         0x17: "6a018d4e04",
         0x1C: "e81ffaffff",
         0x21: "668b7e16",
         0x25: "8b46048946",
         0x2B: "668bc78d",
         0x31: "5f66894e165e",
         0x37: "c3",
         0x38: "508bce8bf8",
         0x3D: "e8fef7ffff",
         0x42: "668b5002",
         0x4F: "5f66895614",
         0x55: "c3",
     }),
)

CALLS = (
    (0x2410671C, 0x24106140),  # growth manager at this+4; opaque
    (0x2410673D, 0x24105F40),  # recycled index record resolver; opaque
    (0x2410876B, 0x24106440),  # Phase236 map insertion placement
    (0x24108772, 0x24106700),  # Phase236 map insertion index allocation
)
COMPARATOR_INDIRECT_SITE = 0x2410648A
COMPARATOR_INDIRECT_OPCODE = b"\xFF\x55\x00"  # CALL DWORD PTR [EBP+0]

def rel_call(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase237 original client PE base mismatch")
    rows = []
    witnesses = 0
    for name, va, length, digest, pad, ret16, ret0, anchors in METHODS:
        if client.section_of(va) != ".text" or \
                client.section_of(va + length - 1) != ".text":
            raise VerificationError("Phase237 complete method outside .text: " + name)
        raw = client.at(va, length)
        if check_hash and hashlib.sha256(raw).hexdigest() != digest:
            raise VerificationError("Phase237 whole-original method SHA256 drift: " + name)
        if client.at(va + length, pad) != b"\xCC" * pad:
            raise VerificationError("Phase237 original INT3 padding drift: " + name)
        for off in ret16:
            if raw[off:off + 3] != b"\xC2\x10\x00":
                raise VerificationError("Phase237 original RET16 drift: " + name)
        for off in ret0:
            if raw[off:off + 1] != b"\xC3":
                raise VerificationError("Phase237 original RET0 drift: " + name)
        for off, encoded in anchors.items():
            witness = bytes.fromhex(encoded)
            if raw[off:off + len(witness)] != witness:
                raise VerificationError(
                    f"Phase237 native opcode witness drift: {name} +{off:#x}")
            witnesses += 1
        rows.append({
            "name": name,
            "native_entry": f"0x{va:08X}",
            "whole_original_native_bytes": length,
            "complete_original_body_sha256": digest if check_hash else None,
            "native_RET16_exits": len(ret16),
            "native_C3_exits": len(ret0),
            "native_trailing_INT3": pad,
        })
    for site, target in CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site, 5) != rel_call(site, target):
            raise VerificationError(f"Phase237 original direct CALL drift: {site:#x}")
    if client.section_of(COMPARATOR_INDIRECT_SITE) != ".text" or \
            client.at(COMPARATOR_INDIRECT_SITE, 3) != COMPARATOR_INDIRECT_OPCODE:
        raise VerificationError("Phase237 original callback CALL [EBP] drift")
    return {
        "phase": 237,
        "new_original_complete_SHA_methods": (
            len(rows) if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes": (
            sum(m[2] for m in METHODS) if check_hash and authentic_provenance else 0
        ),
        "reviewed_original_methods": rows,
        "original_sparse_instruction_witnesses": witnesses,
        "original_direct_E8_CALL_edges": len(CALLS),
        "original_indirect_comparator_CALLs": 1,
        "original_map_record_stride": 16,
        "original_map_root_WORD_offset": 0x10,
        "original_map_free_head_WORD_offset": 0x14,
        "original_map_next_unused_WORD_offset": 0x16,
        "original_map_cached_base_DWORD_offset": 0x18,
        "original_map_capacity_DWORD_offset": 0x08,
        "original_map_storage_DWORD_offset": 0x04,
        "original_map_parent_pointer_output_is_16_bit": True,
        "original_map_branch_output_is_byte": True,
        "opaque_native_helpers": ["0x24106140", "0x24105F40"],
        "scope_limits": [
            "Comparator source is a native manager callback, not reconstructed.",
            "Map growth and recycled node pointer resolution are opaque callbacks.",
            "Native global lazy nil-sentinel initialization is not CRT-reimplemented.",
            "Host safety guards are not claims of original x86 error handling.",
            "No original Windows engine executing reconstructed client/server gameplay modules.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for dll_name, raw in (("server", server), ("client", client)):
        if hashlib.sha256(raw).hexdigest() != ORIGINALS[dll_name]["sha256"]:
            raise VerificationError(
                "Phase237 requires BOTH whole owner-original DLL SHA256s: " + dll_name)
    report = verify_native(PEImage(client), authentic_provenance=True)
    if report["new_original_complete_SHA_methods"] != 2 or \
            report["new_original_native_code_bytes"] != 341:
        raise VerificationError("Phase237 original method-credit drift")
    return report

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive", type=Path)
    source.add_argument("--server", type=Path)
    parser.add_argument("--client", type=Path)
    args = parser.parse_args()
    if args.archive:
        if args.client:
            parser.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll") != 1 or \
                    z.namelist().count("client.dll") != 1:
                raise VerificationError("Phase237 expects one original root DLL pair")
            result = audit(z.read("server.dll"), z.read("client.dll"))
    else:
        if not args.client:
            parser.error("--server requires --client")
        result = audit(args.server.read_bytes(), args.client.read_bytes())
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
