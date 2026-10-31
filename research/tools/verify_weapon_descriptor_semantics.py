#!/usr/bin/env python3
"""Phase 58: verify typedescription flags and vtable getter identity in user-owned SiN DLLs.

Specific to the SHA-256-pinned 2006 binary build. Checks field metadata, not
runtime field replication, save/load execution, or m_bReloadsSingly enabling.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import struct

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_registration_groups import REGISTRATIONS, field_records, read_c_string, inspect as previous_inspect

FIELD_SAVE = 0x0002
FIELD_KEY = 0x0004
FIELD_INSENDTABLE = 0x0100
FIELD_MODELINDEX = 0x0800
STRIDE = 52

# These pointers are the vtables of the build's RTTI-identified
# CBaseCombatWeapon / C_BaseCombatWeapon, not generic address guesses.
VTABLES = {
    "server": (0x2245A578, {0: (0x222AC4E0, 0x2251F7F4),
                              4: (0x222AC540, 0x2251F810)}),
    "client": (0x242845DC, {0: (0x24090EA0, 0x242F6260)}),
}
MAP_CLASSNAME = "CBaseCombatWeapon"


def descriptor_flags(pe: PE32, descriptor_va: int) -> dict:
    """The packed word at byte +16: fieldSize:u16; flags:u16, from typedescription_t."""
    pos = pe.va_to_offset(descriptor_va + 16)
    count, flags = struct.unpack_from("<HH", pe.content, pos)
    return {"field_size": count, "flags": flags}


def parse_group(pe: PE32, va: int, n: int) -> list[dict]:
    out = field_records(pe, va, n)
    for row in out:
        row.update(descriptor_flags(pe, int(row["descriptor_va"], 16)))
    return out


def check_counter(rows: list[dict], expected: dict[int, int], label: str) -> None:
    actual = Counter(e["flags"] for e in rows)
    if actual != expected:
        raise VerificationError(f"{label}: wrong flags distribution {dict(actual)!r} != {expected!r}")
    if any(e["field_size"] != 1 for e in rows):
        raise VerificationError(f"{label}: expected single-field descriptors")


def check_mirror(server: list[dict], client: list[dict]) -> None:
    if len(server) != len(client):
        raise VerificationError("prediction-like groups differ in length")
    for i, (s, c) in enumerate(zip(server, client)):
        keys = ("field", "type_id", "field_size", "flags")
        if any(s[k] != c[k] for k in keys):
            raise VerificationError(f"prediction-like group mismatch at row {i}")


def check_getter(pe: PE32, method_va: int, result_va: int) -> None:
    # mov eax, imm32; ret (the exact getter body in this original build)
    raw = pe.content[pe.va_to_offset(method_va):pe.va_to_offset(method_va) + 6]
    if raw != b"\xB8" + struct.pack("<I", result_va) + b"\xC3":
        raise VerificationError(f"getter at {method_va:#x} has unexpected code")
    # On-disk storage for these maps is zero-initialized until registration runs.
    # The class-name pointer at +8 is statically present and can be checked.
    array, count, nameptr, _base = struct.unpack_from("<IIII", pe.content, pe.va_to_offset(result_va))
    if read_c_string(pe, nameptr) != MAP_CLASSNAME or array != 0 or count != 0:
        raise VerificationError(f"getter at {method_va:#x} returned unexpected uninitialized map record")


def check_vtables(pes: dict[str, PE32]) -> dict:
    summary = {}
    for role, (vtbl_va, slots) in VTABLES.items():
        pe = pes[role]
        matched = {}
        for slot, (getter_va, datamap_va) in slots.items():
            stored = struct.unpack_from("<I", pe.content, pe.va_to_offset(vtbl_va + slot * 4))[0]
            if stored != getter_va:
                raise VerificationError(f"{role}: vtable slot {slot} points to {stored:#x}")
            check_getter(pe, getter_va, datamap_va)
            matched[str(slot)] = {"getter_va": hex(getter_va), "map_va": hex(datamap_va)}
        summary[role] = {"vtable_va": hex(vtbl_va), "slots": matched}
    return summary


def check_repeated_field(rows: list[dict]) -> None:
    values = [(x["index"], x["flags"]) for x in rows if x["field"] == "m_flTimeWeaponIdle"]
    if values != [(7, FIELD_INSENDTABLE), (13, FIELD_INSENDTABLE)]:
        raise VerificationError(f"unexpected duplicate m_flTimeWeaponIdle descriptors: {values}")


def verify(server_file: Path, client_file: Path) -> dict:
    # Includes hash, exact original array registration and tail initialization checks.
    baseline = previous_inspect(server_file, client_file)
    pes = {"server": PE32(server_file.read_bytes()), "client": PE32(client_file.read_bytes())}
    groups = {}
    for label, (role, array_va, _stub, count, _count, _ptr) in REGISTRATIONS.items():
        rows = parse_group(pes[role], array_va, count if label != "server_extended" else 32)
        groups[label] = rows
    expected = {FIELD_SAVE: 15, FIELD_INSENDTABLE: 12, FIELD_INSENDTABLE | FIELD_MODELINDEX: 2}
    check_counter(groups["server_matched"], expected, "server_matched")
    check_counter(groups["client_matched"], expected, "client_matched")
    check_counter(groups["server_extended"], {FIELD_SAVE: 30, FIELD_SAVE | FIELD_KEY: 2}, "server_extended")
    check_mirror(groups["server_matched"], groups["client_matched"])
    check_repeated_field(groups["server_matched"])
    check_repeated_field(groups["client_matched"])
    for label in groups:
        reloads = [x for x in groups[label] if x["field"] == "m_bReloadsSingly"]
        if len(reloads) != 1 or reloads[0]["flags"] != FIELD_SAVE:
            raise VerificationError(f"{label}: unexpected reload-field flags")
    ext = groups["server_extended"]
    if {x["field"] for x in ext if x["flags"] == FIELD_SAVE | FIELD_KEY} != {
        "m_overridePrimaryAmmoCount", "m_overrideSecondaryAmmoCount"
    }:
        raise VerificationError("unexpected extended KEY-flag fields")
    return {
        "result": "pass", "scope": "hash-pinned PE metadata plus virtual getter stubs",
        "sha256": {"server": baseline["server_sha256"], "client": baseline["client_sha256"]},
        "field_flag_counts": {label: {hex(k): v for k, v in sorted(Counter(x["flags"] for x in rows).items())}
                              for label, rows in groups.items()},
        "m_flTimeWeaponIdle": "two INSENDTABLE records at rows 7 and 13 in both matched groups",
        "m_bReloadsSingly": "SAVE flag in all three tables; no claim of active single-round reload",
        "virtual_getters": check_vtables(pes),
        "qualification": "Compiled table flags and getters identified. Their dynamic consumers and live save/replication behavior remain untraced."
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("server", type=Path, help="Original SiN server.dll")
    ap.add_argument("client", type=Path, help="Original SiN client.dll")
    a = ap.parse_args()
    try:
        print(json.dumps(verify(a.server, a.client), indent=2))
    except (OSError, VerificationError, struct.error) as exc:
        ap.exit(1, f"VERIFY FAILED: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
