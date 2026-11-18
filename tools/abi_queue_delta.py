#!/usr/bin/env python3
"""Compare two public ABI evidence queues without promoting unverified claims.

Accepts raw queue input or validated queue reports. Never reads original DLLs.
A tier change is reported as a claim change, not as independently verified progress.
"""
import argparse
import json
from pathlib import Path

from abi_evidence_queue import LEVELS, report, validate


def load_queue(path):
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError("queue must be a JSON object")
    if set(payload) == {"records"}:
        return validate(payload)
    if set(payload) != {"anchor_order", "counts", "note", "queue_sha256", "records", "schema"}:
        raise ValueError("unexpected queue report fields")
    if payload["schema"] != 1:
        raise ValueError("unsupported report schema")
    records = validate({"records": payload["records"]})
    if report(records) != payload:
        raise ValueError("report fingerprint, counts or metadata mismatch")
    return records


def compare(before, after):
    def indexed(records):
        return {(r["module"], r["interface"], r["rva"]): r for r in records}
    old, new = indexed(before), indexed(after)
    added, removed, changed = [], [], []
    for key in sorted(old.keys() | new.keys()):
        if key not in old:
            added.append(new[key])
        elif key not in new:
            removed.append(old[key])
        elif old[key] != new[key]:
            a, b = old[key], new[key]
            changed.append({
                "module": key[0], "interface": key[1], "rva": key[2],
                "from": a["evidence"], "to": b["evidence"],
                "tier_direction": ("up" if LEVELS[b["evidence"]] > LEVELS[a["evidence"]]
                                   else "down" if LEVELS[b["evidence"]] < LEVELS[a["evidence"]]
                                   else "same"),
                "digest_changed": a.get("body_sha256") != b.get("body_sha256"),
                "test_id_changed": a.get("test_id") != b.get("test_id"),
            })
    return {"schema": 1, "before_sha256": report(before)["queue_sha256"],
            "after_sha256": report(after)["queue_sha256"],
            "added": added, "removed": removed, "changed": changed,
            "note": "Changes are metadata claims only; no original methods are authenticated by this comparison."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before")
    parser.add_argument("after")
    parser.add_argument("--output")
    args = parser.parse_args()
    try:
        result = compare(load_queue(args.before), load_queue(args.after))
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            Path(args.output).write_text(rendered, encoding="utf-8")
        else:
            print(rendered, end="")
    except (OSError, ValueError, TypeError, json.JSONDecodeError) as error:
        parser.exit(2, f"queue delta rejected: {error}\n")


if __name__ == "__main__":
    main()
