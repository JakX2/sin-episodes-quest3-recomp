#!/usr/bin/env python3
"""Audit the exact original SiN 2006 VClient011 VGUI input/system Init gate.

Only complete SHA-pinned original PE32 files supplied privately by the owner
can authenticate real machine-code evidence. Public --self-test uses invented
instruction witnesses; it never includes commercial original DLL bytes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_client_init_interface_manifest import (
    CLIENT_SHA, SERVER_SHA, INIT_VA, INIT_LEN, INIT_SHA,
    PE32, Refused, require, check_jz_target, check_ascii,
)

HELPER_VA = 0x241EDFB0
HELPER_BYTES = 123
HELPER_SHA = "64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d"
NOOP_VA = 0x2422ECD0
NOOP_SHA = "ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e"
VGUI_QUERY_VA = 0x2422ECF0
VGUI_QUERY_BYTES = 14
VGUI_QUERY_SHA = "d1f9d41fb9e6c8628fa934c2ac6a848a9aae39e604b44f7b4f1ddab5ceed29b3"
NAMES = (
    (0x242AC524, "CLIENT"),
    (0x242AC50C, "VGUI_InputInternal001"),
    (0x242D32C0, "VGUI_System010"),
    (0x242AC4F8, "CHudTextureHandle"),
)
# Original native callsite, mandatory global destination, exact return gate.
FACTORY_REQUESTS = (
    (0x241EDFCB, 0x242AC50C, 0x241EDFDB, 0x24378AB8, 0x241EDFE0),
    (0x241EDFE2, 0x242D32C0, 0x241EDFF2, 0x24378ABC, 0x241EDFF7),
)


def check_call(read, source: int, target: int) -> None:
    raw = read(source, 5)
    require(len(raw) == 5 and raw[0] == 0xE8,
            f"missing original rel32 call at {source:#x}")
    actual = source + 5 + struct.unpack_from("<i", raw, 1)[0]
    require(actual == target,
            f"original rel32 target mismatch at {source:#x}: {actual:#x}")


def check_short_jcc(read, source: int, target: int, opcode: int) -> None:
    raw = read(source, 2)
    require(len(raw) == 2 and raw[0] == opcode,
            f"original conditional opcode mismatch at {source:#x}")
    actual = source + 2 + struct.unpack("<b", raw[1:2])[0]
    require(actual == target,
            f"original conditional target mismatch at {source:#x}: {actual:#x}")


def verify(server: bytes, client: bytes) -> dict:
    require(hashlib.sha256(server).hexdigest() == SERVER_SHA,
            "original server.dll SHA256 mismatch")
    require(hashlib.sha256(client).hexdigest() == CLIENT_SHA,
            "original client.dll SHA256 mismatch")
    pe = PE32(client)
    require(pe.base == 0x24000000,
            "original client.dll base mismatch")
    require(hashlib.sha256(pe.at(INIT_VA, INIT_LEN)).hexdigest() == INIT_SHA,
            "whole original VClient011.Init SHA256 mismatch")
    for va, size, expected in (
        (HELPER_VA, HELPER_BYTES, HELPER_SHA),
        (NOOP_VA, 1, NOOP_SHA),
        (VGUI_QUERY_VA, VGUI_QUERY_BYTES, VGUI_QUERY_SHA),
    ):
        require(pe.section(va) == ".text",
                f"original helper outside executable .text at {va:#x}")
        require(hashlib.sha256(pe.at(va, size)).hexdigest() == expected,
                f"whole original helper SHA256 mismatch at {va:#x}")
        require(pe.at(va + size, 1) == b"\xCC",
                f"original helper end boundary mismatch at {va:#x}")

    for pointer, name in NAMES:
        check_ascii(pe, pointer, name)

    # Real Init calls helper after the previous mandatory client subsystem gate,
    # passes the application factory and fails if this helper returns false.
    check_call(pe.at, 0x240FE3C7, HELPER_VA)
    require(pe.at(0x240FE3C6, 1) == b"\x56" and
            pe.at(0x240FE3CC, 5) == b"\x83\xC4\x04\x84\xC0",
            "original VGUI helper factory argument or result check changed")
    check_jz_target(pe, 0x240FE3D1, 0x240FE59E)

    # Calls the original CLIENT-tagged list resolver with one factory pointer.
    prefix = b"\x6A\x01\x8D\x44\x24\x08\x50\x68" + struct.pack(
        "<I", NAMES[0][0])
    require(pe.at(HELPER_VA, len(prefix)) == prefix,
            "CLIENT resolver argument order changed")
    check_call(pe.at, 0x241EDFBC, 0x240010D0)
    require(pe.at(0x241EDFC4, 7) == b"\x84\xC0\x75\x03\x32\xC0\xC3",
            "CLIENT resolver bool refusal changed")

    for query_va, name_va, store_va, store_ptr, gate_va in FACTORY_REQUESTS:
        require(pe.at(query_va, 7) == b"\x6A\x00\x68" + struct.pack("<I", name_va),
                f"factory request name or null status changed at {query_va:#x}")
        require(pe.at(query_va + 7, 4) == b"\xFF\x54\x24\x0C",
                f"source factory indirect call changed at {query_va:#x}")
        require(pe.at(store_va, 5) == b"\xA3" + struct.pack("<I", store_ptr),
                f"source global result destination changed at {store_va:#x}")
        check_short_jcc(pe.at, gate_va, 0x241EDFC8, 0x74)

    # Do not silently drop the registration-once flag or pretend failure
    # rolls back the previously written interface pointers.
    require(pe.at(0x241EDFF9, 7) ==
            b"\x80\x3D" + struct.pack("<I", 0x24378B28) + b"\x00",
            "one-time HUD handle registration flag check changed")
    check_short_jcc(pe.at, 0x241EE000, 0x241EE01B, 0x75)
    require(pe.at(0x241EE002, 10) ==
            b"\x68" + struct.pack("<I", 0x2431DDCC) +
            b"\x68" + struct.pack("<I", NAMES[3][0]),
            "CHudTextureHandle registration arguments changed")
    require(pe.at(0x241EE00C, 7) ==
            b"\xC6\x05" + struct.pack("<I", 0x24378B28) + b"\x01",
            "one-time registration flag mutation changed")
    check_call(pe.at, 0x241EE013, 0x24009340)
    check_call(pe.at, 0x241EE01B, NOOP_VA)
    check_call(pe.at, 0x241EE020, VGUI_QUERY_VA)

    # No-op is genuinely empty; the final getter calls original
    # VEngineVGui001 virtual +0x04 with numeric argument 2, returning
    # a non-null condition as the real helper's success bool.
    require(pe.at(NOOP_VA, 1) == b"\xC3",
            "original empty lifecycle helper changed")
    require(pe.at(VGUI_QUERY_VA, VGUI_QUERY_BYTES) ==
            b"\x8B\x0D" + struct.pack("<I", 0x2433F6B0) +
            b"\x8B\x01\x6A\x02\xFF\x50\x04\xC3",
            "original VEngineVGui001 slot1 query changed")
    require(pe.at(0x241EE025, 6) == b"\x85\xC0\x0F\x95\xC0\xC3",
            "final result zero/nonzero conversion changed")

    return {
        "status": "ORIGINAL_CLIENT_VGUI_INPUT_SYSTEM_REQUIRED_INIT_AUTHENTICATED",
        "original_client_sha256": CLIENT_SHA,
        "called_from_vclient011_init": hex(0x240FE3C7),
        "helper_va": hex(HELPER_VA),
        "complete_helper_bytes": HELPER_BYTES,
        "complete_helper_sha256": HELPER_SHA,
        "other_complete_helper_bytes": 1 + VGUI_QUERY_BYTES,
        "factory_interfaces": [NAMES[1][1], NAMES[2][1]],
        "factory_result_globals": [hex(x[3]) for x in FACTORY_REQUESTS],
        "once_registered_literal": NAMES[3][1],
        "nonzero_engine_vgui_virtual_result_required": True,
        "proof_limit": "Original static code only; no retail callback invocation, full client Init, rendered UI or game boot.",
    }


def self_test() -> None:
    source, target = 0x12345000, 0x12346800
    call = b"\xE8" + struct.pack("<i", target - source - 5)
    check_call(lambda at, n: call[:n], source, target)
    for i in range(len(call)):
        mutated = bytearray(call)
        mutated[i] ^= 0x01
        try:
            check_call(lambda at, n: bytes(mutated[:n]), source, target)
        except Refused:
            pass
        else:
            raise AssertionError(f"altered native CALL accepted at byte {i}")
    for opcode in (0x74, 0x75):
        branch = bytes([opcode, 0x50])
        check_short_jcc(lambda at, n: branch[:n], source, source + 2 + 0x50, opcode)
        for i in range(2):
            mutated = bytearray(branch)
            mutated[i] ^= 0x01
            try:
                check_short_jcc(lambda at, n: bytes(mutated[:n]),
                                source, source + 2 + 0x50, opcode)
            except Refused:
                pass
            else:
                raise AssertionError("altered short conditional branch accepted")
    try:
        verify(b"synthetic server", b"synthetic client")
    except Refused:
        pass
    else:
        raise AssertionError("wrong whole original DLL SHA accepted")
    print("PASS original VGUI Init source audit: five rel32 and four short-Jcc mutations refused, SHA mismatch refused")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin-zip", type=Path,
                        help="owner-private SE1/bin.zip with original client.dll and server.dll")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if not args.self_test and args.bin_zip is None:
        parser.error("supply --self-test or --bin-zip")
    if args.self_test:
        self_test()
    if args.bin_zip is not None:
        with zipfile.ZipFile(args.bin_zip) as archive:
            server = archive.read("server.dll")
            client = archive.read("client.dll")
        print(json.dumps(verify(server, client), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
