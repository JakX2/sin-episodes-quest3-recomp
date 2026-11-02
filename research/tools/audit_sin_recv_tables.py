#!/usr/bin/env python3
"""Recover actual 2006 SiN client RecvTable constructors and property offsets.

Exact PE SHA-256 pinned. Decode bounded x86 instruction patterns, not bare
string inventories, and never assume any x86 offset is a Quest ARM64 offset.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, PEImage, VerificationError,
)

RECV_TABLE_CTOR = 0x24126A30
RECV_PROP_STRIDE = 0x3C
CLIENT_BASE = 0x24000000
NAME_RE = re.compile(rb"DT_[A-Za-z0-9_]{2,96}\x00")
PROPERTY_RE = re.compile(rb"[A-Za-z_][A-Za-z0-9_\[\].]{0,95}\x00")


def section_data(pe: PEImage, name: str) -> tuple[int, bytes]:
    for sect, rva, _, raw, size in pe.sections:
        if sect == name:
            if raw + size > len(pe.data):
                raise VerificationError(f"truncated {name} section")
            return pe.base + rva, pe.data[raw:raw + size]
    raise VerificationError(f"missing {name} section")


def strings_by_va(pe: PEImage, section: str, pattern: re.Pattern) -> dict[int, str]:
    base, data = section_data(pe, section)
    return {base + match.start(): match.group()[:-1].decode("ascii")
            for match in pattern.finditer(data)}


def scan_client_tables(client: bytes, *, check_hash: bool = True) -> list[dict]:
    if check_hash and hashlib.sha256(client).hexdigest() != CLIENT_SHA256:
        raise VerificationError("original client.dll SHA-256 mismatch")
    pe = PEImage(client)
    if pe.base != CLIENT_BASE:
        raise VerificationError("unexpected original SiN client PE image base")
    names = strings_by_va(pe, ".rdata", NAME_RE)
    codeva, code = section_data(pe, ".text")
    results = []
    for m in re.finditer(rb"\x68", code):
        i = m.start()
        if i + 22 > len(code):
            continue
        name_va = struct.unpack_from("<I", code, i + 1)[0]
        name = names.get(name_va)
        if name is None:
            continue
        j = i + 5
        if code[j] == 0x6A:
            count = code[j + 1]
            j += 2
        elif code[j] == 0x68:
            count = struct.unpack_from("<I", code, j + 1)[0]
            j += 5
        else:
            continue
        if (count > 256 or j + 15 > len(code) or code[j] != 0x68 or
            code[j + 5] != 0xB9 or code[j + 10] != 0xE8):
            continue
        props_va = struct.unpack_from("<I", code, j + 1)[0]
        obj_va = struct.unpack_from("<I", code, j + 6)[0]
        ctor_va = codeva + j + 15 + struct.unpack_from("<i", code, j + 11)[0]
        if ctor_va != RECV_TABLE_CTOR:
            continue
        if pe.section_of(props_va) != ".data" or pe.section_of(obj_va) != ".data":
            continue
        if props_va + count * RECV_PROP_STRIDE > 0x100000000:
            continue
        if count and pe.section_of(props_va + (count - 1) * RECV_PROP_STRIDE) != ".data":
            continue
        results.append({
            "table": name, "registration_va": f"0x{codeva+i:08x}",
            "property_count": count, "properties_va": f"0x{props_va:08x}",
            "recv_table_va": f"0x{obj_va:08x}",
            "constructor_va": f"0x{ctor_va:08x}",
            "_props": props_va,
        })
    if len({r["registration_va"] for r in results}) != len(results):
        raise VerificationError("duplicate client registration address")
    if check_hash:
        # Cross-check anchors independently found by direct disassembly.
        anchors = {
            "DT_BaseCombatWeapon": (7, 0x2432B79C),
            "DT_LocalWeaponData": (5, 0x2432B62C),
            "DT_LocalActiveWeaponData": (4, 0x2432B4FC),
            "DT_WeaponMagnum": (1, 0x2437EF84),
            "DT_HL2_Player": (6, 0x2437AA4C),
            "DT_NPC_GruntBase": (3, 0x2437BB74),
        }
        for name, (count, ptr) in anchors.items():
            if not any(r["table"] == name and r["property_count"] == count
                       and r["_props"] == ptr for r in results):
                raise VerificationError(f"original client nettable anchor mismatch: {name}")
    return results


def scan_client_properties(client: bytes, tables: list[dict]) -> tuple[list[dict], list[dict]]:
    pe = PEImage(client)
    codeva, code = section_data(pe, ".text")
    names = strings_by_va(pe, ".rdata", PROPERTY_RE)
    slots: dict[int, list[tuple[str, int, str]]] = {}
    for tbl in tables:
        base = tbl["_props"]
        for index in range(tbl["property_count"]):
            slots.setdefault(base + index * RECV_PROP_STRIDE, []).append(
                (tbl["table"], index, tbl["registration_va"]))
    recovered: dict[tuple[str, int, str], dict] = {}
    # Verified instruction shape in original:
    # push <property-name address>; push <RecvProp slot address>; call helper
    for m in re.finditer(rb"\x68", code):
        i = m.start()
        if i + 15 > len(code) or code[i+5] != 0x68 or code[i+10] != 0xE8:
            continue
        prop_name_va = struct.unpack_from("<I", code, i+1)[0]
        prop_name = names.get(prop_name_va)
        if not prop_name:
            continue
        slot_va = struct.unpack_from("<I", code, i+6)[0]
        if slot_va not in slots:
            continue
        helper_va = codeva + i + 15 + struct.unpack_from("<i", code, i+11)[0]
        if pe.section_of(helper_va) != ".text":
            continue
        # Immediate directly before property-name push supplies the field
        # offset for the observed original constructors. Other cases remain
        # unresolved rather than guessing.
        offset = None
        if i >= 5 and code[i-5] == 0x68:
            offset = struct.unpack_from("<I", code, i-4)[0]
        elif i >= 2 and code[i-2] == 0x6A:
            offset = code[i-1]
        if offset is not None and offset > 0x10000:
            offset = None
        for table_name, slot_index, reg in slots[slot_va]:
            key = table_name, slot_index, reg
            value = {
                "table": table_name, "table_registration_va": reg,
                "slot": slot_index, "name": prop_name,
                "client_object_offset": offset,
                "property_constructor_va": f"0x{codeva+i:08x}",
                "property_helper_va": f"0x{helper_va:08x}",
            }
            if key in recovered and recovered[key] != value:
                raise VerificationError(f"multiple initialization candidates for {key}")
            recovered[key] = value
    missing = [{"table": name, "slot": index, "table_registration_va": reg}
               for addr, keys in slots.items() for name, index, reg in keys
               if (name, index, reg) not in recovered]
    return sorted(recovered.values(), key=lambda x: (x["table"],x["table_registration_va"],x["slot"])), missing


def server_dt_name_strings(server: bytes) -> list[str]:
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original server PE base")
    return sorted(set(strings_by_va(pe, ".rdata", NAME_RE).values()))


def analyze(client: bytes, server: bytes, *, check_hash: bool = True) -> dict:
    if check_hash and hashlib.sha256(server).hexdigest() != SERVER_SHA256:
        raise VerificationError("original server.dll SHA-256 mismatch")
    tables = scan_client_tables(client, check_hash=check_hash)
    fields, missing = scan_client_properties(client, tables)
    names = {row["table"] for row in tables}
    server_names = set(server_dt_name_strings(server))
    for row in tables:
        del row["_props"]
    key_fields = {
        table: [{k: v for k, v in row.items() if k in ("slot", "name", "client_object_offset", "property_helper_va")}
                for row in fields if row["table"] == table]
        for table in (
            "DT_LocalWeaponData", "DT_LocalActiveWeaponData", "DT_BaseCombatWeapon",
            "DT_HL2_Player", "DT_NPC_GruntBase", "DT_PropSinVehicle",
        )
    }
    return {
        "scope": "original 2006 x86 client RecvTable constructors/property initialization and server DT string intersection",
        "sha256": {"client.dll": hashlib.sha256(client).hexdigest(),
                   "server.dll": hashlib.sha256(server).hexdigest()},
        "client": {
            "constructor_va": f"0x{RECV_TABLE_CTOR:08x}",
            "recvprop_stride_bytes": RECV_PROP_STRIDE,
            "constructor_call_count": len(tables),
            "unique_recvtable_names": len(names),
            "declared_recvprop_slots": sum(t["property_count"] for t in tables),
            "recovered_named_recvprop_slots": len(fields),
            "unresolved_slots": missing,
            "tables": tables, "named_props": fields, "focus_tables": key_fields,
        },
        "server": {
            "distinct_DT_string_names": len(server_names),
            "name_strings": sorted(server_names),
            "warning": "Server DT_* names are string evidence ONLY: sendprop layouts not reconstructed in this step.",
        },
        "intersection": {
            "client_recvtable_and_server_string_name_count": len(names & server_names),
            "client_only_names": sorted(names - server_names),
            "server_only_names": sorted(server_names - names),
            "warning": "Missing name is not proof of absent behavior. Case and distinct compiler registration paths can differ.",
        },
        "limitations": [
            "Only specific verified x86 call patterns are decoded, not arbitrary generated RecvProp initializers.",
            "Client field offsets belong to the original PE32 object layout, NOT ARM64 structures.",
            "A matching DT name alone cannot prove matching wire-level SendProp and RecvProp types, quantization or proxies.",
            "No Quest client/server module, networked game session or original executable code is produced.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client", type=Path, required=True)
    p.add_argument("--server", type=Path, required=True)
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    try:
        result = analyze(args.client.read_bytes(), args.server.read_bytes())
    except (OSError, VerificationError, struct.error, ValueError) as exc:
        p.exit(1, f"SIN NETWORK TABLE AUDIT FAILED: {exc}\n")
    data = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(data, encoding="utf-8")
    else:
        print(data, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
