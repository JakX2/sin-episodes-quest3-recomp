#!/usr/bin/env python3
"""Phase239: authenticate the original SiN 2006 client PVS tree-attach wrapper.

No original DLL, complete proprietary method bytes or disassembly are shipped.
Only the two exact retail DLL SHA256s may grant native-method review credit.
Synthetic witnesses alone cannot count an original function.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

METHOD = {
    "name": "pvs_map_tree_attach_frontend",
    "entry": 0x241076E0,
    "length": 106,
    "sha256": "fd141c9025d7975a43a24b398a6b5bee1f34cead227171e5319c269e78c8df9f",
    "trailing_int3": 6,
    "ret12_offsets": (0x4A, 0x59, 0x67),
    "sparse_witnesses": {
        0x00: "668b542408",   # parent index on stack
        0x05: "56",           # preserve ESI
        0x06: "8b742408",     # newly allocated record index
        0x0A: "0fb7c6",       # zero extend record index
        0x0D: "c1e004",       # 16-byte record stride
        0x10: "034104",       # map record storage +4
        0x13: "6681faffff",   # parent==0xFFFF?
        0x18: "66895004",     # record+4 parent WORD
        0x1C: "66c74002ffff", # right child starts absent
        0x22: "66c700ffff",  # left child starts absent
        0x27: "66c740060000", # record+6 state/color WORD=0
        0x2D: "742d",        # absent parent -> root
        0x38: "807c241000", # side byte from third stack argument
        0x40: "66893410",   # parent left child index
        0x4A: "c20c00",     # RET 12 left branch
        0x4E: "6689741002", # parent right child index
        0x59: "c20c00",     # RET 12 right branch
        0x5D: "66897110",   # map root WORD+0x10
        0x67: "c20c00",     # RET 12 root branch
    },
}

# The original wrapper ALWAYS delegates to the same native tree balancer.
# The reviewed Phase236 caller is included to pin function ownership.
CALLS = (
    (0x24107724, 0x24106E30),   # linked left branch -> original rebalance
    (0x24107733, 0x24106E30),   # linked right branch -> original rebalance
    (0x24107741, 0x24106E30),   # root branch -> original rebalance
    (0x24108786, 0x241076E0),   # Phase236 map insertion frontend -> attach
)

def e8_call(site: int, destination: int) -> bytes:
    return b"\xE8" + struct.pack("<i", destination - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase239 original PE32 client image base changed")
    va, length = METHOD["entry"], METHOD["length"]
    if (client.section_of(va) != ".text" or
            client.section_of(va + length - 1) != ".text"):
        raise VerificationError("Phase239 native method is not entirely .text")
    native = client.at(va, length)
    if check_hash and hashlib.sha256(native).hexdigest() != METHOD["sha256"]:
        raise VerificationError("Phase239 complete original method SHA256 drift")
    if client.at(va + length, METHOD["trailing_int3"]) != \
            b"\xCC" * METHOD["trailing_int3"]:
        raise VerificationError("Phase239 native function INT3 boundary drift")
    for off in METHOD["ret12_offsets"]:
        if native[off:off+3] != b"\xC2\x0C\x00":
            raise VerificationError(f"Phase239 native RET12 drift at +{off:#x}")
    for off, witness in METHOD["sparse_witnesses"].items():
        expected = bytes.fromhex(witness)
        if native[off:off + len(expected)] != expected:
            raise VerificationError(f"Phase239 native instruction drift at +{off:#x}")
    for site, target in CALLS:
        if client.section_of(site) != ".text" or client.at(site, 5) != e8_call(site, target):
            raise VerificationError(f"Phase239 original E8 rel32 drift at {site:#x}")
    return {
        "phase": 239,
        "new_original_complete_SHA_methods": (
            1 if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes": (
            length if check_hash and authentic_provenance else 0
        ),
        "original_native_entry": f"0x{va:08X}",
        "original_method_native_bytes": length,
        "original_entire_method_sha256": (
            METHOD["sha256"] if check_hash else None
        ),
        "original_ret12_exits": len(METHOD["ret12_offsets"]),
        "original_trailing_INT3": METHOD["trailing_int3"],
        "checked_sparse_original_witnesses": len(METHOD["sparse_witnesses"]),
        "checked_direct_original_E8_edges": len(CALLS),
        "original_node_stride": 16,
        "original_node_parent_WORD_offset": 4,
        "original_node_left_WORD_offset": 0,
        "original_node_right_WORD_offset": 2,
        "original_node_color_WORD_offset": 6,
        "original_map_storage_DWORD_offset": 4,
        "original_map_root_WORD_offset": 16,
        "original_null_index": 0xFFFF,
        "original_balancer_VA": "0x24106E30",
        "scope_limits": [
            "The 0x24106E30 original balancing implementation is NOT recovered here.",
            "Native balancing is invoked on root, left, AND right paths.",
            "C++ host callback is a synthetic dependency, not original engine code.",
            "Original-game reconstructed client/server DLL execution not proven.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for dll_name, content in (("server", server), ("client", client)):
        if hashlib.sha256(content).hexdigest() != ORIGINALS[dll_name]["sha256"]:
            raise VerificationError(
                "Phase239 requires BOTH whole owner-original DLL SHA256s: " + dll_name
            )
    report = verify_native(PEImage(client), authentic_provenance=True)
    if (report["new_original_complete_SHA_methods"] != 1 or
            report["new_original_native_code_bytes"] != 106):
        raise VerificationError("Phase239 reviewed-function accounting drift")
    return report

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive", type=Path)
    source.add_argument("--server", type=Path)
    parser.add_argument("--client", type=Path)
    arguments = parser.parse_args()
    if arguments.archive:
        if arguments.client:
            parser.error("--archive excludes --client")
        with zipfile.ZipFile(arguments.archive) as archive:
            if (archive.namelist().count("server.dll") != 1 or
                    archive.namelist().count("client.dll") != 1):
                raise VerificationError("Phase239 needs one root original DLL pair")
            result = audit(archive.read("server.dll"), archive.read("client.dll"))
    else:
        if not arguments.client:
            parser.error("--server also requires --client")
        result = audit(arguments.server.read_bytes(), arguments.client.read_bytes())
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
