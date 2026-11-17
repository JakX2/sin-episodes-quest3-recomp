#!/usr/bin/env python3
"""Fail-closed, metadata-only summary of private Ghidra C-like decompilation.

Original pseudocode stays in runner-only scratch, never in the status artifact.
This tool does not verify original implementations or call into the engine.
"""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

PRIORITY = frozenset({
    "ServerGameDLL003", "ServerGameClients003", "ServerGameEnts001",
    "VClient011", "VClientPrediction001", "VClientEntityList003",
    "GameMovement001", "GameClientExports001",
})
MODULES = ("client.dll", "server.dll")
OUTCOMES = frozenset({
    "decompiled_candidate", "missing_inferred_function", "not_executable",
    "empty_decompilation", "decompilation_failed",
})
MAX_PER_MODULE = 8


def parse_pe32_address(raw):
    if not isinstance(raw, str):
        raise ValueError("invalid PE32 address")
    digits = raw.strip()
    if digits.lower().startswith("0x"):
        digits = digits[2:]
    if not 1 <= len(digits) <= 8 or any(c not in "0123456789abcdefABCDEF" for c in digits):
        raise ValueError("invalid PE32 address")
    address = int(digits, 16)
    if address == 0:
        raise ValueError("invalid PE32 address: null")
    return address


def read_csv(path, expected_fields):
    with path.open(newline="", encoding="utf-8-sig") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != expected_fields:
            raise ValueError(f"{path.name}: unexpected CSV header")
        result = list(reader)
    if any(None in item or any(value is None for value in item.values()) for item in result):
        raise ValueError(f"{path.name}: malformed CSV row")
    return result


def summarize(seed_dir, status_dir):
    result = {
        "schema": 1,
        "evidence_tier": "ghidra_candidate_pseudocode_only",
        "new_original_whole_body_sha_verified_methods": 0,
        "new_original_engine_load_tested_methods": 0,
        "original_game_pseudocode_embedded": False,
        "modules": {},
    }
    for module in MODULES:
        seeds = read_csv(seed_dir / f"{module}_factory_seeds.csv",
                         ["entry", "interface", "source"])
        statuses = read_csv(status_dir / f"{module}_targeted_decomp_status.csv",
                            ["interface", "entry", "outcome", "inferred_function_bytes"])
        allowed = {}
        for record in seeds:
            name = record["interface"]
            if record["source"] != "reviewed_original_static_registration":
                raise ValueError(f"{module}: unreviewed source")
            if not name or not name.replace("_", "").isalnum():
                raise ValueError(f"{module}: invalid interface")
            va = parse_pe32_address(record["entry"])
            if name in PRIORITY:
                if name in allowed:
                    raise ValueError(f"{module}: duplicate priority seed")
                allowed[name] = va
        if not allowed or len(allowed) > MAX_PER_MODULE:
            raise ValueError(f"{module}: no valid bounded priority seed set")
        if len(statuses) != len(allowed):
            raise ValueError(f"{module}: incomplete or extra decompilation statuses")
        seen = set()
        by_interface = {}
        for record in statuses:
            name = record["interface"]
            if name not in allowed or name in seen:
                raise ValueError(f"{module}: unexpected or duplicate decompilation interface")
            seen.add(name)
            if parse_pe32_address(record["entry"]) != allowed[name]:
                raise ValueError(f"{module}: factory VA disagrees with reviewed seed")
            outcome = record["outcome"]
            if outcome not in OUTCOMES:
                raise ValueError(f"{module}: unsafe or unknown decompilation outcome")
            extent = record["inferred_function_bytes"]
            if extent and (not extent.isascii() or not extent.isdecimal()
                           or int(extent) <= 0 or int(extent) > 50_000_000):
                raise ValueError(f"{module}: invalid inferred function length")
            if outcome == "decompiled_candidate" and not extent:
                raise ValueError(f"{module}: decompiled result lacks inferred function extent")
            by_interface[name] = {
                "entry_va": f"0x{allowed[name]:08X}",
                "outcome": outcome,
                "inferred_function_bytes": int(extent) if extent else None,
                "original_method_verified": False,
            }
        completed = sum(x["outcome"] == "decompiled_candidate" for x in by_interface.values())
        result["modules"][module] = {
            "selected": len(by_interface),
            "candidate_c_decompiled": completed,
            "unavailable": len(by_interface) - completed,
            "interfaces": dict(sorted(by_interface.items())),
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-dir", required=True, type=Path)
    parser.add_argument("--status-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    result = summarize(args.seed_dir, args.status_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        key: {"selected": value["selected"], "candidate_c_decompiled": value["candidate_c_decompiled"]}
        for key, value in result["modules"].items()
    }, sort_keys=True))


if __name__ == "__main__":
    main()
