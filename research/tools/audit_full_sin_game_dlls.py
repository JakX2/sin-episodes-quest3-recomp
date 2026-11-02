#!/usr/bin/env python3
"""Enumerate original SiN x86 server entity registrations and client/server RTTI.

Read-only. Requires the exact user-owned 2006 game DLLs and optionally the
original weapon scripts ZIP. Reports metadata only; no DLL bytes or code dumps.
The scanner is exhaustive for ONE verified 22-byte registration call shape.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, FACTORIES, PEImage, VerificationError,
    REGISTRY_GETTER, verify_registrations,
)

CLASSNAME = re.compile(rb"[A-Za-z_][A-Za-z0-9_]{0,95}\Z")
RTTI = re.compile(rb"\.\?A[VU]([A-Za-z_$][A-Za-z0-9_@$?<>]*)@@\x00")
SCRIPT = re.compile(r"(?:^|/)(weapon_[a-zA-Z0-9_]+)\.txt$", re.I)
END = b"\x8b\xc8\xff\x12\xc3"


def scan_registrations(server: bytes, *, check_hash: bool = True,
                       verify_subset: bool = True) -> list[dict]:
    if check_hash and hashlib.sha256(server).hexdigest() != SERVER_SHA256:
        raise VerificationError("original 2006 SiN server.dll SHA-256 mismatch")
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected server PE image base")
    text = next((x for x in pe.sections if x[0] == ".text"), None)
    if text is None:
        raise VerificationError("missing .text")
    _, rva, vsize, raw, rawsize = text
    code = server[raw:raw + rawsize]
    out = []
    # Require all 22 instruction bytes, the known registry-getter call, and
    # correctly sectioned string and factory pointers. These are not mere hits
    # on a loose ASCII classname string.
    for mark in re.finditer(b"\x8b\x10\x68", code):
        pos = mark.start() - 5
        if pos < 0:
            continue
        op = code[pos:pos + 22]
        if (len(op) != 22 or op[0] != 0xe8 or
            op[5:8] != b"\x8b\x10\x68" or op[12] != 0x68 or
            op[17:] != END):
            continue
        va = pe.base + rva + pos
        target = va + 5 + struct.unpack_from("<i", op, 1)[0]
        if target != REGISTRY_GETTER:
            continue
        name_va = struct.unpack_from("<I", op, 8)[0]
        factory_va = struct.unpack_from("<I", op, 13)[0]
        if pe.section_of(name_va) != ".rdata" or pe.section_of(factory_va) != ".data":
            continue
        try:
            data = pe.at(name_va, 97)
            pe.at(factory_va, 4)
        except VerificationError:
            continue
        name = data.split(b"\x00", 1)[0]
        if not CLASSNAME.fullmatch(name) or not data.startswith(name + b"\x00"):
            continue
        out.append({
            "classname": name.decode("ascii"),
            "registration_va": f"0x{va:08x}",
            "name_va": f"0x{name_va:08x}",
            "factory_object_va": f"0x{factory_va:08x}",
        })
    for field in ("classname", "registration_va", "factory_object_va"):
        if len({entry[field] for entry in out}) != len(out):
            raise VerificationError(f"duplicate {field} in registration scan")
    if verify_subset:
        selected = verify_registrations(server, check_hash=check_hash)
        by_name = {entry["classname"]: entry for entry in out}
        for row in selected:
            actual = by_name.get(row["classname"])
            if actual is None or any(actual[field] != row[field]
                                    for field in ("registration_va", "name_va", "factory_object_va")):
                raise VerificationError(f"failed independent 32-entry cross-check: {row['classname']}")
    return out


def rtti_names(data: bytes) -> list[str]:
    return sorted({m.group(1).decode("ascii") for m in RTTI.finditer(data)})


def category(name: str) -> str:
    for prefix in ("npc_", "weapon_", "item_", "prop_", "trigger_", "logic_",
                   "func_", "env_", "ai_", "info_", "point_", "phys_",
                   "game_", "grenade_", "path_"):
        if name.startswith(prefix):
            return prefix[:-1]
    return "other"


def inspect_weapon_scripts(archive: bytes, registered: set[str]) -> dict:
    with zipfile.ZipFile(__import__("io").BytesIO(archive)) as source:
        names = sorted({m.group(1).lower() for entry in source.namelist()
                        if (m := SCRIPT.search(entry))})
    return {
        "script_weapon_names": names,
        "registered_weapon_names": sorted(n for n in registered if n.startswith("weapon_")),
        "script_only_not_in_this_factory_callshape": sorted(set(names) - registered),
        "registered_without_named_script": sorted(n for n in registered
                                                 if n.startswith("weapon_") and n not in names),
        "note": "A script-only name is not proof that the game never uses that weapon."
    }


def report(server: bytes, client: bytes, *, scripts: bytes | None = None,
           check_hash: bool = True) -> dict:
    if check_hash and hashlib.sha256(client).hexdigest() != CLIENT_SHA256:
        raise VerificationError("original 2006 SiN client.dll SHA-256 mismatch")
    registrations = scan_registrations(server, check_hash=check_hash)
    client_types, server_types = rtti_names(client), rtti_names(server)
    names = {row["classname"] for row in registrations}
    result = {
        "scope": "Static x86 factory registration CALL-SHAPE and MSVC RTTI metadata only",
        "sha256": {"server.dll": hashlib.sha256(server).hexdigest(),
                   "client.dll": hashlib.sha256(client).hexdigest()},
        "registry": {
            "known_registry_getter": f"0x{REGISTRY_GETTER:08x}",
            "selected_previous_crosscheck": len(FACTORIES),
            "matching_callshape_entries": len(registrations),
            "category_by_prefix": dict(sorted(Counter(category(n) for n in names).items())),
            "registrations": registrations,
        },
        "rtti": {
            "client_count": len(client_types),
            "server_count": len(server_types),
            "shared_count": len(set(client_types) & set(server_types)),
            "client_weapon_types": [n for n in client_types if n.startswith("C_Weapon")],
            "server_weapon_types": [n for n in server_types if n.startswith("CWeapon")],
            "client_types": client_types, "server_types": server_types,
        },
        "limitations": [
            "Registration count is exhaustive only for the known 22-byte code pattern, not all possible Source factory styles.",
            "RTTI presence does not prove gameplay use, instantiated objects or implementation semantics.",
            "Original x86 virtual addresses are not ARM64 ABI or recompilable C++ source.",
        ],
    }
    if scripts is not None:
        result["weapon_script_crosscheck"] = inspect_weapon_scripts(scripts, names)
    return result


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--client", required=True, type=Path)
    p.add_argument("--weapon-scripts-zip", type=Path,
                   help="Optional private original scripts.zip or SDK ZIP")
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    try:
        scripts = args.weapon_scripts_zip.read_bytes() if args.weapon_scripts_zip else None
        result = report(args.server.read_bytes(), args.client.read_bytes(), scripts=scripts)
    except (OSError, ValueError, struct.error, zipfile.BadZipFile) as exc:
        p.exit(1, f"SIN FULL REGISTRY AUDIT FAILED: {exc}\n")
    output = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
