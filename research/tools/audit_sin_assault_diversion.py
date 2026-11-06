#!/usr/bin/env python3
"""Read-only, hash-pinned original SiN 2006 assault diversion investigation.

Proves actual CAI_AssaultBehavior vtable methods and x86 branches, corroborated
by the original SDK FGD. Emits evidence only, never original DLL machine code.
Original 32-bit offsets are not reusable in any ARM64 game implementation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# Exact original 2006 server.dll x86 instruction witnesses. Short enough to
# distinguish the branch and dataflow without reproducing whole routines.
INSTRUCTIONS = (
    ("NPC state virtual +0x450 guard", 0x220CEAF8, b"\xff\x90\x50\x04\x00\x00"),
    ("NPC numeric condition 0x3B", 0x220CEB05, b"\x6a\x3b"),
    ("behavior deadline field +0x2C", 0x220CEB10, b"\xd9\x46\x2c"),
    ("deadline/current-time comparison", 0x220CEB19, b"\xd8\x59\x0c"),
    ("behavior state field +0x1C", 0x220CEB2A, b"\x39\x5e\x1c"),
    ("current assaultpoint allowdiversion flag +0x3B9", 0x220CEB83,
     b"\x38\x98\xb9\x03\x00\x00"),
    ("zero-time literal reference", 0x220CEB8B, b"\xd9\x05\x90\x25\x3d\x22"),
    ("behavior +0x28 timestamp load", 0x220CEB91, b"\xd9\x46\x28"),
    ("global curtime load", 0x220CEBA3, b"\xd9\x42\x0c"),
    ("elapsed time subtract +0x28", 0x220CEBA6, b"\xd8\x66\x28"),
    ("4-second constant reference", 0x220CEBA9, b"\xd8\x1d\xf4\xdc\x3d\x22"),
    ("elapsed-time comparison status", 0x220CEBAF, b"\xdf\xe0\xf6\xc4\x05"),
    ("diversion state byte +0x26 becomes one", 0x220CEBB6,
     b"\xc6\x46\x26\x01"),
    ("NPC numeric condition 0x18 push", 0x220CEBCC, b"\x6a\x18"),
    ("diversion state +0x26 becomes zero", 0x220CEBD3, b"\x88\x5e\x26"),
    ("invalid point resets state +0x1C", 0x220CEBE1, b"\x89\x5e\x1c"),
    ("invalid point resets rally arrival +0x24", 0x220CEBE4,
     b"\x88\x5e\x24"),
    ("invalid point resets assault arrival +0x25", 0x220CEBE7,
     b"\x88\x5e\x25"),
    ("separate AI routine checks allowdiversion", 0x220CED57,
     b"\x80\xb9\xb9\x03\x00\x00\x00"),
    ("tests schedule id 0x186A3", 0x220CED62, b"\x68\xa3\x86\x01\x00"),
    ("tests schedule id 0x186A0", 0x220CED74, b"\x68\xa0\x86\x01\x00"),
    ("tests schedule id 0x186A4", 0x220CED86, b"\x68\xa4\x86\x01\x00"),
    ("first clear-condition id 0x19", 0x220CED99, b"\x6a\x19"),
    ("second clear-condition id 0x0A", 0x220CEDA3, b"\x6a\x0a"),
)

DIRECT_CALLS = (
    ("NPC check condition 0x3B", 0x220CEB07, 0x220397C0),
    ("set NPC condition 0x18", 0x220CEBCE, 0x22039760),
    ("schedule id 0x186A3 test", 0x220CED69, 0x220CD3B0),
    ("schedule id 0x186A0 test", 0x220CED7B, 0x220CD3B0),
    ("schedule id 0x186A4 test", 0x220CED8D, 0x220CD3B0),
    ("clear NPC condition 0x19", 0x220CED9B, 0x22039C70),
    ("clear NPC condition 0x0A", 0x220CEDA5, 0x22039C70),
)

VTABLE_VA = 0x223F5544
COL_VA = 0x224B8C24
RTTI_NAME_VA = 0x224E4C3C

VTABLE_ENTRIES = (
    ("conditional state routine", 0x18, 0x220CEAF0),
    ("reset/owner cleanup routine", 0x20, 0x220CEBF0),
    ("diversion schedule cleanup routine", 0x78, 0x220CED00),
)

LITERAL_FLOATS = (
    ("zero reference", 0x223D2590, 0.0),
    ("elapsed threshold", 0x223DDCF4, 4.0),
)


def verify_call(pe: PEImage, address: int, target: int) -> None:
    code = pe.at(address, 5)
    if code[0] != 0xE8:
        raise VerificationError(f"expected direct CALL at {address:#x}")
    actual = (address + 5 + struct.unpack_from("<i", code, 1)[0]) & 0xFFFFFFFF
    if actual != target:
        raise VerificationError(f"call at {address:#x} targeted {actual:#x}, expected {target:#x}")


def audit_code(pe: PEImage) -> dict:
    if struct.unpack("<I", pe.at(VTABLE_VA - 4, 4))[0] != COL_VA:
        raise VerificationError("CAI_AssaultBehavior vtable RTTI locator differs")
    if struct.unpack("<I", pe.at(COL_VA + 12, 4))[0] != RTTI_NAME_VA - 8:
        raise VerificationError("CAI_AssaultBehavior RTTI type descriptor differs")
    expected_rtti = b".?AVCAI_AssaultBehavior@@\x00"
    if pe.at(RTTI_NAME_VA, len(expected_rtti)) != expected_rtti:
        raise VerificationError("CAI_AssaultBehavior RTTI class name differs")
    virtuals = []
    for description, offset, target in VTABLE_ENTRIES:
        if struct.unpack("<I", pe.at(VTABLE_VA + offset, 4))[0] != target:
            raise VerificationError(f"original {description} vtable slot differs")
        virtuals.append({
            "role": description, "vtable_offset": f"0x{offset:X}",
            "method_va": f"0x{target:08X}",
        })
    for role, address, code in INSTRUCTIONS:
        if pe.at(address, len(code)) != code:
            raise VerificationError(f"{role}: x86 instruction mismatch at {address:#x}")
    for _, address, target in DIRECT_CALLS:
        verify_call(pe, address, target)
    floats = {}
    for name, address, expected in LITERAL_FLOATS:
        actual = struct.unpack("<f", pe.at(address, 4))[0]
        if actual != expected:
            raise VerificationError(f"{name}: original constant differs at {address:#x}")
        floats[name] = actual
    return {
        "original_cpp_class": "CAI_AssaultBehavior",
        "rtti_vtable_va": f"0x{VTABLE_VA:08X}",
        "original_virtual_methods": virtuals,
        "original_assaultpoint_member": "m_bAllowDiversion",
        "original_x86_assaultpoint_member_offset": "0x3B9",
        "behavior_fields": {
            "state": "0x1C", "previous_timestamp": "0x28",
            "deadline": "0x2C", "diversion_byte": "0x26"
        },
        "verified_literals": floats,
        "observed_numeric_conditions": {
            "query": [0x3B],
            "set": [0x18],
            "clear_in_schedule_path": [0x19, 0x0A],
        },
        "tested_schedule_ids": ["0x186A3", "0x186A0", "0x186A4"],
        "verified_x86_instruction_witnesses": len(INSTRUCTIONS),
        "verified_direct_calls": len(DIRECT_CALLS),
        "limitations": [
            "The original FGD describes what allowdiversion means, but the "
            "complete AI schedule/task and condition enums are not reconstructed.",
            "The 4.0 float is compared against an elapsed-time expression; "
            "not the FGD's configurable 3.0 default assaulttimeout.",
            "No damage, grenade fuse, or generic reaction time should be "
            "inferred from the 4.0 float.",
            "Original Windows x86 member offsets and vtable layout are not "
            "valid for a new AArch64 NPC class.",
        ],
    }


def corroborate_sdk_fgd(game_fgd: bytes) -> dict:
    text = game_fgd.decode("latin1")
    target = re.search(
        r'allowdiversion\(choices\)\s*:\s*"Allow diversion"\s*:\s*0\s*:\s*'
        r'"(If you come in contact with enemies while approaching the assault point,'
        r' divert to kill them\. Resume the assault once contact is lost\.)"',
        text, flags=re.I)
    if not target:
        raise VerificationError("original SDK allowdiversion description/default mismatch")
    timeout = re.search(
        r'assaulttimeout\(float\)\s*:\s*"Assault time out"\s*:\s*"3\.0"\s*:\s*'
        r'"This point is cleared when no enemies are seen for this long \(seconds\)"',
        text, flags=re.I)
    if not timeout:
        raise VerificationError("original SDK assaulttimeout default/description mismatch")
    for name in ("SetAllowDiversion", "SetClearOnContact"):
        if not re.search(r'input\s+' + name + r'\(integer\)', text):
            raise VerificationError(f"SDK assault input {name} missing")
    return {
        "source": "original SinEpisodesSDK/bin/SinEpisodes.fgd",
        "allowdiversion_default": 0,
        "allowdiversion_documented_effect": target.group(1),
        "assaulttimeout_default_seconds": 3.0,
        "has_runtime_inputs": ["SetAllowDiversion", "SetClearOnContact"],
        "note": "The source FGD default 3.0 and binary elapsed threshold 4.0 are distinct.",
    }


def audit(server: bytes, sdk_fgd: bytes | None = None) -> dict:
    sha = hashlib.sha256(server).hexdigest()
    if sha != SERVER_SHA256:
        raise VerificationError("original SiN 2006 server.dll SHA-256 mismatch")
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original PE image base")
    result = {"server_sha256": sha, "original_x86": audit_code(pe)}
    if sdk_fgd is not None:
        result["sdk_fgd"] = corroborate_sdk_fgd(sdk_fgd)
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--sdk-zip", type=Path)
    parser.add_argument("--output", type=Path, help="create new JSON report only")
    args = parser.parse_args(argv)
    try:
        fgd = None
        if args.sdk_zip:
            with zipfile.ZipFile(args.sdk_zip) as archive:
                fgd = archive.read("SinEpisodesSDK/bin/SinEpisodes.fgd")
        result = audit(args.server.read_bytes(), fgd)
        payload = json.dumps(result, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as report:
                report.write(payload)
            print("Verified original assault diversion/vtable evidence")
        else:
            print(payload, end="")
        return 0
    except (OSError, ValueError, struct.error, KeyError, zipfile.BadZipFile) as error:
        parser.exit(2, f"Original assault diversion audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
