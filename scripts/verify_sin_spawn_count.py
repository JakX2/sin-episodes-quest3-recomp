#!/usr/bin/env python3
"""Verify the original 2006 SiN server.dll spawn-count branch without sharing DLL bytes.

Read-only, SHA-256-pinned PE32 static audit. Does NOT run original gameplay,
recover Ritual source or verify any original DLL's wider game-rule call graph.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

EXPECTED_SHA256 = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
FUNCTION_VA = 0x222d4a00
ZERO_VA = 0x223d2590
# Short instruction signatures only, not a proprietary source/code dump.
SIGNATURES = {
    0x222d4a01: bytes.fromhex("6a 07"),          # modifier id 7
    0x222d4a29: bytes.fromhex("68 00 00 80 3f"), # RNG bound 1.0
    0x222d4a2e: bytes.fromhex("6a 00"),          # RNG bound 0.0
    0x222d4a36: bytes.fromhex("d9 e1"),          # fabs
    0x222d4a3b: bytes.fromhex("de d9"),          # fcompp
    0x222d4a3f: bytes.fromhex("f6 c4 01"),       # test x87 C0
    0x222d4a42: bytes.fromhex("75 29"),          # skip if |mod| < roll
    0x222d4a54: bytes.fromhex("df e0"),          # read x87 status
    0x222d4a5f: bytes.fromhex("83 c0 01"),       # add 1
    0x222d4a66: bytes.fromhex("83 e8 01"),       # subtract 1
}
class PEVerificationError(ValueError):
    pass


def read_pe_va(blob: bytes, va: int, length: int) -> bytes:
    if len(blob) < 0x100 or blob[:2] != b"MZ":
        raise PEVerificationError("invalid PE DOS header")
    e = struct.unpack_from("<I", blob, 0x3c)[0]
    if e + 24 > len(blob) or blob[e:e+4] != b"PE\x00\x00":
        raise PEVerificationError("invalid PE signature")
    machine, sections, _, _, _, optional_size, _ = struct.unpack_from("<HHIIIHH", blob, e+4)
    opt = e + 24
    if machine != 0x14c or optional_size < 96 or opt + optional_size > len(blob):
        raise PEVerificationError("expected PE32 i386 optional header")
    if struct.unpack_from("<H", blob, opt)[0] != 0x10b:
        raise PEVerificationError("expected PE32 magic")
    base = struct.unpack_from("<I", blob, opt + 28)[0]
    rva = va - base
    if rva < 0 or sections < 1 or sections > 96 or length < 0:
        raise PEVerificationError("invalid PE image address or sections")
    table = opt + optional_size
    if table + 40 * sections > len(blob):
        raise PEVerificationError("truncated PE section table")
    for i in range(sections):
        p = table + 40 * i
        virtual_size, vaddr, raw_size, raw_off = struct.unpack_from("<IIII", blob, p+8)
        if vaddr <= rva and rva+length <= vaddr + min(virtual_size, raw_size):
            offset = raw_off + (rva - vaddr)
            if offset + length > len(blob):
                raise PEVerificationError("truncated PE section data")
            return blob[offset:offset+length]
    raise PEVerificationError("address not in file-backed PE section")


def verify(blob: bytes) -> dict:
    if hashlib.sha256(blob).hexdigest() != EXPECTED_SHA256:
        raise PEVerificationError("pinned 2006 server.dll SHA-256 mismatch")
    if read_pe_va(blob, ZERO_VA, 4) != b"\0\0\0\0":
        raise PEVerificationError("reference zero constant mismatch")
    for address, expected in SIGNATURES.items():
        if read_pe_va(blob, address, len(expected)) != expected:
            raise PEVerificationError("branch signature mismatch at " + hex(address))
    return {
        "original_server_dll_sha256_verified": True,
        "function_va": hex(FUNCTION_VA),
        "probability": "x87 branch skips adjustment only if abs(modifier) < randomRoll",
        "threshold_equality": "adjustment at equality for nonzero modifier",
        "direction": "positive adds one NPC; negative subtracts one",
        "original_code_executed": False,
        "full_game_rules_reconstructed": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server_dll", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify(args.server_dll.read_bytes()), indent=2, sort_keys=True))
    except (OSError, PEVerificationError) as error:
        parser.exit(1, "Original SiN DLL verification failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
