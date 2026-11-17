#!/usr/bin/env python3
"""Metadata-only work queue linking known original interface factories to Ghidra.

Inputs: Ghidra inferred starts and call references, plus a separately reviewed
original interface-registration manifest. Does NOT authenticate owner DLL bytes,
verify new original functions, recover vtables, or claim game ABI compatibility.
"""
from __future__ import annotations
import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path

ENGINE_PRIORITIES = {
    "ServerGameDLL003": 100,
    "VClient011": 100,
    "ServerGameClients003": 95,
    "VClientPrediction001": 95,
    "ServerGameEnts001": 90,
    "VClientEntityList003": 90,
    "GameMovement001": 85,
    "GameClientExports001": 85,
    "PlayerInfoManager002": 75,
}
MODULES = ("client", "server")


def va(raw):
    if isinstance(raw, int) and not isinstance(raw, bool):
        if 0 <= raw <= 0xFFFFFFFF:
            return raw
        raise ValueError("address outside PE32")
    if not isinstance(raw, str):
        raise ValueError("address must be hex text or integer")
    digits = raw.strip()
    if digits.lower().startswith("0x"):
        digits = digits[2:]
    if not 1 <= len(digits) <= 8 or any(c not in "0123456789abcdefABCDEF" for c in digits):
        raise ValueError("invalid PE32 hex address")
    return int(digits, 16)


def fmt(value):
    return f"0x{value:08X}"


def rows(path, required):
    with path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames or not set(required).issubset(reader.fieldnames):
            raise ValueError(f"{path.name}: required Ghidra columns missing")
        return list(reader)


def priority(name):
    if name in ENGINE_PRIORITIES:
        return ENGINE_PRIORITIES[name]
    if "_IMaterialProxy" in name:
        return 10
    return 50


def read_seed_report(root, name, registrations):
    path = root / (name + ".dll_factory_seed_results.csv")
    if not path.is_file():
        return None
    expected = {(item["interface"], va(item["factory_va"])) for item in registrations}
    found = {}
    for row in rows(path, ("entry", "interface", "outcome", "result_function_entry")):
        key = row["interface"], va(row["entry"])
        if key not in expected or key in found:
            raise ValueError(f"{name}: seed report contains unknown or duplicated interface factory")
        status = row["outcome"]
        if status not in ("already_inferred", "seeded_candidate", "non_executable",
                          "overlaps_another_inferred_function", "unresolved_candidate") and not status.startswith("seed_error_"):
            raise ValueError(f"{name}: invalid factory seed outcome")
        found[key] = status
    if set(found) != expected:
        raise ValueError(f"{name}: incomplete reviewed factory seed report")
    return found


def analyze_module(root, name, manifest):
    file_functions = root / (name + ".dll_functions.csv")
    file_callers = root / (name + ".dll_callers.csv")
    functions = {}
    for row in rows(file_functions, ("entry", "name", "prototype")):
        addr = va(row["entry"])
        if addr in functions:
            raise ValueError(f"{name}: duplicate inferred function start")
        functions[addr] = row

    incoming, outgoing = defaultdict(set), defaultdict(set)
    for row in rows(file_callers, ("function_entry", "caller_entry", "kind")):
        if not row["caller_entry"].strip():
            continue
        source, target = va(row["caller_entry"]), va(row["function_entry"])
        if "CALL" not in row["kind"].strip().upper():
            continue
        incoming[target].add(source)
        outgoing[source].add(target)

    records = manifest["interface_registrations"]
    if len(records) != manifest["static_registration_count"]:
        raise ValueError(f"{name}: interface-registration manifest count disagrees")
    seed_statuses = read_seed_report(root, name, records)
    seen_names = set()
    queue = []
    for reg in records:
        interface = reg["interface"]
        if interface in seen_names:
            raise ValueError(f"{name}: repeated interface name")
        seen_names.add(interface)
        factory, wrapper = va(reg["factory_va"]), va(reg["registration_va"])
        factory_row = functions.get(factory)
        wrapper_row = functions.get(wrapper)
        seed_outcome = (seed_statuses.get((interface, factory)) if seed_statuses is not None
                        else None)
        if seed_outcome in ("already_inferred", "seeded_candidate") and factory_row is None:
            raise ValueError(f"{name}: seed report claims recovered start absent from Ghidra atlas")
        queue.append({
            "interface": interface,
            "priority": priority(interface),
            "factory_va": fmt(factory),
            "registration_wrapper_va": fmt(wrapper),
            "factory_ghidra_inferred_start_match": factory_row is not None,
            "factory_seed_outcome": seed_outcome,
            "factory_auto_discovered_before_seeding": (
                seed_outcome == "already_inferred" if seed_statuses is not None else None),
            "factory_seeded_candidate_created": seed_outcome == "seeded_candidate",
            "wrapper_ghidra_inferred_start_match": wrapper_row is not None,
            "factory_inferred_signature": factory_row["prototype"] if factory_row else None,
            "factory_direct_caller_functions": len(incoming[factory]),
            "factory_direct_callee_functions": len(outgoing[factory]),
            "factory_direct_callee_addresses": [
                fmt(addr) for addr in sorted(outgoing[factory])[:12]
            ],
            "evidence": "previously reviewed registration VA + Ghidra metadata only",
            "original_function_verified_by_this_tool": False,
            "engine_vtable_verified_by_this_tool": False,
        })
    queue.sort(key=lambda x: (-x["priority"], -int(x["factory_ghidra_inferred_start_match"]),
                              -x["factory_direct_callee_functions"], x["interface"]))
    exported_factory = va(manifest["native_CreateInterface_va"])
    return {
        "module": name + ".dll",
        "ghidra_inferred_function_count": len(functions),
        "reviewed_interface_registrations": len(records),
        "createinterface_va": fmt(exported_factory),
        "createinterface_ghidra_inferred_start_match": exported_factory in functions,
        "matched_factory_inferred_starts": sum(x["factory_ghidra_inferred_start_match"] for x in queue),
        "factory_seed_report_available": seed_statuses is not None,
        "factory_auto_discovered_prior_to_seeding": sum(
            x["factory_auto_discovered_before_seeding"] is True for x in queue),
        "factory_seeds_created_as_candidates": sum(
            x["factory_seeded_candidate_created"] for x in queue),
        "matched_registration_wrapper_inferred_starts": sum(x["wrapper_ghidra_inferred_start_match"] for x in queue),
        "queue": queue,
    }


def build(ghidra_dir, manifest_path):
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    dlls = manifest.get("dlls")
    if not isinstance(dlls, dict) or any(name not in dlls for name in MODULES):
        raise ValueError("expected original client/server interface manifest")
    modules = {name: analyze_module(ghidra_dir, name, dlls[name]) for name in MODULES}
    return {
        "schema": 1,
        "purpose": "queue engine-to-DLL ABI work before low-priority material proxy analysis",
        "original_binary_rehashed_by_this_tool": False,
        "new_sha_verified_original_functions": 0,
        "new_independently_load_tested_original_functions": 0,
        "warning": "Post-seeding function starts are candidates, not independently inferred originals, recovered implementations, or verified virtual tables.",
        "modules": modules,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra-dir", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = build(args.ghidra_dir, args.manifest)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        name: {
            "inferred_functions": data["ghidra_inferred_function_count"],
            "interfaces": data["reviewed_interface_registrations"],
            "factory_starts_matched": data["matched_factory_inferred_starts"],
        } for name, data in result["modules"].items()
    }, indent=2))


if __name__ == "__main__":
    main()
