#!/usr/bin/env python3
"""Read-only SHA-pinned audit for original 2006 CNPC_Jessica item throwing."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ORIGINAL_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
# Original-function exact-binary digests, not redistributed proprietary code.
WITNESSES = {
    "InputThrowItemAtPlayer": (0x223622D0, 46, "95795356f5a3637228191d6114e62189b060fc637fb4358f48c4bc86375e715d"),
    "InputHolsterWeapon": (0x22362300, 30, "b5dc2b861ce1ecf2d7555f18cde466f442d0b4babff71d78d8388c5fe948372f"),
    "InputSetPrimaryAmmoCountToThrow": (0x22362320, 33, "345767040c8fb0d30595e641d679fbf04bef7fbbd99cac8b5e5c299336936b49"),
    "InputSetSecondaryAmmoCountToThrow": (0x22362350, 33, "0bca8cca8bfdce636f4d9443c4ba5a6d727ef352dac5449a8969125e1ce1fba7"),
    "SelectSchedule": (0x22362380, 29, "07a72fa320587c5a2a3e181558bd3c0cea344352cd7711a9998b42e37871a3f9"),
    "HandleAnimEvent": (0x22362420, 292, "23e3f86268257555ae7edee97825848b6d4d41ebb37f41ad03ad32b6830453f4"),
}

class PE32:
    def __init__(self, data):
        self.data = data
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise ValueError("missing MZ header")
        off = struct.unpack_from("<I", data, 0x3C)[0]
        if off + 24 > len(data) or data[off:off+4] != b"PE\0\0":
            raise ValueError("invalid PE header")
        count, optional = struct.unpack_from("<H", data, off+6)[0], struct.unpack_from("<H", data, off+20)[0]
        if count < 1 or count > 32 or optional < 96 or off+24+optional+40*count > len(data):
            raise ValueError("invalid PE32 section table")
        o = off + 24
        if struct.unpack_from("<H", data, o)[0] != 0x10B:
            raise ValueError("not PE32")
        self.base = struct.unpack_from("<I", data, o+28)[0]
        self.sections = []
        for i in range(count):
            p = o + optional + i*40
            virtual_size, rva, raw_size, file_offset = struct.unpack_from("<IIII", data, p+8)
            if file_offset > len(data) or raw_size > len(data)-file_offset:
                raise ValueError("invalid file-backed section")
            self.sections.append((rva, min(virtual_size, raw_size), file_offset))

    def read(self, va, size):
        if size < 0 or va < self.base:
            raise ValueError("invalid VA")
        rva = va - self.base
        for first, length, start in self.sections:
            if first <= rva and rva-first <= length and size <= length-(rva-first):
                return self.data[start+rva-first:start+rva-first+size]
        raise ValueError("VA outside file-backed section")

    def branch_target(self, va, op):
        data = self.read(va, 5)
        if data[0] != op:
            raise ValueError("wrong x86 call/jmp opcode")
        return va+5+struct.unpack_from("<i", data, 1)[0]

def verify(data):
    if hashlib.sha256(data).hexdigest() != ORIGINAL_SHA:
        raise ValueError("different or modified original server.dll")
    image = PE32(data)
    if image.base != 0x22000000:
        raise ValueError("original image base mismatch")
    for name, (va, size, expected) in WITNESSES.items():
        if hashlib.sha256(image.read(va, size)).hexdigest() != expected:
            raise ValueError("function witness mismatch: "+name)
    if image.read(0x22490254, len(b"anim_attachment_RH\0")) != b"anim_attachment_RH\0":
        raise ValueError("right-hand attachment name mismatch")
    schedule = image.read(0x224978C0, 145).split(b"\0", 1)[0]
    pieces = [b"SCHED_THROW_ITEM", b"TASK_FACE_PLAYER", b"TASK_PLAY_SEQUENCE",
              b"ACTIVITY:ACT_THROW_ITEM", b"Interrupts"]
    positions = [schedule.find(x) for x in pieces]
    if any(x < 0 for x in positions) or positions != sorted(positions):
        raise ValueError("original schedule task order mismatch")
    jumps = [(0x223622F9, 0xE9, 0x22039760),
             (0x22362385, 0xE8, 0x220397C0),
             (0x2236243D, 0xE8, 0x220398A0),
             (0x2236248B, 0xE8, 0x22144DA0)]
    for at, opcode, target in jumps:
        if image.branch_target(at, opcode) != target:
            raise ValueError("original branch target mismatch")
    return {"server_sha256": ORIGINAL_SHA, "function_hashes_verified":len(WITNESSES),
            "relative_branches_verified":len(jumps), "original_only":True}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", type=Path, required=True)
    args = p.parse_args()
    try:
        print(json.dumps(verify(args.server.read_bytes()), indent=2))
    except (ValueError, OSError) as error:
        p.exit(1, "FAIL: "+str(error)+"\n")

if __name__ == "__main__":
    main()
