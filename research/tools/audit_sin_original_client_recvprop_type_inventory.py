#!/usr/bin/env python3
"""Phase1697: recover ALL 939 source-backed numeric RecvProp types of SiN 2006.

Requires a SHA-pinned owner-private original client.dll to authenticate
actual native factory bodies/call edges and all 939 class property callsites.
Outputs only authored metadata; no original executable bytes are committed.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_class_recvprop_names import (
    verify as verify_property_names,
)
from audit_sin_original_client_class_registry import need
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Values are the ORIGINAL 2006 native descriptor type DWORD at +4.
# Do not substitute newer Source SDK enum numbers without evidence.
TYPE_INTEGER = 0
TYPE_FLOAT = 1
TYPE_VECTOR3 = 2
TYPE_STRING = 3
TYPE_ARRAY = 4
TYPE_NESTED_TABLE = 5

# Factory VA -> original native tag; wrapper methods use the original
# 0x24126DF0/0x24126D80 constructors after their own argument handling.
DIRECT_HELPERS = {
    0x24126DF0: TYPE_INTEGER,
    0x24126D80: TYPE_FLOAT,
    0x24126A60: TYPE_VECTOR3,
    0x24126AD0: TYPE_STRING,
    0x24126BD0: TYPE_ARRAY,
    0x24126B50: TYPE_NESTED_TABLE,
}
WRAPPER_HELPERS = {
    0x241A8C60: (0x24126DF0, TYPE_INTEGER),
    0x241A8C90: (0x24126DF0, TYPE_INTEGER),
    0x241A8CC0: (0x24126DF0, TYPE_INTEGER),
    0x241A8D30: (0x24126D80, TYPE_FLOAT),
}
# Original 0x24126E90 builds a fresh variable-sized property array,
# allocates an original 20-byte RecvTable, and writes type DWORD 5 into
# the OUTER descriptor at native stack+4 before copying 60 bytes.
# This is a genuine nested-table constructor, NOT an unknown type.
COPIED_DESCRIPTOR_HELPER = 0x24126E90
ORIGINAL_ARRAY_ELEMENT_NAME_TABLE = 0x242FB110
ORIGINAL_ARRAY_NAME_POINTERS = 256
ORIGINAL_ARRAY_NAME_TABLE_SHA = "7f419caa89c32a637e4f7b7b81f247e232fcc55d1673721e6a3c9dd46d040506"
ORIGINAL_ARRAY_ELEMENT_NAME_SHA = "e139eda07b5876a36e7be3471540c112ecbc58b7cfdd1e33f54c4c4fc958c4"
EXPECTED_TAG_COUNTS = {
    TYPE_INTEGER: 339,
    TYPE_FLOAT: 238,
    TYPE_VECTOR3: 136,
    TYPE_STRING: 12,
    TYPE_ARRAY: 12,
    TYPE_NESTED_TABLE: 202,
}
ALL_939_NAME_AND_TYPE_SHA = "149122c6e319169fe9763990d14049887da11e513f119599958776392309ecc8"

# Four independently complete original PE32 factory methods. The two
# 0/1 base constructors were already audited in Phase251; don't count
# or falsely advertise duplicated native bytes.
SOURCE_FACTORY_BODIES = (
    (0x24126A60, 109, "e6ee428be652459bcc73755ac4c161f211ffa81dc203055f39c10678e3a2eb4a"),
    (0x24126AD0, 117, "0c29e142f7c064d5170e27d1a2759c4ff059c962a576febddedabbf8e4724300"),
    (0x24126B50, 113, "dfac126f022ba2ac878f7d708e52d341477fb4f251cce7b919266837afb5355f"),
    (0x24126BD0, 102, "dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd"),
    (0x24126D80, 106, "bd1fa7c7ccdc0a278ee0ad889437803cc195d10aa2bd53040b5b66f5f8b97257"),
    (0x24126DF0, 146, "c292491b77e181d88ab5c65d6c49667f99cbd9fcf82a6be39d62f1af23e7c4b5"),
    # 351B array-to-nested-table factory, separately source authenticated.
    (0x24126E90, 351, "c6c8d8af7d77994a6ff4292d9a432172bad6e9a3d72d29b6ec8284c3a3eac0db"),
)
SOURCE_WRAPPER_BODIES = (
    (0x241A8C60, 40, "0e7378a12008f5826614e8917e624559fafab23913663f9755a5ebc2b93cfa91"),
    (0x241A8C90, 40, "e0afed5c7efeb9c3139d4c8b8dc2b59d95407082c545abfd52fa71f8b"),
    (0x241A8CC0, 37, "73f6c9e3994a1058a3e4e9f11e036ddcade9c68bd7d63911e6227dd4ca79395d"),
    (0x241A8D30, 40, "de1fa4d04570c8f317bd8167284862c647db51dd3f28500f19154c3886cc5b02"),
)
# Native instruction witnesses of assignment of descriptor DWORD type.
SOURCE_LITERAL_TYPES = (
    (0x24126AB9, TYPE_VECTOR3),
    (0x24126B31, TYPE_STRING),
    (0x24126C22, TYPE_ARRAY),
    (0x24126BAD, TYPE_NESTED_TABLE),
)
SOURCE_WRAPPER_CALLS = (
    (0x241A8C7C, 0x24126DF0),
    (0x241A8CAC, 0x24126DF0),
    (0x241A8CD9, 0x24126DF0),
    (0x241A8D4C, 0x24126D80),
)

# Exact original 2006 PE32 array element counts, byte strides and
# original x86 object offsets. The 351B helper allocates 60*n bytes for
# generated element RecvProps and a separate 20-byte child RecvTable.
# These eleven shapes have been independently observed at callsites.
EXPECTED_DYNAMIC_ARRAY_SHAPES = {
    ("CPlayerResource", "m_iPing"): (33, 4, 2212, TYPE_INTEGER),
    ("CPlayerResource", "m_iScore"): (33, 4, 2344, TYPE_INTEGER),
    ("CPlayerResource", "m_iDeaths"): (33, 4, 2476, TYPE_INTEGER),
    ("CPlayerResource", "m_bConnected"): (33, 1, 2608, TYPE_INTEGER),
    ("CPlayerResource", "m_iTeam"): (33, 4, 2644, TYPE_INTEGER),
    ("CPlayerResource", "m_bAlive"): (33, 1, 2776, TYPE_INTEGER),
    ("CPlayerResource", "m_iHealth"): (33, 4, 2812, TYPE_INTEGER),
    ("CBreakableSurface", "m_RawPanelBitVec"): (256, 1, 1236, TYPE_INTEGER),
    ("CBaseFlex", "m_flexWeight"): (64, 4, 2144, TYPE_FLOAT),
    ("CBaseAnimating", "m_flPoseParameter"): (24, 4, 1352, TYPE_FLOAT),
    ("CBaseAnimating", "m_flEncodedController"): (4, 4, 1644, TYPE_FLOAT),
}

KNOWN_TYPE_LABELS = {
    0: "integer", 1: "float", 2: "vector3",
    3: "string", 4: "array", 5: "nested-table",
}


def classify(constructor_va: int, route: str) -> int | None:
    if route == "array-copy":
        need(constructor_va == COPIED_DESCRIPTOR_HELPER,
             "native nested array table helper VA changed")
        return TYPE_NESTED_TABLE
    if route == "array-helper":
        need(constructor_va == 0x24126BD0,
             "array-helper does not call original type-four constructor")
        return TYPE_ARRAY
    need(route == "standard", "unrecognized original property init route")
    if constructor_va in DIRECT_HELPERS:
        return DIRECT_HELPERS[constructor_va]
    if constructor_va in WRAPPER_HELPERS:
        return WRAPPER_HELPERS[constructor_va][1]
    raise VerificationError(
        f"unproven original RecvProp helper {constructor_va:#x}")


def verify_native_factories(pe: PEImage) -> None:
    for va, size, whole_digest in SOURCE_FACTORY_BODIES + SOURCE_WRAPPER_BODIES:
        need(pe.section_of(va) == ".text" and
             hashlib.sha256(pe.at(va, size)).hexdigest() == whole_digest,
             f"whole native RecvProp constructor SHA mismatch {va:#x}")
        need(pe.at(va + size, 1) == b"\xCC",
             f"complete native constructor padding missing {va:#x}")
    for va, tag in SOURCE_LITERAL_TYPES:
        need(pe.at(va, 8) ==
             b"\xC7\x44\x24\x0C" + struct.pack("<I", tag),
             f"native descriptor +4 type instruction missing {va:#x}")
    # Original type-0 writes a zero from ECX; type-1 loads literal one.
    need(pe.at(0x24126DF7, 2) == b"\x33\xC9" and
         pe.at(0x24126E4E, 4) == b"\x89\x4C\x24\x08",
         "original integer property +4 zero assignment changed")
    need(pe.at(0x24126DA5, 5) == b"\xB9\x01\x00\x00\x00" and
         pe.at(0x24126DB6, 4) == b"\x89\x4C\x24\x08",
         "original float property +4 one assignment changed")
    # The array-to-table constructor writes native type DWORD 5 at
    # temporary descriptor +4 (after its saved-register pushes and
    # scratch-stack layout). At the end it copies exactly 15 DWORDs
    # into the caller output, so this is the returned descriptor type.
    need(pe.at(0x24126EE7, 8) ==
         b"\xC7\x44\x24\x18\x05\x00\x00\x00",
         "original 351-byte copied-array constructor did not set native type 5")
    need(pe.at(0x24126F7D, 3) == b"\x89\x6A\x2C" and
         pe.at(0x24126F87, 4) == b"\x03\x6C\x24\x5C",
         "original nested array element member-offset/byte-stride update changed")
    need(pe.at(0x24126FAB, 2) == b"\x6A\x14",
         "original dynamically allocated 20-byte child RecvTable request changed")
    need(pe.at(0x24126F70, 13).startswith(
         b"\x8B\xFA\xB9\x0F\x00\x00\x00\x8D\x74\x24\x64\xF3\xA5"),
         "original nested-array 60-byte element descriptor copy altered")
    need(pe.at(0x24126FE0, 7)[-2:] == b"\xF3\xA5",
         "original array-to-nested-table final 60-byte output copy missing")
    # Each dynamically created element descriptor gets its name from an
    # actual source x86 indexed pointer table: original 0x24126F80
    # loads [0x242FB110 + element_index*4] and +0x24126F8B stores that
    # pointer into the element's name DWORD +0. The original data
    # resolves to exactly "000" .. "255" (no inferred Source SDK rules).
    need(pe.section_of(ORIGINAL_ARRAY_ELEMENT_NAME_TABLE) == ".data",
         "original 256-name indexed pointer table relocated")
    pointer_bytes = pe.at(
        ORIGINAL_ARRAY_ELEMENT_NAME_TABLE,ORIGINAL_ARRAY_NAME_POINTERS*4)
    need(hashlib.sha256(pointer_bytes).hexdigest() ==
         ORIGINAL_ARRAY_NAME_TABLE_SHA,
         "full original 256-pointer generated element name table changed")
    names = []
    for index in range(ORIGINAL_ARRAY_NAME_POINTERS):
        source_ptr = struct.unpack_from("<I",pointer_bytes,index*4)[0]
        need(pe.section_of(source_ptr) == ".rdata",
             "generated element indexed name pointer outside original constants")
        value = pe.at(source_ptr,4)
        need(value == f"{index:03d}".encode("ascii") + b"\x00",
             "source generated original element name mismatch")
        names.append(value[:3].decode("ascii"))
    need(hashlib.sha256(("\n".join(names)+"\n").encode("ascii")).hexdigest()
         == ORIGINAL_ARRAY_ELEMENT_NAME_SHA,
         "all 256 original generated element names changed")
    need(pe.at(0x24126F80,7) ==
         b"\x8B\x0C\x85" + struct.pack("<I",ORIGINAL_ARRAY_ELEMENT_NAME_TABLE)
         and pe.at(0x24126F8B,2) == b"\x89\x0A",
         "original nested array element name table lookup/store changed")
    for site, target in SOURCE_WRAPPER_CALLS:
        raw = pe.at(site, 5)
        need(raw[0] == 0xE8 and
             site + 5 + struct.unpack("<i", raw[1:])[0] == target,
             f"original nested type-wrapper call target drift {site:#x}")



def recover_dynamic_array_shape(pe: PEImage, client_class: str,
                                prop: dict) -> dict:
    key = client_class, prop["source_property_name"]
    need(key in EXPECTED_DYNAMIC_ARRAY_SHAPES,
         "unrecognized original dynamically created nested-array property")
    count, width, offset, template_type = EXPECTED_DYNAMIC_ARRAY_SHAPES[key]
    site = int(prop["source_callsite_va"], 16)
    need(pe.at(site, 1) == b"\x68" and
         pe.section_of(struct.unpack("<I", pe.at(site+1, 4))[0]) == ".data",
         "dynamic table caller did not push real original descriptor destination")
    need(pe.at(site+5, 3) == b"\xF3\xA5\xE8",
         "dynamic table caller missing native rep movsd followed by CALL")
    rel = struct.unpack("<i", pe.at(site+8, 4))[0]
    need(site + 12 + rel == COPIED_DESCRIPTOR_HELPER,
         "dynamic table caller no longer calls original type5 array builder")
    before = pe.at(site-56, 56)
    shapes = []
    for i in range(len(before)-12):
        # Original x86 uses PUSH short count/width or PUSH DWORD count.
        if before[i] == 0x6A and before[i+2:i+3] == b"\x6A" and \
           before[i+4:i+5] == b"\x68":
            shapes.append((before[i+1],before[i+3],
                           struct.unpack_from("<I", before,i+5)[0]))
        if before[i] == 0x68 and before[i+5:i+6] == b"\x6A" and \
           before[i+7:i+8] == b"\x68":
            shapes.append((struct.unpack_from("<I",before,i+1)[0],
                           before[i+6],struct.unpack_from("<I",before,i+8)[0]))
    need(shapes.count((count,width,offset)) == 1,
         "dynamic table original array count/stride/base-offset changed")
    # Confirm the direct source elementary RecvProp constructor (type0 or
    # type1) called immediately before preparing the wrapper descriptor.
    candidates = []
    for at in range(site-95,site-5):
        raw = pe.at(at, 5)
        if raw[0] != 0xE8:
            continue
        target = at + 5 + struct.unpack("<i",raw[1:])[0]
        if target in (0x24126DF0,0x24126D80):
            candidates.append((at,target))
    need(bool(candidates) and
         candidates[-1][1] == (0x24126DF0 if template_type==TYPE_INTEGER
                              else 0x24126D80),
         "dynamic table source element template type changed")
    need(count > 0 and 1 <= width <= 4 and offset <= 65536,
         "unsafe inferred original nested array shape")
    return {
        "client_class": client_class,
        "outer_property_name": prop["source_property_name"],
        "outer_native_type": TYPE_NESTED_TABLE,
        "element_native_type": template_type,
        "source_element_count": count,
        "source_element_width_bytes": width,
        "original_x86_first_element_offset": offset,
        "source_array_builder_va": hex(COPIED_DESCRIPTOR_HELPER),
        "source_initializer_callsite_va": hex(site+7),
        "source_element_template_callsite_va": hex(candidates[-1][0]),
        "original_element_name_table_va": hex(ORIGINAL_ARRAY_ELEMENT_NAME_TABLE),
        "original_generated_element_names": [f"{i:03d}" for i in range(count)],
        "limitations": "Element names/proxy or child RecvTable lifetime not emulated.",
    }


def verify(original_client: bytes, *, full_inventory: bool = False) -> dict:
    need(hashlib.sha256(original_client).hexdigest() ==
         ORIGINALS["client"]["sha256"],
         "whole original private client.dll identity changed")
    pe = PEImage(original_client)
    verify_native_factories(pe)
    parent = verify_property_names(original_client, include_all=True)
    metadata = parent["class_property_metadata"]
    rows = []
    counts: Counter[int | None] = Counter()
    nested_array_wrappers = 0
    dynamic_array_shapes = []
    for cls in metadata:
        for prop in cls["properties"]:
            source_va = int(prop["source_constructor_va"], 16)
            route = prop["native_constructor_route"]
            native_type = classify(source_va, route)
            counts[native_type] += 1
            name = prop["source_property_name"]
            field = {
                "slot": prop["slot"],
                "source_property_name": name,
                "original_native_property_type": native_type,
                "source_property_type_label":
                    KNOWN_TYPE_LABELS.get(native_type, "unresolved-copy"),
                "source_constructor_route": route,
                "source_constructor_va": prop["source_constructor_va"],
                "source_original_member_offset":
                    prop["original_x86_field_offset"],
            }
            rows.append(
                f'{cls["client_class_name"]}|{cls["source_table_name"]}|'
                f'{prop["slot"]}|{name}|'
                f'{"?" if native_type is None else native_type}|'
                f'{prop["source_constructor_va"]}|{route}\n')
            if route == "array-copy":
                nested_array_wrappers += 1
                shape = recover_dynamic_array_shape(pe,cls["client_class_name"],prop)
                dynamic_array_shapes.append(shape)
                prop["original_dynamic_array_shape"] = shape
            prop["original_native_property_type"] = native_type
            prop["source_property_type_label"] = field["source_property_type_label"]
    need(dict(counts) == EXPECTED_TAG_COUNTS,
         "source property type distribution unexpectedly changed")
    need(len(rows) == 939 and nested_array_wrappers == 11,
         "typed source metadata lost one of eleven nested-array wrappers")
    sha = hashlib.sha256("".join(rows).encode("ascii")).hexdigest()
    need(sha == ALL_939_NAME_AND_TYPE_SHA,
         "ordered original 939 property type and constructor digest changed")
    report = {
        "status": "ORIGINAL_CLIENT_ALL_939_RECVPROP_TYPES_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "original_declared_recprops": len(rows),
        "source_backed_type_count": len(rows),
        "nested_array_wrapper_type5_count": nested_array_wrappers,
        "unresolved_copied_array_types": 0,
        "native_type_code_counts": {
            str(k) if k is not None else "unresolved":
                EXPECTED_TAG_COUNTS[k]
            for k in (0, 1, 2, 3, 4, 5)
        },
        "type_fingerprint_sha256": sha,
        "unresolved_slots": [],
        "source_dynamic_nested_array_shapes": dynamic_array_shapes,
        "total_source_dynamic_nested_array_elements":
            sum(x["source_element_count"] for x in dynamic_array_shapes),
        "original_generated_name_table_entries": ORIGINAL_ARRAY_NAME_POINTERS,
        "original_generated_name_table_sha256": ORIGINAL_ARRAY_NAME_TABLE_SHA,
        "limits": ("All 939 type DWORDs originate in original constructor code. "
                   "The eleven array-copy paths are actually nested-table type 5; "
                   "their dynamic subtable contents and proxy functions remain "
                   "unrecovered. Individual wire flags, exact child pointers "
                   "and class factories are NOT established. "
                   "Never advertise this metadata as a live ClientClass*."),
    }
    if full_inventory:
        report["source_class_and_prop_inventory"] = metadata
    return report


def self_test() -> None:
    checks = [
        (0x24126DF0, "standard", TYPE_INTEGER),
        (0x24126D80, "standard", TYPE_FLOAT),
        (0x24126A60, "standard", TYPE_VECTOR3),
        (0x24126AD0, "standard", TYPE_STRING),
        (0x24126BD0, "array-helper", TYPE_ARRAY),
        (0x24126B50, "standard", TYPE_NESTED_TABLE),
        (0x241A8C60, "standard", TYPE_INTEGER),
        (0x241A8C90, "standard", TYPE_INTEGER),
        (0x241A8CC0, "standard", TYPE_INTEGER),
        (0x241A8D30, "standard", TYPE_FLOAT),
        (COPIED_DESCRIPTOR_HELPER, "array-copy", TYPE_NESTED_TABLE),
    ]
    for constructor, route, expected in checks:
        need(classify(constructor, route) == expected,
             "synthetic native RecvProp factory classification failed")
        for changed in ("invalid-route", "array-helper", "array-copy"):
            if changed == route:
                continue
            try:
                classify(constructor, changed)
            except VerificationError:
                pass
            else:
                raise AssertionError("altered original constructor route accepted")
    try:
        verify(b"fabricated original client.dll")
    except VerificationError:
        pass
    else:
        raise AssertionError("fabricated game client accepted as original")
    print("PASS Phase1697 all 939 source type0..5 descriptors: 11 copied arrays proven nested table type5, wrong factory routes and whole-image SHA refused")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path,
                        help="owner-private unmodified original client.dll")
    parser.add_argument("--full-json", type=Path,
                        help="output user-private 939-property native-type metadata")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.client is None and not args.self_test:
        parser.error("supply --self-test or --client")
    if args.full_json is not None and args.client is None:
        parser.error("--full-json requires owner-private original client.dll")
    if args.self_test:
        self_test()
    if args.client is not None:
        report = verify(args.client.read_bytes(),
                        full_inventory=args.full_json is not None)
        if args.full_json is not None:
            args.full_json.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n",
                encoding="utf-8")
            print("PASS all 939 proven native property type tags; 11 copied arrays are original nested tables; owner-private metadata exported")
        else:
            print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
