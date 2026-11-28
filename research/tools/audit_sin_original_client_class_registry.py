#!/usr/bin/env python3
"""Phase1696: authenticate the SiN 2006 PE32 191-client-class head chain.

Private --client checks the owner's unchanged SHA-pinned retail client.dll.
Public --self-test uses fabricated records only. Original code is never shipped.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError, read_name,
)

HEAD = 0x2433FD90
SLOT5 = 0x240FD990
REGISTER = 0x24053C40
CRT_BEGIN = 0x242F3004
CRT_POINTERS = 1582
CLASS_COUNT = 191
ENTITY_CLASSES = 149
EVENT_CLASSES = 42
CATALOG_SHA = "ded698d9f9b6c671e17a03a23a0e909b4acb5d8c6030f905af2899471d616c24"
SLOT5_SHA = "de7983e64b69321348f64408db25c7f2a4215cf4201e17d4e10eb2f43a64d41e"
REGISTER_SHA = "130516addf1c8a0c3e90dc8950e64168eb2c2872441a86e96b2942d6b360341b"
FIRST_CRT_ENTRY = 0x2425E7AF
LAST_CRT_ENTRY = 0x24262450


def need(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError("Phase1696 original client classes: " + message)


def u32(blob: bytes, pos: int = 0) -> int:
    return struct.unpack_from("<I", blob, pos)[0]


def p32(value: int) -> bytes:
    return struct.pack("<I", value)


def digest(records: list[dict]) -> str:
    canonical = "".join(
        f'{r["crt_index"]}|{r["name"]}|{r["node_va"]:08x}|'
        f'{r["entity_creator_va"]:08x}|{r["event_creator_va"]:08x}|'
        f'{r["recvtable_va"]:08x}\n'
        for r in records
    ).encode("ascii")
    return hashlib.sha256(canonical).hexdigest()


def registration_records(pe: PEImage) -> list[dict]:
    """Recover real CRT execution order and source head-prepend list nodes."""
    crt = pe.at(CRT_BEGIN, CRT_POINTERS * 4)
    pointers = struct.unpack("<" + "I" * CRT_POINTERS, crt)
    need(pointers[0] == FIRST_CRT_ENTRY and pointers[-1] == LAST_CRT_ENTRY,
         "CRT initializer start/tail changed")
    need(pe.at(CRT_BEGIN + CRT_POINTERS * 4, 4) == b"\x00" * 4,
         "CRT initializer end delimiter missing")
    need(len(set(pointers)) == CRT_POINTERS and
         all(pe.section_of(addr) == ".text" for addr in pointers),
         "CRT initializer addresses invalid or duplicated")
    sorted_starts = sorted(pointers)
    by_address = {address: i for i, address in enumerate(pointers)}
    text_section = next((s for s in pe.sections if s[0] == ".text"), None)
    need(text_section is not None, "no executable .text section")
    _, section_rva, virtual_size, raw_offset, raw_size = text_section
    text = pe.data[raw_offset:raw_offset + min(virtual_size, raw_size)]
    code_base = pe.base + section_rva
    needle = b"\xC7\x05" + p32(HEAD)
    records = []
    start = 0
    while True:
        site_off = text.find(needle, start)
        if site_off < 0:
            break
        start = site_off + 1
        site = code_base + site_off
        node = u32(text, site_off + len(needle))
        need(pe.section_of(node) == ".data",
             f"class node not writable source .data at {site:#x}")
        at = bisect.bisect_right(sorted_starts, site) - 1
        need(at >= 0, "source registration precedes CRT callback")
        constructor = sorted_starts[at]
        crt_index = by_address[constructor]
        preceding = pe.at(constructor, site - constructor)
        need(preceding.startswith(b"\xA1" + p32(HEAD)),
             f"CRT callback does not initially load global head at {site:#x}")
        need(preceding.count(b"\xA1" + p32(HEAD)) == 1,
             "ambiguous source head reads")
        need(preceding.endswith(b"\xA3" + p32(node + 16)),
             f"missing predecessor write to class node+16 at {site:#x}")
        need(pe.at(site + 10, 1) == b"\xC3",
             "registration initializer not followed by native RET")
        # A static .data node may already have the four members in the PE.
        # A .bss node must instead be reconstructed from real CRT MOV stores.
        try:
            values = list(struct.unpack("<IIIII", pe.at(node, 20)))
        except VerificationError:
            values = [0] * 5
        for member in range(4):
            operand = b"\xC7\x05" + p32(node + member * 4)
            offset = preceding.rfind(operand)
            if offset >= 0:
                need(preceding.count(operand) == 1,
                     "multiple source writes to a class descriptor member")
                values[member] = u32(preceding, offset + len(operand))
        entity, event, name_ptr, recv_table, next_ptr = values
        need(next_ptr == 0, "serialized class next must initially be null")
        need(bool(entity) != bool(event),
             "expected exactly one original creator function")
        need(pe.section_of(entity or event) == ".text",
             "original creator not in client executable region")
        need(pe.section_of(name_ptr) == ".rdata",
             "original network class name not in .rdata")
        need(pe.section_of(recv_table) == ".data",
             "original class RecvTable not in writable source section")
        name = read_name(pe, name_ptr, maximum=100)
        need(bool(name) and all(ch.isalnum() or ch == "_" for ch in name),
             "invalid source class string")
        records.append(dict(name=name, crt_index=crt_index,
                            registration_va=site, constructor_va=constructor,
                            node_va=node, entity_creator_va=entity,
                            event_creator_va=event, recvtable_va=recv_table))
    need(len(records) == CLASS_COUNT,
         f"expected {CLASS_COUNT} distinct class registrations")
    for field in ("name", "node_va", "recvtable_va", "constructor_va"):
        need(len({r[field] for r in records}) == CLASS_COUNT,
             "duplicate source class record field: " + field)
    need(all(a["crt_index"] < b["crt_index"]
             for a, b in zip(records, records[1:])),
         "source code order differs from CRT execution order")
    return list(reversed(records))  # prepend means last initializer = head


def verify(client: bytes, catalog_text: str | None = None) -> dict:
    need(hashlib.sha256(client).hexdigest() == ORIGINALS["client"]["sha256"],
         "entire original client SHA256 changed")
    pe = PEImage(client)
    need(pe.base == 0x24000000, "not original 2006 client PE image base")
    slot5 = pe.at(SLOT5, 6)
    need(hashlib.sha256(slot5).hexdigest() == SLOT5_SHA and
         slot5 == b"\xA1" + p32(HEAD) + b"\xC3" and
         pe.at(SLOT5 + 6, 10) == b"\xCC" * 10,
         "original VClient011.slot5 not complete six-byte registry-head getter")
    registration = pe.at(REGISTER, 46)
    need(hashlib.sha256(registration).hexdigest() == REGISTER_SHA and
         registration[0:4] == b"\x8B\x54\x24\x08" and
         registration[0x1D:0x23] == b"\x8B\x0D" + p32(HEAD) and
         registration[0x23:0x26] == b"\x89\x48\x10" and
         registration[0x26:0x2B] == b"\xA3" + p32(HEAD) and
         registration[-3:] == b"\xC2\x10\x00",
         "original 46-byte node register helper deviates from linked-head ABI")
    rows = registration_records(pe)
    need(digest(rows) == CATALOG_SHA, "entire source catalog digest changed")
    events = sum(bool(r["event_creator_va"]) for r in rows)
    need((len(rows)-events, events) == (ENTITY_CLASSES, EVENT_CLASSES),
         "source entity/event creator split changed")
    need(rows[0]["name"] == "CWeaponMelee" and
         rows[-1]["name"] == "CBaseTempEntity",
         "original source linked-list head/tail changed")
    if catalog_text is not None:
        expected = [line.strip().split("|") for line in catalog_text.splitlines()
                    if line.strip() and not line.startswith("#")]
        need(len(expected) == len(rows), "portable catalog count mismatch")
        for idx, (expected_row, r) in enumerate(zip(expected, rows)):
            source = [r["name"], str(r["crt_index"]),
                      "T" if r["event_creator_va"] else "E",
                      f'{r["node_va"]:08x}', f'{r["recvtable_va"]:08x}']
            need(expected_row == source,
                 f"portable class manifest differs at list node {idx}")
    return {
        "status": "ORIGINAL_CLIENT_CLASS_REGISTRY_191_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "original_slot5_va": hex(SLOT5),
        "global_head": hex(HEAD), "source_node_next_offset": 16,
        "crt_initializer_callbacks": CRT_POINTERS,
        "class_count": CLASS_COUNT,
        "entity_creators": ENTITY_CLASSES,
        "temporary_event_creators": EVENT_CLASSES,
        "head_name": rows[0]["name"], "tail_name": rows[-1]["name"],
        "canonical_source_catalog_sha256": digest(rows),
        "source_records": rows,
        "limits": ("Metadata only, not a live client-class linked list. "
                   "Receivetable descriptors, create functions and class IDs "
                   "are NOT reconstructed; no client boot proven."),
    }


def self_test() -> None:
    invented = [
        dict(crt_index=30, name="CFakeB", node_va=0x10000800,
             entity_creator_va=0, event_creator_va=0x10002000,
             recvtable_va=0x10001020),
        dict(crt_index=10, name="CFakeA", node_va=0x10000400,
             entity_creator_va=0x10003000, event_creator_va=0,
             recvtable_va=0x10001000),
    ]
    signature = digest(invented)
    need(len(signature) == 64 and signature != CATALOG_SHA,
         "fabricated source accidentally canonical")
    for field in invented[0]:
        rows = [dict(x) for x in invented]
        if field == "name":
            rows[0][field] += "X"
        else:
            rows[0][field] += 1
        need(digest(rows) != signature,
             f"fabricated change to {field} was accepted")
    try:
        verify(b"fabricated original retail client")
    except VerificationError:
        pass
    else:
        raise AssertionError("fabricated retail game module was accepted")
    print("PASS Phase1696 191-class registry synthetic mutations, CRT-order fingerprint and wrong whole-image SHA refusal")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path,
                        help="owner-private original 2006 client.dll; never upload")
    parser.add_argument("--catalog", type=Path,
                        help="portable name/kind/node/RecvTable metadata manifest")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.client is None and not args.self_test:
        parser.error("supply --self-test or --client")
    if args.self_test:
        self_test()
    if args.client is not None:
        catalog = args.catalog.read_text("utf-8") if args.catalog else None
        print(json.dumps(verify(args.client.read_bytes(), catalog),
                         indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
