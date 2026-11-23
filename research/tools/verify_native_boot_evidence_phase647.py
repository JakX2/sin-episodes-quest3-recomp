#!/usr/bin/env python3
"""Fail-closed structural check for independently captured SiN Windows boot traces.

The trace is untrusted input. A passing check does NOT authenticate binaries,
prove the process was retail, or replace independent original-game evidence.
"""
import argparse
import json
from pathlib import Path

REQUIRED = (
    ("process", "retail_executable_started"),
    ("server", "create_interface"),
    ("client", "create_interface"),
    ("server", "dll_init_success"),
    ("client", "init_success"),
    ("server", "level_init_success"),
    ("server", "engine_entity_spawned"),
)


def verify(events):
    errors = []
    if not events:
        return ["empty trace"]
    previous = -1
    observed = []
    for i, event in enumerate(events, 1):
        if not isinstance(event, dict):
            errors.append(f"line {i}: expected JSON object")
            continue
        seq = event.get("sequence")
        if type(seq) is not int or seq <= previous:
            errors.append(f"line {i}: sequence must strictly increase as an integer")
        else:
            previous = seq
        if event.get("source") != "retail_windows_process":
            errors.append(f"line {i}: no retail Windows process provenance")
        if event.get("forwarded_to_original") is not False:
            errors.append(f"line {i}: forwarding status is not explicitly false")
        if event.get("module") not in ("process", "client", "server"):
            errors.append(f"line {i}: invalid module")
        if not isinstance(event.get("event"), str):
            errors.append(f"line {i}: invalid event")
        if event.get("event") == "create_interface":
            expected = "ServerGameDLL003" if event.get("module") == "server" else "VClient011"
            if event.get("interface") != expected:
                errors.append(f"line {i}: unexpected interface name")
        observed.append((event.get("module"), event.get("event")))
    cursor = 0
    for milestone in REQUIRED:
        try:
            cursor = observed.index(milestone, cursor) + 1
        except ValueError:
            errors.append(f"missing or out-of-order milestone: {milestone[0]}/{milestone[1]}")
    first = events[0] if isinstance(events[0], dict) else {}
    for field in ("retail_executable_sha256", "client_sha256", "server_sha256"):
        value = first.get(field)
        if not isinstance(value, str) or len(value) != 64 or any(c not in "0123456789abcdef" for c in value):
            errors.append(f"missing valid lowercase SHA-256 field: {field}")
    return errors


def self_test():
    rows = []
    for n, (module, event) in enumerate(REQUIRED, 1):
        item = dict(sequence=n, source="retail_windows_process", forwarded_to_original=False,
                    module=module, event=event)
        if event == "create_interface":
            item["interface"] = "ServerGameDLL003" if module == "server" else "VClient011"
        rows.append(item)
    rows[0].update({k: "a" * 64 for k in
                    ("retail_executable_sha256", "client_sha256", "server_sha256")})
    assert not verify(rows)
    assert verify(rows[:-1])
    assert verify(list(reversed(rows)))
    assert verify([{**x, "forwarded_to_original": True} for x in rows])
    assert verify([{**x, "source": "synthetic_host"} for x in rows])
    assert verify([{**x, "sequence": 1} for x in rows])
    assert verify([{**x, "event": "unknown"} for x in rows])
    assert verify([{**x, "client_sha256": "unknown"} for x in rows])
    print("synthetic gate tests passed; no retail boot claim")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("trace", nargs="?", type=Path)
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.trace is None:
        p.error("trace required unless --self-test")
    try:
        events = [json.loads(line) for line in
                  args.trace.read_text(encoding="utf-8").splitlines() if line.strip()]
    except (ValueError, OSError) as exc:
        p.exit(2, f"Invalid trace: {exc}\n")
    errors = verify(events)
    if errors:
        for error in errors:
            print("FAIL:", error)
        return 1
    print("TRACE STRUCTURE PASS (not independent proof of retail execution or hash authenticity)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
