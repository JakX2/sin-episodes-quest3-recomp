#!/usr/bin/env python3
"""Phase1696: authenticate original 2006 engine's VClient011.slot5 consumer.

Read-only original PE32 client+engine SHA pins, full method hashes, x86
instruction witnesses and bounded source-list topology. Public --self-test
uses invented call encodings. No original DLL executable bytes are shipped.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_client_class_registry import (
    CLASS_COUNT, CATALOG_SHA, digest, need, registration_records,
)
from audit_sin_original_client_class_recvtable_links import verify as verify_class_recv

ENGINE_SHA = "f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
ENGINE_CLIENT_GLOBAL = 0x203E4010
ENGINE_FALLBACK_CLASS_HEAD = 0x203E4024
ENGINE_FIRST_GETTER = 0x20023B90
ENGINE_GETTER_SIZE = 21
ENGINE_GETTER_SHA = "0808ab9dc4dac55fd566a74eb7d4ab3907da8e333853c1541a2caa134c05e7cf"
ENGINE_TABLE_COLLECTOR = 0x20023BB0
ENGINE_COLLECTOR_SIZE = 125
ENGINE_COLLECTOR_SHA = "22716dfdf8b6996b7da5305f3dcf23b27ee181688459cbb27758d3807ae8a869"
ENGINE_RECURSIVE_RECVTABLE = 0x20085630
ENGINE_RECURSIVE_SIZE = 102
ENGINE_RECURSIVE_SHA = "09a4446ebba475508d1b87121e2df0c63752e1d60850150fe0e453b7f9f3a347"
ENGINE_RECVTABLE_LINK_LIMIT = 1024
SOURCE_RECVPROP_TYPE_NESTED = 5
SOURCE_RECVPROP_STRIDE = 60


def check_bytes(pe: PEImage, va: int, want: bytes, why: str) -> None:
    need(pe.at(va, len(want)) == want, "original engine " + why)


def call_target(pe: PEImage, address: int, expected: int) -> None:
    raw = pe.at(address, 5)
    need(raw[0] == 0xE8, "source engine rel32 CALL changed at " + hex(address))
    actual = address + 5 + struct.unpack("<i", raw[1:])[0]
    need(actual == expected,
         f"engine source CALL at {address:#x} target {actual:#x} != {expected:#x}")


def check_short_jcc(pe: PEImage, address: int, opcode: int,
                    expected_target: int) -> None:
    raw = pe.at(address, 2)
    need(raw[0] == opcode and
         address + 2 + struct.unpack("<b", raw[1:])[0] == expected_target,
         "engine source short branch changed at " + hex(address))


def verify(client: bytes, engine: bytes) -> dict:
    need(hashlib.sha256(client).hexdigest() == ORIGINALS["client"]["sha256"],
         "whole original 2006 client SHA256 changed")
    need(hashlib.sha256(engine).hexdigest() == ENGINE_SHA,
         "whole original 2006 engine SHA256 changed")
    classes = verify_class_recv(client)["class_links"]
    need(len(classes) == CLASS_COUNT,
         "original client RecvTable-linked classes not 191")
    cls = PEImage(client)
    eng = PEImage(engine)
    for va, size, digest_expected in (
        (ENGINE_FIRST_GETTER, ENGINE_GETTER_SIZE, ENGINE_GETTER_SHA),
        (ENGINE_TABLE_COLLECTOR, ENGINE_COLLECTOR_SIZE, ENGINE_COLLECTOR_SHA),
        (ENGINE_RECURSIVE_RECVTABLE, ENGINE_RECURSIVE_SIZE, ENGINE_RECURSIVE_SHA),
    ):
        need(eng.section_of(va) == ".text" and
             hashlib.sha256(eng.at(va, size)).hexdigest() == digest_expected,
             "whole original engine client registry method SHA mismatch " + hex(va))
        check_bytes(eng, va + size, b"\xCC" * 3,
                    "complete method padding boundary " + hex(va))

    # First engine entry is a 21-byte forwarding wrapper that tail-calls
    # original VClient011 slot5 if the client exists. Fallback global is
    # used only if this original engine client pointer is null.
    check_bytes(eng, ENGINE_FIRST_GETTER,
                b"\x8B\x0D" + struct.pack("<I", ENGINE_CLIENT_GLOBAL),
                "forward getter global client ptr")
    check_bytes(eng, 0x20023B96, b"\x85\xC9\x74\x05",
                "forward getter test and fallback branch")
    check_bytes(eng, 0x20023B9A, b"\x8B\x01\xFF\x60\x14",
                "forward getter original native TAIL JMP client slot5")
    check_bytes(eng, 0x20023B9F,
                b"\xA1" + struct.pack("<I", ENGINE_FALLBACK_CLASS_HEAD) + b"\xC3",
                "forward getter alternative global return")

    # Later engine routine enumerates the original whole linked class list
    # in SOURCE order and collects recvtable pointers at class node +0x0C.
    # This is the next original engine runtime dependency, after slot5.
    check_bytes(eng, 0x20023BBA,
                b"\x8B\x0D" + struct.pack("<I", ENGINE_CLIENT_GLOBAL),
                "collector original client global")
    check_bytes(eng, 0x20023BC8, b"\x8B\x01\xFF\x50\x14",
                "collector original VClient011.slot5 native call")
    check_bytes(eng, 0x20023BD1,
                b"\x8B\x35" + struct.pack("<I", ENGINE_FALLBACK_CLASS_HEAD),
                "collector fallback global")
    check_bytes(eng, 0x20023BE2, b"\x81\xFF\x00\x04\x00\x00",
                "original 1024-entry static stack-buffer compare")
    check_short_jcc(eng, 0x20023BE8, 0x72, 0x20023BF4)
    check_bytes(eng, 0x20023BF4, b"\x8B\x4E\x0C",
                "original RecvTable pointer load class+0x0C")
    check_bytes(eng, 0x20023BF7, b"\x8B\x76\x10",
                "original next node load class+0x10")
    check_bytes(eng, 0x20023BFA, b"\x89\x4C\xBC\x0C",
                "source stack staging of each RecvTable pointer")
    check_bytes(eng, 0x20023C10, b"\x8B\x54\xB4\x08\x52",
                "source later stage of each saved RecvTable pointer")
    call_target(eng, 0x20023C15, ENGINE_RECURSIVE_RECVTABLE)
    check_bytes(eng, 0x20023C26, b"\x81\xC4\x00\x10\x00\x00\xC3",
                "source collector stack cleanup and RET")

    # Original recursive routine treats input as the REAL x86 RecvTable
    # structure. It verifies and writes a state flag at +0x11, and
    # recursively visits property type 5 using nested table pointer
    # +0x28, not merely class names or fake table descriptors.
    check_bytes(eng, 0x20085630, b"\x56\x8B\x74\x24\x08",
                "nested table single stack argument ABI prefix")
    check_bytes(eng, 0x20085635, b"\x80\x7E\x11\x00",
                "RecvTable native already-processed flag +0x11")
    check_short_jcc(eng, 0x20085639, 0x75, 0x20085694)
    check_bytes(eng, 0x2008563B, b"\x83\x7E\x08\x00",
                "native RecvTable +8 metadata conditional")
    check_bytes(eng, 0x2008565E, b"\xC6\x46\x11\x01",
                "RecvTable +0x11 initialized flag set before nested traversal")
    call_target(eng, 0x20085662, 0x20086930)
    check_bytes(eng, 0x20085669, b"\x39\x5E\x04",
                "source RecvTable property count at +4")
    check_bytes(eng, 0x20085671, b"\x8B\x06\x03\xC7",
                "source RecvTable properties pointer +0")
    check_bytes(eng, 0x20085675, b"\x83\x78\x04\x05",
                "source nested RecvProp type=5 at descriptor +4")
    check_bytes(eng, 0x2008567B, b"\x8B\x50\x28\x52",
                "source nested RecvTable pointer at property+0x28")
    call_target(eng, 0x2008567F, ENGINE_RECURSIVE_RECVTABLE)
    check_bytes(eng, 0x2008568A, b"\x83\xC7\x3C",
                "native RecvProp descriptor 0x3C stride")
    check_bytes(eng, 0x2008568D, b"\x3B\x5E\x04",
                "counted property traversal and recursive return")
    check_bytes(eng, 0x20085693, b"\x5B\x5E\xC3",
                "original nested table helper return ABI")

    need(CLASS_COUNT < ENGINE_RECVTABLE_LINK_LIMIT,
         "real source 191 class tables exceed collector stack guard")
    return {
        "status": "RETAIL_ENGINE_CLIENTCLASS_LINK_LIST_AND_NESTED_RECVTABLE_CONSUMER_AUTHENTICATED",
        "original_client_sha256": ORIGINALS["client"]["sha256"],
        "original_engine_sha256": ENGINE_SHA,
        "distinct_complete_original_engine_functions": 3,
        "distinct_complete_original_engine_function_bytes":
            ENGINE_GETTER_SIZE + ENGINE_COLLECTOR_SIZE + ENGINE_RECURSIVE_SIZE,
        "engine_original_vclient011_slot5_call_va": hex(0x20023BCA),
        "engine_class_node_recvtable_pointer_field": "+0x0C",
        "engine_class_node_next_pointer_field": "+0x10",
        "original_class_link_count": CLASS_COUNT,
        "engine_source_stack_link_threshold": ENGINE_RECVTABLE_LINK_LIMIT,
        "engine_native_recvtable_initialized_flag_offset": "+0x11",
        "engine_nested_recvprop_native_type": SOURCE_RECVPROP_TYPE_NESTED,
        "engine_recvprop_stride_bytes": SOURCE_RECVPROP_STRIDE,
        "engine_nested_recvtable_pointer_offset": "+0x28",
        "source_nested_table_recursive_call": hex(0x2008567F),
        "limits": ("Authenticates original engine machine-code data consumers "
                   "only. No runnable RecvTable or proxy, no class creator "
                   "ABI, no independent retail client slot5 success, "
                   "no game/map boot. Original 1024-check diagnostic "
                   "may terminate; whether its target returns is not proven."),
    }


def self_test() -> None:
    source, target = 0x10005000, 0x10005080
    blob = b"\xE8" + struct.pack("<i", target - source - 5)
    class Mock:
        def at(self, va, length):
            return blob[:length]
    mock = Mock()
    call_target(mock, source, target)
    for pos in range(len(blob)):
        mutated = bytearray(blob)
        mutated[pos] ^= 1
        class Other:
            def at(self, va, length):
                return bytes(mutated[:length])
        try:
            call_target(Other(), source, target)
        except VerificationError:
            pass
        else:
            raise AssertionError("altered invented native engine CALL accepted")
    need(CLASS_COUNT == 191 and CLASS_COUNT < ENGINE_RECVTABLE_LINK_LIMIT,
         "original class count exceeds source stack threshold")
    try:
        verify(b"fabricated original client", b"fabricated original engine")
    except VerificationError:
        pass
    else:
        raise AssertionError("unauthenticated original binaries accepted")
    print("PASS Phase1696 original engine RecvTable consumer: source stack 1024, recursive type5/60B contract, mutated CALL and whole-image SHA refusals")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, help="owner-private original client.dll")
    parser.add_argument("--engine", type=Path, help="owner-private original engine.dll")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.client is None and args.engine is None and not args.self_test:
        parser.error("supply --self-test and/or original --client and --engine")
    if (args.client is None) != (args.engine is None):
        parser.error("--client and --engine must be provided together")
    if args.self_test:
        self_test()
    if args.client is not None:
        print(json.dumps(verify(args.client.read_bytes(), args.engine.read_bytes()),
                         indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
