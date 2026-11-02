#!/usr/bin/env python3
"""Read-only ARM64 ELF SourceVR module-load evidence finder.

No proprietary binary content is included in reports. Pure Python 3 stdlib.
Static xrefs are NOT proof of runtime loading or interface ABI compatibility.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import struct
import sys
from dataclasses import dataclass

WATCH = (
    "server.so", "client.dll", "GAMEBIN", "ServerGameDLL%03d",
    "VClient017", "Could not get client.dll interface from library client",
    "Could not get IServerGameDLL interface from library %s",
    "LoadLibrary: pModule: %s, preloaded SONAME: %s\n",
    "APP_LIB_PATH", "SOURCEVR_DEV_PAYLOAD_PATH",
)


@dataclass(frozen=True)
class Section:
    name: str
    address: int
    offset: int
    size: int
    flags: int


def sections(data: bytes) -> dict[str, Section]:
    if len(data) < 64 or data[:4] != b"\x7fELF" or data[4:6] != b"\x02\x01":
        raise ValueError("expected little-endian ELF64")
    if struct.unpack_from("<H", data, 18)[0] != 183:
        raise ValueError("expected AArch64 (e_machine=183)")
    shoff = struct.unpack_from("<Q", data, 40)[0]
    entsize, count, names_idx = struct.unpack_from("<HHH", data, 58)
    if entsize < 64 or not count or names_idx >= count or shoff + entsize * count > len(data):
        raise ValueError("invalid ELF section table")
    entries = [
        struct.unpack_from("<IIQQQQIIQQ", data, shoff + n * entsize)
        for n in range(count)
    ]
    names = entries[names_idx]
    start, end = names[4], names[4] + names[5]
    if end > len(data):
        raise ValueError("invalid ELF section names")
    name_bytes = data[start:end]
    result = {}
    for e in entries:
        name_idx, _, flags, vma, off, size, *_ = e
        if name_idx >= len(name_bytes) or off + size > len(data):
            continue
        stop = name_bytes.find(b"\0", name_idx)
        if stop < 0:
            continue
        name = name_bytes[name_idx:stop].decode("utf-8", "replace")
        result[name] = Section(name, vma, off, size, flags)
    if ".text" not in result or ".rodata" not in result:
        raise ValueError("missing .text/.rodata")
    return result


def sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def adrp(word: int, pc: int) -> tuple[int, int] | None:
    if word & 0x9f000000 != 0x90000000:
        return None
    immlo = (word >> 29) & 3
    immhi = (word >> 5) & 0x7ffff
    return (word & 31, (pc & ~4095) + (sign_extend((immhi << 2) | immlo, 21) << 12))


def add_immediate(word: int) -> tuple[int, int, int] | None:
    if (word & 0xff000000) not in (0x91000000, 0x91400000):
        return None
    shift = 12 if (word >> 22) & 1 else 0
    return (word & 31, (word >> 5) & 31, ((word >> 10) & 4095) << shift)


def branch_target(word: int, pc: int) -> int | None:
    if word & 0xfc000000 != 0x94000000:
        return None
    return pc + (sign_extend(word & 0x3ffffff, 26) << 2)


def const_true_stub(data: bytes, text: Section, address: int) -> bool:
    off = text.offset + (address - text.address)
    if off < text.offset or off + 8 > text.offset + text.size:
        return False
    a, b = struct.unpack_from("<II", data, off)
    return a == 0x52800020 and b == 0xd65f03c0


def find_targets(data: bytes, rodata: Section) -> dict[int, str]:
    raw = data[rodata.offset:rodata.offset + rodata.size]
    targets = {}
    for string in WATCH:
        needle = string.encode() + b"\0"
        pos = 0
        while True:
            pos = raw.find(needle, pos)
            if pos < 0:
                break
            if not pos or raw[pos - 1] == 0:
                targets[rodata.address + pos] = string
            pos += 1
    return targets


def examine(data: bytes, label: str) -> dict:
    ss = sections(data)
    text, ro = ss[".text"], ss[".rodata"]
    pages: dict[int, dict[int, str]] = {}
    for addr, string in find_targets(data, ro).items():
        pages.setdefault(addr & ~4095, {})[addr] = string
    found: dict[str, list] = {key: [] for key in WATCH}
    for off in range(text.offset, text.offset + text.size - 4, 4):
        word = struct.unpack_from("<I", data, off)[0]
        pc = text.address + off - text.offset
        ar = adrp(word, pc)
        if ar is None or ar[1] not in pages:
            continue
        register, page = ar
        for ahead in range(1, 9):
            j = off + ahead * 4
            if j + 4 > text.offset + text.size:
                break
            add = add_immediate(struct.unpack_from("<I", data, j)[0])
            if add is None or add[0] != register or add[1] != register:
                continue
            target = page + add[2]
            string = pages[page].get(target)
            if string is None:
                continue
            true_calls = []
            for q in range(max(text.offset, off - 40),
                           min(off + 120, text.offset + text.size - 4), 4):
                callpc = text.address + q - text.offset
                callee = branch_target(struct.unpack_from("<I", data, q)[0], callpc)
                if callee is not None and const_true_stub(data, text, callee):
                    true_calls.append(f"0x{callpc:x} -> 0x{callee:x}")
            entry = {
                "adrp_at": f"0x{pc:x}",
                "add_at": f"0x{text.address + j - text.offset:x}",
                "string_va": f"0x{target:x}",
                "nearby_const_true_calls": sorted(set(true_calls))
            }
            if entry not in found[string]:
                found[string].append(entry)
    return {
        "profile": label,
        "sha256": hashlib.sha256(data).hexdigest(),
        "text_va": f"0x{text.address:x}",
        "rodata_va": f"0x{ro.address:x}",
        "string_references": {key: refs for key, refs in found.items() if refs},
        "candidate_missing": [key for key in WATCH if not found[key]],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("engine", nargs="+", type=pathlib.Path,
                   help="Path to private ARM64 SourceVR libengine.so")
    p.add_argument("--output", type=pathlib.Path)
    args = p.parse_args()
    results = []
    for path in args.engine:
        try:
            results.append(examine(path.read_bytes(), path.parent.name))
        except (OSError, ValueError) as error:
            print(f"{path}: {error}", file=sys.stderr)
            return 1
    output = {
        "schema": 1,
        "method": "ELF rodata strings + ARM64 ADRP/ADD xrefs and local BL stubs",
        "limits": [
            "A static reference does not prove the branch executes.",
            "Factory version strings do not establish C++ vtable/ABI compatibility.",
            "A nearby always-true stub is not proof there are no other loader gates.",
            "No SourceVR module was actually loaded during this test.",
        ],
        "engines": results,
    }
    serialized = json.dumps(output, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(serialized, encoding="utf-8")
        print(f"Wrote {args.output}")
    else:
        print(serialized)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
