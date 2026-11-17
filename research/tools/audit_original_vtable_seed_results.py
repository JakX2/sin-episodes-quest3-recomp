#!/usr/bin/env python3
"""Fail-closed metadata reconciliation for SHA-authenticated vtable Ghidra seeds.

These are only candidate function starts, not verified method bodies or ABIs.
"""
import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

MODULES = ("client.dll", "server.dll")
EXPECTED = {
    "client.dll": {"GameClientExports001": 4, "VClient011": 48},
    "server.dll": {"ServerGameDLL003": 31, "ServerGameEnts001": 7,
                   "ServerGameClients003": 15},
}
PROVENANCE = "sha256_authenticated_original_pe32_vtable_prefix"
SEED_HEADER = ["entry", "interface", "slot", "source"]
RESULT_HEADER = ["entry", "interface", "slot", "outcome", "result_function_entry"]
SUCCESS = {"already_inferred", "seeded_candidate"}
OTHER = {"non_executable", "overlaps_another_inferred_function", "unresolved_candidate"}


def va(raw):
    if not isinstance(raw, str):
        raise ValueError("invalid PE32 virtual address")
    raw = raw.strip()
    if raw.lower().startswith("0x"):
        raw = raw[2:]
    if len(raw) > 16 or not re.fullmatch("[0-9a-fA-F]{1,16}", raw):
        raise ValueError("invalid PE32 virtual address")
    value = int(raw, 16)
    if not 0 < value <= 0xffffffff:
        raise ValueError("invalid PE32 virtual address range")
    return value


def read_csv(path, header):
    with path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != header:
            raise ValueError(f"{path.name}: unexpected CSV header")
        rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError(f"{path.name}: malformed CSV row")
    return rows


def audit(seed_dir: Path, result_dir: Path, expected_counts=None):
    expected_counts = EXPECTED if expected_counts is None else expected_counts
    report = {
        "schema": "sin-original-vtable-method-seed-coverage-v1",
        "evidence_tier": "ghidra_function_entry_candidates_only",
        "newly_verified_original_method_bodies": 0,
        "newly_verified_vtable_method_signatures": 0,
        "independently_engine_tested_methods": 0,
        "original_code_or_pseudocode_embedded": False,
        "modules": {},
    }
    for module in MODULES:
        expected = expected_counts[module]
        seeds = read_csv(seed_dir / f"{module}_vtable_method_seeds.csv", SEED_HEADER)
        outcomes = read_csv(result_dir / f"{module}_vtable_seed_results.csv", RESULT_HEADER)
        expected_total = sum(expected.values())
        if len(seeds) != expected_total or len(outcomes) != expected_total:
            raise ValueError(f"{module}: incomplete original vtable seed/status batch")
        reviewed = {}
        addresses = set()
        for row in seeds:
            interface = row["interface"]
            if interface not in expected or row["source"] != PROVENANCE:
                raise ValueError(f"{module}: unreviewed vtable seed")
            if not re.fullmatch("[0-9]{1,3}", row["slot"]):
                raise ValueError(f"{module}: invalid original vtable slot")
            slot = int(row["slot"])
            if not 0 <= slot < expected[interface] or (interface, slot) in reviewed:
                raise ValueError(f"{module}: duplicate/out-of-range slot")
            address = va(row["entry"])
            if address in addresses:
                raise ValueError(f"{module}: duplicate vtable target")
            addresses.add(address)
            reviewed[(interface, slot)] = address
        if any(sum(1 for interface, slot in reviewed if interface == name) != count
               for name, count in expected.items()):
            raise ValueError(f"{module}: incomplete reviewed interface prefix")
        seen = set()
        by_interface = defaultdict(Counter)
        totals = Counter()
        blocked = []
        for row in outcomes:
            interface = row["interface"]
            if not re.fullmatch("[0-9]{1,3}", row["slot"]):
                raise ValueError(f"{module}: invalid result slot")
            slot = int(row["slot"])
            key = (interface, slot)
            if key not in reviewed or key in seen:
                raise ValueError(f"{module}: missing/duplicate/unreviewed result slot")
            seen.add(key)
            address = va(row["entry"])
            if address != reviewed[key]:
                raise ValueError(f"{module}: original vtable entry VA mismatch")
            status = row["outcome"]
            if status not in SUCCESS | OTHER and not re.fullmatch(
                    "seed_error_[A-Za-z0-9_]+", status):
                raise ValueError(f"{module}: unexpected Ghidra result status")
            result_address = row["result_function_entry"]
            if status in SUCCESS:
                if not result_address or va(result_address) != address:
                    raise ValueError(f"{module}: successful seed has wrong function entry")
            elif result_address:
                raise ValueError(f"{module}: unresolved seed has function entry")
            totals[status] += 1
            by_interface[interface][status] += 1
            if status not in SUCCESS:
                blocked.append({
                    "interface": interface,
                    "slot": slot,
                    "candidate_va": f"0x{address:08X}",
                    "status": status,
                })
        if seen != set(reviewed):
            raise ValueError(f"{module}: missing original vtable seed outcomes")
        report["modules"][module] = {
            "reviewed_slots": expected_total,
            "existing_ghidra_starts": totals["already_inferred"],
            "newly_seeded_ghidra_candidates": totals["seeded_candidate"],
            "unresolved_candidates": expected_total - totals["already_inferred"]
                                      - totals["seeded_candidate"],
            "outcomes": dict(sorted(totals.items())),
            "per_interface": {
                name: {"slots": count, "outcomes": dict(sorted(by_interface[name].items()))}
                for name, count in sorted(expected.items())
            },
            "unresolved_review_queue": sorted(
                blocked, key=lambda item: (item["interface"], item["slot"])),
        }
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-dir", required=True, type=Path)
    parser.add_argument("--result-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    report = audit(args.seed_dir, args.result_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n",
                        encoding="utf-8")
    print(json.dumps({
        module: {key: item[key] for key in (
            "reviewed_slots", "existing_ghidra_starts",
            "newly_seeded_ghidra_candidates", "unresolved_candidates")}
        for module, item in report["modules"].items()
    }, sort_keys=True))


if __name__ == "__main__":
    main()
