#!/usr/bin/env python3
"""Phase1696: pin all 191 original client classes to real RecvTable constructors.

No native DLL bytes are emitted or committed. Private --client authenticates
the owner's whole original PE32 file; public --self-test uses invented data.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_client_class_registry import (
    CLASS_COUNT, CATALOG_SHA, digest, need, registration_records,
)

RECV_CTOR = 0x24126A30
RECV_CTOR_SHA = "6f9e7546053378e2146df443c8244c8250fa214ff339c5404fefd8c291a05555"
RECV_PROPERTY_STRIDE = 0x3C
RECV_MATCHED_CALLS = 214
RECV_UNIQUE_TABLES = 211
CLIENT_CLASS_DECLARED_PROP_SLOTS = 939
CLASS_RECV_LINK_SHA = "0ba3a72d88cbf5e0881232cf39a3b00ded5bcf59e73f280a4e73502ca7869fbc"
SPECIAL_UNDERSCORELESS_NAME = "DTKeypad"


def scan_table_constructors(pe: PEImage) -> list[dict]:
    name_section = next(s for s in pe.sections if s[0] == ".rdata")
    _, name_rva, name_virtual, name_raw, name_raw_size = name_section
    name_data = pe.data[name_raw:name_raw + min(name_virtual, name_raw_size)]
    # Include the 2006 CKeypad outlier "DTKeypad", not just "DT_*".
    names = {
        pe.base + name_rva + m.start():
            m.group()[:-1].decode("ascii")
        for m in re.finditer(rb"DT[A-Za-z0-9_]{2,96}\x00", name_data)
    }
    text_section = next(s for s in pe.sections if s[0] == ".text")
    _, text_rva, text_virtual, text_raw, text_raw_size = text_section
    code = pe.data[text_raw:text_raw + min(text_virtual, text_raw_size)]
    base = pe.base + text_rva
    records = []
    for m in re.finditer(b"\x68", code):
        i = m.start()
        if i + 22 > len(code):
            continue
        name_ptr = struct.unpack_from("<I", code, i + 1)[0]
        table_name = names.get(name_ptr)
        if table_name is None:
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
        if count > 256 or j + 15 > len(code) or \
           code[j] != 0x68 or code[j + 5] != 0xB9 or code[j + 10] != 0xE8:
            continue
        props_va = struct.unpack_from("<I", code, j + 1)[0]
        table_va = struct.unpack_from("<I", code, j + 6)[0]
        destination = base + j + 15 + struct.unpack_from("<i", code, j + 11)[0]
        if destination != RECV_CTOR:
            continue
        need(pe.section_of(table_va) == ".data",
             f"source RecvTable destination outside .data {table_va:#x}")
        need(pe.section_of(props_va) == ".data",
             f"source RecvProp array outside .data {props_va:#x}")
        if count:
            need(pe.section_of(props_va + (count - 1) * RECV_PROPERTY_STRIDE)
                 == ".data",
                 "last declared RecvProp slot outside original .data")
        records.append(dict(name=table_name, count=count,
                            table_va=table_va, props_va=props_va,
                            constructor_call_va=base + j + 10))
    return records


def verify(client: bytes, metadata: str | None = None) -> dict:
    need(hashlib.sha256(client).hexdigest() == ORIGINALS["client"]["sha256"],
         "whole original client SHA mismatch for class RecvTables")
    pe = PEImage(client)
    need(hashlib.sha256(pe.at(RECV_CTOR, 34)).hexdigest() == RECV_CTOR_SHA,
         "source RecvTable constructor changed")
    classes = registration_records(pe)
    need(digest(classes) == CATALOG_SHA, "class head chain changed")
    tables = scan_table_constructors(pe)
    need(len(tables) == RECV_MATCHED_CALLS,
         f"original RecvTable constructor calls != {RECV_MATCHED_CALLS}")
    by_va = {}
    for table in tables:
        old = by_va.get(table["table_va"])
        if old is not None:
            need((old["name"],old["count"],old["props_va"]) ==
                 (table["name"],table["count"],table["props_va"]),
                 "repeated original RecvTable initializer is not identical")
        by_va[table["table_va"]] = table
    need(len(by_va) == RECV_UNIQUE_TABLES,
         "original distinct RecvTable object count changed")
    matched = []
    for cls in classes:
        table = by_va.get(cls["recvtable_va"])
        need(table is not None,
             "missing original client RecvTable for " + cls["name"])
        matched.append(dict(
            class_name=cls["name"], table_name=table["name"],
            property_count=table["count"],
            property_array_va=table["props_va"],
            recvtable_va=table["table_va"],
            original_creator_kind=("temp-event" if cls["event_creator_va"]
                                   else "entity"),
        ))
    need(len(matched) == CLASS_COUNT and
         len({m["table_name"] for m in matched}) == CLASS_COUNT,
         "class ReceiveTable name linkage count changed")
    need(sum(m["property_count"] for m in matched) ==
         CLIENT_CLASS_DECLARED_PROP_SLOTS,
         "expected exact 939 source RecvProp slots across class tables")
    need(sum(m["property_count"] for m in matched
             if m["original_creator_kind"] == "temp-event") == 210,
         "temporary-event class declared RecvProp count changed")
    need(sum(m["property_count"] for m in matched
             if m["original_creator_kind"] == "entity") == 729,
         "normal entity class declared RecvProp count changed")
    need(sum(m["property_count"]==0 for m in matched)==2,
         "source zero-property class table count changed")
    keypad = next(m for m in matched if m["class_name"]=="CKeypad")
    need(keypad["table_name"] == SPECIAL_UNDERSCORELESS_NAME and
         keypad["property_count"] == 3 and
         keypad["recvtable_va"] == 0x2437BA84 and
         keypad["property_array_va"] == 0x2437B9CC,
         "nonstandard original DTKeypad constructor no longer matches")
    canonical = "".join(
        f'{r["class_name"]}|{r["table_name"]}|{r["property_count"]}|'
        f'{r["property_array_va"]:08x}|{r["recvtable_va"]:08x}\n'
        for r in matched).encode("ascii")
    need(hashlib.sha256(canonical).hexdigest() == CLASS_RECV_LINK_SHA,
         "whole ordered class-to-RecvTable source mapping fingerprint drift")
    return {
        "status": "ORIGINAL_CLIENT_ALL_191_CLASS_RECVTABLE_LINKS_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "constructor_va": hex(RECV_CTOR),
        "source_registered_classes": len(classes),
        "total_source_recvtable_init_calls": len(tables),
        "distinct_source_recvtable_objects": len(by_va),
        "class_recvtable_links": len(matched),
        "declared_class_recprop_slots": CLIENT_CLASS_DECLARED_PROP_SLOTS,
        "normal_entity_recprop_slots": 729,
        "temp_event_recprop_slots": 210,
        "nonstandard_name": "CKeypad -> DTKeypad (3 props)",
        "canonical_links_sha256": CLASS_RECV_LINK_SHA,
        "class_links": matched,
        "limitations": ("Declared property slot counts and table pointers "
                        "only; RecvProp types, factories, proxies, flags and "
                        "live engine-compatible RecvTable objects are "
                        "NOT implemented; no retail boot."),
    }


def self_test() -> None:
    fake = b"DTKeypad\x00other\x00DT_NPC_GruntBase\x00"
    names = [m.group()[:-1].decode("ascii") for m in
             re.finditer(rb"DT[A-Za-z0-9_]{2,96}\x00",fake)]
    need(names == ["DTKeypad","DT_NPC_GruntBase"],
         "fabricated special DT-name scan lost underscoreless class table")
    need(RECV_PROPERTY_STRIDE == 60, "original 2006 RecvProp size changed")
    need(CLASS_RECV_LINK_SHA != CATALOG_SHA,
         "table/registry hashes should be independent")
    try:
        verify(b"fabricated original client")
    except VerificationError:
        pass
    else:
        raise AssertionError("fabricated game client accepted as original")
    print("PASS Phase1696 191-class RecvTable source audit: underscoreless DTKeypad, 60-byte props, wrong whole-original SHA refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path,
                        help="owner-private unmodified client.dll")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if not args.self_test and args.client is None:
        parser.error("supply --self-test and/or --client")
    if args.self_test:
        self_test()
    if args.client is not None:
        print(json.dumps(verify(args.client.read_bytes()),
                         indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
