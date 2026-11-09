#!/usr/bin/env python3
"""Build a searchable, conservative recovery atlas of BOTH original SiN DLLs.

Source inputs: exact SHA-pinned private 2006 PE32 DLLs, existing RTTI/vtable
JSON, whole-.text candidate CSVs, 22-byte entity-factory and InterfaceReg
audit JSON, and a tiny manually reviewed evidence ledger. All original DLL
bytes stay local. A CALL target is not a decompiled method.

The atlas preserves source evidence and labels discovery/reconstruction
separately; it does not claim coverage of all true native C++ functions.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re

from audit_sin_entire_dll_code_census import HASHES, CensusError, PE
from build_sin_dll_recovery_queue import GROUPS

SIDES = ("client", "server")
LEVELS = ("candidate_only", "machine_code_verified",
          "behavior_reconstructed", "engine_runtime_verified")
HEX_VA = re.compile(r"^0x[0-9a-fA-F]{8}$")
HEX_SHA = re.compile(r"^[0-9a-f]{64}$")


def fail(reason: str) -> None:
    raise CensusError("SiN recovery atlas: " + reason)


def validate_original_files(client: Path, server: Path) -> dict[str, bytes]:
    originals = {}
    for side, source in (("client", client), ("server", server)):
        original = source.read_bytes()
        if hashlib.sha256(original).hexdigest() != HASHES[side]:
            fail("incorrect original SHA256 for " + side + ".dll")
        originals[side] = original
    return originals


def check_code_region(text_va: int, text: bytes, entry_va: str,
                      length: int, digest: str) -> None:
    """Pin exact original bytes; a nearby string or vtable is not enough."""
    address = int(entry_va, 16)
    if not isinstance(length, int) or not 1 <= length <= 16384:
        fail("invalid original function code length at " + entry_va)
    offset = address - text_va
    if offset < 0 or offset + length > len(text):
        fail("verified function body outside original .text: " + entry_va)
    actual = hashlib.sha256(text[offset:offset+length]).hexdigest()
    if actual != digest:
        fail("original function-region SHA256 mismatch at " + entry_va)


def verify_manifest_against_original_code(
        originals: dict[str, bytes], manifest: dict) -> None:
    texts = {}
    for side in SIDES:
        image = PE(originals[side])
        expected_base = 0x24000000 if side == "client" else 0x22000000
        if image.base != expected_base:
            fail("incorrect original PE32 load base for " + side)
        texts[side] = image.section(".text")
    for method in manifest["methods"]:
        side = method["dll"]
        if side not in SIDES:
            fail("unknown verified method owner")
        va, text = texts[side]
        check_code_region(va, text, method["entry_va"],
                          method["original_code_length"],
                          method["original_code_sha256"])


def check_side_metadata(rtti: dict, census: dict, factory: dict, manifest: dict) -> None:
    if manifest.get("schema_version") != 1 or manifest.get("dll_sha256") != HASHES:
        fail("evidence manifest does not match both original DLL hashes")
    for side in SIDES:
        if rtti.get(side, {}).get("sha256") != HASHES[side]:
            fail("RTTI mismatch for " + side)
        if census.get(side, {}).get("sha256") != HASHES[side]:
            fail("whole-code census mismatch for " + side)
    if factory.get("sha256") != {side+".dll": HASHES[side] for side in SIDES}:
        fail("factory metadata mismatch for the original DLLs")


def va_key(va: str, side: str) -> str:
    if not isinstance(va, str) or HEX_VA.fullmatch(va) is None:
        fail("not a complete eight-digit VA: " + repr(va))
    value = int(va, 16)
    base = 0x24000000 if side == "client" else 0x22000000
    if not base <= value < base + 0x10000000:
        fail("VA outside expected original image range: " + va)
    return f"0x{value:08X}"


def load_candidates(side: str, rows: list[dict], expected: int) -> dict[str, dict]:
    found = {}
    for row in rows:
        va = va_key(row["entry_va"], side)
        if va in found:
            fail("duplicate " + side + " candidate " + va)
        seed = row.get("seed_evidence", "")
        if seed not in ("vtable", "direct_call", "vtable+direct_call"):
            fail("unknown candidate evidence: " + str(seed))
        try:
            calls = int(row.get("direct_callsite_count", "0"))
            jumps = int(row.get("direct_jump_to_entry_count", "0"))
        except (TypeError, ValueError) as exc:
            raise CensusError("malformed callsite count") from exc
        if calls < 0 or jumps < 0:
            fail("negative callsite count")
        found[va] = {
            "dll": side+".dll", "entry_va": va,
            "census_candidate": True,
            "seed_evidence": seed,
            "class_hints": [x.strip() for x in
                            row.get("owner_examples", "").split("|") if x.strip()],
            "direct_callsite_count": calls,
            "direct_jump_to_entry_count": jumps,
            "nearby_string_hints": [x.strip() for x in
                                    row.get("nearby_string_examples", "").split("|")
                                    if x.strip()],
            "recovery_status": "candidate_only",
            "label": "", "owner_verified": "", "implementation_path": "",
            "test_path": "", "evidence_path": "", "original_code_sha256": "",
        }
    if len(found) != expected:
        fail(side + " census row count does not match pinned summary: " +
             str(len(found)) + " vs " + str(expected))
    return found


def merge_verified(functions: dict, manifest: dict, repository_root: Path) -> dict:
    manually_verified = defaultdict(int)
    outside_census = defaultdict(int)
    evidence_seen = set()
    for row in manifest.get("methods", []):
        side = row.get("dll")
        if side not in SIDES:
            fail("unknown verified DLL " + str(side))
        va = va_key(row.get("entry_va"), side)
        key = (side, va)
        if key in evidence_seen:
            fail("duplicate manually verified function " + str(key))
        evidence_seen.add(key)
        level = row.get("recovery_status")
        if level not in LEVELS[1:]:
            fail("unsupported manual recovery status " + str(level))
        code_sha = row.get("original_code_sha256", "")
        if not HEX_SHA.fullmatch(code_sha):
            fail("verified code requires a 64-character SHA256 region pin")
        if not isinstance(row.get("original_code_length"), int) or not (
                1 <= row["original_code_length"] <= 16384):
            fail("verified code requires a precise original code length")
        if not isinstance(row.get("label"), str) or not row["label"].strip():
            fail("verified method missing meaningful label")
        evidence_path = row.get("evidence_path", "")
        if not evidence_path or not (repository_root / evidence_path).is_file():
            fail("verified evidence doc missing: " + str(evidence_path))
        implementation = row.get("implementation_path", "")
        test = row.get("test_path", "")
        if level in ("behavior_reconstructed", "engine_runtime_verified"):
            if not implementation or not test:
                fail("reconstructed code must have both implementation and test path")
            if not (repository_root / implementation).is_file():
                fail("implementation does not exist: " + implementation)
            if not (repository_root / test).is_file():
                fail("test does not exist: " + test)
        if level == "engine_runtime_verified":
            fail("a real game-engine run log must be manually audited before that status")
        by_va = functions[side]
        if va not in by_va:
            outside_census[side] += 1
            by_va[va] = {
                "dll": side+".dll", "entry_va": va,
                "census_candidate": False, "seed_evidence": "manual_original_code",
                "class_hints": [], "direct_callsite_count": 0,
                "direct_jump_to_entry_count": 0, "nearby_string_hints": [],
                "recovery_status": "candidate_only", "label": "",
                "owner_verified": "", "implementation_path": "", "test_path": "",
                "evidence_path": "", "original_code_sha256": "",
            }
        record = by_va[va]
        record.update({
            "recovery_status": level,
            "label": row["label"].strip(),
            "owner_verified": row.get("owner", ""),
            "original_code_sha256": code_sha,
            "implementation_path": implementation,
            "test_path": test,
            "evidence_path": evidence_path,
        })
        manually_verified[side] += 1
    return {
        "evidence_ledger_entries": dict(manually_verified),
        "verified_entries_not_found_by_census": dict(outside_census),
    }


def class_catalog(rtti: dict) -> list[dict]:
    all_rows = []
    for side in SIDES:
        meta = rtti[side]
        tables = defaultdict(list)
        for vt in meta["vtables"]:
            tables[vt["class"]].append(vt)
        classes = {
            x["class"] for x in meta.get("types", [])
            if isinstance(x.get("class"), str) and x["class"]
        } | set(meta.get("hierarchies", {})) | set(tables)
        for name in sorted(classes):
            matches = tables[name]
            # The per-class vtable set may contain repeated/inherited targets.
            distinct = set()
            slots = 0
            for table in matches:
                methods = table.get("functions", table.get("function_va", []))
                slots += int(table.get("slot_count", len(methods)))
                distinct.update(methods)
            groups = [system for system, selected in GROUPS[side].items()
                      if name in selected]
            all_rows.append({
                "dll": side+".dll",
                "rtti_class": name,
                "has_verified_hierarchy": name in meta.get("hierarchies", {}),
                "vtable_count": len(matches),
                "vtable_slot_references": slots,
                "distinct_virtual_targets": len(distinct),
                "prioritized_system_groups": "|".join(groups),
                "semantic_status": "rtti_identity_and_virtual_metadata_only",
            })
    return all_rows


def assemble(rtti: dict, census: dict, candidates: dict[str, list[dict]],
             factories: dict, manifest: dict, repo_root: Path) -> dict:
    check_side_metadata(rtti, census, factories, manifest)
    methods = {}
    for side in SIDES:
        expected = census[side]["counts"]["union_entrypoint_candidates"]
        methods[side] = load_candidates(side, candidates[side], expected)
    manual_counts = merge_verified(methods, manifest, repo_root)
    classes = class_catalog(rtti)
    result_methods = sorted(
        (row for side in SIDES for row in methods[side].values()),
        key=lambda row: (row["dll"], int(row["entry_va"], 16)))
    interface_rows = []
    fact = factories["actual_createinterface_registrations"]
    for side in SIDES:
        for row in fact[side]:
            interface_rows.append({
                "dll": side+".dll", "name": row["name"],
                "registration_va": row["registration_va"],
                "factory_callback_va": row["factory_callback_va"],
                "evidence": "original_InterfaceReg_constructor_call",
            })
    entities = []
    for row in factories["registry"]["registrations"]:
        entities.append({
            "dll": "server.dll",
            "classname": row["classname"],
            "registration_va": row["registration_va"],
            "factory_object_va": row["factory_object_va"],
            "evidence": "verified_22_byte_compiler_registration_pattern",
        })
    counts = {}
    for side in SIDES:
        statuses = Counter(row["recovery_status"] for row in methods[side].values())
        counts[side] = {
            "census_entrypoint_candidates": len(candidates[side]),
            "documented_additional_verified_entries": sum(
                not row["census_candidate"] for row in methods[side].values()),
            "manually_reviewed_original_code_entries":
                statuses["machine_code_verified"] + statuses["behavior_reconstructed"],
            "behavior_reconstructed_entries_in_this_ledger":
                statuses["behavior_reconstructed"],
            "engine_runtime_verified": 0,
            "candidate_only": statuses["candidate_only"],
            "rtti_types_or_classes_in_catalog": sum(
                row["dll"] == side+".dll" for row in classes),
            "rtti_classes_with_hierarchy":
                rtti[side]["counts"]["classes_with_hierarchy"],
            "vtable_count": rtti[side]["counts"]["distinct_vftables"],
            "interface_registration_count": sum(
                row["dll"] == side+".dll" for row in interface_rows),
        }
    counts["server"]["recognized_entity_factory_registrations"] = len(entities)
    summary = {
        "schema_version": 1,
        "original_dll_sha256": HASHES,
        "counts": counts,
        "method_ledger": manual_counts,
        "full_decompilation_percentage": None,
        "census_is_not_an_exhaustive_function_list": True,
        "recovery_ledger_is_not_exhaustive_of_previous_research": True,
        "source_files": [
            "research/tools/audit_sin_entire_dll_code_census.py",
            "research/tools/audit_sin_rtti_vtables.py",
            "research/tools/audit_full_sin_game_dlls.py",
        ],
        "limitations": [
            "Census combines vtable pointer targets and linearly decoded CALL targets; not all are functions.",
            "Some original function entrypoints are absent from the census; do not infer coverage percentage.",
            "Class catalog contains RTTI metadata, not class methods or sources.",
            "Nearby strings are hints, not function identities.",
            "The manual verified-method list is a reviewed sample, not an exhaustive audit of older phases.",
            "No game-engine runtime, PC client/server replacement or Android/Quest port is proven.",
        ],
    }
    return {"summary": summary, "classes": classes, "functions": result_methods,
            "entities": entities, "interfaces": interface_rows}


def csv_rows(path: Path, rows: list[dict], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            formatted = {k: " | ".join(row[k]) if isinstance(row.get(k), list)
                         else row.get(k, "") for k in fields}
            writer.writerow(formatted)


def write_catalog(out_dir: Path, atlas: dict) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir/"SiN_2006_Both_DLLs_Recovery_Atlas.json").write_text(
        json.dumps(atlas["summary"], indent=2, sort_keys=True)+"\n", encoding="utf-8")
    layouts = {
        "functions": ("SiN_2006_Function_Recovery_Atlas.csv",
            ["dll","entry_va","census_candidate","seed_evidence","class_hints",
             "direct_callsite_count","nearby_string_hints","recovery_status",
             "owner_verified","label","original_code_sha256",
             "implementation_path","test_path","evidence_path"]),
        "classes": ("SiN_2006_RTTI_Class_Atlas.csv",
            ["dll","rtti_class","has_verified_hierarchy","vtable_count",
             "vtable_slot_references","distinct_virtual_targets",
             "prioritized_system_groups","semantic_status"]),
        "entities": ("SiN_2006_Server_Entity_Factory_Atlas.csv",
            ["dll","classname","registration_va","factory_object_va","evidence"]),
        "interfaces": ("SiN_2006_Engine_Interface_Atlas.csv",
            ["dll","name","registration_va","factory_callback_va","evidence"]),
    }
    for kind,(file_name,fields) in layouts.items():
        csv_rows(out_dir/file_name, atlas[kind], fields)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client", type=Path, required=True,
                    help="Exact owner-held original client.dll")
    ap.add_argument("--server", type=Path, required=True,
                    help="Exact owner-held original server.dll")
    ap.add_argument("--rtti", type=Path, required=True,
                    help="JSON from audit_sin_rtti_vtables.py")
    ap.add_argument("--census-dir", type=Path, required=True,
                    help="Directory from audit_sin_entire_dll_code_census.py")
    ap.add_argument("--factories", type=Path, required=True,
                    help="JSON from audit_full_sin_game_dlls.py")
    ap.add_argument("--verified", type=Path,
                    default=Path("research/catalog/original_dll_verified_methods.json"))
    ap.add_argument("--repo-root", type=Path, default=Path("."))
    ap.add_argument("--output-dir", type=Path, required=True)
    args = ap.parse_args()
    try:
        originals = validate_original_files(args.client,args.server)
        rtti = json.loads(args.rtti.read_text(encoding="utf-8"))
        factories = json.loads(args.factories.read_text(encoding="utf-8"))
        manifest = json.loads(args.verified.read_text(encoding="utf-8"))
        verify_manifest_against_original_code(originals, manifest)
        census = {}
        candidates = {}
        combined = json.loads(
            (args.census_dir/"SiN_2006_DLL_Code_Census_Summary.json").read_text(
                encoding="utf-8"))
        for side in SIDES:
            # Summary produced by existing scanner includes full hash/counts.
            census[side] = {"sha256":combined[side]["sha256"],
                            "counts": {"union_entrypoint_candidates":
                                       combined[side]["union_entrypoint_candidates"]}}
            with (args.census_dir/(
                "SiN_2006_"+side.capitalize()+"_Code_Entry_Candidates.csv")).open(
                    newline="",encoding="utf-8") as handle:
                candidates[side] = list(csv.DictReader(handle))
        atlas = assemble(rtti,census,candidates,factories,manifest,args.repo_root)
        write_catalog(args.output_dir, atlas)
        print(json.dumps(atlas["summary"],indent=2,sort_keys=True))
    except (OSError,ValueError,KeyError,TypeError) as error:
        ap.exit(1, "SiN recovery atlas failed: "+str(error)+"\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
