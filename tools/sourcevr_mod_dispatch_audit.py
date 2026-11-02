#!/usr/bin/env python3
"""Version-pinned read-only SourceVR APP_MOD_LIB/APP_LIB_PATH routing audit.

Use on the owner's private, extracted native engine/launcher and .load files.
No game binaries are embedded or modified. Unknown revisions fail closed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import re
import struct
import sys

ENGINES = {
    "hl2": "265c675d6f0d7a57a841bf6a0daf1bb7708a3c4c1216d800299f6d448ce75830",
    "entropyzero": "81c6041998d4dcace9739bae16ebfcff62d3a66f8079de59872a103c7f5f7cee",
}
LAUNCHERS = {
    "hl2": "84a9b779eabc5eef2e8ad33a217f0669d005990f085402b9a3766df66a64f18b",
    "entropyzero": "b06a4003ff9daa6d4390075406e1bccd52dd94df2bc4247fb70b8086f5ce31f7",
}

def sha256(b):
    return hashlib.sha256(b).hexdigest()

def signed(n, bits):
    return (n ^ (1 << (bits - 1))) - (1 << (bits - 1))

def adrp(word, pc):
    if word & 0x9f000000 != 0x90000000:
        return None
    imm = signed((((word >> 5) & 0x7ffff) << 2) | ((word >> 29) & 3), 21)
    return word & 31, (pc & ~4095) + (imm << 12)

def add(word):
    if word & 0xff000000 not in (0x91000000, 0x91400000):
        return None
    return word & 31, (word >> 5) & 31, ((word >> 10) & 4095) << (12 if word & (1 << 22) else 0)

def bl(word, pc):
    return pc + (signed(word & 0x3ffffff, 26) << 2) if word & 0xfc000000 == 0x94000000 else None

def cbnz(word, pc):
    return pc + (signed((word >> 5) & 0x7ffff, 19) << 2) if word & 0x7f000000 == 0x35000000 else None

class ELF64:
    def __init__(self, data):
        if len(data) < 64 or data[:6] != b"\x7fELF\x02\x01":
            raise ValueError("expected little-endian ELF64")
        if struct.unpack_from("<H", data, 18)[0] != 183:
            raise ValueError("expected AArch64 ELF")
        self.data = data
        off = struct.unpack_from("<Q", data, 40)[0]
        size, count, names_index = struct.unpack_from("<HHH", data, 58)
        if size < 64 or not count or names_index >= count or off + count * size > len(data):
            raise ValueError("invalid ELF section table")
        rows = [struct.unpack_from("<IIQQQQIIQQ", data, off + i * size) for i in range(count)]
        names = rows[names_index]
        if names[4] + names[5] > len(data):
            raise ValueError("invalid ELF section names")
        raw_names = data[names[4]:names[4] + names[5]]
        self.sections = {}
        for item in rows:
            idx, _, _, addr, offset, length, *_ = item
            if idx >= len(raw_names) or offset + length > len(data):
                continue
            end = raw_names.find(b"\0", idx)
            if end >= 0:
                self.sections[raw_names[idx:end].decode("ascii", "replace")] = (addr, offset, length)
        if ".text" not in self.sections or ".rodata" not in self.sections:
            raise ValueError("expected .text and .rodata")

    def read(self, va, length):
        for addr, off, size in self.sections.values():
            if addr <= va and va + length <= addr + size:
                return self.data[off + va - addr:off + va - addr + length]
        raise ValueError(f"unmapped ELF virtual address 0x{va:x}")

    def instruction(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]

    def reference(self, pc1, pc2):
        a = adrp(self.instruction(pc1), pc1)
        b = add(self.instruction(pc2))
        if a is None or b is None or a[0] != b[0] or a[0] != b[1]:
            raise ValueError(f"expected ADRP/ADD at 0x{pc1:x}/0x{pc2:x}")
        address = a[1] + b[2]
        raw = bytearray()
        for i in range(120):
            c = self.read(address + i, 1)[0]
            if c == 0:
                return raw.decode("utf-8", "replace")
            raw.append(c)
        raise ValueError("unterminated ELF string")

def inspect_engine(data):
    e = ELF64(data)
    if e.reference(0x53f3c0, 0x53f3c4) != "APP_LIB_PATH":
        raise ValueError("APP_LIB_PATH lookup changed")
    if e.reference(0x53f3d0, 0x53f3d4) != "APP_MOD_LIB":
        raise ValueError("APP_MOD_LIB lookup changed")
    expected = {
        0x53f3f8: 0x53f1a0,  # candidate path construction / stat
        0x53f408: 0x53f0c0,  # module dlopen wrapper
        0x53f438: 0x53f1a0,  # APP_LIB_PATH candidate
    }
    for callsite, target in expected.items():
        if bl(e.instruction(callsite), callsite) != target:
            raise ValueError(f"loader call 0x{callsite:x} changed")
    if cbnz(e.instruction(0x53f40c), 0x53f40c) != 0x53f424:
        raise ValueError("APP_MOD_LIB successful-load branch changed")
    return {
        "env_lookups": {"APP_LIB_PATH": "0x53f3c0", "APP_MOD_LIB": "0x53f3d0"},
        "mod_file_candidate_and_open": ["0x53f3f8", "0x53f408"],
        "mod_load_failure_is_fatal": "0x53f41c",
        "conditional_mod_handle_return": "0x53f454..0x53f45c",
        "caveat": "An APP_LIB_PATH miss goes to further SONAME fallback; mod handle is not an unconditional winner.",
    }

def inspect_launcher(data):
    e = ELF64(data)
    for start, value in ((0x37308, "APP_LIB_PATH"), (0x37368, "APP_LIB_PATH"),
                         (0x3737c, "APP_MOD_LIB")):
        if e.reference(start, start + 4) != value:
            raise ValueError(f"launcher environment initialization changed at 0x{start:x}")
    if cbnz(e.instruction(0x37314), 0x37314) != 0x37390:
        raise ValueError("launcher environment default skip branch changed")
    if bl(e.instruction(0x37378), 0x37378) != bl(e.instruction(0x3738c), 0x3738c):
        raise ValueError("expected both defaults to call setenv")
    return {
        "environment_initialization": "0x37308..0x37390",
        "condition": "Only if APP_LIB_PATH is absent",
        "effect": "Set APP_LIB_PATH and APP_MOD_LIB to launcher directory",
    }

def parse_load(text, profile):
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if not lines:
        raise ValueError("empty .load")
    names = []
    for line in lines:
        if not line.startswith("sourcevr/v1/native/blobs/"):
            raise ValueError("unrecognized native blob path")
        parts = line.split("/")
        if len(parts) != 6 or not re.fullmatch(r"[0-9a-f]{64}", parts[4]):
            raise ValueError("invalid blob hash/name")
        names.append(parts[5])
    required = ("libclient.so", "libengine.so", "libserver.so", "liblauncher.so")
    if len(set(names)) != len(names) or any(name not in names for name in required):
        raise ValueError("native graph has duplicates or missing required modules")
    if names[-1] != "liblauncher.so":
        raise ValueError("native graph does not end with liblauncher.so")
    return {
        "staged_count": len(names),
        "staged_positions": {name: names.index(name) for name in required},
        "client_before_engine": names.index("libclient.so") < names.index("libengine.so"),
        "server_before_engine": names.index("libserver.so") < names.index("libengine.so"),
        "client_server_before_launcher": all(names.index(n) < names.index("liblauncher.so") for n in required[:-1]),
        "caveat": "Staging order does not prove actual Android linker handle selection.",
    }

def audit(profile, engine, launcher, manifest, dex=None):
    ehash, lhash = sha256(engine), sha256(launcher)
    if ehash != ENGINES[profile] or lhash != LAUNCHERS[profile]:
        raise ValueError(f"unsupported binary revisions: engine={ehash} launcher={lhash}")
    return {
        "schema": 1, "profile": profile,
        "engine_sha256": ehash, "launcher_sha256": lhash,
        "engine_dispatch": inspect_engine(engine), "launcher_defaults": inspect_launcher(launcher),
        "native_load_graph": parse_load(manifest, profile),
        "dex_string_presence": None if dex is None else {
            "APP_LIB_PATH": b"APP_LIB_PATH" in dex, "APP_MOD_LIB": b"APP_MOD_LIB" in dex,
            "caveat": "String absence does not prove absence of indirect Java control.",
        },
        "unverified": ["actual app environment", "custom game library selection", "preloaded SONAME behavior", "Source factory ABI"],
    }

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--profile", choices=sorted(ENGINES), required=True)
    ap.add_argument("--engine", type=pathlib.Path, required=True)
    ap.add_argument("--launcher", type=pathlib.Path, required=True)
    ap.add_argument("--load", type=pathlib.Path, required=True)
    ap.add_argument("--dex", type=pathlib.Path)
    ap.add_argument("--output", type=pathlib.Path)
    args = ap.parse_args()
    try:
        result = audit(args.profile, args.engine.read_bytes(), args.launcher.read_bytes(),
                       args.load.read_text(encoding="utf-8"), args.dex.read_bytes() if args.dex else None)
    except (OSError, ValueError) as exc:
        print(f"Audit failed: {exc}", file=sys.stderr)
        return 1
    output = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8")
        print(f"Wrote {args.output}")
    else:
        print(output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
