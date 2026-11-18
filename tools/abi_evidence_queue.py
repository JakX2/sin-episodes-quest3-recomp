#!/usr/bin/env python3
"""Deterministic, fail-closed review queue for public ABI metadata only.

Input JSON: {"records":[{"module":"client","interface":"VClient011",
"rva":"0x1234","evidence":"inferred","body_sha256":null}, ...]}.
Never accepts binary bytes, disassembly or pseudocode.
"""
import argparse
import hashlib
import json
import re
import sys

ANCHORS = ("VClient011", "ServerGameDLL003", "ServerGameClients003",
           "ServerGameEnts001", "VClientPrediction001", "VClientEntityList003")
LEVELS = {"candidate": 0, "inferred": 1, "sha_reviewed": 2, "engine_tested": 3}
SHA = re.compile(r"^[0-9a-f]{64}$")
RVA = re.compile(r"^0x[0-9a-f]{1,8}$")
ALLOWED = {"module", "interface", "rva", "evidence", "body_sha256", "test_id"}
ROOT = {"records"}
MODULE_FOR_ANCHOR = {anchor: ("server" if anchor.startswith("Server") else "client") for anchor in ANCHORS}


def validate(payload):
    if not isinstance(payload, dict) or set(payload) != ROOT:
        raise ValueError("root must contain only records")
    records = payload["records"]
    if not isinstance(records, list):
        raise ValueError("records must be a list")
    result = []
    seen = set()
    for index, record in enumerate(records):
        if not isinstance(record, dict) or set(record) - ALLOWED:
            raise ValueError(f"record {index}: unknown fields or invalid object")
        module = record.get("module")
        interface = record.get("interface")
        rva = record.get("rva")
        level = record.get("evidence")
        digest = record.get("body_sha256")
        test = record.get("test_id")
        if module not in ("client", "server") or interface not in ANCHORS:
            raise ValueError(f"record {index}: invalid module/interface")
        if module != MODULE_FOR_ANCHOR[interface]:
            raise ValueError(f"record {index}: interface belongs to {MODULE_FOR_ANCHOR[interface]} module")
        if not isinstance(rva, str) or not RVA.fullmatch(rva):
            raise ValueError(f"record {index}: invalid normalized RVA")
        if level not in LEVELS:
            raise ValueError(f"record {index}: invalid evidence tier")
        if level in ("sha_reviewed", "engine_tested") and (
            not isinstance(digest, str) or not SHA.fullmatch(digest)):
            raise ValueError(f"record {index}: reviewed methods require full body SHA")
        if digest is not None and (not isinstance(digest, str) or not SHA.fullmatch(digest)):
            raise ValueError(f"record {index}: invalid SHA")
        if level == "engine_tested" and (not isinstance(test, str) or not re.fullmatch(r"[A-Za-z0-9_.:/-]{1,120}", test)):
            raise ValueError(f"record {index}: engine test identifier required")
        if test is not None and (not isinstance(test, str) or not re.fullmatch(r"[A-Za-z0-9_.:/-]{1,120}", test)):
            raise ValueError(f"record {index}: invalid test identifier")
        if level in ("candidate", "inferred") and (digest is not None or test is not None):
            raise ValueError(f"record {index}: unreviewed evidence cannot carry authenticated fields")
        if level == "sha_reviewed" and test is not None:
            raise ValueError(f"record {index}: engine test ID requires engine_tested tier")
        key = (module, interface, rva)
        if key in seen:
            raise ValueError(f"record {index}: duplicate identity")
        seen.add(key)
        result.append(record)
    result.sort(key=lambda r: (ANCHORS.index(r["interface"]), r["module"], int(r["rva"], 16)))
    return result


def report(records):
    counts = {tier: sum(r["evidence"] == tier for r in records) for tier in LEVELS}
    canonical = json.dumps(records, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return {"schema": 1, "anchor_order": list(ANCHORS), "counts": counts,
            "queue_sha256": hashlib.sha256(canonical).hexdigest(), "records": records,
            "note": "Tier labels are supplied evidence claims, not independently verified by this tool."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="Public metadata JSON, never original binary content")
    parser.add_argument("--output", help="Write deterministic report here (default stdout)")
    args = parser.parse_args()
    try:
        with open(args.input, encoding="utf-8") as handle:
            result = report(validate(json.load(handle)))
        output = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            with open(args.output, "w", encoding="utf-8", newline="\n") as handle:
                handle.write(output)
        else:
            sys.stdout.write(output)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.exit(2, f"evidence queue rejected: {error}\n")


if __name__ == "__main__":
    main()
