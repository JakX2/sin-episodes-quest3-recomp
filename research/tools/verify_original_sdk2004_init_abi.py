#!/usr/bin/env python3
"""Verify selected *x86 stack-cleanup shapes* of owner SiN 2006 virtual methods.

Reads SHA256-pinned retail PE32 DLLs privately. No original instructions,
pseudocode, binary contents, or whole-method signatures are emitted.

This does not imply the selected Source SDK 2004 method *name*, parameter
types, implementation, or complete vtable are proven identical to SiN.
It confirms only that manually inspected anchored return instructions
at five reviewed virtual entries pop the SDK-compatible byte count.
"""
from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path

from recover_original_vtable_prefixes import MODULES, PE32, u16, verify_and_recover


@dataclass(frozen=True)
class Shape:
    module: str
    interface: str
    sdk2004_candidate: str
    expected_stack_pop: int
    return_sites: tuple[int, ...]
    max_entry_to_return: int
    slot: int = 0


# RET immediate sites were independently located by bounded GNU objdump
# of the SHA256-pinned original retail DLLs. These addresses are metadata;
# the program intentionally NEVER reproduces original opcode bytes.
SHAPES = (
    Shape("client.dll", "VClient011", "Init", 12,
          (0x240FE59B, 0x240FE5A1), 0x600),
    Shape("client.dll", "GameClientExports001", "~IBaseInterface", 4,
          (0x240FE9CC,), 0x80),
    Shape("server.dll", "ServerGameDLL003", "DLLInit", 16,
          (0x221E797F, 0x221E799E, 0x221E79A8), 0x400),
    Shape("server.dll", "ServerGameClients003", "GetPlayerLimits", 12,
          (0x221E640E,), 0x80),
    Shape("server.dll", "ServerGameEnts001", "~IServerGameEnts", 4,
          (0x221E937C,), 0x80),
)


def verify_module(blob: bytes, expected: dict, cases: tuple[Shape, ...]) -> list[dict]:
    """Authenticate full original + vptr/RTTI; inspect only explicitly pinned returns."""
    recovered = verify_and_recover(blob, expected)
    pe = PE32(blob, expected["base"])
    interfaces = {item["interface"]: item for item in recovered["interfaces"]}
    results = []
    seen = set()
    for shape in cases:
        key = (shape.interface, shape.slot)
        if key in seen:
            raise ValueError("Repeated virtual ABI shape")
        seen.add(key)
        if shape.interface not in interfaces:
            raise ValueError("SDK2004 ABI shape references an unreviewed interface")
        item = interfaces[shape.interface]
        if shape.slot >= item["prefix_slots"]:
            raise ValueError("SDK2004 ABI slot outside reviewed vtable prefix")
        entry = int(item["slots"][shape.slot]["target_va"], 16)
        if not 1 <= len(shape.return_sites) <= 16:
            raise ValueError("Invalid manually reviewed return site count")
        if not 0 <= shape.expected_stack_pop <= 255:
            raise ValueError("Unbounded x86 return-cleanup byte count")
        if len(shape.return_sites) != len(set(shape.return_sites)):
            raise ValueError("Duplicate reviewed x86 return site")
        for site in shape.return_sites:
            if site < entry or site-entry > shape.max_entry_to_return:
                raise ValueError("Return site does not belong to bounded virtual entry window")
            section, offset = pe.locate(site, 3)
            if not section.executable or section.name != ".text":
                raise ValueError("Reviewed x86 return site is not executable .text")
            # C2 iw (near RET with immediate stack cleanup), ordinary x86
            # instruction semantics. Return-site addresses were separately
            # established by a real disassembler; a byte match alone does not
            # independently prove an instruction boundary or whole function.
            if blob[offset] != 0xC2:
                raise ValueError("x86 reviewed return no longer RET imm16")
            observed = u16(blob, offset+1)
            if observed != shape.expected_stack_pop:
                raise ValueError("Original method stack cleanup contradicts SDK2004 candidate")
        results.append({
            "interface": shape.interface,
            "slot": shape.slot,
            "original_entry_va": f"0x{entry:08X}",
            "source_sdk_2004_method_name_hypothesis": shape.sdk2004_candidate,
            "x86_callee_stack_cleanup_bytes": shape.expected_stack_pop,
            "reviewed_return_instruction_count": len(shape.return_sites),
            "bounded_return_site_metadata": [f"0x{site:08X}" for site in shape.return_sites],
            "observed_stack_cleanup_matches_sdk_2004_candidate": True,
            "sdk_method_semantics_or_name_fully_verified": False,
            "sdk_argument_types_fully_verified_from_original": False,
            "original_method_body_reconstructed": False,
            "retail_engine_invocation_tested": False,
        })
    return results


def inspect_originals(original_dir: Path) -> dict:
    by_module = {}
    for module, spec in MODULES.items():
        cases = tuple(case for case in SHAPES if case.module == module)
        if not cases:
            raise ValueError("Original PE32 module lacks reviewed stack-cleanup cases")
        records = verify_module((original_dir / module).read_bytes(), spec, cases)
        by_module[module] = {
            "authenticated_original_sha256": spec["sha256"],
            "selected_virtual_method_entries": len(records),
            "retail_virtual_calls_executed": 0,
            "verified_stack_cleanup_shapes": records,
        }
    if sum(len(x["verified_stack_cleanup_shapes"]) for x in by_module.values()) != 5:
        raise ValueError("Incomplete 2004 SDK ABI startup priority set")
    return {
        "schema": "sin-original-sdk2004-x86-ret-stack-shape-v1",
        "evidence_tier": "full_SHA256_original_PE32_plus_original_return_instruction_stack_cleanup",
        "source_sdk_reference": "Source-SDK-Archives/source-sdk-2004@962ba835716d3e6d778054ab3fb4534bf3b35e1a",
        "instruction_bytes_or_pseudocode_embedded": False,
        "verified_original_cpp_method_signatures": 0,
        "original_reconstructed_complete_method_bodies": 0,
        "independent_retail_game_startup_tested": False,
        "modules": by_module,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = inspect_originals(args.original_dir)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    print("SIN_X86_SDK2004_ABI_STACK_SHAPES original_sha_gated="
          "5 ret_sites_checked=8 full_original_signatures_verified=0 "
          "real_engine_calls=0")


if __name__ == "__main__":
    main()
