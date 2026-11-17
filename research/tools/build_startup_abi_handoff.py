#!/usr/bin/env python3
"""Fail-closed, metadata-only startup ABI handoff for Windows runtime research.

No original binaries, instructions, proprietary C, or private paths are read.
Only pinned reviewed slot-zero return cleanup is treated as original evidence.
SDK 2004 prototypes remain separate hypotheses until independently corroborated.
"""
import argparse
import json
import re
from pathlib import Path

CONTRACTS = {
    "client.dll": {
        "interface": "VClient011", "slot": 0,
        "original_entry_va": "0x240FE0E0",
        "original_ret_imm_bytes": 12,
        "sdk2004_method_name_hypothesis": "Init",
        "sdk2004_argument_type_hypotheses": [
            "CreateInterfaceFn appSystemFactory",
            "CreateInterfaceFn physicsFactory",
            "CGlobalVarsBase* pGlobals",
        ],
        "sdk2004_first_engine_factory_query_hypothesis": "VEngineClient012",
    },
    "server.dll": {
        "interface": "ServerGameDLL003", "slot": 0,
        "original_entry_va": "0x221E7640",
        "original_ret_imm_bytes": 16,
        "sdk2004_method_name_hypothesis": "DLLInit",
        "sdk2004_argument_type_hypotheses": [
            "CreateInterfaceFn engineFactory",
            "CreateInterfaceFn physicsFactory",
            "CreateInterfaceFn fileSystemFactory",
            "CGlobalVars* pGlobals",
        ],
        "sdk2004_first_engine_factory_query_hypothesis": "VEngineServer021",
    },
}
ALLOWED_STATUS = {
    "missing_exact_ghidra_function", "unbounded_inferred_function",
    "bounded_inferred_function", "excessive_instruction_count",
}
VERSION = re.compile(
    r"(?:V[A-Za-z][A-Za-z0-9_]{1,48}|ServerGame[A-Za-z0-9_]{1,48}|GameClientExports)[0-9]{3}\\Z"
)


def handoff(census: dict) -> dict:
    if not isinstance(census, dict) or census.get("schema") != "sin-private-startup-factory-census-v1":
        raise ValueError("Wrong or missing private startup census schema")
    if census.get("original_instruction_bytes_or_pseudocode_embedded") is not False:
        raise ValueError("Unsafe original instruction or pseudocode payload")
    if census.get("independently_tested_retail_startup") is not False:
        raise ValueError("Unexpected retail startup claim")
    modules = census.get("modules")
    if not isinstance(modules, dict) or set(modules) != set(CONTRACTS):
        raise ValueError("Expected exactly two pinned modules")
    output = {}
    for module, contract in CONTRACTS.items():
        row = modules[module]
        if not isinstance(row, dict):
            raise ValueError("Invalid module census")
        for key in ("interface", "slot", "original_entry_va",
                    "sdk2004_method_name_hypothesis"):
            if row.get(key) != contract[key]:
                raise ValueError("Unreviewed original entry or SDK method mismatch")
        if row.get("ghidra_inferred_method_status") not in ALLOWED_STATUS:
            raise ValueError("Unrecognized Ghidra method status")
        for key in ("original_factory_argument_types_verified",
                    "original_first_failure_branch_verified",
                    "original_method_body_verified"):
            if row.get(key) is not False:
                raise ValueError("Unreviewed original method claim")
        literals = row.get("direct_string_reference_interface_version_candidates")
        if (not isinstance(literals, list) or len(literals) > 32 or
                any(not isinstance(x, str) or not VERSION.fullmatch(x) for x in literals) or
                literals != sorted(set(literals))):
            raise ValueError("Unsafe interface-version candidates")
        output[module] = {
            "interface": contract["interface"],
            "slot": 0,
            "reviewed_original_entry_va": contract["original_entry_va"],
            "reviewed_original_ret_imm_bytes": contract["original_ret_imm_bytes"],
            "reviewed_original_stack_argument_count":
                contract["original_ret_imm_bytes"] // 4,
            "original_factory_argument_types_verified": False,
            "original_factory_request_order_verified": False,
            "original_first_failure_branch_verified": False,
            "sdk2004_method_name_hypothesis":
                contract["sdk2004_method_name_hypothesis"],
            "sdk2004_argument_type_hypotheses":
                contract["sdk2004_argument_type_hypotheses"],
            "sdk2004_first_engine_factory_query_hypothesis":
                contract["sdk2004_first_engine_factory_query_hypothesis"],
            "ghidra_inferred_method_status": row["ghidra_inferred_method_status"],
            "ghidra_direct_string_reference_candidates": literals,
            "first_query_literal_observed_as_direct_reference":
                contract["sdk2004_first_engine_factory_query_hypothesis"] in literals,
            "next_owner_private_review":
                "Verify first actual indirect factory call, calling convention, "
                "and conditional failure edge in SHA-authenticated original; "
                "do not infer from SDK literal matches.",
        }
    return {
        "schema": "sin-startup-abi-runtime-handoff-v1",
        "evidence_tier": "reviewed_original_ret_cleanup_plus_separate_sdk2004_hypotheses",
        "original_binaries_or_pseudocode_embedded": False,
        "retail_engine_startup_verified": False,
        "modules": output,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--census", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    # Strict upper bound prevents accidentally ingesting arbitrary private dumps.
    if args.census.stat().st_size > 32768:
        raise ValueError("Census too large for bounded metadata-only handoff")
    result = handoff(json.loads(args.census.read_text(encoding="utf-8")))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    print("PASS: bounded client/server startup ABI handoff; no retail claims")


if __name__ == "__main__":
    main()
