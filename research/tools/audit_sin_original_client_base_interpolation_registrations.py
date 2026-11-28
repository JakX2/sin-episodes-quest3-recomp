#!/usr/bin/env python3
"""Private source audit: original base entity interpolation registrations."""
import argparse
import hashlib
import struct
from pathlib import Path
from audit_sin_original_client_base_entity_constructor import SOURCE_SHA, need


def verify(path):
    data = path.read_bytes()
    need(hashlib.sha256(data).hexdigest() == SOURCE_SHA, "wrong original client SHA")
    def read(va, size):
        off = va - 0x24000000
        need(0 <= off <= len(data) - size, "source read outside pinned image")
        return data[off:off + size]
    def u32(va):
        return struct.unpack("<I", read(va, 4))[0]
    for va, length, digest in (
        (0x240CCF9A, 79, "d0b8c0354e3582ee058fa2ee2ac14f83e075c7a420c6aa80ce211ab16e494405"),
        (0x24061220, 121, "3e3868e86685fa86f4bcbb796a35de8b8277cddb8b69e17f203a673757edb948"),
        (0x24060870, 115, "2dc1e61743a119f0043dfc270e07363ff9d3d7946a27e68855145a1fca9407a9"),
    ):
        need(hashlib.sha256(read(va, length)).hexdigest() == digest, f"source SHA mismatch {va:#x}")
    for va, target in ((0x240CCFBE, 0x24061220), (0x240CCFE4, 0x24061220),
                       (0x24061239, 0x24060870)):
        b = read(va, 5)
        need(b[0] == 0xE8 and va + 5 + struct.unpack_from("<i", b, 1)[0] == target,
             f"source call mismatch {va:#x}")
    for va, expected in ((0x240CCF9C, 0x220), (0x240CCEF8, 0x22C),
                         (0x240CCFCC, 0x25C), (0x240CCF3D, 0x268)):
        need(u32(va) == expected, "source owner-relative field mismatch")
    for vt, name in ((0x24278AA4, b".?AV?$CInterpolatedVar@VVector@@@@\0"),
                     (0x24289418, b".?AV?$CInterpolatedVar@VQAngle@@@@\0"),
                     (0x2429806C, b".?AVCCollisionProperty@@\0")):
        col = u32(vt - 4)
        need(read(u32(col + 12) + 8, len(name)) == name, "source RTTI mismatch")
    need(read(0x24061296, 3) == b"\xC2\x08\x00", "insert ABI return mismatch")
    need(read(0x240608A1, 7) == bytes.fromhex("c7460403000000"), "initial capacity mismatch")
    print("PASS original base interpolation records, 12-byte insertion and zero-grow-size capacity")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, required=True)
    verify(parser.parse_args().client)
