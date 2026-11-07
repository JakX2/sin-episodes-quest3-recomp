#!/usr/bin/env python3
"""Hash-pinned original SiN 2006 Source NPC condition-ID registry audit.

Finds original COND_* symbolic names and their engine-assigned IDs by
verifying actual x86 native registration argument pushes and CALL targets.
Cross-checks IDs in CAI_AssaultBehavior and ai_goal_assault call sites.

This is a read-only exact-binary verifier; no original binary or executable
routine is written, and original x86 numeric IDs must NOT become native
ARM64/SourceVR condition ABI values without engine registration.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

REGISTRY_FUNCTION_VA = 0x22073760
REGISTRY_THIS_VA = 0x2254760C
REGISTRY_CONTEXT_STRING_VA = 0x223E05C8
REGISTRY_SOURCE_FILE_VA = 0x223D96F0

# Original condition registration: (name, numeric ID, original C string VA,
# immediate numeric PUSH VA, string PUSH VA, CALL VA).
# These entries are specifically the conditions exercised by the reconstructed
# assault scripts, cue gate, goal input and diversion paths.
SOURCE_CONDITIONS = (
    ("COND_NO_PRIMARY_AMMO", 0x04, 0x223E0B0C, 0x2205A816, 0x2205A818, 0x2205A822),
    ("COND_SEE_ENEMY", 0x0A, 0x223E0AA0, 0x2205A8B8, 0x2205A8BA, 0x2205A8C4),
    ("COND_CAN_RANGE_ATTACK1", 0x14, 0x223E09B8, 0x2205A9E1, 0x2205A9E3, 0x2205A9ED),
    ("COND_PROVOKED", 0x18, 0x223E0960, 0x2205AA4D, 0x2205AA4F, 0x2205AA59),
    ("COND_NEW_ENEMY", 0x19, 0x223E0950, 0x2205AA68, 0x2205AA6A, 0x2205AA74),
    ("COND_HEAR_DANGER", 0x30, 0x223E073C, 0x2205ACD5, 0x2205ACD7, 0x2205ACE1),
    ("COND_HEAR_COMBAT", 0x33, 0x223E0714, 0x2205AD0B, 0x2205AD0D, 0x2205AD17),
    ("COND_HEAR_BULLET_IMPACT", 0x36, 0x223E06D8, 0x2205AD5C, 0x2205AD5E, 0x2205AD68),
    ("COND_RECEIVED_ORDERS", 0x3B, 0x223E0654, 0x2205ADFE, 0x2205AE00, 0x2205AE0A),
)

# The numeric condition is proven to have meaning in these original routines
# only after checking both the registration AND the concrete use-site PUSH
# with its direct call to the original NPC condition helper.
# (role, PUSH ID instruction VA, numeric ID, CALL VA, helper VA)
ASSAULT_USE_SITES = (
    ("ai_goal_assault.BeginAssault signals COND_PROVOKED",
     0x220CFFAA, 0x18, 0x220CFFAC, 0x22039760),
    ("CAI_AssaultBehavior diversion queries COND_RECEIVED_ORDERS",
     0x220CEB05, 0x3B, 0x220CEB07, 0x220397C0),
    ("CAI_AssaultBehavior gunfire cue queries COND_HEAR_COMBAT",
     0x220CEAB3, 0x33, 0x220CEAB5, 0x220397C0),
    ("CAI_AssaultBehavior clear-on-contact queries COND_SEE_ENEMY",
     0x220CF970, 0x0A, 0x220CF972, 0x220397C0),
    ("CAI_AssaultBehavior schedule cleanup clears COND_NEW_ENEMY",
     0x220CED99, 0x19, 0x220CED9B, 0x22039C70),
    ("CAI_AssaultBehavior schedule cleanup clears COND_SEE_ENEMY",
     0x220CEDA3, 0x0A, 0x220CEDA5, 0x22039C70),
    ("CAI_AssaultBehavior diversion sets COND_PROVOKED",
     0x220CEBCC, 0x18, 0x220CEBCE, 0x22039760),
)


# A separate original CAI_AssaultBehavior helper at 0x220CE520 accepts an
# arbitrary integer argument and stores it into m_ReceivedAssaultCue (+0x20),
# resets two owner NPC fields, then tail-jumps to SetCondition(COND_PROVOKED).
# Its name and any callsite passing 4 remain UNKNOWN. This demonstrates an
# integer cue signaling mechanism, NOT proof of the source of cue override 4.
RECEIVED_CUE_SETTER_VA = 0x220CE520
RECEIVED_CUE_SETTER_WITNESSES = (
    (0x220CE520, bytes.fromhex("8b 41 04 33 d2 3b c2 74 0c")),
    (0x220CE529, bytes.fromhex("89 90 64 08 00 00")),
    (0x220CE52F, bytes.fromhex("89 90 5c 08 00 00")),
    (0x220CE535, bytes.fromhex("8b 44 24 04")),
    (0x220CE539, bytes.fromhex("89 41 20")),
    (0x220CE53C, bytes.fromhex("8b 49 04")),
    (0x220CE53F, bytes.fromhex("c7 44 24 04 18 00 00 00")),
)
RECEIVED_CUE_TAIL_JUMP_VA = 0x220CE547
RECEIVED_CUE_TAIL_TARGET_VA = 0x22039760


def verify_received_cue_signaling_helper(pe: PEImage) -> dict:
    for va, opcode in RECEIVED_CUE_SETTER_WITNESSES:
        require(pe.at(va, len(opcode)) == opcode,
                f"generic assault received-cue helper changed at 0x{va:08X}")
    code = pe.at(RECEIVED_CUE_TAIL_JUMP_VA, 5)
    require(code[0] == 0xE9, "generic cue helper no longer tail-jumps")
    target = (RECEIVED_CUE_TAIL_JUMP_VA + 5 +
              struct.unpack_from("<i", code, 1)[0]) & 0xFFFFFFFF
    require(target == RECEIVED_CUE_TAIL_TARGET_VA,
            "generic cue helper no longer signals COND_PROVOKED")
    return {
        "original_helper_va": "0x220CE520",
        "input": "integer argument at original stack +0x04",
        "effect": "store integer into m_ReceivedAssaultCue at behavior +0x20",
        "additional_effects": [
            "if owner NPC exists, zero owner fields +0x864 and +0x85C",
            "tail jump into original SetCondition(COND_PROVOKED, ID 0x18)",
        ],
        "tail_jump_va": "0x220CE547",
        "tail_jump_target": "0x22039760",
        "important_limit":
            "No verified caller passing value 4 was found. "
            "This helper can accept integer 4 in principle, but does not prove "
            "which engine event or map input sets received cue override 4.",
    }


def require(condition: bool, explanation: str) -> None:
    if not condition:
        raise VerificationError(explanation)


def verify_direct_call(pe: PEImage, site: int, expected: int, label: str) -> None:
    code = pe.at(site, 5)
    require(code[0] == 0xE8, f"{label}: missing x86 relative CALL at 0x{site:08X}")
    target = (site + 5 + struct.unpack_from("<i", code, 1)[0]) & 0xFFFFFFFF
    require(target == expected,
            f"{label}: call at 0x{site:08X} changed to 0x{target:08X}")


def audit_original(pe: PEImage) -> dict:
    conditions = []
    ids = set()
    names = set()
    for name, condition_id, text_va, immediate_va, name_push_va, call_va in SOURCE_CONDITIONS:
        require(condition_id < 0x80, f"{name}: expected short PUSH encoding")
        require(name not in names and condition_id not in ids,
                f"{name}: repeated original condition name or ID")
        names.add(name)
        ids.add(condition_id)
        require(pe.at(text_va, len(name) + 1) == name.encode("ascii") + b"\0",
                f"{name}: original string literal mismatch")
        # Verify not just the numeric ID, but the full native registration
        # call sequence: source file/context, push ID, push exact symbol,
        # correct receiver, and relative CALL to condition registry helper.
        prefix = (b"\x68" + struct.pack("<I", REGISTRY_SOURCE_FILE_VA) +
                  b"\x68" + struct.pack("<I", REGISTRY_CONTEXT_STRING_VA))
        require(pe.at(immediate_va - 10, len(prefix)) == prefix,
                f"{name}: wrong registration prefix")
        require(pe.at(immediate_va, 2) == bytes((0x6A, condition_id)),
                f"{name}: original ID instruction mismatch")
        require(pe.at(name_push_va, 5) ==
                b"\x68" + struct.pack("<I", text_va),
                f"{name}: original symbol pointer not passed")
        require(pe.at(name_push_va + 5, 5) ==
                b"\xB9" + struct.pack("<I", REGISTRY_THIS_VA),
                f"{name}: original condition registry receiver changed")
        verify_direct_call(pe, call_va, REGISTRY_FUNCTION_VA, name)
        conditions.append({
            "symbol": name, "original_numeric_id": f"0x{condition_id:02X}",
            "symbol_va": f"0x{text_va:08X}",
            "registration_call_va": f"0x{call_va:08X}",
        })

    by_id = {numeric: name for name, numeric, *_ in SOURCE_CONDITIONS}
    callers = []
    for role, push_va, numeric_id, call_va, destination in ASSAULT_USE_SITES:
        require(numeric_id in by_id, f"unmapped observed condition ID 0x{numeric_id:X}")
        require(pe.at(push_va, 2) == bytes((0x6A, numeric_id)),
                f"{role}: original condition argument changed")
        verify_direct_call(pe, call_va, destination, role)
        callers.append({
            "site": role,
            "condition": by_id[numeric_id],
            "original_numeric_id": f"0x{numeric_id:02X}",
            "call_va": f"0x{call_va:08X}",
            "original_condition_helper_va": f"0x{destination:08X}",
        })

    return {
        "original_condition_registry_va": f"0x{REGISTRY_FUNCTION_VA:08X}",
        "verified_registration_count": len(conditions),
        "verified_assault_callsites": len(callers),
        "original_conditions": conditions,
        "original_assault_condition_use_sites": callers,
        "original_received_cue_signaling_helper": verify_received_cue_signaling_helper(pe),
        "semantic_milestones": {
            "BeginAssault": "COND_PROVOKED (0x18), not COND_RECEIVED_ORDERS (0x3B)",
            "gunfire_assault_cue": "COND_HEAR_COMBAT (0x33)",
            "diversion_received_orders": "COND_RECEIVED_ORDERS (0x3B)",
            "clear_on_contact": "COND_SEE_ENEMY (0x0A)",
            "schedule_cleanup": "COND_NEW_ENEMY (0x19) and COND_SEE_ENEMY (0x0A)",
        },
        "limits": [
            "Symbols are verified from the original 2006 Windows x86 native condition registration calls.",
            "The meaning of each registered condition is resolved, not its entire NPC sensor/state propagation logic.",
            "This is a selected assault-relevant subset, not an exhaustive list of Source AI conditions.",
            "Original 2006 numeric condition IDs are diagnostic metadata, not a SourceVR/AArch64 engine ABI.",
            "A verified generic integer cue setter exists at 0x220CE520, but no caller supplying override value 4 has been proven.",
        ],
    }


def audit(server: bytes) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    require(digest == SERVER_SHA256, "original 2006 SiN server.dll SHA-256 mismatch")
    pe = PEImage(server)
    require(pe.base == 0x22000000, "unexpected original 2006 PE32 image base")
    return {"server_sha256": digest, **audit_original(pe)}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, type=Path)
    parser.add_argument("--output", type=Path, help="write a new JSON report; refuse overwrite")
    args = parser.parse_args(argv)
    try:
        content = json.dumps(audit(args.server.read_bytes()), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf-8") as report:
                report.write(content)
        else:
            print(content, end="")
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f"Original SiN AI condition registry audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
