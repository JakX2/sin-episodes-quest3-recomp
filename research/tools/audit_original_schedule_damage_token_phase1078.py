#!/usr/bin/env python3
"""Verify retail bare DMG_BURN conversion evidence, without redistributing DLL."""
import argparse
import hashlib
import pathlib
import struct

BASE = 0x22000000
SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
RANGES = (
    (0x2209f182, 0x2209f1c8, "214804469d89e9f86ccdab7d8d52b2f7d33c2575157f8b059339a049634bc2f3"),
    (0x223a5c8b, 0x223a5cd6, "dedaf6a490ba077b552d0819933be6c46a82f67ea26b3ae13b6e990ca172f841"),
    (0x223a8849, 0x223a88e0, "c41a057cd62fd8d3a33f4d1df0b7bd5eb87c2cae86393ab010aead47deca117b"),
    (0x223b02f6, 0x223b074d, "e0b18e7b9a3d7b85881f1c2fdec30252fcc0ebf3eec1cb19d2ad71eea7683cdd"),
)
SCRIPTS = (
    (0x22491ec0, 264, "6a3d17cce9db0c51f52fcb2d80385b4229da5848fcb9130687048938579e1032"),
    (0x22498a20, 244, "6181feb7f7e0b50fc343dce694f3c853e5d6b081a6fbeeb033ab73cd9cea3e22"),
)
def audit(data):
    if hashlib.sha256(data).hexdigest() != SHA:
        raise ValueError("not authenticated retail server.dll")
    def read(a, n):
        return data[a-BASE:a-BASE+n]
    for a, z, digest in RANGES:
        if hashlib.sha256(read(a, z-a)).hexdigest() != digest:
            raise ValueError(f"code witness changed: {a:08x}")
    for a, n, digest in SCRIPTS:
        raw = read(a, n)
        if read(a+n, 1) != b"\0" or hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError(f"schedule string changed: {a:08x}")
        tokens = raw.split()
        pos = tokens.index(b"TASK_GRUNT_DIE_INSTANTLY")
        if tokens[pos+1] != b"DMG_BURN":
            raise ValueError("expected bare damage token")
    for call, target in ((0x2209f1b9, 0x223a5c8b), (0x223a5cc9, 0x223a8849), (0x223a886d, 0x223b02f6)):
        op, delta = struct.unpack("<Bi", read(call,5))
        if op != 0xe8 or call+5+delta != target:
            raise ValueError("conversion edge changed")
    # State-zero jump-table entry; unrecognized first char exits with
    # no digits, status4, then helper sets both halves of double to zero.
    if struct.unpack("<I", read(0x223b074d,4))[0] != 0x223b0362:
        raise ValueError("initial parser state changed")
    if read(0x223b06fb,7) != bytes.fromhex("c745b804000000"):
        raise ValueError("no-conversion status changed")
    if read(0x223a8881,6) != bytes.fromhex("8975e88975ec"):
        raise ValueError("zero result path changed")
    return True
if __name__ == "__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--server",type=pathlib.Path,required=True)
    args=p.parse_args()
    audit(args.server.read_bytes())
    print("PASS original bare DMG_BURN reaches no-conversion positive-zero branch")
