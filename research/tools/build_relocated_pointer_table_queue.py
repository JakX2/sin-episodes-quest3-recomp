#!/usr/bin/env python3
"""Rank contiguous 32-bit code-pointer runs from Ghidra's original DLL atlas.

Each run is only a DATA-layout candidate for a C++ virtual table or another
function-pointer table. No virtual method, ABI, or source class is verified.
The input is relocation-target metadata, not game bytes or instructions.
"""
from __future__ import annotations

import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path

MODULES = ("client.dll", "server.dll")
REQUIRED = ("source", "target", "target_function", "source_block")
DEFAULT_MIN_SLOTS = 4
DEFAULT_MAX_PER_BUCKET = 8
BUCKETS = (
    (4, 16, "04-16"),
    (17, 64, "17-64"),
    (65, 128, "65-128"),
    (129, 256, "129-256"),
    (257, 0xFFFFFFFF, "257-plus"),
)


def va(raw: str, context: str) -> int:
    if not isinstance(raw, str):
        raise ValueError(f"{context}: expected PE32 hex text")
    digits = raw.strip()
    if digits.lower().startswith("0x"):
        digits = digits[2:]
    if not 1 <= len(digits) <= 8 or any(c not in "0123456789abcdefABCDEF" for c in digits):
        raise ValueError(f"{context}: invalid PE32 hex address")
    value = int(digits, 16)
    if value == 0:
        raise ValueError(f"{context}: null PE32 address")
    return value


def fmt(address: int) -> str:
    return f"0x{address:08X}"


def read_pointers(path: Path) -> tuple[list[dict], int]:
    with path.open(newline="", encoding="utf-8-sig") as source:
        reader = csv.DictReader(source)
        if tuple(reader.fieldnames or ()) != REQUIRED:
            raise ValueError(f"{path.name}: unexpected pointer CSV schema")
        rows = {}
        duplicate_records = 0
        for number, row in enumerate(reader, start=2):
            if None in row or any(value is None for value in row.values()):
                raise ValueError(f"{path.name} line {number}: malformed CSV row")
            address = va(row["source"], "source")
            target = va(row["target"], "target")
            target_fn = va(row["target_function"], "target_function") if row["target_function"] else None
            block = row["source_block"]
            if not block or len(block) > 128 or "\n" in block or "\r" in block:
                raise ValueError(f"{path.name} line {number}: invalid source block")
            if target_fn is not None and target != target_fn:
                raise ValueError(f"{path.name} line {number}: function start mismatches target")
            # x86 data may contain valid packed 4-byte relocations at offset +2.
            # Require aligned only when selecting read-only pointer table candidates.
            if address % 4 and block == ".rdata":
                raise ValueError(f"{path.name} line {number}: unaligned read-only pointer source")
            normalized = {"source": address, "target": target,
                          "target_function": target_fn, "source_block": block}
            if address in rows:
                if rows[address] != normalized:
                    raise ValueError(f"{path.name}: conflicting pointer source {fmt(address)}")
                duplicate_records += 1
            else:
                rows[address] = normalized
    return [rows[key] for key in sorted(rows)], duplicate_records


def contiguous_runs(rows: list[dict]):
    run = []
    for pointer in rows:
        if run and (pointer["source"] != run[-1]["source"] + 4 or
                    pointer["source_block"] != run[-1]["source_block"]):
            yield run
            run = []
        run.append(pointer)
    if run:
        yield run


def size_bucket(slot_count: int) -> str:
    for minimum, maximum, label in BUCKETS:
        if minimum <= slot_count <= maximum:
            return label
    raise ValueError("invalid table length")


def candidate(run: list[dict]) -> dict:
    targets = [item["target"] for item in run]
    recognized = sum(item["target_function"] is not None for item in run)
    return {
        "table_candidate_va": fmt(run[0]["source"]),
        "last_pointer_va": fmt(run[-1]["source"]),
        "source_block": run[0]["source_block"],
        "contiguous_pointer_slots": len(run),
        "distinct_target_addresses": len(set(targets)),
        "ghidra_exact_function_start_matches": recognized,
        "non_inferred_function_targets": len(run) - recognized,
        "size_bucket": size_bucket(len(run)),
        "first_eight_target_addresses": [fmt(value) for value in targets[:8]],
        "not_verified_as_vtable": True,
    }


def analyze_module(path: Path, min_slots: int = DEFAULT_MIN_SLOTS,
                   max_per_bucket: int = DEFAULT_MAX_PER_BUCKET) -> dict:
    if min_slots < 4 or max_per_bucket < 1 or max_per_bucket > 100:
        raise ValueError("invalid candidate queue bounds")
    rows, identical_duplicates = read_pointers(path)
    by_block = Counter(row["source_block"] for row in rows)
    candidates = []
    counts = Counter()
    for run in contiguous_runs(rows):
        if len(run) < min_slots or run[0]["source_block"] != ".rdata":
            continue
        if len({row["target"] for row in run}) < 4:
            continue
        c = candidate(run)
        candidates.append(c)
        counts[c["size_bucket"]] += 1
    by_bucket = defaultdict(list)
    for c in candidates:
        by_bucket[c["size_bucket"]].append(c)
    priority_queue = []
    for _, _, label in BUCKETS:
        bucket = by_bucket[label]
        bucket.sort(key=lambda c: (-c["ghidra_exact_function_start_matches"],
                                    -c["distinct_target_addresses"],
                                    -c["contiguous_pointer_slots"],
                                    c["table_candidate_va"]))
        priority_queue.extend(bucket[:max_per_bucket])
    return {
        "relocated_data_code_pointer_records_total": len(rows) + identical_duplicates,
        "relocated_data_code_pointers_unique_sources": len(rows),
        "deduplicated_identical_relocation_records": identical_duplicates,
        "source_block_pointers": dict(sorted(by_block.items())),
        "rdata_pointer_runs_4plus_distinct_targets": len(candidates),
        "candidate_runs_by_size_bucket": {label: counts[label] for _, _, label in BUCKETS},
        "review_queue": priority_queue,
    }


def build(input_dir: Path, min_slots: int = DEFAULT_MIN_SLOTS,
          max_per_bucket: int = DEFAULT_MAX_PER_BUCKET) -> dict:
    modules = {name: analyze_module(input_dir / f"{name}_relocated_code_pointers.csv",
                                    min_slots, max_per_bucket)
               for name in MODULES}
    return {
        "schema": "sin-relocated-code-pointer-runs-v1",
        "evidence_tier": "ghidra_relocated_code_pointer_array_candidates_only",
        "description": "Contiguous relocations to executable code are possible pointer arrays, not proven vtables or class methods.",
        "new_verified_original_methods": 0,
        "new_engine_load_tested_original_methods": 0,
        "original_code_or_pseudocode_included": False,
        "modules": modules,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra-dir", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--min-slots", type=int, default=DEFAULT_MIN_SLOTS)
    parser.add_argument("--max-per-bucket", type=int, default=DEFAULT_MAX_PER_BUCKET)
    args = parser.parse_args()
    report = build(args.ghidra_dir, args.min_slots, args.max_per_bucket)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for module, stats in report["modules"].items():
        print(f"{module}: unique pointer sources={stats['relocated_data_code_pointers_unique_sources']} "
              f"contiguous read-only candidates={stats['rdata_pointer_runs_4plus_distinct_targets']} "
              f"ranked review={len(stats['review_queue'])}")


if __name__ == "__main__":
    main()
