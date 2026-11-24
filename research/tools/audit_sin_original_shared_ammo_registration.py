#!/usr/bin/env python3
"""Owner-only original registration argument comparison. No raw code emitted."""
import argparse
import hashlib
import struct
from pathlib import Path

BUILDS = (
    ("server", 0x22000000, 0x222D54E4, 0x222D57F6, 0x221E61D0,
     "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"),
    ("client", 0x24000000, 0x24212EF4, 0x24213206, 0x24089AB0,
     "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"),
)
EXPECTED = (
    ("MAG", 0x2, 4, 15, 0x40400000, 80, 0x462a66ba, 0x42800000, 0, 4, 8, ""),
    ("AR", 0x2, 4, 7, 0x40400000, 330, 0x44a6fe46, 0x42000000, 0, 4, 8, ""),
    ("BUCK", 0x40000002, 1, 6, 0x40000000, 30, 0x452395d1, 0x41c00000, 0, 4, 8, ""),
    ("SR", 0x80000002, 0, 20, 0x41200000, 30, 0x46262428, 0x42800000, 0, 4, 8, ""),
    ("ROCKET", 0x8, 0, 100, 0x42480000, 5, 0x0, 0x0, 0, 4, 8, ""),
    ("ARNADE", 0x8, 0, 100, 0x41c80000, 3, 0x0, 0x0, 0, 4, 8, "Player.PickupSecondary"),
    ("GRENADE", 0x8, 0, 100, 0x42480000, 5, 0x0, 0x0, 0, 4, 8, ""),
    ("FLARE", 0x8, 0, 0, 0x0, 30, 0x0, 0x0, 0, 4, 8, ""),
    ("DUAG", 0x400002, 2, 50, 0x41a00000, 8, 0x471fc04e, 0x43fa0000, 0, 4, 8, "Player.PickupSecondary"),
    ("FLAK", 0x2, 0, 10, 0x40800000, 10, 0x457f9a17, 0x0, 0, 4, 8, "Player.PickupSecondary"),
    ("CHAIN", 0x40000002, 4, 2, 0x40000000, 750, 0x4426fe46, 0x42000000, 0, 4, 8, ""),
    ("PULSE", 0x400, 0, 10, 0x41200000, 10, 0x457f9a17, 0x0, 0, 4, 8, ""),
    ("HELIGUN", 0x42, 4, 3, 0x40c00000, 320, 0x44a6fe46, 0x42800000, 3, 4, 8, ""),
    ("HELIMISL", 0x8, 0, 100, 0x42480000, 320, 0x0, 0x0, 0, 4, 8, ""),
    ("TURRET1", 0x2, 4, 2, 0x3f800000, 320, 0x4426fe46, 0x0, 0, 4, 8, ""),
    ("TURRET2", 0x2, 4, 4, 0x40000000, 320, 0x4426fe46, 0x0, 0, 4, 8, ""),
    ("TURRET3", 0x2, 4, 8, 0x40800000, 320, 0x4426fe46, 0x0, 0, 4, 8, ""),
)

def extract(path, spec):
    name, base, start, end, target, sha = spec
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != sha:
        raise ValueError(name + " original SHA256 mismatch")
    # SHA-pinned builds have identity file offset for these sections.
    i = start - base
    rows = []
    def string_at(address):
        if not address:
            return ""
        j = address - base
        if not 0 <= j < len(data):
            raise ValueError("bad string location")
        end = data.index(0, j, min(j + 64, len(data)))
        return data[j:end].decode("ascii")
    for row_index in range(17):
        args = []
        for _ in range(12):
            op = data[i]
            if op not in (0x68, 0x6A):
                raise ValueError("registration push mismatch")
            n = 4 if op == 0x68 else 1
            args.append(int.from_bytes(data[i+1:i+1+n], "little", signed=n == 1) & 0xffffffff)
            i += n + 1
        if data[i] != 0xB9:
            raise ValueError("registration object mismatch")
        i += 5
        if row_index == 0:
            if data[i:i+2] != b"\xc6\x05":
                raise ValueError("first registration guard mismatch")
            i += 7
        if data[i] != 0xE8 or base + i + 5 + struct.unpack_from("<i", data, i+1)[0] != target:
            raise ValueError("registration call mismatch")
        i += 5
        a = list(reversed(args))
        rows.append((string_at(a[0]), *a[1:11], string_at(a[11])))
    if i + base != end:
        raise ValueError("registration end mismatch")
    return tuple(rows)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", type=Path, required=True)
    p.add_argument("--client", type=Path, required=True)
    args = p.parse_args()
    s = extract(args.server, BUILDS[0])
    c = extract(args.client, BUILDS[1])
    if s != EXPECTED or c != EXPECTED:
        raise ValueError("ammo metadata mismatch")
    print("PASS original SHA-pinned client/server 17/17 registrations (12 args each)")
if __name__ == "__main__":
    main()
