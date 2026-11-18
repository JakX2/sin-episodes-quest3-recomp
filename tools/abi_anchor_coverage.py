#!/usr/bin/env python3
"""Batch coverage summary for six public ABI anchors; evidence claims are not verification."""
import argparse
import json
from pathlib import Path

from abi_evidence_queue import ANCHORS, LEVELS, MODULE_FOR_ANCHOR
from abi_queue_delta import load_queue


def summarize(records):
    by_anchor = []
    for anchor in ANCHORS:
        subset = [record for record in records if record["interface"] == anchor]
        by_anchor.append({
            "interface": anchor,
            "module": MODULE_FOR_ANCHOR[anchor],
            "counts": {tier: sum(record["evidence"] == tier for record in subset) for tier in LEVELS},
            "records": len(subset),
            "has_engine_test_claim": any(record["evidence"] == "engine_tested" for record in subset),
        })
    return {
        "schema": 1,
        "anchors": by_anchor,
        "missing_anchors": [item["interface"] for item in by_anchor if not item["records"]],
        "without_engine_test_claim": [item["interface"] for item in by_anchor if not item["has_engine_test_claim"]],
        "total_records": len(records),
        "note": "Counts and test identifiers are supplied metadata claims, not independent SHA or retail engine verification.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("queue", help="Raw or integrity-checked public queue JSON")
    parser.add_argument("--require-all-anchors", action="store_true",
                        help="Exit 3 if any priority interface has zero records")
    parser.add_argument("--output")
    args = parser.parse_args()
    try:
        summary = summarize(load_queue(Path(args.queue)))
        rendered = json.dumps(summary, sort_keys=True, indent=2) + "\n"
        if args.output:
            Path(args.output).write_text(rendered, encoding="utf-8")
        else:
            print(rendered, end="")
        if args.require_all_anchors and summary["missing_anchors"]:
            parser.exit(3, "missing ABI anchor records: " + ", ".join(summary["missing_anchors"]) + "\n")
    except (OSError, ValueError, TypeError, json.JSONDecodeError) as exc:
        parser.exit(2, f"anchor coverage rejected: {exc}\n")


if __name__ == "__main__":
    main()
