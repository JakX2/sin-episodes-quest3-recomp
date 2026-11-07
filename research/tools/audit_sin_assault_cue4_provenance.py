#!/usr/bin/env python3
"""2006 SiN server.dll: verify which class owns the tempting +0x20=4 write.

Read-only SHA-pinned evidence. Direct E8/E9 and pointer scans do not rule out
indirect/runtime-computed references to the generic assault cue setter.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

CUE_SETTER = 0x220CE520
STAT_RTTI = 0x2252191C
STAT_COL = 0x224CA970
STAT_VTABLE_LOCATOR = 0x2245CBD0
STAT_VTABLE_ENTRIES = ((0x2245CBF8, 0x222C4D00),
                       (0x2245CBFC, 0x222C4D40),
                       (0x2245CC00, 0x222C4D80))
WRITES = (
    ("CStatAdvisor writes 3", 0x222C4D1A, "c7 41 20 03 00 00 00"),
    ("CStatAdvisor writes 4", 0x222C4D2B, "c7 41 20 04 00 00 00"),
    ("CStatAdvisor writes 0", 0x222C4D6A, "c7 41 20 00 00 00 00"),
    ("CStatAdvisor writes 2", 0x222C4D80, "c7 41 20 02 00 00 00"),
    ("AssaultBehavior mode 3 writes 3", 0x220CEAC1, "c7 41 20 03 00 00 00"),
    ("goal BeginAssault writes 1", 0x220CFFA0, "c7 41 20 01 00 00 00"),
    ("AssaultBehavior generic cue helper writes caller argument", 0x220CE539, "89 41 20"),
)


def require(ok, message):
    if not ok:
        raise VerificationError(message)


def u32(pe, va):
    return struct.unpack("<I", pe.at(va, 4))[0]


def raw_section(pe, name):
    """Raw section bytes, never virtual zero padding; no disassembler dependency."""
    for section_name, rva, _vsize, raw_offset, raw_size in pe.sections:
        if section_name == name:
            require(raw_offset + raw_size <= len(pe.data), f"{name}: invalid raw section")
            return pe.base + rva, pe.data[raw_offset:raw_offset + raw_size]
    raise VerificationError(f"missing original {name} section")


def direct_e8_e9_candidates(base, code, target):
    """Bytewise overapproximation: does not assert x86 instruction boundaries."""
    matches = []
    for index in range(len(code) - 4):
        if code[index] in (0xE8, 0xE9):
            destination = (base + index + 5 + struct.unpack_from("<i", code, index+1)[0]) & 0xFFFFFFFF
            if destination == target:
                matches.append(f"0x{base+index:08X}")
    return matches


def literal_pointers(base, data, target):
    pattern = struct.pack("<I", target)
    found, offset = [], 0
    while (index := data.find(pattern, offset)) != -1:
        found.append(f"0x{base+index:08X}")
        offset = index + 1
    return found


def audit_original(pe):
    require(pe.base == 0x22000000, "wrong original image base")
    require(pe.at(STAT_RTTI+8, 19) == b".?AVCStatAdvisor@@\0", "CStatAdvisor RTTI mismatch")
    require(pe.at(0x224E4C3C, 26) == b".?AVCAI_AssaultBehavior@@\0", "AssaultBehavior RTTI mismatch")
    require(u32(pe, STAT_VTABLE_LOCATOR) == STAT_COL, "CStatAdvisor vtable COL pointer mismatch")
    require(u32(pe, STAT_COL+12) == STAT_RTTI, "CStatAdvisor COL owner mismatch")
    for slot, target in STAT_VTABLE_ENTRIES:
        require(u32(pe, slot) == target, f"CStatAdvisor method pointer changed at {slot:#x}")
    for role, va, signature in WRITES:
        expected = bytes.fromhex(signature)
        require(pe.at(va, len(expected)) == expected, f"{role}: altered opcode at {va:#x}")
    require(pe.at(CUE_SETTER, 9) == bytes.fromhex("8b 41 04 33 d2 3b c2 74 0c"),
            "generic assault cue setter prologue changed")
    require(pe.at(0x220CE535, 7) == bytes.fromhex("8b 44 24 04 89 41 20"),
            "generic assault cue setter no longer writes its parameter")
    tail = pe.at(0x220CE547, 5)
    require(tail[0] == 0xE9 and
            (0x220CE54C + struct.unpack_from("<i", tail, 1)[0]) & 0xFFFFFFFF == 0x22039760,
            "generic assault cue setter tail no longer calls original SetCondition")
    base, code = raw_section(pe, ".text")
    transfers = direct_e8_e9_candidates(base, code, CUE_SETTER)
    addresses = {name: literal_pointers(*raw_section(pe, name), CUE_SETTER)
                 for name in (".rdata", ".data")}
    require(not transfers, "new direct CALL/JMP candidate to generic cue setter: investigate")
    require(not any(addresses.values()), "new literal data pointer to cue setter: investigate")
    return {
        "original_class_rtti": {"CStatAdvisor": f"0x{STAT_RTTI:08X}",
                                "CAI_AssaultBehavior": "0x224E4C34"},
        "statadvisor_vtable_locator_va": f"0x{STAT_VTABLE_LOCATOR:08X}",
        "statadvisor_vtable_verified_methods": [f"0x{v:08X}" for _, v in STAT_VTABLE_ENTRIES],
        "statadvisor_write_four_va": "0x222C4D2B",
        "original_assault_cue_writes": {
            "goal_BeginAssault_received_1": "0x220CFFA0",
            "mode_3_received_3": "0x220CEAC1",
            "parameterized_received_cue_setter": f"0x{CUE_SETTER:08X}"},
        "generic_cue_setter_direct_e8_e9_candidates": transfers,
        "generic_cue_setter_literal_addresses_in_data": addresses,
        "conclusion": "The +0x20=4 write is in a verified CStatAdvisor virtual method, not an established AssaultBehavior cue-4 producer.",
        "limits": [
            "The .text scan covers raw relative E8/E9 encodings, not complete decoded control flow or indirect dispatch.",
            "The .rdata/.data scan checks literal pointers, not dynamically constructed addresses.",
            "No original producer of CAI_AssaultBehavior.received_cue=4 has been proven.",
            "This is native x86 evidence only; no Quest/SourceVR gameplay implementation is claimed.",
        ],
    }


def audit(server):
    digest = hashlib.sha256(server).hexdigest()
    require(digest == SERVER_SHA256, "original server.dll SHA-256 mismatch")
    return {"server_sha256": digest, **audit_original(PEImage(server))}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        print(json.dumps(audit(args.server.read_bytes()), indent=2))
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(2, f"Original SiN cue-4 provenance audit failed: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
