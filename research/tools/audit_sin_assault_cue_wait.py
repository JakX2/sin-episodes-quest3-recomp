#!/usr/bin/env python3
"""Read-only original 2006 SiN AssaultCue predicate and NPC wait-clock audit.

Verifies x86 switch cases/condition call and the NPC wait setup/completion
routines against the SHA256-pinned original server.dll. Emits normalized
evidence only. No original executable code or complete function dumps.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# Original CAI_AssaultBehavior cue helper at 0x220CEA90. These original 32-bit
# object offsets were identified by independently verified datamap records.
CUE_FUNCTION_VA = 0x220CEA90
CUE_JUMP_TABLE_VA = 0x220CEAD0
CUE_TARGETS = (
    (0, 0x220CEACB, "false"),
    (1, 0x220CEAA7, "compare received cue with 1"),
    (2, 0x220CEAB0, "check NPC condition 0x33"),
    (3, 0x220CEAC1, "store received cue = 3 and return true"),
    (4, 0x220CEACB, "false unless received cue already equals 4"),
)

# Original x86 task callsites use the deterministic zero-spread path in the
# NPC wait helper 0x220325C0; 0x220326D0 checks the current game clock
# against NPC float field at +0x9DC.
WITNESSES = (
    (0x220CEA90, "8b 51 20", "reads m_ReceivedAssaultCue +0x20"),
    (0x220CEA93, "83 fa 04", "received cue == 4 short circuit"),
    (0x220CEA98, "8b 41 1c", "reads m_AssaultCue +0x1C"),
    (0x220CEA9B, "83 f8 04", "bounds the 0..4 cue decision table"),
    (0x220CEAA0, "ff 24 85 d0 ea 0c 22", "cue switch jump table"),
    (0x220CEAA7, "33 c0", "entity input mode initial false"),
    (0x220CEAA9, "83 fa 01", "entity input mode requires received value 1"),
    (0x220CEAAC, "0f 94 c0", "comparison sets cue success boolean"),
    (0x220CEAB0, "8b 49 04", "gunfire mode reads owning NPC pointer"),
    (0x220CEAB3, "6a 33", "gunfire mode checks numeric NPC condition 0x33"),
    (0x220CEAC1, "c7 41 20 03 00 00 00", "mode 3 writes received cue = 3"),
    (0x220CEAC8, "b0 01", "true cue result"),
    (0x220CEACB, "32 c0", "false cue result"),
    (0x220325C0, "83 ec 0c", "NPC wait setup routine entry"),
    (0x220325C3, "d9 44 24 10", "wait setup reads input duration float"),
    (0x220325C8, "d8 0d 14 8f 3d 22", "multiplies duration by original float 10"),
    (0x220325E1, "25 ff f3 ff ff", "clears x87 rounding-control bits"),
    (0x220325E6, "0d 00 08 00 00", "sets x87 rounding toward +infinity"),
    (0x220325F3, "db 5c 24 14", "converts rounded duration into integer"),
    (0x22032624, "d9 9e dc 09 00 00", "fixed wait stores absolute NPC deadline +0x9DC"),
    (0x2203269F, "d9 9e dc 09 00 00", "alternate randomized wait also stores NPC deadline"),
    (0x220326D0, "a1 90 21 57 22", "wait completion reads global clock pointer"),
    (0x220326D5, "d9 40 0c", "loads current game time from +0xC"),
    (0x220326D8, "d8 99 dc 09 00 00", "compares curtime with NPC deadline +0x9DC"),
    (0x220326E0, "f6 c4 01", "tests original x87 comparison status"),
    (0x220326E5, "b8 01 00 00 00", "returns wait-completed true"),
)
GUNFIRE_CONDITION_CALL = (0x220CEAB5, 0x220397C0)
DURATION_SCALE_VA = 0x223D8F14
DECISECOND_MULTIPLIER_VA = 0x223D8FE8


def require(test: bool, detail: str):
    if not test:
        raise VerificationError(detail)


def audit_code(pe: PEImage) -> dict:
    for va, code, description in WITNESSES:
        raw = bytes.fromhex(code)
        require(pe.at(va, len(raw)) == raw,
                f"{description}: original instruction differs at 0x{va:08X}")

    table = []
    for mode, expected, meaning in CUE_TARGETS:
        target = struct.unpack("<I", pe.at(CUE_JUMP_TABLE_VA + mode * 4, 4))[0]
        require(target == expected, f"AssaultCue mode {mode} target differs")
        table.append({"original_cue_mode": mode,
                      "original_case_va": f"0x{target:08X}",
                      "control_flow": meaning})
    callsite, expected_target = GUNFIRE_CONDITION_CALL
    data = pe.at(callsite, 5)
    require(data[0] == 0xE8, "cue gunfire mode has no direct NPC condition call")
    target = (callsite + 5 + struct.unpack("<i", data[1:])[0]) & 0xFFFFFFFF
    require(target == expected_target, "cue gunfire mode NPC condition target changed")

    factor, = struct.unpack("<f", pe.at(DURATION_SCALE_VA, 4))
    multiplier, = struct.unpack("<d", pe.at(DECISECOND_MULTIPLIER_VA, 8))
    require(factor == 10.0 and multiplier == 0.1,
            "original wait quantum differs from 10.0 and 0.1 seconds")

    return {
        "original_CAI_AssaultBehavior_cue_helper_va": "0x220CEA90",
        "verified_cue_cases": table,
        "received_cue_override": {
            "received_value": 4, "result": True,
            "description": "returns true before checking requested mode",
        },
        "gunfire_mode": {
            "original_requested_mode": 2, "npc_condition_numeric_id": "0x33",
            "native_condition_query_target": "0x220397C0",
        },
        "dont_wait_mode": {
            "original_requested_mode": 3, "sets_received_cue_value": 3,
            "result": True,
        },
        "original_wait": {
            "setup_function_va": "0x220325C0",
            "completion_function_va": "0x220326D0",
            "npc_deadline_field_offset": "0x9DC",
            "duration_scale": factor,
            "duration_quantum_seconds": multiplier,
            "rounding": "x87 upward (toward positive infinity) integer conversion",
            "fixed_zero_spread_path":
                "ceil(input_seconds * 10) * 0.1 + curtime, stored as NPC float deadline",
            "completed_when": "current game time >= NPC deadline (finite positive normal values)",
        },
        "verified_instruction_witnesses": len(WITNESSES),
        "limitations": [
            "Original cue mode descriptions come from the original SinEpisodesSDK FGD, not a reconstructed enum header.",
            "NPC numeric condition 0x33 is verified but its native C++ enum name is not yet proven.",
            "Original NPC wait setup also supports a randomized nonzero-spread path not implemented here.",
            "x87 NaN, overflow, timer tick precision and other unusual values require separate verification.",
            "No original SourceVR or ARM64 NPC timer implementation is present.",
        ],
    }


def audit(original_server: bytes) -> dict:
    sha = hashlib.sha256(original_server).hexdigest()
    require(sha == SERVER_SHA256, "not original SHA256-pinned 2006 server.dll")
    pe = PEImage(original_server)
    require(pe.base == 0x22000000, "original PE32 image base changed")
    return {"server_sha256": sha, **audit_code(pe)}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--output", type=Path, help="create a new JSON report only")
    args = parser.parse_args(argv)
    try:
        text = json.dumps(audit(args.server.read_bytes()), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as target:
                target.write(text)
        else:
            print(text, end="")
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f"Original SiN cue/wait audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
