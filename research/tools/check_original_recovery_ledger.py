#!/usr/bin/env python3
"""Check recovery ledger identities without requiring private original binaries.

Use --lookup before starting a new recovery. A miss means absent from this
incomplete ledger, NOT proof that the method has never been recovered.
"""
from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/catalog/original_dll_verified_methods.json"
DLL_SHA = {
    "client": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}
VA = re.compile(r"0x[0-9a-f]{8}", re.I)
SHA = re.compile(r"[0-9a-f]{64}")

def index(ledger):
    if ledger.get("schema_version") != 1 or ledger.get("dll_sha256") != DLL_SHA:
        raise ValueError("unexpected recovery ledger identity")
    rows = ledger.get("methods")
    if not isinstance(rows, list):
        raise ValueError("methods must be a list")
    result = {}
    for row in rows:
        role = row.get("dll")
        address = row.get("entry_va")
        size = row.get("original_code_length")
        digest = row.get("original_code_sha256")
        if role not in DLL_SHA or not isinstance(address, str) or not VA.fullmatch(address):
            raise ValueError(f"invalid method identity: {role!r} {address!r}")
        if type(size) is not int or size < 1:
            raise ValueError(f"invalid method extent: {address}")
        if not isinstance(digest, str) or not SHA.fullmatch(digest):
            raise ValueError(f"invalid original region digest: {address}")
        key = (DLL_SHA[role], int(address, 16))
        if key in result:
            prior = result[key]
            raise ValueError(f"duplicate method VA {role}:{address}; "
                             f"prior evidence={prior.get('evidence_path')!r}, "
                             f"new evidence={row.get('evidence_path')!r}")
        result[key] = row
    return result

def lookup(entries, role, address, digest=None):
    if role not in DLL_SHA or not VA.fullmatch(address):
        raise ValueError("lookup requires client/server and an eight-digit hex VA")
    row = entries.get((DLL_SHA[role], int(address, 16)))
    if row is not None and digest is not None and row["original_code_sha256"] != digest:
        raise ValueError("known address has a different region digest; resolve evidence conflict")
    return row

def self_test():
    row = {"dll": "server", "entry_va": "0x2237E8C0",
           "original_code_length": 56, "original_code_sha256": "a" * 64,
           "evidence_path": "one.md"}
    ledger = {"schema_version": 1, "dll_sha256": dict(DLL_SHA), "methods": [row]}
    entries = index(ledger)
    assert lookup(entries, "server", "0x2237e8c0", "a" * 64) == row
    assert lookup(entries, "client", row["entry_va"]) is None
    def reject(action):
        try:
            action()
        except ValueError:
            return
        raise AssertionError("invalid or duplicate identity admitted")
    reject(lambda: lookup(entries, "server", row["entry_va"], "b" * 64))
    for other in (dict(row, entry_va="0x2237e8c0"),
                  dict(row, original_code_sha256="b" * 64),
                  dict(row, original_code_length=57)):
        reject(lambda: index(dict(ledger, methods=[row, other])))
    reject(lambda: index(dict(ledger, methods=[dict(row, original_code_length=True)])))
    reject(lambda: index(dict(ledger, methods=[dict(row, original_code_sha256="bad")])))
    reject(lambda: index(dict(ledger, dll_sha256={})))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--lookup", nargs=2, metavar=("DLL", "VA"))
    parser.add_argument("--sha256", help="optional expected original method digest")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    entries = index(json.loads(args.ledger.read_text(encoding="utf-8")))
    if args.lookup:
        result = lookup(entries, *args.lookup, args.sha256)
        print(json.dumps({"status": "KNOWN_METHOD" if result else "NOT_INDEXED_CHECK_OTHER_EVIDENCE",
                          "method": result}, indent=2))
    else:
        print(json.dumps({"status": "RECOVERY_LEDGER_IDENTITIES_VALID",
                          "distinct_indexed_methods": len(entries),
                          "by_dll": dict(Counter(x["dll"] for x in entries.values())),
                          "limits": "Incomplete ledger; not a decompilation coverage percentage."}))

if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, TypeError, KeyError) as error:
        raise SystemExit("REFUSED: " + str(error))
