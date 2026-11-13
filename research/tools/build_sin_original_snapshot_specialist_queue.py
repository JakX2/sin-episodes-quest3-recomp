#!/usr/bin/env python3
"""Triage Phase212 original client serializer callees using Phase213's verified ledger.

This consumes pre-existing SHA-pinned RESEARCH METADATA only; it does not read
or decompile a DLL, establish new method boundaries, or claim new recoveries.
"""
from __future__ import annotations

import argparse
import bisect
import json
from collections import defaultdict
from pathlib import Path

from audit_sin_original_client_snapshot_record_router import (
    ENTRY, SIZE, SHA, CALL_EDGES, CASE_DESTINATIONS, TYPE_CASES,
)
from audit_sin_original_client_snapshot_field_copy import METHODS
from audit_sin_original_dll_interface_abi import ORIGINALS


class QueueError(ValueError):
    """The source metadata disagree or are insufficient for a safe queue."""


def _parse_va(value: str) -> int:
    if not isinstance(value, str) or not value.startswith("0x") or len(value) != 10:
        raise QueueError("ledger entry requires an eight-digit hexadecimal VA")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise QueueError("invalid ledger VA") from exc


def _manifest_index(ledger: dict) -> dict[tuple[str, int], dict]:
    if ledger.get("schema_version") != 1 or ledger.get("dll_sha256") != {
        side: ORIGINALS[side]["sha256"] for side in ("server", "client")
    }:
        raise QueueError("original dual-DLL provenance is missing or inconsistent")
    indexed = {}
    for entry in ledger.get("methods", []):
        if entry.get("dll") not in ("server", "client"):
            raise QueueError("unknown method DLL")
        key = (entry["dll"], _parse_va(entry["entry_va"]))
        if key in indexed:
            raise QueueError("duplicate method VA in the reviewed atlas")
        indexed[key] = entry
    expected = [(ENTRY, SIZE, SHA)] + [
        (row[0], row[1], row[2]) for row in METHODS.values()
    ]
    for va, length, digest in expected:
        row = indexed.get(("client", va))
        if (row is None or row.get("original_code_length") != length
                or row.get("original_code_sha256") != digest):
            raise QueueError(f"Phase212/213 original client evidence drift at {va:#x}")
    return indexed


def build_queue(ledger: dict) -> dict:
    """Classify CALL evidence, without claiming any new native validation."""
    indexed = _manifest_index(ledger)
    if len(CALL_EDGES) != 39 or len({target for _, target in CALL_EDGES}) != 36:
        raise QueueError("Phase212 native CALL graph cardinality drift")
    if len(CASE_DESTINATIONS) != 13 or len(TYPE_CASES) != 27:
        raise QueueError("Phase212 original inline switch table drift")
    order = sorted((va, case) for case, va in enumerate(CASE_DESTINATIONS))
    starts = [va for va, _ in order]
    if len(set(starts)) != len(starts) or any(
        va < ENTRY or va >= ENTRY + SIZE for va in starts
    ) or any(case >= len(CASE_DESTINATIONS) for case in TYPE_CASES):
        raise QueueError("invalid original router case locations")

    cases = defaultdict(list)
    targets = defaultdict(list)
    seen_sites = set()
    for site, target in sorted(CALL_EDGES):
        if site in seen_sites or not (ENTRY <= site < ENTRY + SIZE - 4):
            raise QueueError("duplicate/out-of-bounds native CALL site")
        seen_sites.add(site)
        place = bisect.bisect_right(starts, site) - 1
        if place < 0:
            raise QueueError("CALL precedes the first native case")
        case = order[place][1]
        cases[case].append((site, target))
        targets[target].append((site, case))

    type_numbers = {
        case: [i for i, selected in enumerate(TYPE_CASES) if selected == case]
        for case in range(len(CASE_DESTINATIONS))
    }
    per_case = []
    for case, start in enumerate(CASE_DESTINATIONS):
        edges = cases[case]
        known = sum(("client", target) in indexed for _, target in edges)
        per_case.append({
            "case": case, "case_va": f"0x{start:08X}",
            "field_types": type_numbers[case],
            "direct_calls": len(edges),
            "catalogued_target_calls": known,
            "unreviewed_target_calls": len(edges) - known,
        })

    candidates = []
    known = []
    for target, edges in targets.items():
        assigned_cases = sorted({case for _, case in edges})
        field_types = sorted({t for case in assigned_cases for t in type_numbers[case]})
        record = {
            "target_va": f"0x{target:08X}",
            "native_call_sites": [f"0x{site:08X}" for site, _ in edges],
            "native_call_count": len(edges),
            "case_indices": assigned_cases,
            "field_types": field_types,
        }
        if ("client", target) in indexed:
            record["status"] = "already_in_reviewed_native_method_ledger"
            known.append(record)
        else:
            tier = (0 if any(t in (1, 2) for t in field_types)
                    else 1 if len(edges) > 1 else 2)
            record.update({
                "status": "callee_body_not_reviewed_in_current_ledger",
                "priority": ["P0", "P1", "P2"][tier],
                "priority_reason": (
                    "completes type 1/2 snapshot specialist chain" if tier == 0
                    else "reused by multiple original cases" if tier == 1
                    else "single-site original specialist target"
                ),
                "evidence": "Phase212 exact original native CALL rel32 metadata",
            })
            candidates.append(record)
    candidates.sort(key=lambda item: (
        int(item["priority"][1]), -item["native_call_count"],
        int(item["target_va"], 16),
    ))
    known.sort(key=lambda item: int(item["target_va"], 16))
    return {
        "scope": "2006 client.dll 0x241996F0 prediction snapshot router only",
        "source": "Phase212/213 SHA-pinned evidence and the reviewed-method ledger",
        "new_original_binary_validation_performed": False,
        "counts": {
            "native_call_sites": len(CALL_EDGES),
            "distinct_native_destinations": len(targets),
            "reviewed_destinations_in_ledger": len(known),
            "unreviewed_destinations_in_ledger": len(candidates),
            "unreviewed_call_sites": sum(x["native_call_count"] for x in candidates),
        },
        "cases": per_case,
        "unreviewed_target_queue": candidates,
        "already_reviewed_targets": known,
        "limits": [
            "Callee addresses are instruction-boundary CALL targets, not proven method starts/sizes.",
            "Ledger presence does not establish a runnable engine replacement.",
            "Priorities are investigation order, not confidence or percent complete.",
            "No retail DLL bytes or original disassembly are distributed or read.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=(
        Path(__file__).resolve().parents[1] / "catalog" /
        "original_dll_verified_methods.json"
    ))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        result = build_queue(json.loads(args.ledger.read_text(encoding="utf-8")))
        data = json.dumps(result, indent=2) + "\n"
        if args.output:
            args.output.write_text(data, encoding="utf-8")
        else:
            print(data, end="")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.exit(1, f"SiN specialist triage failed: {exc}\n")


if __name__ == "__main__":
    main()
