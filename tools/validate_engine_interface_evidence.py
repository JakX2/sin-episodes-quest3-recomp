#!/usr/bin/env python3
"""Fail-closed, source-free review gate for engine interface ABI evidence.

Input is a *reviewed* JSON document, not raw Ghidra output. Never promotes
candidate addresses to authenticated functions or retail-tested interfaces.
Run --self-test without any proprietary binaries or external dependencies.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ANCHORS = (
    "VClient011", "ServerGameDLL003", "ServerGameClients003",
    "ServerGameEnts001", "VClientPrediction001", "VClientEntityList003",
)
MODULE_FOR_ANCHOR = {name: ("client" if name.startswith("VClient") else "server") for name in ANCHORS}
TIERS = ("candidate", "sha_reviewed", "native_synthetic", "owner_retail")
REQUIRED = ("interface", "module", "rva", "tier", "body_sha256",
            "boundary_verified", "native_engine_tested")
HEX = set("0123456789abcdef")


def validate(rows):
    if not isinstance(rows, list):
        raise ValueError("input must be a JSON array")
    errors, output, seen = [], [], set()
    for index, row in enumerate(rows):
        tag = "row %d" % index
        if not isinstance(row, dict):
            errors.append(tag + ": not an object")
            continue
        missing = [key for key in REQUIRED if key not in row]
        if missing:
            errors.append(tag + ": missing " + ",".join(missing))
            continue
        interface, module, rva, tier = (row[k] for k in
                                         ("interface", "module", "rva", "tier"))
        if type(interface) is not str or type(module) is not str or interface not in ANCHORS or module not in ("client", "server"):
            errors.append(tag + ": unknown interface or module")
            continue
        if module != MODULE_FOR_ANCHOR[interface]:
            errors.append(tag + ": interface registered in wrong DLL module")
            continue
        unknown = sorted(set(row) - set(REQUIRED))
        if unknown:
            errors.append(tag + ": unknown evidence fields " + ",".join(unknown))
            continue
        if type(rva) is not int or not 0 <= rva < 0x100000000:
            errors.append(tag + ": invalid PE32 RVA")
            continue
        if type(tier) is not str or tier not in TIERS:
            errors.append(tag + ": unknown evidence tier")
            continue
        if type(row["boundary_verified"]) is not bool or type(row["native_engine_tested"]) is not bool:
            errors.append(tag + ": evidence flags must be booleans")
            continue
        sha = row["body_sha256"]
        if sha is not None and (not isinstance(sha, str) or
                                len(sha) != 64 or any(c not in HEX for c in sha)):
            errors.append(tag + ": invalid lowercase SHA256")
            continue
        if tier != "candidate" and (not row["boundary_verified"] or sha is None):
            errors.append(tag + ": promoted without verified boundary and body hash")
            continue
        if tier == "candidate" and (row["boundary_verified"] or row["native_engine_tested"]):
            errors.append(tag + ": candidate contains promoted evidence flags")
            continue
        if tier == "owner_retail" and not row["native_engine_tested"]:
            errors.append(tag + ": retail tier lacks native engine test")
            continue
        if tier != "owner_retail" and row["native_engine_tested"]:
            errors.append(tag + ": native engine evidence assigned to lower tier")
            continue
        key = (module, interface, rva)
        if key in seen:
            errors.append(tag + ": duplicate module/interface/RVA")
            continue
        seen.add(key)
        output.append({k: row[k] for k in REQUIRED})
    output.sort(key=lambda x: (ANCHORS.index(x["interface"]), x["module"], x["rva"]))
    return errors, output


def report(rows):
    errors, reviewed = validate(rows)
    counts = {anchor: {tier: 0 for tier in TIERS} for anchor in ANCHORS}
    for row in reviewed:
        counts[row["interface"]][row["tier"]] += 1
    canonical = json.dumps(reviewed, sort_keys=True, separators=(",", ":")).encode()
    return {"schema": 1, "valid": not errors, "errors": errors,
            "reviewed_count": len(reviewed), "counts": counts,
            "reviewed_sha256": hashlib.sha256(canonical).hexdigest(),
            "entries": reviewed}


def self_test():
    good = {"interface": ANCHORS[0], "module": "client", "rva": 4096,
            "tier": "candidate", "body_sha256": None,
            "boundary_verified": False, "native_engine_tested": False}
    assert report([good])["valid"]
    try:
        json.loads('{"tier":"candidate","tier":"owner_retail"}',
                   object_pairs_hook=reject_duplicate_keys)
        raise AssertionError("duplicate JSON keys accepted")
    except ValueError:
        pass
    assert report([good, good])["valid"] is False
    assert not report([dict(good, module="server")])["valid"]
    assert not report([dict(good, interface="ServerGameDLL003")])["valid"]
    assert not report([dict(good, original_engine_tested=True)])["valid"]
    assert not report([dict(good, native_engine_tested=1)])["valid"]
    for field in ("interface", "module", "tier"):
        for malformed in ([], {}, 0, None):
            assert not report([dict(good, **{field: malformed})])["valid"]
    assert report([dict(good, tier="sha_reviewed")])["valid"] is False
    reviewed = dict(good, tier="sha_reviewed", body_sha256="a" * 64,
                    boundary_verified=True)
    assert report([reviewed])["valid"]
    assert not report([dict(reviewed, tier="owner_retail")])["valid"]
    assert report([dict(reviewed, tier="owner_retail",
                            native_engine_tested=True)])["valid"]
    assert report([dict(good, rva=True)])["valid"] is False
    assert report([dict(good, body_sha256="BAD")])["valid"] is False
    assert report([dict(good, boundary_verified="false")])["valid"] is False
    second = dict(good, interface=ANCHORS[1], module="server")
    assert report([good, second])["reviewed_sha256"] == report([second, good])["reviewed_sha256"]


def reject_duplicate_keys(pairs):
    """Reject ambiguous JSON objects rather than silently keeping the last key."""
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key: " + key)
        result[key] = value
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path, help="reviewed JSON array")
    parser.add_argument("--output", type=Path, help="deterministic JSON report path")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("interface evidence gate: self-test passed")
        return 0
    if args.input is None:
        parser.error("input JSON required unless --self-test")
    try:
        data = json.loads(args.input.read_text(encoding="utf-8"),
                          object_pairs_hook=reject_duplicate_keys)
        result = report(data)
    except (OSError, ValueError, TypeError) as exc:
        print("interface evidence gate: " + str(exc), file=sys.stderr)
        return 2
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    return 0 if result["valid"] else 1


if __name__ == "__main__":
    sys.exit(main())
