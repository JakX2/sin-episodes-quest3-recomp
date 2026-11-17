#!/usr/bin/env python3
"""Validate private startup-call review CSV; emit only non-sensitive aggregate metadata.

Never print raw input rows, offsets, version strings, or proprietary instructions.
This validates shape, NOT SHA256 authentication or a verified factory/branch.
"""
import argparse
import csv
import json
import re
from pathlib import Path

HEADER = [
    "interface", "slot", "entry", "candidate_rank",
    "computed_call_relative_offset", "nearest_prior_version_literal",
    "prior_literal_relative_offset", "next_conditional_relative_offset",
    "next_conditional_distance_in_instructions", "evidence_tier",
]
TIER = "ghidra_inferred_nearby_sites_not_verified_factory_or_failure"
MODULES = {
    "client.dll": ("VClient011", "0x240FE0E0"),
    "server.dll": ("ServerGameDLL003", "0x221E7640"),
}
VERSION = re.compile(
    r"(?:V[A-Za-z][A-Za-z0-9_]{1,48}|ServerGame[A-Za-z0-9_]{1,48}|"
    r"GameClientExports)[0-9]{3}\Z"
)
DECIMAL = re.compile(r"(?:0|[1-9][0-9]*)\Z")


def number(value, maximum):
    if not DECIMAL.fullmatch(value):
        raise ValueError("invalid decimal metadata")
    result = int(value)
    if result > maximum:
        raise ValueError("metadata out of bounded range")
    return result


def validate(path, module):
    if module not in MODULES:
        raise ValueError("unknown startup module")
    # The Ghidra exporter emits at most 8 records; prevent oversized private
    # files from being parsed or inadvertently disclosed in error diagnostics.
    if path.stat().st_size > 8192:
        raise ValueError("oversized private metadata")
    with path.open(newline="", encoding="utf-8-sig") as stream:
        rows = list(csv.reader(stream, strict=True))
    if not rows or rows[0] != HEADER or len(rows) > 9:
        raise ValueError("unexpected private metadata schema or row count")
    iface, entry = MODULES[module]
    prior_count = branch_count = 0
    for rank, row in enumerate(rows[1:], start=1):
        if len(row) != len(HEADER):
            raise ValueError("unexpected private metadata row width")
        (observed_iface, slot, observed_entry, observed_rank, call_offset,
         version, prior_offset, branch_offset, branch_distance, tier) = row
        if (observed_iface, slot, observed_entry, tier) != (
            iface, "0", entry, TIER
        ) or number(observed_rank, 8) != rank:
            raise ValueError("unreviewed interface, entry, tier or rank")
        call = number(call_offset, 65535)
        if version:
            if not VERSION.fullmatch(version) or not prior_offset:
                raise ValueError("invalid prior literal evidence")
            prior = number(prior_offset, 65535)
            if prior >= call:
                raise ValueError("prior literal not before call")
            prior_count += 1
        elif prior_offset:
            raise ValueError("orphaned prior literal offset")
        if branch_offset or branch_distance:
            if not branch_offset or not branch_distance:
                raise ValueError("incomplete branch metadata")
            branch = number(branch_offset, 65535)
            distance = number(branch_distance, 16)
            if branch <= call or distance == 0:
                raise ValueError("invalid nearby branch ordering")
            branch_count += 1
    return {
        "schema": "sin_first_startup_call_review_v1",
        "module": module,
        "candidate_count": len(rows) - 1,
        "candidates_with_prior_literal": prior_count,
        "candidates_with_nearby_conditional": branch_count,
        "verified_factory_calls": 0,
        "verified_failure_edges": 0,
        "original_sha256_authenticated_by_this_tool": False,
        "evidence_tier": "inferred_proximity_only",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", required=True, choices=tuple(MODULES))
    parser.add_argument("--private-csv", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.private_csv, args.module)
    except (ValueError, OSError, UnicodeError, csv.Error):
        parser.exit(2, "Rejected invalid private startup metadata (details withheld)\n")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
