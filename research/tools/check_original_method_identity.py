#!/usr/bin/env python3
"""Check original DLL method identity before claiming or implementing a recovery.

Metadata-only: no commercial binaries are needed, read, or emitted.
The catalogue is deliberately incomplete; a new-to-catalog result is
not proof that no earlier research note or source already covers a method.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

CATALOG = Path(__file__).resolve().parents[1] / "catalog" / "original_dll_verified_methods.json"
HASHES = {
    "client": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}

class CatalogError(ValueError):
    pass

def digest(value):
    if not isinstance(value, str) or not re.fullmatch(r"[a-fA-F0-9]{64}", value):
        raise CatalogError("invalid SHA-256")
    return value.lower()

def address(value):
    if not isinstance(value, str) or not re.fullmatch(r"0[xX][a-fA-F0-9]{8}", value):
        raise CatalogError("expected 32-bit original VA in 0x12345678 form")
    return int(value, 16)

def identity(row, hashes):
    dll = row.get("dll")
    if dll not in hashes:
        raise CatalogError("unknown original DLL")
    va = address(row.get("entry_va"))
    length = row.get("original_code_length")
    if type(length) is not int or length <= 0 or va + length > 2**32:
        raise CatalogError("invalid original code extent")
    return (hashes[dll], va), (length, digest(row.get("original_code_sha256")))

def index_catalog(doc):
    if doc.get("schema_version") != 1 or doc.get("dll_sha256") != HASHES:
        raise CatalogError("unexpected schema or original DLL identity")
    rows = doc.get("methods")
    if not isinstance(rows, list):
        raise CatalogError("missing methods array")
    index = {}
    for row in rows:
        if not isinstance(row, dict):
            raise CatalogError("invalid method row")
        key, fp = identity(row, HASHES)
        if key in index:
            previous_fp, previous = index[key]
            kind = "duplicate" if previous_fp == fp else "conflicting fingerprint"
            raise CatalogError(f"{kind}: {row['dll']} {row['entry_va']} "
                               f"({previous.get('label')} / {row.get('label')})")
        index[key] = (fp, row)
    return index

def classify(index, row):
    key, fp = identity(row, HASHES)
    if key in index:
        old_fp, old = index[key]
        return {
            "status": "EXISTING_METHOD" if old_fp == fp else "CONFLICTING_FINGERPRINT",
            "canonical": old,
            "instruction": "Extend existing evidence/implementation; do not create a second method.",
        }
    # The same fingerprint at another address is legitimate (e.g. small
    # identical stubs). Overlapping extents need review, not auto-merging.
    length, code_hash = fp
    overlaps = []
    identical_code = []
    for (dll_hash, va), ((n, sha), old) in index.items():
        if dll_hash != key[0]:
            continue
        if key[1] < va+n and va < key[1]+length:
            overlaps.append(old["entry_va"])
        if n == length and sha == code_hash:
            identical_code.append(old["entry_va"])
    return {
        "status": "NEW_TO_CATALOG_REVIEW_REQUIRED",
        "overlapping_regions": overlaps,
        "identical_code_other_addresses": identical_code,
        "instruction": "Check earlier research and source before adding a method.",
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=CATALOG)
    parser.add_argument("--dll", choices=tuple(HASHES))
    parser.add_argument("--va")
    parser.add_argument("--length", type=int)
    parser.add_argument("--sha256")
    args = parser.parse_args()
    index = index_catalog(json.loads(args.catalog.read_text(encoding="utf-8")))
    fields = (args.dll, args.va, args.length, args.sha256)
    if not any(x is not None for x in fields):
        print(json.dumps({"status": "CATALOG_IDENTITIES_UNIQUE", "methods": len(index)}))
        return 0
    if not all(x is not None for x in fields):
        parser.error("--dll, --va, --length and --sha256 must be supplied together")
    result = classify(index, {"dll": args.dll, "entry_va": args.va,
        "original_code_length": args.length, "original_code_sha256": args.sha256})
    print(json.dumps(result, indent=2))
    return {"EXISTING_METHOD": 2, "CONFLICTING_FINGERPRINT": 3}.get(result["status"], 0)

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (CatalogError, OSError, json.JSONDecodeError) as error:
        raise SystemExit("REFUSED: "+str(error))
