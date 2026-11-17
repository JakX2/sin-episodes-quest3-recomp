#!/usr/bin/env python3
"""Metadata-only factory-to-global-data reference triage.

References prove only that Ghidra found a direct instruction reference
to nonexecutable data. A reference does NOT prove object identity, a
constructor, vptr assignment, class ownership or vtable layout.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

MODULES = ("client.dll", "server.dll")
PRIORITY_INTERFACES = frozenset({
    "VClient011", "VClientEntityList003", "VClientPrediction001",
    "GameClientExports001", "GameMovement001", "ServerGameDLL003",
    "ServerGameEnts001", "ServerGameClients003",
})
STATUS_FIELDS = ["interface", "entry", "outcome", "inferred_function_bytes", "data_references"]
REF_FIELDS = [
    "interface", "factory_entry", "instruction_va", "data_target_va",
    "target_block", "reference_kind",
]
SEED_FIELDS = ["entry", "interface", "source"]
OUTCOMES = {"missing_inferred_function", "analyzed_direct_data_references"}


def pe32(text):
    if not isinstance(text, str):
        raise ValueError("invalid PE32 hexadecimal address")
    digits = text.strip()
    if digits.lower().startswith("0x"):
        digits = digits[2:]
    if not 1 <= len(digits) <= 8 or not re.fullmatch(r"[0-9a-fA-F]+", digits):
        raise ValueError("invalid PE32 hexadecimal address")
    address = int(digits, 16)
    if address == 0:
        raise ValueError("invalid PE32 hexadecimal address: null")
    return address


def read_csv(path, fields):
    with path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != fields:
            raise ValueError(f"{path.name}: bad CSV schema")
        records = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in records):
        raise ValueError(f"{path.name}: malformed CSV row")
    return records


def bounded_decimal(text, maximum=20000000):
    if not isinstance(text, str) or not text.isascii() or not text.isdecimal():
        raise ValueError("invalid unsigned decimal metric")
    value = int(text)
    if value > maximum:
        raise ValueError("metric exceeds allowable limit")
    return value


def summarize(seed_dir, data_dir, max_examples=12):
    if not 1 <= max_examples <= 32:
        raise ValueError("invalid example bound")
    output = {
        "schema": "sin-factory-data-reference-triage-v1",
        "evidence_tier": "ghidra_direct_instruction_to_data_references_only",
        "new_verified_original_methods": 0,
        "new_verified_vtables": 0,
        "new_engine_load_tested_original_methods": 0,
        "original_code_or_pseudocode_included": False,
        "modules": {},
    }
    for module in MODULES:
        seeds = read_csv(seed_dir / f"{module}_factory_seeds.csv", SEED_FIELDS)
        statuses = read_csv(data_dir / f"{module}_factory_data_ref_status.csv", STATUS_FIELDS)
        references = read_csv(data_dir / f"{module}_factory_direct_data_refs.csv", REF_FIELDS)
        expected = {}
        for seed in seeds:
            name = seed["interface"]
            if not re.fullmatch(r"[A-Za-z0-9_]+", name):
                raise ValueError(f"{module}: bad interface name")
            if seed["source"] != "reviewed_original_static_registration":
                raise ValueError(f"{module}: unreviewed factory provenance")
            if name in expected:
                raise ValueError(f"{module}: duplicate reviewed seed")
            expected[name] = pe32(seed["entry"])
        if not expected:
            raise ValueError(f"{module}: missing reviewed seed records")
        parsed = {}
        for status in statuses:
            name = status["interface"]
            if name not in expected or name in parsed:
                raise ValueError(f"{module}: unknown or repeated factory status")
            if pe32(status["entry"]) != expected[name]:
                raise ValueError(f"{module}: reported factory VA mismatch")
            outcome = status["outcome"]
            if outcome not in OUTCOMES:
                raise ValueError(f"{module}: invalid factory status")
            ref_count = bounded_decimal(status["data_references"], 1000)
            extent_text = status["inferred_function_bytes"]
            if outcome == "missing_inferred_function":
                if extent_text or ref_count:
                    raise ValueError(f"{module}: unresolved factory has inferred evidence")
                extent = None
            else:
                extent = bounded_decimal(extent_text)
                if extent == 0:
                    raise ValueError(f"{module}: empty inferred function extent")
            parsed[name] = {
                "factory_va": f"0x{expected[name]:08X}",
                "outcome": outcome,
                "inferred_function_bytes": extent,
                "direct_data_reference_count": ref_count,
            }
        if set(parsed) != set(expected):
            raise ValueError(f"{module}: missing factory reference status")
        seen = set()
        per_interface = defaultdict(list)
        for ref in references:
            name = ref["interface"]
            if name not in expected:
                raise ValueError(f"{module}: unknown interface in data references")
            if parsed[name]["outcome"] != "analyzed_direct_data_references":
                raise ValueError(f"{module}: unresolved factory has direct data references")
            if pe32(ref["factory_entry"]) != expected[name]:
                raise ValueError(f"{module}: data reference factory entry mismatch")
            source_va, target_va = pe32(ref["instruction_va"]), pe32(ref["data_target_va"])
            block = ref["target_block"]
            kind = ref["reference_kind"]
            if not re.fullmatch(r"[.A-Za-z0-9_$+-]{1,96}", block):
                raise ValueError(f"{module}: invalid data reference block")
            if not re.fullmatch(r"[A-Za-z0-9_ -]{1,96}", kind):
                raise ValueError(f"{module}: invalid reference kind")
            key = (name, source_va, target_va, kind)
            if key in seen:
                raise ValueError(f"{module}: duplicated direct data reference")
            seen.add(key)
            per_interface[name].append({
                "instruction_va": f"0x{source_va:08X}",
                "data_target_va": f"0x{target_va:08X}",
                "block": block,
                "kind": kind,
                "not_verified_as_object_or_vptr": True,
            })
        for name, report in parsed.items():
            actual = len(per_interface[name])
            if actual != report["direct_data_reference_count"]:
                raise ValueError(f"{module}: direct data reference count mismatch for {name}")
            ordered = sorted(per_interface[name], key=lambda item: (
                item["data_target_va"], item["instruction_va"], item["kind"]))
            report["data_block_histogram"] = dict(sorted(
                Counter(item["block"] for item in ordered).items()))
            report["data_targets_for_manual_review"] = ordered[:max_examples]
            report["priority_engine_interface"] = name in PRIORITY_INTERFACES
        priority = {name: parsed[name] for name in sorted(parsed)
                    if name in PRIORITY_INTERFACES}
        output["modules"][module] = {
            "reviewed_factory_count": len(expected),
            "factories_with_inferred_entries": sum(
                r["outcome"] == "analyzed_direct_data_references" for r in parsed.values()),
            "direct_nonexecuting_data_references": len(references),
            "priority_interface_factory_data_refs": priority,
            "all_factory_statuses": dict(sorted(parsed.items())),
        }
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-dir", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = summarize(args.seed_dir, args.data_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    for name, report in result["modules"].items():
        print(f"{name}: {report['factories_with_inferred_entries']}/"
              f"{report['reviewed_factory_count']} candidate starts, "
              f"{report['direct_nonexecuting_data_references']} data xrefs "
              f"(NOT verified object or vtables)")


if __name__ == "__main__":
    main()
