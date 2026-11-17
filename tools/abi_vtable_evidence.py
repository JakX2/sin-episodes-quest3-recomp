#!/usr/bin/env python3
"""Metadata-only vtable pointer review queue. Never credits inferred pointers as verified methods."""
import argparse
import json
from collections import defaultdict
from pathlib import Path

ANCHORS = ("VClient011", "ServerGameDLL003", "ServerGameClients003", "ServerGameEnts001", "VClientPrediction001", "VClientEntityList003")

def address(value):
    if not isinstance(value, str) or not value.startswith("0x"):
        raise ValueError("address must be canonical hex")
    n = int(value[2:], 16)
    if not 0 < n <= 0xffffffff or value != f"0x{n:08x}":
        raise ValueError("invalid PE32 address")
    return n

def reconcile(data):
    if data.get("schema") != "sin-vtable-evidence-v1" or not isinstance(data.get("observations"), list):
        raise ValueError("invalid schema")
    seen = set()
    groups = defaultdict(list)
    for r in data["observations"]:
        if not isinstance(r, dict):
            raise ValueError("invalid observation")
        module, interface, slot = (r.get(k) for k in ("module", "interface", "slot"))
        if module not in ("client.dll", "server.dll") or interface not in ANCHORS:
            raise ValueError("unknown module/interface")
        if type(slot) is not int or not 0 <= slot < 1024:
            raise ValueError("invalid slot")
        key = (module, interface, slot)
        if key in seen:
            raise ValueError("duplicate slot")
        seen.add(key)
        va = address(r.get("target_va"))
        provenance = r.get("provenance")
        if provenance not in ("original-pe-relocation", "ghidra-vtable-pointer"):
            raise ValueError("invalid provenance")
        reviewed = r.get("signature_reviewed")
        signature = r.get("signature")
        if type(reviewed) is not bool:
            raise ValueError("invalid review flag")
        if reviewed and (not isinstance(signature, str) or not signature.strip()):
            raise ValueError("reviewed signature requires text")
        if not reviewed and signature is not None:
            raise ValueError("unreviewed signature must be null")
        groups[(module, interface)].append({"slot": slot, "target_va": f"0x{va:08x}",
            "provenance": provenance, "tier": "signature-reviewed" if reviewed else "pointer-candidate",
            "signature": signature if reviewed else None})
    items = []
    for module, interface in sorted(groups):
        rows = sorted(groups[(module, interface)], key=lambda r: r["slot"])
        items.append({"module": module, "interface": interface, "observed_slots": len(rows),
            "reviewed_signatures": sum(r["tier"] == "signature-reviewed" for r in rows),
            "candidate_slots": sum(r["tier"] == "pointer-candidate" for r in rows), "slots": rows})
    return {"schema": "sin-vtable-review-queue-v1", "interfaces": items,
        "verified_original_methods_added": 0, "engine_load_tests_added": 0}

def main():
    p = argparse.ArgumentParser()
    p.add_argument("input", type=Path)
    p.add_argument("output", type=Path)
    a = p.parse_args()
    result = reconcile(json.loads(a.input.read_text(encoding="utf8")))
    a.output.write_text(json.dumps(result, sort_keys=True, indent=2) + "\n", encoding="utf8")

if __name__ == "__main__":
    main()
