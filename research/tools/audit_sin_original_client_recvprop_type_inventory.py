#!/usr/bin/env python3
"""Phase1697: recover source-backed numeric RecvProp types of SiN 2006.

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
# Original 0x24126E90 copies PRECONSTRUCTED variable-size arrays; the
# destination slot's exact type is NOT proven by that helper alone.
COPIED_DESCRIPTOR_HELPER = 0x24126E90
EXPECTED_TAG_COUNTS = {
    TYPE_INTEGER: 339,
    TYPE_FLOAT: 238,
    TYPE_VECTOR3: 136,
    TYPE_STRING: 12,
    TYPE_ARRAY: 12,
    TYPE_NESTED_TABLE: 191,
    None: 11,
}
ALL_939_NAME_AND_TYPE_SHA = "43b5a4634ed0566c1065794dcdb051aca7bd663ec187fe4295432ede5515106c"

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
KNOWN_TYPE_LABELS = {
    0: "integer", 1: "float", 2: "vector3",
    3: "string", 4: "array", 5: "nested-table",
}


def classify(constructor_va: int, route: str) -> int | None:
    if route == "array-copy":
        need(constructor_va == COPIED_DESCRIPTOR_HELPER,
             "copied descriptor uses unknown original constructor")
        return None
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
    for site, target in SOURCE_WRAPPER_CALLS:
        raw = pe.at(site, 5)
        need(raw[0] == 0xE8 and
             site + 5 + struct.unpack("<i", raw[1:])[0] == target,
             f"original nested type-wrapper call target drift {site:#x}")


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
    unresolved = []
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
            if native_type is None:
                unresolved.append((cls["client_class_name"], name))
            prop["original_native_property_type"] = native_type
            prop["source_property_type_label"] = field["source_property_type_label"]
    need(dict(counts) == EXPECTED_TAG_COUNTS,
         "source property type distribution unexpectedly changed")
    need(len(rows) == 939 and len(unresolved) == 11,
         "typed source metadata lost a property")
    sha = hashlib.sha256("".join(rows).encode("ascii")).hexdigest()
    need(sha == ALL_939_NAME_AND_TYPE_SHA,
         "ordered original 939 property type and constructor digest changed")
    report = {
        "status": "ORIGINAL_CLIENT_928_OF_939_RECVPROP_TYPES_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "original_declared_recprops": len(rows),
        "source_backed_type_count": len(rows) - len(unresolved),
        "unresolved_copied_array_types": len(unresolved),
        "native_type_code_counts": {
            str(k) if k is not None else "unresolved":
                EXPECTED_TAG_COUNTS[k]
            for k in (0, 1, 2, 3, 4, 5, None)
        },
        "type_fingerprint_sha256": sha,
        "unresolved_slots": unresolved,
        "limits": ("Types originate in original constructor code; "
                   "not all per-property proxy functions, quantization, "
                   "flags or nested RecvTable pointers are known. "
                   "Never advertise this metadata as a live ClientClass* "
                   "or assume the 11 copied properties have types."),
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
        (COPIED_DESCRIPTOR_HELPER, "array-copy", None),
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
    print("PASS Phase1697 source type0..5 constructors, four wrapper calls, "
          "copied-array unknown refusal and wrong whole-image SHA")


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
            print("PASS 928 proven original type tags; 11 unresolved "
                  "copied-array types; owner-private metadata exported")
        else:
            print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
