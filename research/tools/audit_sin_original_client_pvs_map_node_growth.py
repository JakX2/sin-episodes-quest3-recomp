#!/usr/bin/env python3
"""Phase238: verify original 2006 SiN client PVS map resolver and growth.

Counts complete original functions only after authenticating the complete
owner-original client.dll AND server.dll SHA256 digests. This script does not
contain proprietary original function bodies, original binary files or full
native disassemblies. Synthetic fixtures cannot receive original-method credit.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Name; native original VA; exact whole method size; whole original SHA256;
# trailing INT3 count; RET4 offsets; sparse independent byte witnesses.
METHODS = (
    (
        "map_lazy_sentinel_or_node_resolver",
        0x24105F40, 81,
        "b9b6f5ec6f6a914b682e44461b33942b02006a864bb2bd5e0f8bf695f52028c1",
        15, (0x46, 0x4E), {
            0x00: "b801000000",
            0x05: "8405c4fd3324",
            0x0B: "baffff0000",
            0x10: "7521",
            0x12: "0905c4fd3324",
            0x18: "668915bcfd3324",
            0x1F: "668915befd3324",
            0x26: "668915c0fd3324",
            0x2D: "66a3c2fd3324",
            0x33: "668b442404",
            0x38: "663bc2",
            0x40: "c1e004",
            0x43: "034104",
            0x46: "c20400",
            0x49: "b8bcfd3324",
            0x4E: "c20400",
        },
    ),
    (
        "map_sixteen_byte_record_capacity_growth",
        0x24106140, 110,
        "8adf0bd0a02d5471c712cbbe4b97bca99a4d6c92b30450f309ab865df37914ae",
        2, (0x56, 0x6B), {
            0x00: "568bf1",
            0x03: "8b4e08",
            0x06: "83f9ff",
            0x09: "745f",
            0x0B: "8b4604",
            0x0E: "8b542408",
            0x12: "03d0",
            0x16: "7d25",
            0x1B: "85c0",
            0x31: "c7460402000000",
            0x38: "395604",
            0x3D: "8b06",
            0x43: "8b4e04",
            0x46: "c1e104",
            0x4B: "e8d0970600",
            0x56: "c20400",
            0x59: "8b5604",
            0x5C: "c1e204",
            0x60: "e84b970600",
            0x6B: "c20400",
        },
    ),
)
# Includes caller edges from Phase237, direct sibling calls to the resolver
# and exact native memory helpers used by original growth.
CALLS = (
    (0x2410673D, 0x24105F40),  # Phase237 reused-index resolver
    (0x2410671C, 0x24106140),  # Phase237 capacity growth request
    (0x24105FA5, 0x24105F40),  # node flag accessor
    (0x24106105, 0x24105F40),  # node flag predicate A
    (0x24106125, 0x24105F40),  # node flag predicate B
    (0x2410618B, 0x2416F960),  # original realloc when non-null
    (0x241061A0, 0x2416F8F0),  # original alloc when null
)

def call_bytes(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase238 native client PE base drift")
    rows = []
    sparse = 0
    for name, va, size, digest, pad, returns, anchors in METHODS:
        if client.section_of(va) != ".text" or \
                client.section_of(va + size - 1) != ".text":
            raise VerificationError("Phase238 original body outside .text: " + name)
        raw = client.at(va, size)
        if check_hash and hashlib.sha256(raw).hexdigest() != digest:
            raise VerificationError("Phase238 whole original method SHA256 drift: " + name)
        if client.at(va + size, pad) != b"\xCC" * pad:
            raise VerificationError("Phase238 original INT3 boundary drift: " + name)
        for off in returns:
            if raw[off:off + 3] != b"\xC2\x04\x00":
                raise VerificationError(f"Phase238 native RET4 changed: {name}+{off:#x}")
        for off, encoded in anchors.items():
            expected = bytes.fromhex(encoded)
            if raw[off:off + len(expected)] != expected:
                raise VerificationError(
                    f"Phase238 native opcode witness drift: {name}+{off:#x}"
                )
            sparse += 1
        rows.append({
            "name": name,
            "native_entry": f"0x{va:08X}",
            "whole_original_native_bytes": size,
            "whole_original_code_sha256": digest if check_hash else None,
            "original_RET4_paths": len(returns),
            "original_trailing_INT3": pad,
        })
    for site, target in CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site, 5) != call_bytes(site, target):
            raise VerificationError(f"Phase238 native E8 target drift: {site:#x}")
    return {
        "phase": 238,
        "new_original_complete_SHA_methods":
            len(rows) if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":
            sum(m[2] for m in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods": rows,
        "original_sparse_opcode_witnesses": sparse,
        "original_direct_CALL_edges": len(CALLS),
        "resolver_index_stack_WORD_offset": 4,
        "resolver_record_stride": 16,
        "resolver_map_record_pointer_DWORD_offset": 4,
        "resolver_missing_index": 0xFFFF,
        "resolver_global_init_flag_VA": "0x2433FDC4",
        "resolver_global_sentinel_VA": "0x2433FDBC",
        "resolver_sentinel_header_WORDs": [0xFFFF, 0xFFFF, 0xFFFF, 1],
        "growth_storage_pointer_DWORD_at_this": 0,
        "growth_capacity_DWORD_at_this": 4,
        "growth_policy_DWORD_at_this": 8,
        "growth_seed_records": 2,
        "growth_record_stride": 16,
        "opaque_native_memory_helpers": ["0x2416F960", "0x2416F8F0"],
        "scope_limits": [
            "The original memory helper bodies are not reconstructed.",
            "The native sentinel is global; the host state models its first 8 bytes only.",
            "Original CRT/global static lifecycle is not reconstructed.",
            "Host models guard invalid index/overflow; native x86 does not.",
            "No original-game gameplay DLL loading or full map balancing is proved.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for dll, raw in (("server", server), ("client", client)):
        if hashlib.sha256(raw).hexdigest() != ORIGINALS[dll]["sha256"]:
            raise VerificationError(
                "Phase238 requires BOTH whole original retail DLL SHA256s: " + dll
            )
    report = verify_native(PEImage(client), authentic_provenance=True)
    if report["new_original_complete_SHA_methods"] != 2 or \
            report["new_original_native_code_bytes"] != 191:
        raise VerificationError("Phase238 original method accounting drift")
    return report

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    arg = p.add_mutually_exclusive_group(required=True)
    arg.add_argument("--archive", type=Path)
    arg.add_argument("--server", type=Path)
    p.add_argument("--client", type=Path)
    a = p.parse_args()
    if a.archive:
        if a.client:
            p.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll") != 1 or \
                    z.namelist().count("client.dll") != 1:
                raise VerificationError("Phase238 requires one root retail DLL pair")
            result = audit(z.read("server.dll"), z.read("client.dll"))
    else:
        if not a.client:
            p.error("--server requires --client")
        result = audit(a.server.read_bytes(), a.client.read_bytes())
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
