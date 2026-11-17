#!/usr/bin/env python3
"""Sanitize private original-vtable Ghidra method decompilation status ONLY.

Never reads, hashes, copies, or uploads C pseudocode; only validates
reviewed original method VA, slot, status, and inferred candidate size.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
from pathlib import Path

MODULES = ("client.dll", "server.dll")
PRIORITY = {
    "client.dll": (
        ("VClient011", 0), ("VClient011", 1),
        ("VClient011", 2), ("VClient011", 3),
        ("GameClientExports001", 0), ("GameClientExports001", 1),
    ),
    "server.dll": (
        ("ServerGameDLL003", 0), ("ServerGameDLL003", 1),
        ("ServerGameDLL003", 2), ("ServerGameDLL003", 3),
        ("ServerGameClients003", 0), ("ServerGameClients003", 1),
        ("ServerGameEnts001", 0), ("ServerGameEnts001", 1),
    ),
}
SEED_HEADER = ["entry", "interface", "slot", "source"]
STATUS_HEADER = ["interface", "slot", "entry", "outcome", "inferred_function_bytes"]
SOURCE = "sha256_authenticated_original_pe32_vtable_prefix"
OUTCOMES = {
    "decompiled_candidate", "missing_inferred_function", "not_executable",
    "decompilation_failed", "empty_decompilation",
    "excessive_inferred_function_extent", "oversized_decompilation",
}
MAX_BODY = 50_000_000


def pe32(text):
    if not isinstance(text, str):
        raise ValueError("invalid PE32 method entry")
    digits = text.strip()
    if digits.lower().startswith("0x"):
        digits = digits[2:]
    if not re.fullmatch(r"[0-9a-fA-F]{1,16}", digits):
        raise ValueError("invalid PE32 method entry")
    value = int(digits, 16)
    if value == 0 or value > 0xffffffff:
        raise ValueError("invalid PE32 method entry")
    return value


def slot(text):
    if not isinstance(text, str) or not re.fullmatch(r"[0-9]{1,3}", text):
        raise ValueError("invalid vtable slot")
    value = int(text)
    if value > 255:
        raise ValueError("vtable slot exceeds bound")
    return value


def read(path, header):
    with path.open("r", newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != header:
            raise ValueError(f"{path.name}: unexpected CSV fields")
        rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError(f"{path.name}: malformed or extra CSV fields")
    return rows


def summarize(seed_dir: Path, status_dir: Path) -> dict:
    report = {
        "schema": "sin-reviewed-vtable-private-decomp-status-v1",
        "evidence_tier": "ghidra_inferred_C_like_candidates_not_verified_original_ABI",
        "new_original_whole_body_sha_verified_methods": 0,
        "new_original_method_signatures_verified": 0,
        "new_independently_engine_load_tested_original_methods": 0,
        "original_game_pseudocode_embedded": False,
        "modules": {},
    }
    for module in MODULES:
        expected = PRIORITY[module]
        seeds = read(seed_dir / f"{module}_vtable_method_seeds.csv", SEED_HEADER)
        statuses = read(status_dir / f"{module}_vtable_targeted_decomp_status.csv",
                        STATUS_HEADER)
        reviewed = {}
        seen_addresses = set()
        for row in seeds:
            name = row["interface"]
            index = slot(row["slot"])
            address = pe32(row["entry"])
            if not re.fullmatch(r"[A-Za-z0-9_]+", name) or row["source"] != SOURCE:
                raise ValueError(f"{module}: unreviewed vtable seed source")
            key = (name, index)
            if key in reviewed or address in seen_addresses:
                raise ValueError(f"{module}: duplicate reviewed vtable seed")
            reviewed[key] = address
            seen_addresses.add(address)
        if any(key not in reviewed for key in expected):
            raise ValueError(f"{module}: missing priority interface/slot seed")
        if len(statuses) != len(expected):
            raise ValueError(f"{module}: incorrect bounded status count")
        outcomes = {}
        for record in statuses:
            key = (record["interface"], slot(record["slot"]))
            if key not in expected or key in outcomes:
                raise ValueError(f"{module}: unexpected or repeated decompilation slot")
            address = pe32(record["entry"])
            if address != reviewed[key]:
                raise ValueError(f"{module}: decompilation entry disagrees with SHA-reviewed seed")
            status = record["outcome"]
            if status not in OUTCOMES:
                raise ValueError(f"{module}: unsafe private decompilation status")
            text_length = record["inferred_function_bytes"]
            if text_length:
                if not text_length.isascii() or not text_length.isdecimal():
                    raise ValueError(f"{module}: non-numeric inferred function extent")
                length = int(text_length)
                if not 0 < length <= MAX_BODY:
                    raise ValueError(f"{module}: invalid inferred function extent")
            else:
                length = None
            if status in ("decompiled_candidate", "excessive_inferred_function_extent",
                          "oversized_decompilation", "decompilation_failed",
                          "empty_decompilation") and length is None:
                raise ValueError(f"{module}: inferred function status lacks extent")
            if status in ("missing_inferred_function", "not_executable") and length is not None:
                raise ValueError(f"{module}: missing method falsely has inferred extent")
            outcomes[key] = {
                "entry_va": f"0x{address:08X}",
                "status": status,
                "ghidra_inferred_function_bytes": length,
                "verified_original_signature": False,
                "verified_original_body": False,
            }
        if set(outcomes) != set(expected):
            raise ValueError(f"{module}: incomplete priority method outcomes")
        by_interface = {}
        for name, index in expected:
            by_interface.setdefault(name, {})[f"slot_{index}"] = outcomes[(name, index)]
        successes = sum(item["status"] == "decompiled_candidate"
                        for item in outcomes.values())
        report["modules"][module] = {
            "priority_slots": len(expected),
            "private_pseudocode_candidates": successes,
            "unavailable_or_failed": len(expected) - successes,
            "priority_interfaces": by_interface,
        }
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-dir", required=True, type=Path)
    parser.add_argument("--status-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    report = summarize(args.seed_dir, args.status_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n",
                        encoding="utf-8")
    for module, data in report["modules"].items():
        print(f"{module}: priority_slots={data['priority_slots']} "
              f"candidate_C_private={data['private_pseudocode_candidates']} "
              "verified_original_methods=0")


if __name__ == "__main__":
    main()
