#!/usr/bin/env python3
"""Phase235: authenticate original client 8/16-byte PVS storage growth.

Only counts whole-native functions after verifying BOTH complete owner-original
SiN Episodes (2006) retail PE32 DLL SHA256 hashes. This publishes no original
commercial code bodies, disassembly dumps or assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Label, native entry, exact full code length, SHA256 of entire original body,
# count of trailing INT3 pads, RET4 instruction offsets, sparse opcode witnesses.
# Sparse witnesses are independent of the whole-body SHA verification.
METHODS = (
    ("aux_eight_byte_storage_growth", 0x24106010, 116,
     "4b28687cce6676118984f4bb2c61efedb173f6e8093cd355181020e5e8c9c160",
     12, (0x59, 0x71), {
         0x00: "568bf18b4e08",
         0x06: "83f9ff",
         0x09: "7465",
         0x0E: "8b54240803d0",
         0x16: "7d25",
         0x1F: "85c97407",
         0x25: "894604",
         0x31: "c7460404000000",
         0x38: "3956047cdb",
         0x3D: "8b0685c0",
         0x43: "8b4e04",
         0x46: "03c903c903c9",
     }),
    ("pvs_sixteen_byte_storage_growth", 0x24106090, 110,
     "39507159482f561fce8300268e7f9798266b8b6592fb76513959267c669274ba",
     2, (0x56, 0x6B), {
         0x00: "568bf18b4e08",
         0x06: "83f9ff",
         0x09: "745f",
         0x0E: "8b54240803d0",
         0x16: "7d25",
         0x1F: "85c97407",
         0x25: "894604",
         0x31: "c7460402000000",
         0x38: "3956047cdb",
         0x3D: "8b0685c0",
         0x43: "8b4e04",
         0x46: "c1e1045150",
     }),
)

# The actual native E8 rel32 instructions. The +0x6F960/+0x6F8F0
# callees are separate original memory helpers, not recovered in this phase.
CALLS = (
    (0x2410605E, 0x2416F960),  # aux: existing storage path
    (0x24106076, 0x2416F8F0),  # aux: null storage path
    (0x241060DB, 0x2416F960),  # PVS: existing storage path
    (0x241060F0, 0x2416F8F0),  # PVS: null storage path
    (0x2410658A, 0x24106010),  # Phase234 auxiliary index allocator
    (0x2410662A, 0x24106090),  # Phase234 PVS index allocator
)

def rel_call(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase235 original client PE image base mismatch")
    rows = []
    witnesses = 0
    for name, va, length, digest, pad, ret_offsets, anchors in METHODS:
        if client.section_of(va) != ".text" or \
                client.section_of(va + length - 1) != ".text":
            raise VerificationError("Phase235 method outside original .text: " + name)
        raw = client.at(va, length)
        if check_hash and hashlib.sha256(raw).hexdigest() != digest:
            raise VerificationError("Phase235 whole-original body SHA256 drift: " + name)
        if client.at(va + length, pad) != b"\xCC" * pad:
            raise VerificationError("Phase235 trailing INT3 boundary drift: " + name)
        for off in ret_offsets:
            if raw[off:off + 3] != b"\xC2\x04\x00":
                raise VerificationError("Phase235 original RET4 ABI drift: " + name)
        for off, encoded in anchors.items():
            witness = bytes.fromhex(encoded)
            if raw[off:off + len(witness)] != witness:
                raise VerificationError(
                    f"Phase235 native instruction witness drift: {name} +{off:#x}"
                )
            witnesses += 1
        rows.append({
            "name": name,
            "native_entry": f"0x{va:08X}",
            "native_code_bytes": length,
            "whole_original_sha256": digest if check_hash else None,
            "native_RET4_exits": len(ret_offsets),
            "original_trailing_INT3": pad,
        })
    for site, target in CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site, 5) != rel_call(site, target):
            raise VerificationError(f"Phase235 original E8 CALL drift: {site:#x}")
    return {
        "phase": 235,
        "new_original_complete_SHA_methods": (
            len(rows) if check_hash and authentic_provenance else 0
        ),
        "new_original_native_bytes": (
            sum(method[2] for method in METHODS)
            if check_hash and authentic_provenance else 0
        ),
        "reviewed_original_methods": rows,
        "checked_sparse_opcode_witnesses": witnesses,
        "checked_original_CALL_edges": len(CALLS),
        "original_manager_storage_pointer_DWORD": 0,
        "original_manager_capacity_DWORD": 4,
        "original_manager_growth_policy_DWORD": 8,
        "record_strides": [8, 16],
        "zero_capacity_initial_values": [4, 2],
        "opaque_native_memory_helpers": [
            "0x2416F960", "0x2416F8F0",
        ],
        "scope_limits": [
            "Native memory helper implementations are not reconstructed.",
            "No complete original C++ allocator or engine-owned object is constructed.",
            "Host simulation can reject native undefined/overflow cases safely.",
            "No proof yet that reconstructed game DLLs run in original Windows engine.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for name, raw in (("server", server), ("client", client)):
        if hashlib.sha256(raw).hexdigest() != ORIGINALS[name]["sha256"]:
            raise VerificationError(
                "Phase235 requires BOTH complete original DLL SHA256s: " + name
            )
    result = verify_native(PEImage(client), authentic_provenance=True)
    if result["new_original_complete_SHA_methods"] != 2 or \
            result["new_original_native_bytes"] != 226:
        raise VerificationError("Phase235 original function ledger drift")
    return result

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    source = p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive", type=Path)
    source.add_argument("--server", type=Path)
    p.add_argument("--client", type=Path)
    args = p.parse_args()
    if args.archive:
        if args.client:
            p.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll") != 1 or \
                    z.namelist().count("client.dll") != 1:
                raise VerificationError("Phase235 needs one root retail DLL pair")
            result = audit(z.read("server.dll"), z.read("client.dll"))
    else:
        if not args.client:
            p.error("--server also requires --client")
        result = audit(args.server.read_bytes(), args.client.read_bytes())
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
