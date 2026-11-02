#!/usr/bin/env python3
"""Find original x86 helper routines called by multiple SiN gameplay classes.

Input is metadata JSON produced by audit_sin_gameplay_control_flow.py, NOT the
original commercial DLLs. Only depth-zero changed-vtable callsites count, so a
popular common libc or engine helper reached in a second hop does not inflate
the first-layer evidence.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path

from verify_gameplay_entity_registry import SERVER_SHA256, CLIENT_SHA256


def shared_helpers(reports: dict[str, dict], minimum: int = 2) -> dict:
    if len(reports) < minimum or minimum < 2:
        raise ValueError("require at least two differently named class reports")
    by_callee: dict[str, dict[str, list[dict]]] = {}
    dll = None
    sha = None
    for label, report in reports.items():
        kind, digest = report["dll"], report["sha256"]
        if kind not in ("server.dll", "client.dll"):
            raise ValueError("unknown original game DLL")
        expected = SERVER_SHA256 if kind == "server.dll" else CLIENT_SHA256
        if digest != expected:
            raise ValueError("original DLL fingerprint mismatch")
        if dll is None:
            dll, sha = kind, digest
        if kind != dll or digest != sha:
            raise ValueError("cannot combine different original game DLLs")
        layers = report["layers"]
        if not layers or layers[0]["depth"] != 0:
            raise ValueError("missing depth-zero vtable-origin layer")
        for method in layers[0]["functions"]:
            caller = method["start_va"]
            for callee in method.get("direct_callees", []):
                by_callee.setdefault(callee["va"], {}).setdefault(label, []).append({
                    "caller_va": caller, "callsites": callee["count"],
                    "virtual_origin": method.get("vtable_origin", []),
                })
    matches = []
    for address, classes in sorted(by_callee.items()):
        if len(classes) < minimum:
            continue
        matches.append({
            "callee_va": address,
            "distinct_class_groups": len(classes),
            "total_call_instructions": sum(entry["callsites"]
                                           for rows in classes.values()
                                           for entry in rows),
            "callers_by_group": classes,
        })
    matches.sort(key=lambda e: (-e["distinct_class_groups"],
                                -e["total_call_instructions"], e["callee_va"]))
    return {
        "dll": dll, "original_sha256": sha, "class_groups": sorted(reports),
        "minimum_independent_class_groups": minimum,
        "distinct_shared_direct_callees": len(matches),
        "shared_helpers": matches,
        "limits": [
            "Address sharing proves only a common direct-call target in the original DLL.",
            "Specific helper identities and original side effects still require disassembly.",
            "A direct-call xref is not proof of engine-side compatibility on ARM64.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--report", action="append", required=True,
                   help="CLASSNAME=path/to/metadata.json; repeat per same-DLL class")
    p.add_argument("--min-groups", type=int, default=2)
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    try:
        reports = {}
        for item in args.report:
            label, sep, path = item.partition("=")
            if not sep or not label or not path or label in reports:
                raise ValueError("expected unique CLASSNAME=path entries")
            reports[label] = json.loads(Path(path).read_text(encoding="utf-8"))
        result = shared_helpers(reports, minimum=args.min_groups)
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        p.exit(1, "SIN SHARED HELPER AUDIT FAILED: " + str(exc) + "\n")
    print(json.dumps({
        "dll": result["dll"], "groups": result["class_groups"],
        "distinct_shared_direct_callees": result["distinct_shared_direct_callees"],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
