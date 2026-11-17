#!/usr/bin/env python3
"""Join factory direct data references with read-only relocated code-pointer runs.

Only exact address overlap is reported. This is not proof of a vptr or vtable.
"""
import argparse
import json
from collections import defaultdict
from pathlib import Path

from build_relocated_pointer_table_queue import (
    MODULES, contiguous_runs, read_pointers, va, fmt,
)
from summarize_factory_data_references import (
    read_csv, REF_FIELDS, SEED_FIELDS, STATUS_FIELDS, pe32, bounded_decimal,
)

MAX_MATCHES = 256


def join(seed_dir: Path, data_dir: Path):
    result = {
        "schema": "sin-factory-pointer-run-overlap-v1",
        "evidence_tier": "exact_factory_instruction_data_reference_to_candidate_pointer_run",
        "original_code_or_pseudocode_included": False,
        "new_verified_original_methods": 0,
        "new_verified_vtables": 0,
        "new_engine_load_tested_original_methods": 0,
        "modules": {},
    }
    for module in MODULES:
        seeds = read_csv(seed_dir / f"{module}_factory_seeds.csv", SEED_FIELDS)
        statuses = read_csv(data_dir / f"{module}_factory_data_ref_status.csv", STATUS_FIELDS)
        refs = read_csv(data_dir / f"{module}_factory_direct_data_refs.csv", REF_FIELDS)
        seed_map = {}
        for seed in seeds:
            if seed["source"] != "reviewed_original_static_registration":
                raise ValueError("unreviewed factory provenance")
            name = seed["interface"]
            if name in seed_map or not name or not name.replace("_", "").isalnum():
                raise ValueError("duplicate or malformed interface")
            seed_map[name] = pe32(seed["entry"])
        if not seed_map:
            raise ValueError("empty reviewed factory set")
        status_map = {}
        for row in statuses:
            name = row["interface"]
            if name not in seed_map or name in status_map:
                raise ValueError("missing/duplicate/unknown factory status")
            if pe32(row["entry"]) != seed_map[name]:
                raise ValueError("factory status address mismatch")
            if row["outcome"] not in ("missing_inferred_function", "analyzed_direct_data_references"):
                raise ValueError("invalid factory status")
            count = bounded_decimal(row["data_references"], 1000)
            if row["outcome"] == "missing_inferred_function":
                if count or row["inferred_function_bytes"]:
                    raise ValueError("unresolved factory has data evidence")
            elif not bounded_decimal(row["inferred_function_bytes"]):
                raise ValueError("invalid inferred body size")
            status_map[name] = (row["outcome"], count)
        if set(status_map) != set(seed_map):
            raise ValueError("incomplete factory statuses")
        reference_counts = defaultdict(int)
        seen = set()
        for row in refs:
            name = row["interface"]
            if name not in seed_map or pe32(row["factory_entry"]) != seed_map[name]:
                raise ValueError("unreviewed factory reference")
            if status_map[name][0] != "analyzed_direct_data_references":
                raise ValueError("unresolved factory has references")
            source = pe32(row["instruction_va"])
            target = pe32(row["data_target_va"])
            if not row["target_block"] or not row["reference_kind"]:
                raise ValueError("incomplete reference metadata")
            key = (name, source, target, row["reference_kind"])
            if key in seen:
                raise ValueError("duplicate factory reference")
            seen.add(key)
            reference_counts[name] += 1
        for name, (_, expected) in status_map.items():
            if reference_counts[name] != expected:
                raise ValueError("factory reference count mismatch")

        pointers, _ = read_pointers(data_dir / f"{module}_relocated_code_pointers.csv")
        eligible = []
        for run in contiguous_runs(pointers):
            if (len(run) >= 4 and run[0]["source_block"] == ".rdata"
                    and len({row["target"] for row in run}) >= 4):
                eligible.append((run[0]["source"], run[-1]["source"], len(run),
                                 sum(row["target_function"] is not None for row in run)))
        matches = []
        for row in refs:
            target = pe32(row["data_target_va"])
            for start, end, slots, recognized in eligible:
                if start <= target <= end and (target - start) % 4 == 0:
                    matches.append({
                        "interface": row["interface"],
                        "factory_entry": fmt(pe32(row["factory_entry"])),
                        "instruction_va": fmt(pe32(row["instruction_va"])),
                        "referenced_data_va": fmt(target),
                        "candidate_run_start": fmt(start),
                        "candidate_run_end": fmt(end),
                        "candidate_slot_index": (target - start) // 4,
                        "candidate_slot_count": slots,
                        "ghidra_exact_function_start_matches": recognized,
                        "reference_kind": row["reference_kind"],
                        "not_verified_as_vptr_or_vtable": True,
                    })
        matches.sort(key=lambda row: (row["interface"], row["candidate_run_start"],
                                      row["referenced_data_va"], row["instruction_va"]))
        if len(matches) > MAX_MATCHES:
            raise ValueError("candidate overlap exceeds review safety bound")
        result["modules"][module] = {
            "reviewed_factories": len(seed_map),
            "factory_direct_data_references": len(refs),
            "candidate_read_only_pointer_runs": len(eligible),
            "exact_address_overlaps": len(matches),
            "review_queue": matches,
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-dir", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = join(args.seed_dir, args.data_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for module, details in report["modules"].items():
        print(f"{module}: {details['exact_address_overlaps']} exact factory-to-pointer-run overlaps "
              "(not verified vtables)")


if __name__ == "__main__":
    main()
