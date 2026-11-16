#!/usr/bin/env python3
"""Fail-closed SHA256 and PE32 authentication of BOTH owner original gameplay DLLs.

Outputs metadata only; never copies game bytes, disassembly or assets.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

EXPECTED = {
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}


def verify(path, expected):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
        if digest.hexdigest() != expected:
            raise ValueError(f"{path.name}: original SHA256 mismatch")
        stream.seek(0)
        dos = stream.read(64)
        if len(dos) != 64 or dos[:2] != b"MZ":
            raise ValueError(f"{path.name}: invalid DOS header")
        pe_offset = struct.unpack_from("<I", dos, 0x3c)[0]
        if pe_offset < 64 or pe_offset > 1024 * 1024:
            raise ValueError(f"{path.name}: unreasonable PE header offset")
        stream.seek(pe_offset)
        coff = stream.read(24)
        if len(coff) != 24 or coff[:4] != bytes((80, 69, 0, 0)):
            raise ValueError(f"{path.name}: invalid PE signature")
        machine, sections = struct.unpack_from("<HH", coff, 4)
        optional_size, characteristics = struct.unpack_from("<HH", coff, 20)
        optional = stream.read(optional_size)
        if (machine != 0x14c or not 1 <= sections <= 96
                or not characteristics & 0x2000 or optional_size < 96
                or len(optional) != optional_size
                or struct.unpack_from("<H", optional)[0] != 0x10b):
            raise ValueError(f"{path.name}: not a valid PE32 i386 DLL")
    return {"sha256": digest.hexdigest(), "size": path.stat().st_size,
            "machine": "I386", "format": "PE32", "sections": sections}


def authenticate(directory):
    # All-or-nothing: do not write success metadata for one valid DLL.
    modules = {name: verify(directory / name, digest)
               for name, digest in EXPECTED.items()}
    return {"schema": 1, "authenticated": True, "modules": modules,
            "evidence_tier": "original_full_file_sha256",
            "new_whole_body_sha_verified_functions": 0,
            "new_independently_load_tested_original_functions": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = authenticate(args.original_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Both original PE32 DLLs authenticated; metadata report written")


if __name__ == "__main__":
    main()
