#!/usr/bin/env python3
"""Phase197 original SiN cache tree balancing evidence, owner-only SHA audit.

Full original game DLLs are required at runtime, never stored in source.
No native disassembly or full proprietary machine-code body is embedded.
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
ENTRY = {
    "scene": (0x221A8F20, 0x2256E1F0,
              "d283bb9cfb88ea76182c67edf5e9a4e747e3f1005f7bd6f03af9a0d9b7707132"),
    "modelsounds": (0x221429D0, 0x22558D50,
                    "3398245617c372d63f037cb7496f1308e298867ab501c982a7103b63741101aa"),
}
LENGTH = 0x56B
NORMALIZED_SHA = "2883dbef8df10e4d3cdf27e81d0b513c2b4c5d71467fc24618c880a0bce2c683"

# Each is the start of a four-byte original absolute address operand.
# Its value must be sentinel base + corresponding byte delta.
ADDRESS_OFFSETS = tuple(map(int, """
34 51 58 65 72 79 103 119 126 133 140 147 171 194 201 208 215 222
246 262 269 276 283 290 314 330 337 344 351 358 388 404 411 418
425 432 462 483 490 497 504 511 541 557 564 571 578 585 615 692
699 706 713 720 754 770 777 784 791 798 822 924 931 938 945
952 982 997 1004 1011 1018 1025 1055 1132 1139 1146 1153
1160 1194 1210 1217 1224 1231 1238 1262
""".split()))
ADDRESS_DELTAS = tuple(map(int, """
8 8 0 2 4 6 0 8 0 2 4 6 0 8 0 2 4 6
0 8 0 2 4 6 0 8 0 2 4 6 0 8 0 2
4 6 0 8 0 2 4 6 0 8 0 2 4 6 0 8
0 2 4 6 0 8 0 2 4 6 0 8 0 2 4 6
0 8 0 2 4 6 0 8 0 2 4 6 0 8 0 2
4 6 0
""".split()))
assert len(ADDRESS_OFFSETS) == len(ADDRESS_DELTAS) == 85
# Same eight local CALL instructions but two repository-specific target sets.
# First and final calls rotate in one direction; other rotation uses the
# second helper. Actual orientation semantics still need callee audits.
CALL_TARGETS = {
    0x34C: (0x221A6BB0, 0x221409B0),
    0x354: (0x221A4D90, 0x2213E990),
    0x360: (0x221A4D90, 0x2213E990),
    0x38A: (0x221A6CA0, 0x22140AA0),
    0x503: (0x221A6CA0, 0x22140AA0),
    0x50B: (0x221A4D90, 0x2213E990),
    0x517: (0x221A4D90, 0x2213E990),
    0x541: (0x221A6BB0, 0x221409B0),
}
INCOMING = {0x221AA8D7: 0x221A8F20,
            0x221AA8E6: 0x221A8F20,
            0x221AA8F4: 0x221A8F20,
            0x221441F7: 0x221429D0,
            0x22144206: 0x221429D0,
            0x22144214: 0x221429D0}
# Sparse opcode witnesses, well below the full original native method body.
WITNESSES = {
    0x000: "8b542404",
    0x008: "663b5610",
    0x00C: "bb01000000",
    0x011: "0f8440050000",
    0x017: "5557eb05",
    0x01B: "8b54241490",
    0x026: "84cbbdffff0000",
    0x059: "7e040fb7c2",
    0x06B: "84cb668b4004",
    0x0B4: "0f859b040000",
    0x280: "0fb7ca8d1449",
    0x370: "0c4966895cca06",
    0x548: "442414663b4610",
    0x557: "0fb746108b5604",
    0x562: "66895cca065bc20400",
}

def rel32(source: int, target: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target-source-5)

def verify_native(pe: PE32, *, check_hash: bool = True) -> dict:
    if pe.base != 0x22000000:
        raise VerificationError("original SiN 2006 server PE image base mismatch")
    twins = []
    for i, (name, (va, sentinel, digest)) in enumerate(ENTRY.items()):
        if pe.section(va) != ".text" or pe.section(sentinel) != ".data":
            raise VerificationError("Phase197 original native section mismatch " + name)
        code = bytearray(pe.at(va, LENGTH))
        if check_hash and hashlib.sha256(code).hexdigest() != digest:
            raise VerificationError("Phase197 complete native method hash mismatch " + name)
        for off, anchor in WITNESSES.items():
            expected = bytes.fromhex(anchor)
            if code[off:off+len(expected)] != expected:
                raise VerificationError("Phase197 sparse opcode mismatch at %#x" % (va+off))
        for off, delta in zip(ADDRESS_OFFSETS, ADDRESS_DELTAS):
            expected = sentinel + delta
            if struct.unpack_from("<I", code, off)[0] != expected:
                raise VerificationError("Phase197 native static operand drift at %#x" % (va+off))
            code[off:off+4] = b"\x00"*4
        for off, targets in CALL_TARGETS.items():
            if code[off:off+5] != rel32(va+off, targets[i]):
                raise VerificationError("Phase197 native direct call drift at %#x" % (va+off))
            code[off+1:off+5] = b"\x00"*4
        if pe.at(va+LENGTH, 5) != b"\xcc"*5:
            raise VerificationError("Phase197 ret4+padding boundary mismatch")
        twins.append(bytes(code))
    for site, destination in INCOMING.items():
        if pe.at(site, 5) != rel32(site, destination):
            raise VerificationError("Phase196 caller to balance mismatch at %#x" % site)
    if twins[0] != twins[1]:
        raise VerificationError("Phase197 bodies differ beyond 85 globals and 8 calls")
    if check_hash and hashlib.sha256(twins[0]).hexdigest() != NORMALIZED_SHA:
        raise VerificationError("Phase197 normalized body hash mismatch")
    return {
        "new_sha_verified_original_functions": 2 if check_hash else 0,
        "new_sha_verified_original_x86_bytes": 2*LENGTH if check_hash else 0,
        "original_full_method_bytes_each": LENGTH,
        "sparse_opcode_witnesses": 2*len(WITNESSES),
        "exact_static_address_operands": 2*len(ADDRESS_OFFSETS),
        "exact_outgoing_direct_calls": 2*len(CALL_TARGETS),
        "exact_incoming_direct_calls": len(INCOMING),
        "normalized_complete_method_sha256": NORMALIZED_SHA if check_hash else None,
        "original_root_index_offset": 0x10,
        "node_color_word_offset": 6,
        "color_0": "red", "color_1": "black",
        "root_color_forced_black_on_return": True,
        "limitations": [
            "The full loop, recoloring and tree rotations are NOT reconstructed",
            "Native rotation helpers must be recovered before enabling red-parent paths",
            "A synthetic test is not a real SiN original-engine loading test",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    if hashlib.sha256(server).hexdigest() != SERVER_SHA or hashlib.sha256(client).hexdigest() != CLIENT_SHA:
        raise VerificationError("original complete dual-DLL SHA256 mismatch")
    return verify_native(PE32(server))

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--client", required=True, type=Path)
    p.add_argument("--output", type=Path)
    a = p.parse_args()
    report = json.dumps(audit(a.server.read_bytes(), a.client.read_bytes()), indent=2) + "\n"
    if a.output:
        a.output.write_text(report, encoding="utf8")
    else:
        print(report, end="")

if __name__ == "__main__":
    main()
