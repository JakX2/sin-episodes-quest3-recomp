#!/usr/bin/env python3
"""Phase215 original SiN 2006 client.dll type-2 string specialist verification.

Requires both SHA-matched, OWNER-LOCAL original Windows x86 DLLs. Publishes
no complete original native body, commercial data or disassembly.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA, BASE
from audit_sin_original_client_snapshot_record_router import CALL_EDGES

# (complete native VA, length, complete original method SHA256,
#  following INT3 bytes, original router callsite, stack bytes popped,
#  sparse native x86 instruction bytes)
METHODS = {
    "type2_string_equality_status": (
        0x24198B80, 83,
        "94b49be4773d3a2acc1dee70c21e4551b0f9891843d9c98942da89d4dd57b268",
        13, 0x24199883, 8, {
            0x00: "80791400", 0x06: "8b4118",
            0x09: "f6401304", 0x0F: "8b442404",
            0x18: "8a10", 0x1C: "3a16",
            0x38: "33c0", 0x3C: "1bc0",
            0x46: "33c0", 0x48: "c20800",
            0x4B: "b801000000", 0x50: "c20800",
        }),
    "type2_string_diagnostics": (
        0x24197D00, 65,
        "b4673143787130ff0988b900b3427626a3d9fdcda8d265e5385ae93cb7274986",
        15, 0x24199899, 12, {
            0x00: "56", 0x03: "807e1400",
            0x13: "85ff", 0x1D: "686c3f2a24",
            0x2C: "685c3f2a24", 0x3E: "c20c00",
        }),
    "type2_string_preview": (
        0x241984A0, 30,
        "ac12b87f0269180714911a5f6c8b0c28f41ba2eb4e6d1f33c2774a68223dfe23",
        2, 0x241998A3, 12, {
            0x00: "8b4134", 0x03: "3b4118",
            0x08: "8b542408", 0x0D: "684c402a24",
            0x1B: "c20c00",
        }),
}
NESTED_CALLS = (
    (0x24197D23, 0x24197910),
    (0x24197D33, 0x241979C0),
    (0x241984B3, 0x24197FE0),
)
STRINGS = {
    0x242A3F6C: "string differs (net %s pred %s)\n",
    0x242A3F5C: "string (%s)\n",
    0x242A404C: "string (%s)",
}


def rel_call(site: int, target: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target - site - 5)


def verify_native(client: PEImage, *, check_hash: bool = True) -> dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase215 wrong client PE32 image base")
    rows = {}
    count_witnesses = 0
    for name, (va, length, digest, padding, caller, stack_bytes, witnesses) in METHODS.items():
        if client.section_of(va) != ".text" or client.section_of(va + length - 1) != ".text":
            raise VerificationError("Phase215 original method not in .text: " + name)
        raw = client.at(va, length)
        if check_hash and hashlib.sha256(raw).hexdigest() != digest:
            raise VerificationError("Phase215 complete native function SHA mismatch: " + name)
        if raw[-3:] != b"\xc2" + struct.pack("<H", stack_bytes):
            raise VerificationError("Phase215 final native RET stack bytes drift: " + name)
        if client.at(va + length, padding) != b"\xcc" * padding:
            raise VerificationError("Phase215 exact original INT3 boundary drift: " + name)
        for offset, hx in witnesses.items():
            witness = bytes.fromhex(hx)
            if raw[offset:offset + len(witness)] != witness:
                raise VerificationError("Phase215 x86 instruction witness drift: " + name)
            count_witnesses += 1
        if (caller, va) not in CALL_EDGES or client.at(caller, 5) != rel_call(caller, va):
            raise VerificationError("Phase215 original Phase212 caller edge drift: " + name)
        if client.section_of(caller) != ".text":
            raise VerificationError("Phase215 router caller outside .text")
        rows[name] = {
            "native_va": f"0x{va:08X}", "native_bytes": length,
            "whole_native_sha256": digest if check_hash else None,
            "router_callsite": f"0x{caller:08X}",
            "native_ret_stack_bytes": stack_bytes,
        }

    for caller, callee in NESTED_CALLS:
        if (client.section_of(caller) != ".text" or
                client.section_of(callee) != ".text" or
                client.at(caller, 5) != rel_call(caller, callee)):
            raise VerificationError("Phase215 original diagnostic/preview callback CALL drift")
    for va, value in STRINGS.items():
        if client.section_of(va) != ".rdata" or (
                client.at(va, len(value) + 1) != value.encode("ascii") + b"\0"):
            raise VerificationError("Phase215 original diagnostic literal drift: " + hex(va))
    return {
        "new_original_SHA_verified_native_functions": len(METHODS) if check_hash else 0,
        "new_complete_original_x86_bytes": sum(x[1] for x in METHODS.values()) if check_hash else 0,
        "method_evidence": rows,
        "sparse_instruction_witnesses": count_witnesses,
        "direct_router_calls": len(METHODS),
        "nested_diagnostic_calls": len(NESTED_CALLS),
        "original_diagnostic_format_literals": list(STRINGS.values()),
        "limits": [
            "String equality and logging behavior is verified at method level; larger serializer ownership remains unknown.",
            "Native code is not memory-safe; host-only buffer guards are not original behavior.",
            "No complete original method code, commercial DLLs, or proprietary assets distributed.",
            "No reconstructed Windows game module has been run inside the real retail engine.",
        ],
    }


def audit(server_bytes: bytes, client_bytes: bytes) -> dict:
    if (hashlib.sha256(server_bytes).hexdigest() != FULL_SHA["server"] or
            hashlib.sha256(client_bytes).hexdigest() != FULL_SHA["client"]):
        raise VerificationError("Phase215 requires both full original SiN 2006 DLL SHA256s")
    return verify_native(PEImage(client_bytes))


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", type=Path, required=True)
    p.add_argument("--client", type=Path, required=True)
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    report = json.dumps(audit(args.server.read_bytes(), args.client.read_bytes()), indent=2) + "\n"
    if args.output:
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report, end="")


if __name__ == "__main__":
    main()
