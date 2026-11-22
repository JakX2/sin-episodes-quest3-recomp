#!/usr/bin/env python3
"""Fail-closed validation of *observed* Win32 Source game-DLL startup traces.

This is an evidence classifier, not an ABI implementation or an engine emulator.
Input is JSON Lines emitted by an independently instrumented owner-side process.
No original binaries, memory dumps, or proprietary payloads are accepted.
"""
import argparse
import json
import sys

INTERFACES = {
    "client": ("VClient011", "VClientPrediction001", "VClientEntityList003"),
    "server": ("ServerGameDLL003", "ServerGameClients003", "ServerGameEnts001"),
}
LIFECYCLE = (
    ("server", "ServerGameDLL003", "DLLInit"),
    ("client", "VClient011", "Init"),
)
ALLOWED = {"process_start", "module_load", "factory", "lifecycle_enter",
           "lifecycle_return", "entity_registered", "entity_spawned", "map_active"}


def validate(lines):
    """Return a deterministic report; never infer success from missing events."""
    errors, events, seen_ids = [], [], set()
    for number, raw in enumerate(lines, 1):
        if not raw.strip():
            continue
        try:
            item = json.loads(raw)
        except (ValueError, TypeError):
            errors.append(f"line {number}: invalid JSON")
            continue
        if not isinstance(item, dict) or item.get("event") not in ALLOWED:
            errors.append(f"line {number}: unsupported event")
            continue
        if type(item.get("seq")) is not int or item["seq"] < 0:
            errors.append(f"line {number}: invalid sequence")
            continue
        if item["seq"] in seen_ids or (events and item["seq"] <= events[-1]["seq"]):
            errors.append(f"line {number}: non-increasing sequence")
        seen_ids.add(item["seq"])
        events.append(item)
    if not events or events[0]["event"] != "process_start":
        errors.append("missing first process_start")
    if any(e.get("origin") != "retail_process" for e in events):
        errors.append("events must explicitly identify retail_process origin")
    if len({e.get("pid") for e in events}) != 1 or any(
        type(e.get("pid")) is not int or e["pid"] <= 0 for e in events
    ):
        errors.append("events must share one positive integer PID")
    if len([e for e in events if e["event"] == "process_start"]) != 1:
        errors.append("exactly one process_start required")
    loaded = set()
    factory = set()
    pending = {}
    returned = set()
    registered = set()
    spawned = set()
    active = False
    for e in events:
        kind = e["event"]
        module = e.get("module")
        interface = e.get("interface")
        if kind == "module_load":
            if module not in INTERFACES or e.get("pe32") is not True or e.get("result") != "ok":
                errors.append(f"seq {e['seq']}: invalid PE32 module load")
            else:
                loaded.add(module)
        elif kind == "factory":
            if module not in loaded or interface not in INTERFACES.get(module, ()) or e.get("result") != "ok":
                errors.append(f"seq {e['seq']}: invalid factory evidence")
            else:
                factory.add((module, interface))
        elif kind == "lifecycle_enter":
            key = (module, interface, e.get("method"))
            if key not in LIFECYCLE or (module, interface) not in factory or key in pending or key in returned:
                errors.append(f"seq {e['seq']}: invalid lifecycle entry")
            else:
                pending[key] = e["seq"]
        elif kind == "lifecycle_return":
            key = (module, interface, e.get("method"))
            if key not in pending or e.get("result") != "ok":
                errors.append(f"seq {e['seq']}: invalid lifecycle return")
            else:
                del pending[key]
                returned.add(key)
        elif kind == "entity_registered":
            classname = e.get("classname")
            if LIFECYCLE[0] not in returned or not isinstance(classname, str) or not classname.startswith("npc_"):
                errors.append(f"seq {e['seq']}: ungrounded entity registration")
            else:
                registered.add(classname)
        elif kind == "entity_spawned":
            classname = e.get("classname")
            if classname not in registered or e.get("result") != "ok":
                errors.append(f"seq {e['seq']}: ungrounded entity spawn")
            else:
                spawned.add(classname)
        elif kind == "map_active":
            if not all(k in returned for k in LIFECYCLE) or e.get("result") != "ok" or not isinstance(e.get("map"), str) or not e["map"]:
                errors.append(f"seq {e['seq']}: ungrounded map activation")
            else:
                active = True
    if pending:
        errors.append("unfinished lifecycle calls")
    missing = [f"{m}:{i}" for m, interfaces in INTERFACES.items()
               for i in interfaces if (m, i) not in factory]
    if missing:
        errors.append("missing factory interfaces: " + ", ".join(missing))
    if not all(k in returned for k in LIFECYCLE):
        errors.append("Init/DLLInit not both observed returning successfully")
    return {
        "schema": "sin-win32-startup-evidence-v1",
        "valid": not errors,
        "errors": sorted(set(errors)),
        "factory_interfaces": sorted(f"{m}:{i}" for m, i in factory),
        "lifecycle_returned": sorted(f"{m}:{i}:{fn}" for m, i, fn in returned),
        "registered_npcs": sorted(registered),
        "spawned_npcs": sorted(spawned),
        "map_active_observed": active,
        "retail_gameplay_boot_proven": False,
        "note": "Trace consistency is not independent verification of executable, original DLL provenance, ABI correctness, or gameplay.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", help="JSONL event trace; use - for stdin")
    args = parser.parse_args()
    if args.trace == "-":
        report = validate(sys.stdin)
    else:
        with open(args.trace, encoding="utf-8") as stream:
            report = validate(stream)
    print(json.dumps(report, sort_keys=True, indent=2))
    return 0 if report["valid"] else 1


if __name__ == "__main__":
    sys.exit(main())
