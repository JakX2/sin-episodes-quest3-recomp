#!/usr/bin/env python3
"""Validate bounded private Ghidra startup-call census; emit safe metadata only.

Ghidra inferred instruction/call counts are NOT proof of actual factory
argument types, call targets, control flow, or engine startup success.
"""
import argparse
import csv
import json
import re
from pathlib import Path

EXPECTED = {
    "client.dll": ("VClient011", "Init", 0x240FE0E0),
    "server.dll": ("ServerGameDLL003", "DLLInit", 0x221E7640),
}
HEADER = ["interface", "slot", "entry", "sdk_method_hypothesis", "outcome",
          "inferred_body_bytes", "instruction_count", "direct_call_sites",
          "indirect_call_sites", "interface_literal_count",
          "interface_version_literals"]
OUTCOMES = {"missing_exact_ghidra_function", "unbounded_inferred_function",
            "bounded_inferred_function", "excessive_instruction_count"}
VERSION = re.compile(
    r"(?:V[A-Za-z][A-Za-z0-9_]{1,48}|ServerGame[A-Za-z0-9_]{1,48}|GameClientExports)[0-9]{3}\Z"
)


def decimal(text, maximum):
    if not isinstance(text, str) or not re.fullmatch(r"[0-9]{1,9}", text):
        raise ValueError("Invalid or unbounded census counter")
    number = int(text)
    if number > maximum:
        raise ValueError("Census counter exceeds maximum")
    return number


def summarize(directory: Path) -> dict:
    modules = {}
    for module, (iface, method, address) in EXPECTED.items():
        path = directory / (module + "_startup_factory_census.csv")
        with path.open(newline="", encoding="utf-8-sig") as stream:
            reader = csv.DictReader(stream)
            if reader.fieldnames != HEADER:
                raise ValueError("Unexpected private startup census fields")
            rows = list(reader)
        if len(rows) != 1 or None in rows[0] or None in rows[0].values():
            raise ValueError("Startup census must contain exactly one complete row")
        row = rows[0]
        if (row["interface"], row["slot"], row["sdk_method_hypothesis"]) != (iface, "0", method):
            raise ValueError("Wrong startup method or slot")
        if row["entry"] != f"0x{address:08X}":
            raise ValueError("Original method entry does not match reviewed pin")
        status = row["outcome"]
        if status not in OUTCOMES:
            raise ValueError("Unexpected Ghidra method status")
        extent = decimal(row["inferred_body_bytes"], 2**20)
        instructions = decimal(row["instruction_count"], 25001)
        direct = decimal(row["direct_call_sites"], 25000)
        indirect = decimal(row["indirect_call_sites"], 25000)
        count = decimal(row["interface_literal_count"], 32)
        literals = row["interface_version_literals"].split(";") if count else []
        if (len(literals) != count or len(set(literals)) != count or
                literals != sorted(literals) or
                any(not VERSION.fullmatch(s) for s in literals)):
            raise ValueError("Unsafe, unbounded or duplicate interface literal")
        if status == "missing_exact_ghidra_function" and (
                extent or instructions or direct or indirect or count):
            raise ValueError("Missing method cannot have observations")
        if status == "bounded_inferred_function" and not 0 < extent <= 65536:
            raise ValueError("Inferred function size exceeds research bound")
        if status == "unbounded_inferred_function" and (
                not extent or instructions or direct or indirect or count):
            raise ValueError("Unbounded method must not have observations")
        if status == "excessive_instruction_count" and instructions != 25001:
            raise ValueError("Instruction cap not reached as claimed")
        if direct + indirect > instructions:
            raise ValueError("More calls than inferred instructions")
        modules[module] = {
            "interface": iface, "slot": 0, "original_entry_va": row["entry"],
            "sdk2004_method_name_hypothesis": method,
            "ghidra_inferred_method_status": status,
            "ghidra_inferred_function_bytes": extent,
            "ghidra_inferred_instruction_count": instructions,
            "ghidra_direct_call_site_count": direct,
            "ghidra_indirect_call_site_count": indirect,
            "direct_string_reference_interface_version_candidates": literals,
            "original_factory_argument_types_verified": False,
            "original_first_failure_branch_verified": False,
            "original_method_body_verified": False,
        }
    return {
        "schema": "sin-private-startup-factory-census-v1",
        "evidence_tier": "sha_reviewed_method_entries_ghidra_inferred_callsite_counts",
        "original_instruction_bytes_or_pseudocode_embedded": False,
        "independently_tested_retail_startup": False,
        "modules": modules,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--private-census-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = summarize(args.private_census_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n",
                        encoding="utf-8")
    for module, item in report["modules"].items():
        print(f"{module} {item['interface']}[0] "
              f"indirect_calls={item['ghidra_indirect_call_site_count']} "
              f"interface_literals={len(item['direct_string_reference_interface_version_candidates'])} "
              "verified_argument_types=0 first_failure_branch_verified=0")


if __name__ == "__main__":
    main()
