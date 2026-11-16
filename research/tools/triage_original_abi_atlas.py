#!/usr/bin/env python3
"""Metadata-only batch ranking of original PE candidate entries for ABI work.

Never upgrades a candidate to a verified function. Original inputs must be
separately authenticated by the upstream SHA256-gated inventory pipeline.
"""
import argparse
import csv
import json
from collections import Counter
from pathlib import Path

EXPECTED_SHA256 = {
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}

def read_csv(path, required):
    with path.open(newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames or not set(required).issubset(reader.fieldnames):
            raise ValueError(f"{path}: missing required columns {required}")
        return list(reader)

def address(value):
    if not value or not value.startswith("0x"):
        raise ValueError("expected hex VA with 0x prefix")
    n = int(value[2:], 16)
    if n < 0 or n > 0xffffffff:
        raise ValueError("VA outside PE32")
    return n

def check_provenance(directory):
    # Checks reported inventory digests. The upstream scanner must hash actual
    # original bytes; this metadata check is NOT an independent rehash.
    summary = json.loads((directory / "summary.json").read_text(encoding="utf-8"))
    for dll, digest in EXPECTED_SHA256.items():
        got = summary.get("modules", {}).get(dll, {}).get("sha256", "")
        if got.lower() != digest:
            raise ValueError(f"{dll}: inventory SHA256 does not match owner original")

def build(directory, module, ghidra=None):
    if module not in ("client", "server"):
        raise ValueError("unknown module")
    candidates = read_csv(directory / f"{module}_function_candidates.csv",
                          ("address", "tier", "call_sites", "data_pointer_references"))
    exports = read_csv(directory / f"{module}_exports.csv", ("address", "name"))
    edges = read_csv(directory / f"{module}_direct_edges.csv", ("caller", "target", "kind"))
    ghidra_starts = set()
    if ghidra:
        ghidra_starts = {address(r["entry"]) for r in read_csv(
            ghidra / f"{module}.dll_functions.csv", ("entry",))}
    export_by_address = {}
    for item in exports:
        export_by_address.setdefault(address(item["address"]), []).append(item["name"])
    inbound = Counter()
    for edge in edges:
        if edge["kind"] == "call":
            inbound[address(edge["target"])] += 1
    seen = set()
    result = []
    tiers = ("reviewed_catalog", "strong_anchor", "corroborated_anchor",
             "call_target", "address_taken", "heuristic_int3_boundary")
    for item in candidates:
        va = address(item["address"])
        if va in seen:
            raise ValueError("duplicate candidate VA")
        seen.add(va)
        if item["tier"] not in tiers:
            raise ValueError("unknown candidate tier")
        calls, ptrs = int(item["call_sites"]), int(item["data_pointer_references"])
        if min(calls, ptrs) < 0:
            raise ValueError("negative evidence count")
        names = sorted(export_by_address.get(va, []))
        inferred = va in ghidra_starts
        # Heuristic ranking, NOT function confidence or coverage percentage.
        score = (1000 if any("CreateInterface" in n for n in names) else 0)
        score += (300 if names else 0) + (80 if inferred else 0)
        score += min(calls, 100) * 2 + min(ptrs, 100)
        result.append({
            "address": f"0x{va:08X}", "tier": item["tier"],
            "exports": ";".join(names), "ghidra_inferred_start": inferred,
            "direct_call_sites": calls, "edge_call_observations": inbound[va],
            "relocated_pointer_refs": ptrs, "priority_score": score,
            "whole_body_sha_verified_by_this_tool": False,
            "independently_load_tested_by_this_tool": False,
        })
    result.sort(key=lambda x: (-x["priority_score"], x["address"]))
    return {
        "module": module, "candidate_count": len(result),
        "ghidra_inferred_starts_matched": sum(x["ghidra_inferred_start"] for x in result),
        "export_anchored_candidates": sum(bool(x["exports"]) for x in result),
        "new_verified_original_functions": 0,
        "new_independently_load_tested_original_functions": 0,
        "warning": "Metadata-only triage; inferred starts are not authenticated function boundaries.",
        "ranked_candidates": result,
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", type=Path, required=True)
    parser.add_argument("--original-dir", type=Path, help="Rehash both owner originals before ranking; required for authenticated results")
    parser.add_argument("--ghidra", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    check_provenance(args.inventory)
    if args.original_dir:
        from verify_original_batch import authenticate
        verified = authenticate(args.original_dir)
        summary = json.loads((args.inventory / "summary.json").read_text(encoding="utf-8"))
        for dll, details in verified["modules"].items():
            if summary["modules"][dll]["sha256"].lower() != details["sha256"]:
                raise ValueError(f"{dll}: inventory digest disagrees with independently hashed original")
    report = {name: build(args.inventory, name, args.ghidra)
              for name in ("client", "server")}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    report["_authentication"] = {"original_bytes_rehashed": bool(args.original_dir), "both_originals_verified": bool(args.original_dir), "candidate_counts_are_not_verified_functions": True}
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({name: {
        key: data[key] for key in ("candidate_count", "ghidra_inferred_starts_matched",
                                   "export_anchored_candidates")}
        for name, data in report.items()}, indent=2))

if __name__ == "__main__":
    main()
