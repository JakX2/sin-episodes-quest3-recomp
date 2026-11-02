#!/usr/bin/env python3
"""Bounded direct-call and branch traversal in ORIGINAL 2006 SiN game DLLs.

GNU objdump is a linear x86 decoder. CFG results are intentionally provisional:
jump-table dispatch, mixed code/data, exception paths and virtual calls remain
unresolved. Original game binaries are read only and never copied to output.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from audit_sin_rtti_vtables import scan
from verify_gameplay_entity_registry import PEImage, VerificationError

LINE = re.compile(r"^\s*([0-9a-f]{8,}):\s+([a-z][\w.]*)\s*(.*)$")
TARGET = re.compile(r"^(?:0x)?([0-9a-f]{8,})(?:\s|$|<)")
IMM = re.compile(r"(?<![0-9a-z_])0x([0-9a-f]{7,8})(?![0-9a-z_])")
MAX_METHOD_BYTES = 16384
MAX_INSTRUCTIONS = 6000
MAX_FOLLOWUP_TARGETS = 512


def disassemble(path: Path) -> dict[int, tuple[str, str]]:
    process = subprocess.run(
        ["objdump", "-d", "-M", "intel", "--no-show-raw-insn", str(path)],
        capture_output=True, text=True, check=True, timeout=90,
    )
    decoded = {}
    for line in process.stdout.splitlines():
        m = LINE.match(line)
        if m:
            decoded[int(m[1], 16)] = m[2], m[3].strip()
    if not decoded:
        raise ValueError("GNU objdump did not decode an executable section")
    return decoded


def direct_target(operand: str) -> int | None:
    m = TARGET.match(operand)
    return int(m[1], 16) if m else None


def rdata_string(pe: PEImage, address: int) -> str | None:
    if pe.section_of(address) != ".rdata":
        return None
    # Read a bounded string in the original file. A pointer near the end of
    # .rdata need not have 160 mapped bytes, so attempt progressively less.
    for cap in (160, 64, 16):
        try:
            raw = pe.at(address, cap)
            break
        except VerificationError:
            continue
    else:
        return None
    value = raw.split(b"\x00", 1)[0]
    if not 4 <= len(value) <= 150 or not all(32 <= c < 127 for c in value):
        return None
    return value.decode("ascii")


def traverse(start: int, decoded: dict[int, tuple[str, str]],
             addresses: list[int], vtable_entries: set[int],
             *, byte_limit: int = MAX_METHOD_BYTES,
             instruction_limit: int = MAX_INSTRUCTIONS) -> dict:
    """Follow local branches and fallthrough until returns or explicit limits.

    The linear objdump next-instruction boundary can be incorrect in embedded
    data. Cross-vtable starts are not folded into the current function.
    """
    if start not in decoded:
        return {"start_va": f"0x{start:08X}", "error": "not a decoded instruction boundary"}
    pending = [start]
    visited: set[int] = set()
    calls: Counter[int] = Counter()
    references: set[int] = set()
    indirect: list[dict] = []
    unresolved: list[str] = []
    external: list[str] = []
    ends: Counter[str] = Counter()
    conditional_branches = 0
    exceeded = False
    while pending and len(visited) < instruction_limit:
        address = pending.pop()
        if address in visited:
            continue
        if address not in decoded:
            unresolved.append(f"no decoded instruction at 0x{address:08X}")
            continue
        if abs(address - start) > byte_limit:
            exceeded = True
            continue
        if address != start and address in vtable_entries:
            external.append(f"0x{address:08X}")
            continue
        visited.add(address)
        op, operand = decoded[address]
        if op in ("int3", "ud2", "hlt", "iret", "iretd", "retf") or op.startswith("ret"):
            ends[op] += 1
            continue
        if op == "call":
            target = direct_target(operand)
            if target is None:
                indirect.append({"site_va": f"0x{address:08X}", "operand": operand})
            elif target in decoded:
                calls[target] += 1
        if op in ("push", "mov", "lea"):
            references.update(int(m[1], 16) for m in IMM.finditer(operand))
        n = bisect_right(addresses, address)
        fallthrough = addresses[n] if n < len(addresses) else None
        if op in ("jmp", "jmpq"):
            target = direct_target(operand)
            if target is None:
                unresolved.append(f"indirect jump at 0x{address:08X}: {operand}")
            elif target in decoded:
                if abs(target-start) <= byte_limit and target not in vtable_entries:
                    pending.append(target)
                else:
                    external.append(f"0x{target:08X}")
            else:
                unresolved.append(f"unresolved jump at 0x{address:08X}")
            ends["unconditional_jump"] += 1
        elif op.startswith("j") or op in ("loop", "loope", "loopne", "jecxz"):
            conditional_branches += 1
            target = direct_target(operand)
            if target in decoded:
                if abs(target-start) <= byte_limit and target not in vtable_entries:
                    pending.append(target)
                else:
                    external.append(f"0x{target:08X}")
            else:
                unresolved.append(f"unresolved branch at 0x{address:08X}")
            if fallthrough is not None:
                pending.append(fallthrough)
        elif fallthrough is not None:
            pending.append(fallthrough)
    if pending:
        exceeded = True
    return {
        "start_va": f"0x{start:08X}",
        "reachable_instruction_count": len(visited),
        "conditional_branches": conditional_branches,
        "exit_kinds": dict(ends),
        "direct_callees": [
            {"va": f"0x{target:08X}", "count": count}
            for target, count in sorted(calls.items())
        ],
        "indirect_call_count": len(indirect),
        "indirect_calls": indirect[:64],
        "immediate_addresses": sorted(references),
        "unresolved_branches": unresolved[:48],
        "external_or_other_vtable_branch_targets": external[:48],
        "possibly_truncated": exceeded,
    }


def analyze(path: Path, side: str, focus: str, depth: int) -> dict:
    binary = path.read_bytes()
    inventory = scan(binary, side)  # original exact SHA-256 pin + PE checks
    pe = PEImage(binary)
    instructions = disassemble(path)
    addresses = sorted(instructions)
    entries = {int(fn, 16) for vtable in inventory["vtables"]
               for fn in vtable["functions"]}
    roots = {}
    for comparison in inventory["focus_override_comparisons"]:
        if comparison["derived_class"] != focus:
            continue
        for change in comparison["changed"]:
            target = int(change["child_va"], 16)
            roots.setdefault(target, []).append({
                "virtual_slot": change["slot"],
                "parent_class": comparison["base_class"],
                "parent_va": change["parent_va"],
            })
    if not roots:
        raise ValueError(f"no selected comparison for class {focus!r}; see focus_override_comparisons")
    level = set(roots)
    completed = set()
    layers = []
    for hop in range(depth + 1):
        current = sorted(level - completed)
        if len(current) > MAX_FOLLOWUP_TARGETS:
            raise ValueError("callee expansion exceeded safety limit")
        results = []
        next_level: set[int] = set()
        for address in current:
            record = traverse(address, instructions, addresses, entries)
            strings = []
            for string_address in record.pop("immediate_addresses", []):
                value = rdata_string(pe, string_address)
                if value is not None:
                    strings.append({"va": f"0x{string_address:08X}", "literal": value})
            record["referenced_strings"] = strings[:96]
            record["strings_truncated"] = len(strings) > 96
            record["vtable_origin"] = roots.get(address, [])
            results.append(record)
            if hop < depth:
                next_level.update(int(x["va"], 16) for x in record.get("direct_callees", []))
        completed.update(current)
        layers.append({"depth": hop, "targets": len(results), "functions": results})
        level = next_level
    return {
        "dll": side + ".dll", "sha256": hashlib.sha256(binary).hexdigest(),
        "focus_class": focus, "changed_vtable_targets": len(roots),
        "examined_unique_entry_addresses": len(completed),
        "layers": layers,
        "limitations": [
            "GNU objdump linear decoding is not an authoritative function boundary decoder.",
            "Only selected changed virtual entries and bounded direct-call descendants are inspected.",
            "Indirect/virtual calls, exception paths, switch tables and inlined code are unresolved.",
            "Source original Windows x86 addresses and pointers cannot be reused in Quest ARM64.",
            "This is original-function evidence, not complete C++ reconstruction.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--side", choices=["server", "client"], required=True)
    p.add_argument("--focus-class", required=True)
    p.add_argument("--depth", type=int, choices=[0, 1], default=1)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    try:
        data = analyze(a.binary, a.side, a.focus_class, a.depth)
        a.output.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        p.exit(1, "SIN ORIGINAL CFG AUDIT FAILED: " + str(exc) + "\n")
    print(json.dumps({
        "class": data["focus_class"],
        "virtual_seeds": data["changed_vtable_targets"],
        "unique_method_entries": data["examined_unique_entry_addresses"],
        "level_sizes": [x["targets"] for x in data["layers"]],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
