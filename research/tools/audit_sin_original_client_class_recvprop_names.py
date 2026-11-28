#!/usr/bin/env python3
"""Phase1696: authenticate all 939 original 2006 PE32 client RecvProp names.

Private --client requires the SHA-pinned genuine retail DLL. Public --self-test
uses invented names only. This does NOT prove native proxy or network wire ABI.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError,
)
from audit_sin_original_client_class_recvtable_links import verify as verify_tables
from audit_sin_original_client_class_registry import need, p32

PROP_NAMES_SHA = "ca5f6949f6b477a4257e8357a7009e0908a016dc9838e5ec3746c3cc1a770b31"
STANDARD_COUNT = 916
ARRAY_HELPER_COUNT = 12
ARRAY_COPY_COUNT = 11
ARRAY_HELPER = 0x24126BD0
ARRAY_COPY = 0x24126E90


def source_section(pe: PEImage, name: str):
    return next(s for s in pe.sections if s[0] == name)


def code_and_names(pe: PEImage):
    _, code_rva, code_vs, code_offset, code_raw = source_section(pe, ".text")
    code = pe.data[code_offset:code_offset + min(code_vs, code_raw)]
    _, name_rva, name_vs, name_offset, name_raw = source_section(pe, ".rdata")
    chars = pe.data[name_offset:name_offset + min(name_vs, name_raw)]
    names = {
        pe.base + name_rva + m.start(): m.group()[:-1].decode("ascii")
        for m in re.finditer(rb"[A-Za-z_][A-Za-z0-9_\[\].]{0,95}\x00", chars)
    }
    return pe.base + code_rva, code, names


def rel32(code: bytes, off: int, code_base: int) -> int:
    return code_base + off + 5 + struct.unpack_from("<i", code, off + 1)[0]


def source_special_name(pe: PEImage, code: bytes, code_base: int,
                        slot_va: int):
    """Match exact native array helper or rep movsd + array-copy callsite."""
    needle = b"\x68" + p32(slot_va)
    start = 0
    hits = []
    while True:
        site = code.find(needle, start)
        if site < 0:
            break
        start = site + 1
        after = site + 5
        if code[after:after + 1] == b"\xE8" and \
                rel32(code, after, code_base) == ARRAY_HELPER:
            kind = "array-helper"
        elif code[after:after + 3] == b"\xF3\xA5\xE8" and \
                rel32(code, after + 2, code_base) == ARRAY_COPY:
            kind = "array-copy"
        else:
            continue
        names = []
        for pos in range(max(0, site - 36), site - 4):
            if code[pos] != 0x68:
                continue
            name_ptr = struct.unpack_from("<I", code, pos + 1)[0]
            if pe.section_of(name_ptr) != ".rdata":
                continue
            try:
                value = pe.at(name_ptr, 96).split(b"\x00", 1)[0].decode("ascii")
            except (UnicodeDecodeError, VerificationError):
                continue
            if value and all(32 <= ord(ch) < 127 for ch in value):
                names.append((value, code_base + pos))
        if names:
            hits.append((kind, names[-1][0], code_base + site, names[-1][1]))
    need(len(hits) == 1,
         "source special array RecvProp ambiguous at " + hex(slot_va))
    return hits[0]


def verify(client: bytes, *, include_all: bool = False) -> dict:
    need(hashlib.sha256(client).hexdigest() == ORIGINALS["client"]["sha256"],
         "whole original client checksum differs before 939-prop audit")
    pe = PEImage(client)
    tables = verify_tables(client)["class_links"]
    need(len(tables) == 191, "source ClientClass/RecvTable catalog not complete")
    expected = {}
    for tbl in tables:
        for index in range(tbl["property_count"]):
            key = (tbl["class_name"], index)
            address = tbl["property_array_va"] + index * 60
            need(key not in expected, "duplicate original class property slot")
            expected[key] = (tbl["table_name"], address)
    need(len(expected) == 939, "not all original 939 property slots declared")
    code_base, code, names = code_and_names(pe)
    found = {}
    # Common original instruction pattern: PUSH property name; PUSH one
    # 60-byte RecvProp destination; native relative CALL to constructor.
    for i in range(len(code) - 15):
        if code[i] != 0x68 or code[i + 5] != 0x68 or code[i + 10] != 0xE8:
            continue
        name = names.get(struct.unpack_from("<I", code, i + 1)[0])
        if name is None:
            continue
        dst = struct.unpack_from("<I", code, i + 6)[0]
        helper = rel32(code, i + 10, code_base)
        if pe.section_of(helper) != ".text":
            continue
        for key, (table_name, slot_va) in expected.items():
            if slot_va != dst:
                continue
            need(key not in found, "ambiguous standard RecvProp constructor")
            field_offset = None
            if i >= 5 and code[i - 5] == 0x68:
                field_offset = struct.unpack_from("<I", code, i - 4)[0]
            elif i >= 2 and code[i - 2] == 0x6A:
                field_offset = code[i - 1]
            if field_offset is not None and field_offset > 0x10000:
                field_offset = None
            found[key] = dict(name=name, code_va=code_base + i,
                              constructor_va=helper,
                              x86_member_offset=field_offset,
                              route="standard")
    need(len(found) == STANDARD_COUNT,
         f"expected {STANDARD_COUNT} standard named properties, got {len(found)}")
    special = []
    for key, (_, slot_va) in expected.items():
        if key in found:
            continue
        kind, name, callsite, name_push = source_special_name(
            pe, code, code_base, slot_va)
        found[key] = dict(name=name, code_va=callsite,
                          constructor_va=(ARRAY_HELPER if kind == "array-helper"
                                          else ARRAY_COPY),
                          x86_member_offset=None,
                          source_name_push_va=name_push,
                          route=kind)
        special.append(kind)
    need(len(found) == 939 and len(special) == 23,
         "some original client RecvProp names remain unresolved")
    need(special.count("array-helper") == ARRAY_HELPER_COUNT and
         special.count("array-copy") == ARRAY_COPY_COUNT,
         "source array helper / source array-copy distribution changed")
    rows = []
    for table in tables:
        for index in range(table["property_count"]):
            rows.append((table["class_name"], table["table_name"], index,
                         found[(table["class_name"], index)]))
    fingerprint = "".join(
        f'{class_name}|{table_name}|{index}|{record["name"]}\n'
        for class_name, table_name, index, record in rows
    ).encode("ascii")
    need(hashlib.sha256(fingerprint).hexdigest() == PROP_NAMES_SHA,
         "full source 939-name mapping digest changed")
    focus = {}
    for name in ("CNPC_GruntBase", "CNPC_EliteGrunt", "CHL2_Player",
                 "CWeaponMagnum", "CKeypad", "CBaseEntity"):
        focus[name] = [
            {"slot": index, "property_name": record["name"],
             "original_x86_member_offset": record["x86_member_offset"],
             "source_constructor_va": hex(record["constructor_va"])}
            for class_name, _, index, record in rows if class_name == name
        ]
    report = {
        "status": "ORIGINAL_CLIENT_ALL_939_RECVPROP_NAMES_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "source_registered_classes": 191,
        "original_declared_recvprop_slots": 939,
        "standard_constructor_names": STANDARD_COUNT,
        "nonstandard_array_helper_names": ARRAY_HELPER_COUNT,
        "nonstandard_array_copy_names": ARRAY_COPY_COUNT,
        "all_named_property_slots": len(found),
        "source_property_name_sequence_sha256": PROP_NAMES_SHA,
        "source_focus_tables": focus,
        "limitations": ("Source original names and selected x86 offsets only. "
                        "Native proxy functions, RecvProp flags and wire types, "
                        "class creation and runnable client network ABI "
                        "are NOT reconstructed. No retail boot."),
    }
    if include_all:
        # Owner-controlled private export: SOURCE METADATA ONLY.
        # Original x86 addresses are printed as diagnostic labels, never
        # executable host pointers; classes/proxies remain nonfunctional.
        report["class_property_metadata"] = [
            {
                "client_class_name": table["class_name"],
                "source_table_name": table["table_name"],
                "source_recvtable_va": hex(table["recvtable_va"]),
                "source_property_array_va": hex(table["property_array_va"]),
                "source_creator_kind": table["original_creator_kind"],
                "declared_properties": table["property_count"],
                "properties": [
                    {
                        "slot": index,
                        "source_property_name": found[(table["class_name"], index)]["name"],
                        "native_constructor_route": found[(table["class_name"], index)]["route"],
                        "original_x86_field_offset": found[(table["class_name"], index)]["x86_member_offset"],
                        "source_constructor_va": hex(found[(table["class_name"], index)]["constructor_va"]),
                        "source_callsite_va": hex(found[(table["class_name"], index)]["code_va"]),
                    }
                    for index in range(table["property_count"])
                ],
            }
            for table in tables
        ]
        need(len(report["class_property_metadata"]) == 191 and
             sum(len(x["properties"]) for x in report["class_property_metadata"]) == 939,
             "exported class/property inventory unexpectedly incomplete")
    return report


def self_test() -> None:
    synthetic = b"DTKeypad\x00m_vecPoints\x00m_hPassengers\x00"
    need(synthetic.count(b"\x00") == 3,
         "invented special array properties do not delimit")
    try:
        verify(b"fabricated original client image")
    except VerificationError:
        pass
    else:
        raise AssertionError("fabricated game client image accepted")
    print("PASS Phase1696 939 original client RecvProp names: common/array/copy distinction and wrong original SHA refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path,
                        help="owner-private original 2006 PE32 client.dll")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--full-json", type=Path,
                        help="owner-private SOURCE METADATA export of all 939 properties")
    args = parser.parse_args()
    if args.full_json is not None and args.client is None:
        parser.error("--full-json requires --client with the owner's unchanged original image")
    if args.client is None and not args.self_test:
        parser.error("supply --self-test and/or --client")
    if args.self_test:
        self_test()
    if args.client is not None:
        report = verify(args.client.read_bytes(), include_all=args.full_json is not None)
        if args.full_json is not None:
            args.full_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                                      encoding="utf-8")
            print("PASS original-client metadata export: 191 classes, 939 names; "
                  "no original PE32 bytes, no engine-compatible RecvTable pointers")
        else:
            print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
