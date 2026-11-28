#!/usr/bin/env python3
"""Original SiN 2006 client sound-name/pool initialization, SHA-pinned audit.

Public --self-test uses invented instruction bytes only. Real verification
requires the owner's original local SE1/bin.zip; no commercial code is emitted.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_client_init_interface_manifest import (
    CLIENT_SHA, SERVER_SHA, PE32, Refused, require,
)

# Three DISTINCT complete methods, no parent method / nested subroutine
# counted twice. Names below are semantic labels, not original symbols.
METHODS = (
    (0x241BF880, 341, "020c1810c4b24ab08688d946bf5f784fcd467938aa66c9bf8928d77b9abc1cf5",
     "client_sound_name_and_parameters_initializer"),
    (0x241BCC50, 41, "2cc1aeb6d3d13d2e5f6f437f360ed3a958d665c3ffb0fae99cb7b7f517c142fc",
     "sound_emitter_two_virtual_calls"),
    (0x241BF5F0, 195, "ee6abaccb339549ba4ef800f200a39c408d80adc8ad9b1ef54a882a8fed80b1a",
     "caller_parameter_record_flags_and_list_copy"),
    # Four independently identified original source object-creation methods.
    (0x241C00C0, 217, "9b6de2cd6391cd721ca2546a40273eb2389c3d93299d405b943b6a53f8760dad",
     "sound_record_creator_with_source_handle"),
    (0x241C01A0, 290, "61215820358176a1c53e6fbb1b28fff51f7039308ac3b3f750bde8f56b2265ed",
     "sound_record_creator_with_numeric_conversion"),
    (0x241C02D0, 220, "3c1135ed68e80e85178fd951f93652ce847c625d44878b211b006f693e720bad",
     "sound_record_creator_with_pooled_handle"),
    (0x241C03B0, 289, "e535cfee079f01106cebacc156ef0e2fccfff932f5ec9468fdda3af4a2658019",
     "sound_record_creator_with_two_parameter_updates"),
)
SOUND_GLOBAL = 0x2433F6E4
STRISTR_IAT = 0x24276238
NAME_INTERN = 0x2413D250
SEARCH_TOKENS = ((0x242A6BD4, ".wav"), (0x242A6BCC, ".mp3"))
INTERN_CALLS = (0x241BF934, 0x241BF94A, 0x241BF967)
# Four independent client object-creation paths call the same initializer.
INITIALIZER_CALLERS = (0x241C018D, 0x241C02B6, 0x241C03A0, 0x241C047E)


def need_bytes(pe: PE32, address: int, expected: bytes, reason: str) -> None:
    require(pe.at(address, len(expected)) == expected,
            f"{reason} at {address:#x}")


def rel_call(read, address: int, expected_target: int) -> None:
    raw = read(address, 5)
    require(len(raw) == 5 and raw[0] == 0xE8,
            f"missing relative CALL at {address:#x}")
    actual = address + 5 + struct.unpack("<i", raw[1:5])[0]
    require(actual == expected_target,
            f"wrong relative CALL at {address:#x}, target {actual:#x}")


def short_branch(read, address: int, expected_target: int, opcode: int) -> None:
    raw = read(address, 2)
    require(len(raw) == 2 and raw[0] == opcode,
            f"wrong conditional opcode at {address:#x}")
    actual = address + 2 + struct.unpack("<b", raw[1:2])[0]
    require(actual == expected_target,
            f"wrong short branch at {address:#x}, target {actual:#x}")


def verify(server: bytes, client: bytes) -> dict:
    require(hashlib.sha256(server).hexdigest() == SERVER_SHA,
            "original server.dll SHA256 mismatch")
    require(hashlib.sha256(client).hexdigest() == CLIENT_SHA,
            "original client.dll SHA256 mismatch")
    pe = PE32(client)
    require(pe.base == 0x24000000, "original client image base mismatch")

    for va, size, digest, label in METHODS:
        require(pe.section(va) == ".text", label + " not original .text")
        require(hashlib.sha256(pe.at(va, size)).hexdigest() == digest,
                label + " whole-function SHA256 mismatch")
        need_bytes(pe, va + size, b"\xCC",
                   label + " complete method boundary missing")

    # The original uses the Q_stristr IAT (C++ decorated name in original
    # import table) for case-insensitive SUBSTRING checks, not suffix tests.
    need_bytes(pe, 0x241BF8B2, b"\x8B\x2D" + struct.pack("<I", STRISTR_IAT),
               "Q_stristr imported pointer")
    for address, name in SEARCH_TOKENS:
        need_bytes(pe, address, name.encode("ascii") + b"\x00",
                   "original sound token")
    need_bytes(pe, 0x241BF8C5, b"\x68" + struct.pack("<I", SEARCH_TOKENS[0][0]),
               "first .wav lookup")
    need_bytes(pe, 0x241BF901, b"\xFF\xD5", "first Q_stristr invocation")
    short_branch(pe.at, 0x241BF908, 0x241BF949, 0x75)
    need_bytes(pe, 0x241BF90A, b"\x68" + struct.pack("<I", SEARCH_TOKENS[1][0]),
               "second .mp3 lookup")
    need_bytes(pe, 0x241BF910, b"\xFF\xD5", "second Q_stristr invocation")
    short_branch(pe.at, 0x241BF917, 0x241BF949, 0x75)

    # Real native client constructor/setup callers, NOT guessed SDK methods.
    for site in INITIALIZER_CALLERS:
        rel_call(pe.at, site, 0x241BF880)

    # Four entire native creator methods, source-verified against original
    # whole-method hashes and complete padded function boundaries.
    # All request 0x64 object bytes from the same native allocator and
    # initialize named-parameter object storage before common Init.
    for begin, alloc_site, vtptr_site, counter_site in (
        (0x241C00C2, 0x241C00C9, 0x241C00F2, 0x241C010B),
        (0x241C01A2, 0x241C01A9, 0x241C01D2, 0x241C01EB),
        (0x241C02D2, 0x241C02D9, 0x241C0302, 0x241C031B),
        (0x241C03B3, 0x241C03BA, 0x241C03E2, 0x241C03FB),
    ):
        need_bytes(pe, begin, b"\x6A\x64", "original 0x64-byte record allocation request")
        rel_call(pe.at, alloc_site, 0x241703E0)
        need_bytes(pe, vtptr_site,
                   b"\xC7\x40\x48" + struct.pack("<I", 0x242A7108),
                   "original +0x48 record descriptor pointer")
        need_bytes(pe, counter_site,
                   b"\x83\x05" + struct.pack("<I", 0x243757F8) + b"\x01",
                   "original allocated-record count increment")
    # Two creator variants return with 5 stack arguments; the first and
    # fourth return with 3. Do NOT infer
    # full higher-level signatures or pretend null allocator safety.
    for address, tail in (
        (0x241C0196, b"\xC2\x0C\x00"),
        (0x241C02BF, b"\xC2\x14\x00"),
        (0x241C03A9, b"\xC2\x14\x00"),
        (0x241C04CE, b"\xC2\x0C\x00"),
    ):
        need_bytes(pe, address, tail, "creator source ABI tail")
    rel_call(pe.at, 0x241C04AC, 0x241BE4A0)
    rel_call(pe.at, 0x241C04C4, 0x241BE4A0)
    need_bytes(pe, 0x241C0486, b"\x83\x4E\x3C\x01",
               "creator 3 parameter source flag 1")
    need_bytes(pe, 0x241C04B4, b"\x83\x4E\x3C\x02",
               "creator 3 parameter source flag 2")

    # Only when neither substring is present, the caller routes through
    # two ORIGINAL VSoundEmitter001 virtual methods (+0x78 then +0x1C).
    rel_call(pe.at, 0x241BF920, 0x241BCC50)
    need_bytes(pe, 0x241BF928, b"\x84\xC0", "resolver Boolean test")
    short_branch(pe.at, 0x241BF92A, 0x241BF949, 0x74)
    need_bytes(pe, 0x241BCC50, b"\x8B\x0D" + struct.pack("<I", SOUND_GLOBAL),
               "sound emitter global")
    need_bytes(pe, 0x241BCC5D, b"\xFF\x50\x78", "emitter virtual byte offset 0x78")
    need_bytes(pe, 0x241BCC60, b"\x8B\x0D" + struct.pack("<I", SOUND_GLOBAL),
               "emitter reloaded after first virtual")
    need_bytes(pe, 0x241BCC75, b"\xFF\x52\x1C", "emitter virtual byte offset 0x1C")
    need_bytes(pe, 0x241BCC78, b"\xC3", "emitter wrapper return")

    # Both the successful resolver and literal/fallback branches intern the
    # original filename with original CGameStringPool::allocate; never use
    # caller's temporary buffer as stored ownership.
    for site in INTERN_CALLS:
        rel_call(pe.at, site, NAME_INTERN)
    for site in (0x241BF93D, 0x241BF94F):
        need_bytes(pe, site, b"\x89\x46\x30",
                   "owned filename pointer write to this+0x30")
    need_bytes(pe, 0x241BF972, b"\x89\x46\x2C",
               "second owned pointer write to this+0x2C")
    need_bytes(pe, 0x241BF931, b"\x89\x4E\x44",
               "resolver-derived numeric/float field")
    need_bytes(pe, 0x241BF959, b"\xC7\x46\x44\x00\x00\x80\x3F",
               "literal/failure default float 1.0 at this+0x44")
    short_branch(pe.at, 0x241BF947, 0x241BF963, 0xEB)
    rel_call(pe.at, 0x241BF9C5, 0x241BF5F0)
    need_bytes(pe, 0x241BF9D2, b"\xC2\x14\x00",
               "initializer x86 return with 5 stack arguments")

    # Post-name helper: sets base flag 1, tests optional input slots
    # and marks bit 2/4, then walks its record provider with byte-offset
    # +0x0C (count) and +0x10 (entry). Do not assign original C++ types.
    need_bytes(pe, 0x241BF5FA, b"\xC7\x46\x04\x01\x00\x00\x00",
               "initial parameter bitfield 1")
    for at, expected in (
        (0x241BF609, b"\xFF\x50\x04"),  # virtual byte +4
        (0x241BF618, b"\xFF\x52\x08"),  # virtual byte +8
        (0x241BF629, b"\xFF\x50\x0C"),  # record count
        (0x241BF636, b"\xFF\x52\x10"),  # indexed record lookup
        (0x241BF610, b"\x83\x4E\x04\x02"),
        (0x241BF61F, b"\x83\x4E\x04\x04"),
    ):
        need_bytes(pe, at, expected, "source parameter virtual/flag witness")
    rel_call(pe.at, 0x241BF65B, 0x2401B4C0)
    rel_call(pe.at, 0x241BF682, 0x24248B50)
    need_bytes(pe, 0x241BF6B0, b"\xC2\x04\x00",
               "parameter adapter native return")

    return {
        "status": "ORIGINAL_CLIENT_SOUND_NAME_STRINGPOOL_INIT_AUTHENTICATED",
        "original_client_sha256": CLIENT_SHA,
        "distinct_complete_functions": len(METHODS),
        "new_complete_method_bytes": sum(x[1] for x in METHODS),
        "complete_method_hashes": {hex(va): digest for va, _, digest, _ in METHODS},
        "source_sound_substring_tokens": [x[1] for x in SEARCH_TOKENS],
        "source_sound_emitter_virtual_offsets": ["0x78", "0x1c"],
        "client_string_pool_intern_calls": len(INTERN_CALLS),
        "original_client_initializer_callers": [hex(x) for x in INITIALIZER_CALLERS],
        "client_interned_pointer_fields": ["this+0x30", "this+0x2c"],
        "limits": "Original static source only; no confirmed C++ type, live sound call, complete Init, Android or gameplay.",
    }


def self_test() -> None:
    source, target = 0x10010000, 0x10012300
    call = b"\xE8" + struct.pack("<i", target - source - 5)
    rel_call(lambda address, n: call[:n], source, target)
    for i in range(len(call)):
        changed = bytearray(call)
        changed[i] ^= 0x01
        try:
            rel_call(lambda address, n: bytes(changed[:n]), source, target)
        except Refused:
            pass
        else:
            raise AssertionError("altered relative CALL accepted")
    for opcode in (0x74, 0x75, 0xEB):
        original = bytes((opcode, 0x44))
        short_branch(lambda address, n: original[:n],
                     source, source + 2 + 0x44, opcode)
        for i in range(2):
            changed = bytearray(original)
            changed[i] ^= 0x01
            try:
                short_branch(lambda address, n: bytes(changed[:n]),
                             source, source + 2 + 0x44, opcode)
            except Refused:
                pass
            else:
                raise AssertionError("altered native branch accepted")
    try:
        verify(b"fabricated server.dll", b"fabricated client.dll")
    except Refused:
        pass
    else:
        raise AssertionError("non-original DLLs were accepted")
    print("PASS original sound-name source: rel32 CALL/short branch mutations and wrong whole-image SHA rejected")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--bin-zip", type=Path,
                        help="owner-private SE1/bin.zip, never uploaded to CI")
    args = parser.parse_args()
    if not args.self_test and args.bin_zip is None:
        parser.error("provide --self-test and/or --bin-zip")
    if args.self_test:
        self_test()
    if args.bin_zip is not None:
        with zipfile.ZipFile(args.bin_zip) as archive:
            client, server = archive.read("client.dll"), archive.read("server.dll")
        print(json.dumps(verify(server, client), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
