#!/usr/bin/env python3
"""Phase1702: reproducible source-offset audit (no private DLL in CI).

The public mode authenticates the 2220-byte authored source offset stream.
The owner-private mode reconstructs 1082 canonical RecvProp member offsets
from the unchanged SHA-pinned 2006 PE32 client.dll. No original executable
bytes are committed or written by this tool.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import struct
import sys

ORIGINAL_CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
ORIGINAL_OFFSETS_SHA = "155043d70cc995d37f5812bb818d257224627c9fbbbff8628e384fab7a59dd04"
STATIC_TABLES = 211
CLIENT_CLASSES = 191
STATIC_PROPERTIES = 1110
CANONICAL_SOURCE_PROPERTIES = 1082
SPECIAL_SOURCE_PROPERTIES = 28
UNRESOLVED = 0xFFFF
TABLE_CONSTRUCTOR = 0x24126A30
CANONICAL_HELPERS = frozenset((
    0x24126DF0, 0x24126D80, 0x24126A60, 0x24126AD0, 0x24126B50,
    0x241A8C60, 0x241A8C90, 0x241A8CC0, 0x241A8D30, 0x241A8D80,
))


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def public_manifest_bytes(path: Path) -> bytes:
    text = path.read_text(encoding="utf-8")
    match = re.search(r"kSourceLittleEndianHex\[\]\s*=\s*(.*?);", text, re.S)
    need(match is not None, "missing source member offset encoding")
    source = "".join(re.findall(r'"([0-9a-f]+)"', match.group(1)))
    need(len(source) == STATIC_PROPERTIES * 4, "source offset bytes changed")
    result = bytes.fromhex(source)
    need(len(result) == STATIC_PROPERTIES * 2, "wrong decoded stream length")
    need(hashlib.sha256(result).hexdigest() == ORIGINAL_OFFSETS_SHA,
         "source offset manifest SHA mismatch")
    values = struct.unpack("<" + "H" * STATIC_PROPERTIES, result)
    need(values.count(UNRESOLVED) == SPECIAL_SOURCE_PROPERTIES,
         "source special constructor unknown count changed")
    return result


class PE:
    def __init__(self, data: bytes):
        self.data = data
        need(data[:2] == b"MZ", "not a PE MZ executable")
        pe_offset, = struct.unpack_from("<I", data, 0x3C)
        need(data[pe_offset:pe_offset + 4] == b"PE\x00\x00", "not PE32")
        coff = pe_offset + 4
        nsections, = struct.unpack_from("<H", data, coff + 2)
        optional = coff + 20
        magic, = struct.unpack_from("<H", data, optional)
        need(magic == 0x10B, "2006 original is 32-bit PE, not PE32+")
        image_base, = struct.unpack_from("<I", data, optional + 28)
        start = optional + struct.unpack_from("<H", data, coff + 16)[0]
        self.sections = {}
        for i in range(nsections):
            loc = start + i * 40
            name = data[loc:loc + 8].split(b"\x00")[0].decode("ascii")
            virtual_size, rva, file_size, file_offset = struct.unpack_from(
                "<IIII", data, loc + 8)
            accessible = min(virtual_size, file_size)
            need(file_offset + accessible <= len(data), "invalid PE section")
            self.sections[name] = (
                image_base + rva, data[file_offset:file_offset + accessible])
        need(".text" in self.sections and ".rdata" in self.sections,
             "missing original PE32 sections")

    def cstring(self, va: int) -> str:
        for name in (".rdata", ".data"):
            if name not in self.sections:
                continue
            base, raw = self.sections[name]
            if base <= va < base + len(raw):
                offset = va - base
                value = raw[offset:offset + 100].split(b"\x00")[0]
                result = value.decode("ascii")
                need(bool(result), "empty source original name")
                return result
        raise ValueError("string pointer outside source metadata")


def original_table_map(pe: PE):
    base, code = pe.sections[".text"]
    calls = []
    for i in range(25, len(code) - 5):
        if code[i] != 0xE8:
            continue
        va = base + i
        dest = va + 5 + struct.unpack_from("<i", code, i + 1)[0]
        if dest != TABLE_CONSTRUCTOR:
            continue
        pos = i - 5
        need(code[pos] == 0xB9 and code[pos - 5] == 0x68,
             "source table constructor argument sequence drift")
        table_va, = struct.unpack_from("<I", code, pos + 1)
        props_va, = struct.unpack_from("<I", code, pos - 4)
        pos -= 5
        if code[pos - 2] == 0x6A:
            count = code[pos - 1]
            pos -= 2
        else:
            need(code[pos - 5] == 0x68, "unresolved RecvTable property count")
            count, = struct.unpack_from("<I", code, pos - 4)
            pos -= 5
        need(code[pos - 5] == 0x68 and count <= 256,
             "unproven RecvTable name or slot count")
        name_va, = struct.unpack_from("<I", code, pos - 4)
        calls.append((table_va, props_va, count, pe.cstring(name_va)))
    need(len(calls) == 214, "wrong number of original 2006 table construction calls")
    tables = {}
    for table_va, props_va, count, name in calls:
        if table_va in tables:
            need(tables[table_va] == (props_va, count, name),
                 "conflicting table constructor reuse")
        else:
            tables[table_va] = (props_va, count, name)
    need(len(tables) == STATIC_TABLES, "unexpected source static table count")
    need(sum(c for _, c, _ in tables.values()) == STATIC_PROPERTIES,
         "wrong original source property cardinality")
    return tables


def original_canonical_offsets(pe: PE, tables):
    base, code = pe.sections[".text"]
    slots = {}
    for table_va, (props_va, count, _) in tables.items():
        for slot in range(count):
            native_descriptor_va = props_va + 60 * slot
            need(native_descriptor_va not in slots, "overlapping source descriptors")
            slots[native_descriptor_va] = (table_va, slot)
    found = {}
    for i in range(24, len(code) - 16):
        if code[i] != 0x68 or code[i + 5] != 0x68 or code[i + 10] != 0xE8:
            continue
        destination, = struct.unpack_from("<I", code, i + 6)
        if destination not in slots:
            continue
        factory = base + i + 15 + struct.unpack_from("<i", code, i + 11)[0]
        if factory not in CANONICAL_HELPERS:
            continue
        name_va, = struct.unpack_from("<I", code, i + 1)
        pe.cstring(name_va)  # Original property name must be readable.
        if code[i - 2] == 0x6A:
            member_offset = code[i - 1]
        else:
            need(code[i - 5] == 0x68, "unresolved original member offset PUSH")
            member_offset, = struct.unpack_from("<I", code, i - 4)
        need(member_offset < UNRESOLVED, "source 16-bit member offset overflow")
        key = slots[destination]
        need(key not in found, "duplicate source property constructor")
        found[key] = member_offset
    need(len(found) == CANONICAL_SOURCE_PROPERTIES,
         "incomplete canonical source constructor offset inventory")
    return found


def original_catalog_order(path: Path) -> list[int]:
    records = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("|")
        need(len(fields) == 5, "invalid source class catalog record")
        records.append(int(fields[4], 16))
    need(len(records) == CLIENT_CLASSES and len(set(records)) == CLIENT_CLASSES,
         "invalid source original class registry order")
    return records


def verify_original(client_path: Path, catalog: Path, expected: bytes) -> None:
    data = client_path.read_bytes()
    need(hashlib.sha256(data).hexdigest() == ORIGINAL_CLIENT_SHA,
         "owner-original 2006 client DLL SHA mismatch")
    pe = PE(data)
    tables = original_table_map(pe)
    found = original_canonical_offsets(pe, tables)
    roots = original_catalog_order(catalog)
    need(set(roots).issubset(tables), "class roots absent from static table source")
    aux = sorted(set(tables) - set(roots))
    need(len(aux) == STATIC_TABLES - CLIENT_CLASSES,
         "wrong count of source auxiliary tables")
    result = []
    for table_va in roots + aux:
        _, count, _ = tables[table_va]
        for slot in range(count):
            offset = found.get((table_va, slot), UNRESOLVED)
            result.append(offset)
    need(len(result) == STATIC_PROPERTIES, "wrong source property order/count")
    raw = struct.pack("<" + "H" * STATIC_PROPERTIES, *result)
    need(raw == expected, "owner-private source offsets disagree with public manifest")
    need(hashlib.sha256(raw).hexdigest() == ORIGINAL_OFFSETS_SHA,
         "original source member offset fingerprint drift")
    print("PASS Phase1702 owner-original 1082 member offsets / 28 guarded special routes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--client", type=Path,
                        help="read-only exact original private 2006 client.dll")
    parser.add_argument("--header", type=Path,
                        default=Path("include/sin_compat/original_client_native_member_offsets.hpp"))
    parser.add_argument("--catalog", type=Path,
                        default=Path("research/manifests/original_client_class_catalog_phase1696.tsv"))
    args = parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    expected = public_manifest_bytes(args.header)
    if args.self_test:
        # Refuse even a valid-looking replacement tag while guaranteeing that
        # private executable bytes are never needed in public CI.
        mutated = bytearray(expected)
        mutated[0] ^= 0x01
        need(hashlib.sha256(mutated).hexdigest() != ORIGINAL_OFFSETS_SHA,
             "original-free mutation SHA refusal failed")
        print("PASS Phase1702 original-free 1082 source offsets SHA / 28 guarded unknown")
    if args.client is not None:
        verify_original(args.client, args.catalog, expected)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, UnicodeError, struct.error) as error:
        print(f"FAIL Phase1702: {error}", file=sys.stderr)
        sys.exit(1)
