#!/usr/bin/env python3
"""Read-only x86 verification of original SiN 2006 assault-task execution paths.

The user's original PE32 server.dll is pinned by its full SHA256 hash. This
reports small verified instruction witnesses and normalized control-flow facts,
not copyrighted code dumps, reconstructed executable code, or a native
Quest 3 NPC implementation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# Distinct task IDs verified separately in audit_sin_native_assault_task_ids.py.
# These original StartTask/RunTask vtable method labels remain ABI candidates.
BEHAVIOR_VTABLE = 0x223F5544
START_TASK_VTABLE_OFFSET = 0x60
RUN_TASK_VTABLE_OFFSET = 0x64
START_TASK_VA = 0x220CF190
RUN_TASK_VA = 0x220CF770
JUMP_TABLE_VA = 0x220CF74C
JUMP_TABLE = (
    (0x186A6, "TASK_AWAIT_CUE", 0x220CF67D),
    (0x186A7, "TASK_AWAIT_ASSAULT_TIMEOUT", 0x220CF631),
    (0x186A8, "TASK_ANNOUNCE_CLEAR", 0x220CF5A2),
    (0x186A9, "TASK_WAIT_ASSAULT_DELAY", 0x220CF600),
    (0x186AA, "unregistered ID routes to inherited fallback", 0x220CF740),
    (0x186AB, "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION", 0x220CF582),
)

# Observed byte shape at specific *original virtual addresses* (VA, hex, role).
# This makes code-layout/data-flow assertions auditable without claiming the
# complete original member functions have been faithfully reimplemented.
WITNESSES = (
    (0x220CF19D, "3d a5 86 01 00", "StartTask compares TASK_HIT_RALLY_POINT"),
    (0x220CF410, "2d a3 86 01 00", "StartTask offsets assault task IDs"),
    (0x220CF41B, "83 e8 01", "StartTask resolves hit assaultpoint branch"),
    (0x220CF424, "8d 7e 14", "hit assaultpoint reads behavior+0x14 handle"),
    (0x220CF44A, "c6 46 25 01", "sets m_bHitAssaultPoint to one"),
    (0x220CF461, "81 c1 bc 03 00 00", "assaultpoint OnArrival at +0x3BC"),
    (0x220CF473, "83 b8 94 03 00 00 00", "checks assault hint/group field +0x394"),
    (0x220CF4FF, "8d 7e 18", "hit rallypoint reads behavior+0x18 handle"),
    (0x220CF539, "c6 46 24 01", "sets m_bHitRallyPoint to one"),
    (0x220CF550, "81 c1 a4 03 00 00", "rallypoint OnArrival at +0x3A4"),
    (0x220CF56D, "2d a6 86 01 00", "StartTask jump-table base 0x186A6"),
    (0x220CF572, "83 f8 05", "checks six-case jump-table bounds"),
    (0x220CF57B, "ff 24 85 4c f7 0c 22", "original indirect jump-table dispatch"),
    (0x220CF582, "a1 90 21 57 22", "reads global time via original pointer"),
    (0x220CF587, "d9 40 0c", "loads global curtime +0x0C"),
    (0x220CF58C, "d8 41 04", "adds task data float at +0x04"),
    (0x220CF593, "d9 5e 2c", "stores m_flTimeDeferScheduleSelection +0x2C"),
    (0x220CF5A2, "8d 7e 14", "announce clear gets assaultpoint handle"),
    (0x220CF5CA, "80 b8 ba 03 00 00 00", "checks m_bNeverTimeout at point+0x3BA"),
    (0x220CF5DA, "83 b8 98 03 00 00 00", "checks next assaultpoint name at +0x398"),
    (0x220CF600, "8d 7e 18", "wait assault delay gets rallypoint handle"),
    (0x220CF617, "8b 88 9c 03 00 00", "loads rally m_flAssaultDelay +0x39C"),
    (0x220CF631, "8d 7e 14", "await timeout gets assaultpoint handle"),
    (0x220CF663, "8b 88 b4 03 00 00", "loads point m_flAssaultTimeout +0x3B4"),
    (0x220CF67D, "8b ce", "await cue starts at 0x220CF67D"),
    (0x220CF77A, "3d a6 86 01 00", "RunTask compares TASK_AWAIT_CUE"),
    (0x220CF998, "8d 7e 18", "RunTask await cue accesses rallypoint handle"),
    (0x220CFA0E, "2d a7 86 01 00", "RunTask timeout compare 0x186A7"),
    (0x220CFA15, "83 e8 02", "RunTask delay compare 0x186A9"),
    (0x220CFA7A, "80 b9 b8 03 00 00 00", "checks clear-on-contact point flag"),
)

# Direct relative CALL x86 paths, independently checked against original DLL.
CALLS = (
    (0x220CF467, 0x22156900, "assaultpoint OnArrival output dispatch"),
    (0x220CF556, 0x22156900, "rallypoint OnArrival output dispatch"),
    (0x220CF5E9, 0x220CE5B0, "announce-clear next-point/terminal chain advance"),
    (0x220CF621, 0x220325C0, "rally configured delay passed into NPC wait setup"),
    (0x220CF66D, 0x220325C0, "assault configured timeout passed into NPC wait setup"),
    (0x220CF67F, 0x220CEA90, "StartTask cue gate helper"),
    (0x220CF9C1, 0x220CEA90, "RunTask cue gate helper"),
    (0x220CFA9D, 0x220326D0, "RunTask time completion check"),
)


def require(test: bool, message: str) -> None:
    if not test:
        raise VerificationError(message)


def audit_code(pe: PEImage) -> dict:
    for slot, target, description in (
        (START_TASK_VTABLE_OFFSET, START_TASK_VA, "first task dispatcher"),
        (RUN_TASK_VTABLE_OFFSET, RUN_TASK_VA, "second task dispatcher"),
    ):
        raw = pe.at(BEHAVIOR_VTABLE + slot, 4)
        require(struct.unpack("<I", raw)[0] == target,
                f"CAI_AssaultBehavior {description} virtual pointer differs")

    for va, pattern, description in WITNESSES:
        expected = bytes.fromhex(pattern)
        require(pe.at(va, len(expected)) == expected,
                f"{description}: original x86 witness mismatch at 0x{va:08X}")

    resolved = []
    for task_id, label, target in JUMP_TABLE:
        index = task_id - 0x186A6
        actual = struct.unpack("<I", pe.at(JUMP_TABLE_VA + index * 4, 4))[0]
        require(actual == target, f"task ID 0x{task_id:X} jump target differs")
        resolved.append({
            "original_task_id": f"0x{task_id:05X}",
            "task": label, "case_va": f"0x{target:08X}",
        })

    calls = []
    for callsite, destination, description in CALLS:
        code = pe.at(callsite, 5)
        require(code[0] == 0xE8,
                f"{description}: no direct CALL at 0x{callsite:08X}")
        actual = (callsite + 5 + struct.unpack_from("<i", code, 1)[0]) & 0xFFFFFFFF
        require(actual == destination,
                f"{description}: wrong CALL destination at 0x{callsite:08X}")
        calls.append({"callsite_va": f"0x{callsite:08X}",
                      "target_va": f"0x{destination:08X}",
                      "role": description})
    return {
        "class": "CAI_AssaultBehavior",
        "original_task_dispatch_virtuals": [
            {"vtable_offset": "0x60", "function_va": "0x220CF190",
             "inferred_label": "StartTask candidate (not ABI-proved)"},
            {"vtable_offset": "0x64", "function_va": "0x220CF770",
             "inferred_label": "RunTask candidate (not ABI-proved)"},
        ],
        "original_task_jump_table": resolved,
        "verified_instruction_witnesses": len(WITNESSES),
        "verified_direct_call_targets": calls,
        "recovered_behaviors": {
            "TASK_HIT_RALLY_POINT": {
                "original_case_va": "0x220CF4FF",
                "arrival_flag_field": "m_bHitRallyPoint +0x24",
                "event_field": "CAssaultRallyPoint.m_OnArrival +0x3A4",
                "event_fire_call": "0x220CF556",
            },
            "TASK_HIT_ASSAULT_POINT": {
                "original_case_va": "0x220CF424",
                "arrival_flag_field": "m_bHitAssaultPoint +0x25",
                "event_field": "CAssaultPoint.m_OnArrival +0x3BC",
                "event_fire_call": "0x220CF467",
            },
            "TASK_WAIT_ASSAULT_DELAY": {
                "original_case_va": "0x220CF600",
                "float_source": "CAssaultRallyPoint.m_flAssaultDelay +0x39C",
                "wait_setup_call_va": "0x220CF621",
            },
            "TASK_AWAIT_ASSAULT_TIMEOUT": {
                "original_case_va": "0x220CF631",
                "float_source": "CAssaultPoint.m_flAssaultTimeout +0x3B4",
                "wait_setup_call_va": "0x220CF66D",
            },
            "TASK_ANNOUNCE_CLEAR": {
                "original_case_va": "0x220CF5A2",
                "inspected_point_flags": ["m_bNeverTimeout +0x3BA",
                                          "m_NextAssaultPointName +0x398"],
                "advance_call_va": "0x220CF5E9",
            },
            "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION": {
                "original_case_va": "0x220CF582",
                "deadline": "global curtime + task float parameter",
                "destination": "m_flTimeDeferScheduleSelection +0x2C",
            },
            "TASK_AWAIT_CUE": {
                "start_case_va": "0x220CF67D",
                "run_case_va": "0x220CF998",
                "shared_gate_helper": "0x220CEA90",
            },
            "TASK_AWAIT_ASSAULT_TIMEOUT and TASK_WAIT_ASSAULT_DELAY": {
                "run_task_shared_completion_path_va": "0x220CFA2B",
                "engine_wait_completion_query_va": "0x220CFA9D",
            },
        },
        "limitations": [
            "These are verified native control-flow/data-flow sites, not recovered entire original C++.",
            "Some original task branches have additional invalid-handle and NPC-condition gates.",
            "0x220325C0 and 0x220326D0 are observed wait-related calls; exact inherited engine method names not proved.",
            "No native entity factory, engine pathfinding, task executor or AArch64 Quest integration is implemented.",
            "Original 2006 x86 pointer/object offsets are not portable ABI layouts.",
        ],
    }


def audit(server: bytes) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    require(digest == SERVER_SHA256,
            "expected unmodified original 2006 SiN server.dll SHA256")
    pe = PEImage(server)
    require(pe.base == 0x22000000, "unexpected original DLL PE32 image base")
    return {"original_server_sha256": digest, **audit_code(pe)}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--output", type=Path, help="new JSON report only; no overwrite")
    args = parser.parse_args(argv)
    try:
        payload = json.dumps(audit(args.server.read_bytes()), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as target:
                target.write(payload)
        else:
            print(payload, end="")
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f"Original SiN assault task executor audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
