#!/usr/bin/env python3
"""Bind a Phase 647 startup trace to locally supplied Windows PE32 files.

This checks actual bytes, not labels in JSONL. It does NOT establish that a
retail process ran, that a DLL was loaded, or that startup callbacks executed.
Never upload original binaries, file paths, or private hashes to CI artifacts.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

FIELDS = (("retail_executable_sha256", "exe"), ("client_sha256", "client"),
          ("server_sha256", "server"))


def inspect_pe(path):
    data = Path(path).read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError("not an MZ executable")
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    if offset > len(data) - 24 or data[offset:offset + 4] != bytes((80, 69, 0, 0)):
        raise ValueError("invalid PE signature/offset")
    machine, sections, _, _, _, optional_size, characteristics = struct.unpack_from(
        "<HHIIIHH", data, offset + 4)
    optional = offset + 24
    if optional_size < 2 or optional_size > len(data) - optional:
        raise ValueError("invalid optional header length")
    magic = struct.unpack_from("<H", data, optional)[0]
    if machine != 0x14c or magic != 0x10b:
        raise ValueError("not Windows x86 PE32")
    if sections == 0:
        raise ValueError("PE has no sections")
    return {"sha256": hashlib.sha256(data).hexdigest(),
            "dll": bool(characteristics & 0x2000), "bytes": len(data)}


def verify(trace, paths):
    errors = []
    if not isinstance(trace, list) or not trace or not isinstance(trace[0], dict):
        return ["missing first trace event"]
    first = trace[0]
    for field, kind in FIELDS:
        try:
            meta = inspect_pe(paths[kind])
        except (OSError, ValueError, struct.error) as exc:
            errors.append(f"{kind}: cannot verify local PE32: {exc}")
            continue
        if meta["dll"] != (kind != "exe"):
            errors.append(f"{kind}: unexpected PE executable/DLL flag")
        if first.get(field) != meta["sha256"]:
            errors.append(f"{kind}: trace SHA-256 differs from local PE32 bytes")
    return errors


def self_test():
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        def fixture(dll):
            data = bytearray(512)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 0x3c, 0x80)
            data[0x80:0x84] = bytes((80, 69, 0, 0))
            struct.pack_into("<HHIIIHH", data, 0x84, 0x14c, 1, 0, 0, 0,
                             0xe0, 0x2000 if dll else 0x0002)
            struct.pack_into("<H", data, 0x98, 0x10b)
            return bytes(data)
        paths = {}
        trace = [{}]
        for field, kind in FIELDS:
            path = Path(tmp) / kind
            path.write_bytes(fixture(kind != "exe"))
            paths[kind] = path
            trace[0][field] = hashlib.sha256(path.read_bytes()).hexdigest()
        assert not verify(trace, paths)
        trace[0]["server_sha256"] = "a" * 64
        assert any("server: trace SHA" in x for x in verify(trace, paths))
        trace[0]["server_sha256"] = hashlib.sha256(paths["server"].read_bytes()).hexdigest()
        paths["client"].write_bytes(b"not a PE")
        assert any("client: cannot verify" in x for x in verify(trace, paths))
    print("synthetic PE32 evidence-binding tests passed; no retail execution claim")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--client", type=Path)
    parser.add_argument("--server", type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if any(x is None for x in (args.trace, args.exe, args.client, args.server)):
        parser.error("--trace, --exe, --client and --server are all required")
    try:
        events = [json.loads(x) for x in args.trace.read_text(encoding="utf-8").splitlines()
                  if x.strip()]
    except (OSError, ValueError) as exc:
        parser.exit(2, f"invalid trace: {exc}\\n")
    errors = verify(events, {"exe": args.exe, "client": args.client,
                             "server": args.server})
    if errors:
        for error in errors:
            print("FAIL:", error)
        return 1
    print("LOCAL PE32 HASH BINDING PASS; no proof of retail execution or callbacks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
