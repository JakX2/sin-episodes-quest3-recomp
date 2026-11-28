#!/usr/bin/env python3
"""Private source audit: shared C_BaseEntity constructor and world prelude."""
import argparse
import hashlib
import struct
from pathlib import Path

SOURCE_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
CTOR = 0x240CCE30
METHODS = (
    (CTOR, 573, 3, "63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca"),
    (0x24085750, 11, 5, "aba87c91c86af852806714c85edd4ef7306ff59fcad6802998b71cfb142fdf23"),
    (0x24129480, 11, 5, "2ad7aa22b1d77fc40b01104d20e31f8dc14cd9ea2b4b3374f9e4ae9465eeb3b5"),
    (0x240FCB10, 44, 4, "305044e8173fc81a50952318532ec1a5638e4c956dc056285e048ef92448b35d"),
)
CALLS = ((0x240CCE86, 0x24195260), (0x240CCEDE, 0x2411AEB0),
         (0x240CCF17, 0x24061540), (0x240CCF5C, 0x240CB2F0),
         (0x240CCFBE, 0x24061220), (0x240CCFE4, 0x24061220),
         (0x240CD05E, 0x240C9F40))
FACTORIES = (0x24216540, 0x2412B030, 0x240FC880, 0x240F2260,
             0x240E6A60, 0x240E40B0, 0x240E2770, 0x240E25A0,
             0x240DCC70, 0x240DA0A0, 0x240B3B50, 0x240560D0)
BASE_VTABLES = (0x242895EC, 0x24289584, 0x24289550, 0x24289538)
WORLD_VTABLES = (0x2429632C, 0x242962C4, 0x2429628C, 0x24296274)


def need(ok, message):
    if not ok:
        raise ValueError(message)


def verify(path):
    data = path.read_bytes()
    need(hashlib.sha256(data).hexdigest() == SOURCE_SHA, "wrong original client SHA")
    # This pinned PE32 has identical section raw offsets and RVAs for
    # .text, .rdata and the RTTI-bearing part of .data. Never execute it.
    def read(va, size):
        off = va - 0x24000000
        need(0 <= off <= len(data) - size, f"out of image: {va:#x}")
        return data[off:off + size]

    def u32(va):
        return struct.unpack("<I", read(va, 4))[0]

    def call(va, target):
        b = read(va, 5)
        need(b[0] == 0xE8 and va + 5 + struct.unpack_from("<i", b, 1)[0] == target,
             f"call target mismatch: {va:#x}")

    for va, size, pad, digest in METHODS:
        b = read(va, size)
        need(hashlib.sha256(b).hexdigest() == digest, f"method SHA mismatch: {va:#x}")
        need(read(va + size, pad) == b"\xCC" * pad and
             read(va + size + pad, 1) != b"\xCC", f"boundary mismatch: {va:#x}")
        need(b.endswith(b"\xC2\x08\x00" if va == 0x240FCB10 else b"\xC3"),
             f"return mismatch: {va:#x}")
    for va, target in CALLS:
        call(va, target)
    for va in FACTORIES:
        call(va + 23, CTOR)
        need(read(va + 85, 1) == b"\xC3", "canonical factory end mismatch")
    call(0x240FCDD0 + 22, CTOR)
    for vt, expected_name in ((BASE_VTABLES, b".?AVC_BaseEntity@@\0"),
                              (WORLD_VTABLES, b".?AVC_World@@\0")):
        for i, pointer in enumerate(vt):
            col = u32(pointer - 4)
            need(u32(col + 4) == 4 * i, "RTTI subobject offset mismatch")
            need(read(u32(col + 12) + 8, len(expected_name)) == expected_name,
                 "RTTI type mismatch")
    # Constructor installs base vtables before the caller overwrites them.
    for off, vt in zip((31, 38, 45, 52), BASE_VTABLES):
        need(u32(CTOR + off) == vt, "constructor base vtable mismatch")
    for va, glob in ((0x24085750, 0x2432B254), (0x24129480, 0x2436115C)):
        need(read(va, 11) == b"\xC7\x05" + struct.pack("<I", glob) + b"\0" * 4 + b"\xC3",
             "global reset helper mismatch")
    call(0x240FCB1D, 0x24085750)
    call(0x240FCB22, 0x24129480)
    call(0x240FCB33, 0x240C8FE0)
    need(read(0x240FCB13, 10) == bytes.fromhex("c7868004000000000000"),
         "world field clear mismatch")
    print("PASS original C_BaseEntity constructor, 12 canonical callers + world, RTTI and world prelude")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, required=True)
    verify(parser.parse_args().client)
