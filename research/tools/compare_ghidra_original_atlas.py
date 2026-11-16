#!/usr/bin/env python3
"""Compare Ghidra's inferred PE function starts with independently SHA-reviewed
original SiN DLL method catalog. NEVER promote inferred starts into verified
original methods, or misrepresent the inferred count as decompilation percent.
"""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


class AtlasError(ValueError):
    pass


def parse_entry(raw: str) -> int:
    value = raw.strip()
    if value.lower().startswith("0x"):
        value = value[2:]
    if not value or len(value) > 16:
        raise AtlasError("missing or oversize Ghidra address")
    try:
        return int(value, 16)
    except ValueError as error:
        raise AtlasError("malformed Ghidra function address") from error


def module_atlas(ghidra_csv: Path, methods: list[dict],
                 dll_name: str) -> dict:
    expected = {
        parse_entry(m["entry_va"]): m
        for m in methods if m.get("dll") == dll_name.removesuffix(".dll")
    }
    seen: set[int] = set()
    with ghidra_csv.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None or "entry" not in reader.fieldnames:
            raise AtlasError("Ghidra atlas CSV missing entry header")
        for row in reader:
            addr = parse_entry(row.get("entry") or "")
            if addr in seen:
                raise AtlasError("duplicate Ghidra inferred function entry")
            seen.add(addr)
    overlap = seen & set(expected)
    not_found = set(expected) - seen
    return {
        "dll": dll_name,
        "ghidra_inferred_function_entries": len(seen),
        "already_SHA_reviewed_catalog_entries": len(expected),
        "sha_reviewed_entries_matched_by_ghidra": len(overlap),
        "sha_reviewed_entries_not_identified_by_ghidra": len(not_found),
        "inferred_entries_not_SHA_reviewed": len(seen - set(expected)),
        "matched_original_method_addresses": [
            f"0x{va:08X}" for va in sorted(overlap)],
        "catalog_entries_missing_in_ghidra": [
            f"0x{va:08X}" for va in sorted(not_found)],
        "new_original_SHA_verified_methods": 0,
        "provenance_warning": (
            "Ghidra results are inferred function candidates, not exact "
            "verified original method boundaries or recovered C++ source."),
    }


def analyze(ghidra_dir: Path, catalog: Path) -> dict:
    # The standalone deployment PR can run from main without the hundreds
    # of still-unmerged research branches. A missing SHA catalog must not
    # turn inferred Ghidra starts into a fictitious verified-method count.
    catalog_available = catalog.is_file()
    data = (json.loads(catalog.read_text(encoding="utf-8"))
            if catalog_available else {"methods": []})
    if not isinstance(data, dict) or not isinstance(data.get("methods"), list):
        raise AtlasError("expected verified original method catalog")
    results = {}
    for name in ("client.dll", "server.dll"):
        results[name] = module_atlas(
            ghidra_dir / (name + "_functions.csv"),
            data["methods"], name)
    return {
        "note": "Ghidra function discovery is independent of SHA-reviewed methods.",
        "catalog_loaded": catalog_available,
        "catalog_limitation": (
            "The reviewed method catalog is not present on this deployment "
            "branch; zero overlap is NOT zero project-wide verified methods."
            if not catalog_available else None),
        "total_new_verified_methods": 0,
        "modules": results,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra-dir", type=Path, required=True)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = analyze(args.ghidra_dir, args.catalog)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n",
                        encoding="utf-8")
    print(json.dumps({
        name: {
            "ghidra_candidates": module["ghidra_inferred_function_entries"],
            "sha_reviewed_overlap": module[
                "sha_reviewed_entries_matched_by_ghidra"],
        }
        for name, module in report["modules"].items()
    }, indent=2))


if __name__ == "__main__":
    main()
