#!/usr/bin/env python3
"""Phase196: original SiN 2006 server cache tree link wrapper SHA auditor.

Requires the owner's private, exact original client.dll and server.dll. Stores
no game binaries, full method bytes, or executable disassembly in this repo.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_post_emitter import PE32, VerificationError

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
ORIGINALS = {
    "scene": {
        "va": 0x221AA890,
        "length": 109,
        "sha": "7df2e686b670494a7683bd72793a5dc87594159ce7916ce8354ae8821cfc1fc5",
        "balance": 0x221A8F20,
    },
    "modelsounds": {
        "va": 0x221441B0,
        "length": 109,
        "sha": "2430a2e9c122c44c12371b9c63ed7aaeddfe2dd7add4fb42f61a0a914edf2e25",
        "balance": 0x221429D0,
    },
}
INCOMING = {0x221ACD36: 0x221AA890, 0x22146F26: 0x221441B0}
OUTGOING_CALL_OFFSETS = (0x47, 0x56, 0x64)
NORMALIZED_SHA = "2b1dd748abfd04da2818f807c2ce84be6be4e28aef95c8d92f8ad6b41573d461"

# Sparse independent machine-code witnesses, deliberately NOT a full body.
WITNESSES = {
    0x00: "8b5104", 0x04: "8b742408", 0x08: "0fb7c6",
    0x0B: "8d0440", 0x11: "668b54240c", 0x16: "6681faffff",
    0x1B: "66895004", 0x25: "66c700ffff", 0x2A: "66c740060000",
    0x30: "742d", 0x32: "807c241000", 0x37: "0fb7c2",
    0x43: "668934c2", 0x4D: "c20c00", 0x51: "668974c202",
    0x5C: "c20c00", 0x60: "66897110", 0x6A: "c20c00",
}

def relative_call(site: int, target: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target - site - 5)

def verify_native(pe: PE32, *, check_hash: bool = True) -> dict:
    if pe.base != 0x22000000:
        raise VerificationError("Phase196 original server image base mismatch")
    normalized = []
    for name, spec in ORIGINALS.items():
        va, length, digest, balance = (
            spec["va"], spec["length"], spec["sha"], spec["balance"]
        )
        if pe.section(va) != ".text" or pe.section(balance) != ".text":
            raise VerificationError("Phase196 original function/callee section mismatch")
        original = bytearray(pe.at(va, length))
        if check_hash and hashlib.sha256(original).hexdigest() != digest:
            raise VerificationError("Phase196 original whole function hash mismatch: " + name)
        for offset, witness_hex in WITNESSES.items():
            witness = bytes.fromhex(witness_hex)
            if original[offset:offset + len(witness)] != witness:
                raise VerificationError(
                    "Phase196 original instruction witness mismatch at %#x" % (va + offset)
                )
        for offset in OUTGOING_CALL_OFFSETS:
            site = va + offset
            if original[offset:offset + 5] != relative_call(site, balance):
                raise VerificationError(
                    "Phase196 original balancing helper CALL mismatch at %#x" % site
                )
            original[offset + 1:offset + 5] = b"\x00" * 4
        if pe.at(va + length, 3) != b"\xcc" * 3:
            raise VerificationError("Phase196 original function INT3 boundary mismatch")
        normalized.append(bytes(original))
    for site, target in INCOMING.items():
        if pe.at(site, 5) != relative_call(site, target):
            raise VerificationError(
                "Phase196 native insertion-wrapper CALL mismatch at %#x" % site
            )
    if normalized[0] != normalized[1]:
        raise VerificationError("Phase196 native twins differ beyond three CALL operands")
    if check_hash and hashlib.sha256(normalized[0]).hexdigest() != NORMALIZED_SHA:
        raise VerificationError("Phase196 normalized complete body hash mismatch")
    return {
        "sha_verified_original_functions": 2 if check_hash else 0,
        "sha_verified_native_code_bytes": 218 if check_hash else 0,
        "sparse_opcode_witnesses": len(WITNESSES) * 2,
        "exact_outgoing_balance_calls": len(OUTGOING_CALL_OFFSETS) * 2,
        "exact_incoming_insert_calls": len(INCOMING),
        "native_wrapper_size_each": 109,
        "normalized_sha256": NORMALIZED_SHA if check_hash else None,
        "original_parent_index_width": 16,
        "original_node_stride": 24,
        "original_new_node_offsets": {"left": 0, "right": 2, "parent": 4, "flags": 6},
        "native_stack_cleanup_bytes": 12,
        "limitations": [
            "Original balancing helper implementation remains opaque",
            "Original native unchecked indexing is not a safe host implementation",
            "No playable original-engine replacement game DLL is demonstrated",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    if hashlib.sha256(server).hexdigest() != SERVER_SHA:
        raise VerificationError("original full server.dll SHA256 mismatch")
    if hashlib.sha256(client).hexdigest() != CLIENT_SHA:
        raise VerificationError("original full client.dll SHA256 mismatch")
    return verify_native(PE32(server))

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, type=Path)
    parser.add_argument("--client", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = json.dumps(
        audit(args.server.read_bytes(), args.client.read_bytes()), indent=2
    ) + "\n"
    if args.output:
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report, end="")

if __name__ == "__main__":
    main()
