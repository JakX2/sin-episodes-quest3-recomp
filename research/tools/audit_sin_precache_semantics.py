#!/usr/bin/env python3
"""Verify original 2006 SiN server precache functions and concrete resource calls.

Distinguishes PrecacheOther, PrecacheModel and PrecacheScriptSound from actual
entity spawning, attacks and drops. The owner's original server.dll is read
locally, and only metadata is written. GNU objdump required for disassembly.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# Exact original 2006 x86 server.dll virtual function ranges. Ends are
# explicit; the adjacent Quadralex function starts without int3 padding.
FUNCTIONS = {
    "weapon_assault_rifle": (0x22313780, 0x2231379A),
    "prop_phone": (0x223027C0, 0x223028C7),
    "npc_sintek_grunt_shared_base": (0x2235DE00, 0x2235DF0E),
    "npc_quadralex": (0x223746B0, 0x223747C0),
}
HELPERS = {
    0x22287CB0: "entity",      # UTIL_PrecacheOther
    0x22149B70: "model",       # model precaching
    0x221B6170: "sound_script", # script sound precaching
}
# Original diagnostic literals are stored only as small text provenance
# anchors, NOT executable bytes or method source.
ORIGINAL_DIAGNOSTICS = {
    0x224561BC: b"NULL Ent in UTIL_PrecacheOther\n",
    0x224542A4: b"CreateEntityByName( %s, %d ) - CreateEdict failed.",
    0x2240BAFC: b"Attempting to precache model, but model name is NULL\n",
    0x2240BB34: b"Late precache of %s\n",
    0x22431544: b"PrecacheScriptSound '%s' failed, no such sound script entry\n",
}
LINE = re.compile(r"^\s*([a-f0-9]{8,}):\s+([a-z][a-z0-9.]*)\s*(.*)$")
IMM = re.compile(r"^0x([a-f0-9]+)$")


def disassemble(file: Path, start: int, end: int) -> list[tuple[int, str, str]]:
    if not shutil.which("objdump"):
        raise ValueError("GNU objdump is required")
    proc = subprocess.run(
        ["objdump", "-d", "-M", "intel", "--no-show-raw-insn",
         "--start-address=" + hex(start),
         "--stop-address=" + hex(end), str(file)],
        text=True, capture_output=True, check=True, timeout=15,
    )
    instructions = []
    for line in proc.stdout.splitlines():
        match = LINE.match(line)
        if match and start <= int(match[1], 16) < end:
            instructions.append((int(match[1], 16), match[2], match[3].strip()))
    return instructions


def immediate(operand: str) -> int | None:
    match = IMM.fullmatch(operand)
    return int(match[1], 16) if match else None


def printable_rdata(pe: PEImage, address: int | None) -> str | None:
    if address is None or pe.section_of(address) != ".rdata":
        return None
    for size in (160, 80, 32, 16):
        try:
            data = pe.at(address, size)
            break
        except VerificationError:
            continue
    else:
        return None
    literal = data.split(b"\x00", 1)[0]
    if not 3 <= len(literal) <= 160 or not all(32 <= c < 127 for c in literal):
        return None
    return literal.decode("ascii")


def precache_calls(pe: PEImage,
                   rows: list[tuple[int, str, str]]) -> list[dict]:
    calls = []
    for index, (va, op, operand) in enumerate(rows):
        if op != "call" or (callee := immediate(operand)) not in HELPERS:
            continue
        # In these verified entrypoints, the final immediate push in the
        # preceding five instructions is the classname/model/sound argument.
        # For dynamic arguments, report unresolved rather than guessing.
        recent_pushes = [
            immediate(arg) for _, prior_op, arg in rows[max(0, index-5):index]
            if prior_op == "push" and immediate(arg) is not None
        ]
        candidate = recent_pushes[-1] if recent_pushes else None
        literal = printable_rdata(pe, candidate)
        calls.append({
            "callsite_va": f"0x{va:08X}",
            "helper_va": f"0x{callee:08X}",
            "helper_kind": HELPERS[callee],
            "literal": literal,
            "literal_va": f"0x{candidate:08X}" if literal else None,
            "argument_status": "literal" if literal else "dynamic_or_unresolved",
        })
    return calls


def verify_origins(pe: PEImage, file: Path) -> None:
    for address, text in ORIGINAL_DIAGNOSTICS.items():
        if pe.at(address, len(text)) != text:
            raise VerificationError(
                f"original SiN diagnostic text changed at {address:#x}")
    # Confirm the call chain, rather than labeling by text alone:
    # UTIL_PrecacheOther -> CreateEntityByName -> registry getter;
    # temporary entity destruction; sound wrapper -> script precacher.
    edges = {
        0x22287CB0: (0x22266B00, 0x22286C10),
        0x22266B00: (0x2228B110,),
        0x221B6170: (0x221B5DB0,),
    }
    for function, callees in edges.items():
        rows = disassemble(file, function, function + 0x130)
        observed = {immediate(arg) for _, op, arg in rows if op == "call"}
        for target in callees:
            if target not in observed:
                raise VerificationError(
                    f"original helper call chain changed: {function:#x} -> {target:#x}")


def inspect(file: Path, *, check_hash: bool = True,
            check_origins: bool = True) -> dict:
    data = file.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if check_hash and digest != SERVER_SHA256:
        raise VerificationError("original SiN server.dll SHA-256 mismatch")
    pe = PEImage(data)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original server.dll image base")
    if check_origins:
        verify_origins(pe, file)
    classes = {}
    for name, (start, end) in FUNCTIONS.items():
        rows = disassemble(file, start, end)
        if not rows or rows[0][0] != start:
            raise VerificationError(f"no original instructions at {start:#x}")
        calls = precache_calls(pe, rows)
        classes[name] = {
            "original_code_range": [f"0x{start:08X}", f"0x{end:08X}"],
            "helper_call_count": len(calls),
            "known_literals": sum(x["literal"] is not None for x in calls),
            "callsites": calls,
        }
    return {
        "server_sha256": digest,
        "verified_original_helper_labels": {
            "0x22287CB0": "UTIL_PrecacheOther",
            "0x22266B00": "CreateEntityByName",
            "0x22286C10": "temporary entity removal/destruction path",
            "0x22149B70": "model precache",
            "0x221B6170": "PrecacheScriptSound wrapper",
        },
        "classes": classes,
        "limitations": [
            "Only four selected original precache entrypoints, not all DLL routines.",
            "Resource precaching is not an attack, spawn/drop condition or AI schedule.",
            "Dynamic model/sound arguments remain unresolved.",
            "Original x86 code addresses are not ARM64 function pointers.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    try:
        data = inspect(args.server)
        args.output.write_text(json.dumps(data, indent=2) + "\n",
                               encoding="utf-8")
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        p.exit(1, "SIN PRECACHE AUDIT FAILED: " + str(exc) + "\n")
    print(json.dumps({
        name: {"calls": row["helper_call_count"],
               "literal": row["known_literals"]}
        for name, row in data["classes"].items()
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
