#!/usr/bin/env python3
"""Recover verified task/interrupt skeletons for all 9 original SiN assault schedules.

Uses locally supplied original 2006 server.dll only, SHA-256 pinned.
Outputs normalized schedule evidence, never copies original proprietary DLL,
entire text scripts or executable instructions into the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# Original source-string VAs are from the hash-pinned 2006 PE32 .rdata.
# Ordered task (symbol, parameter) and interrupt names are the meaningful
# recovered logic. Full original text schedule scripts are NOT redistributed.
SCHEDULE_SKELETONS = (
    ("SCHED_ASSAULT_MOVE_AWAY", 0x223F4508, (
        ("TASK_MOVE_AWAY_PATH", "120"), ("TASK_RUN_PATH", "0"),
        ("TASK_WAIT_FOR_MOVEMENT", "0"),
    ), ()),
    ("SCHED_CLEAR_ASSAULT_POINT", 0x223F45B0, (
        ("TASK_ANNOUNCE_CLEAR", "0"),
    ), ()),
    ("SCHED_WAIT_AND_CLEAR", 0x223F4620, (
        ("TASK_FACE_ASSAULT_POINT", "0"),
        ("TASK_SET_ACTIVITY", "ACTIVITY:ACT_IDLE"),
        ("TASK_AWAIT_ASSAULT_TIMEOUT", "0"),
        ("TASK_ANNOUNCE_CLEAR", "0"),
    ), (
        "COND_NEW_ENEMY", "COND_CAN_RANGE_ATTACK1", "COND_CAN_MELEE_ATTACK1",
        "COND_CAN_RANGE_ATTACK2", "COND_CAN_MELEE_ATTACK2", "COND_HEAR_DANGER",
        "COND_HEAR_BULLET_IMPACT",
    )),
    ("SCHED_MOVE_TO_ASSAULT_POINT", 0x223F4788, (
        ("TASK_SET_FAIL_SCHEDULE", "SCHEDULE:SCHED_ASSAULT_FAILED_TO_MOVE"),
        ("TASK_GATHER_CONDITIONS", "0"),
        ("TASK_GET_PATH_TO_ASSAULT_POINT", "0"), ("TASK_RUN_PATH", "0"),
        ("TASK_WAIT_FOR_MOVEMENT", "0"), ("TASK_FACE_ASSAULT_POINT", "0"),
        ("TASK_HIT_ASSAULT_POINT", "0"),
    ), ("COND_NO_PRIMARY_AMMO", "COND_HEAR_DANGER")),
    ("SCHED_HOLD_ASSAULT_POINT", 0x223F4900, (
        ("TASK_SET_ACTIVITY", "ACTIVITY:ACT_IDLE"), ("TASK_WAIT", "3"),
    ), (
        "COND_NEW_ENEMY", "COND_ENEMY_DEAD", "COND_CAN_RANGE_ATTACK1",
        "COND_CAN_MELEE_ATTACK1", "COND_CAN_RANGE_ATTACK2",
        "COND_CAN_MELEE_ATTACK2", "COND_TOO_CLOSE_TO_ATTACK",
        "COND_LOST_ENEMY", "COND_HEAR_DANGER", "COND_HEAR_BULLET_IMPACT",
        "COND_NO_PRIMARY_AMMO",
    )),
    ("SCHED_HOLD_RALLY_POINT", 0x223F4A80, (
        ("TASK_AWAIT_CUE", "0"), ("TASK_WAIT_ASSAULT_DELAY", "0"),
    ), (
        "COND_NEW_ENEMY", "COND_CAN_RANGE_ATTACK1", "COND_CAN_MELEE_ATTACK1",
        "COND_LIGHT_DAMAGE", "COND_HEAVY_DAMAGE", "COND_PLAYER_PUSHING",
        "COND_HEAR_DANGER", "COND_HEAR_BULLET_IMPACT", "COND_NO_PRIMARY_AMMO",
    )),
    ("SCHED_FAIL_MOVE_TO_RALLY_POINT", 0x223F4BC8, (
        ("TASK_WAIT", "1"),
    ), ("COND_HEAR_DANGER", "COND_CAN_RANGE_ATTACK1", "COND_CAN_MELEE_ATTACK1")),
    ("SCHED_ASSAULT_FAILED_TO_MOVE", 0x223F4C78, (
        ("TASK_ASSAULT_DEFER_SCHEDULE_SELECTION", "1"),
    ), ()),
    ("SCHED_MOVE_TO_RALLY_POINT", 0x223F4D00, (
        ("TASK_SET_FAIL_SCHEDULE", "SCHEDULE:SCHED_ASSAULT_FAILED_TO_MOVE"),
        ("TASK_GET_PATH_TO_RALLY_POINT", "0"), ("TASK_RUN_PATH", "0"),
        ("TASK_WAIT_FOR_MOVEMENT", "0"), ("TASK_STOP_MOVING", "0"),
        ("TASK_FACE_RALLY_POINT", "0"), ("TASK_HIT_RALLY_POINT", "0"),
        ("TASK_SET_SCHEDULE", "SCHEDULE:SCHED_HOLD_RALLY_POINT"),
    ), ("COND_HEAR_DANGER", "COND_PROVOKED", "COND_NO_PRIMARY_AMMO")),
)

SCHEDULE_RE = re.compile(
    r"^\s*Schedule\s+(SCHED_[A-Z0-9_]+)\s+Tasks\s+(.*?)\s+Interrupts\s*(.*?)\s*$",
    flags=re.S,
)
TASK_RE = re.compile(r"\bTASK_[A-Z0-9_]+\b")
CONDITION_RE = re.compile(r"\bCOND_[A-Z0-9_]+\b")
PARAMETER_RE = re.compile(r"(?:[+-]?\d+(?:\.\d+)?|SCHEDULE:SCHED_[A-Z0-9_]+|ACTIVITY:ACT_[A-Z0-9_]+)")


def parse_schedule_string(original_text: str) -> tuple[str, tuple, tuple]:
    match = SCHEDULE_RE.fullmatch(original_text)
    if not match:
        raise VerificationError("original schedule text header or sections changed")
    name, task_body, interrupts_body = match.groups()
    located = list(TASK_RE.finditer(task_body))
    if not located:
        raise VerificationError(f"{name}: schedule has no TASK_ fields")
    if task_body[:located[0].start()].strip():
        raise VerificationError(f"{name}: unrecognized content before first task")
    tasks = []
    for index, task in enumerate(located):
        end = located[index + 1].start() if index + 1 < len(located) else len(task_body)
        parameter = task_body[task.end():end].strip()
        if not PARAMETER_RE.fullmatch(parameter):
            raise VerificationError(f"{name}: unrecognized task parameter for {task.group()}")
        tasks.append((task.group(), parameter))
    conditions = tuple(CONDITION_RE.findall(interrupts_body))
    if CONDITION_RE.sub("", interrupts_body).strip():
        raise VerificationError(f"{name}: unexpected interrupt text")
    return name, tuple(tasks), conditions


def audit_schedules(pe: PEImage) -> dict:
    verified = []
    names = {item[0] for item in SCHEDULE_SKELETONS}
    if len(names) != len(SCHEDULE_SKELETONS):
        raise VerificationError("duplicated original assault schedule names")
    for name, va, expected_tasks, expected_interrupts in SCHEDULE_SKELETONS:
        raw = pe.at(va, 960).split(b"\0", 1)[0]
        try:
            schedule = raw.decode("ascii")
        except UnicodeDecodeError as error:
            raise VerificationError(f"{name}: original string is not ASCII") from error
        found_name, tasks, interrupts = parse_schedule_string(schedule)
        if found_name != name or tasks != expected_tasks or interrupts != expected_interrupts:
            raise VerificationError(f"{name}: original task order, parameters or interrupts differ")
        for task, argument in tasks:
            if argument.startswith("SCHEDULE:") and argument[9:] not in names:
                raise VerificationError(f"{name}: references unknown assault schedule")
        verified.append({
            "schedule": name,
            "script_string_original_va": f"0x{va:08X}",
            "task_count": len(tasks), "interrupt_count": len(interrupts),
            "tasks": [{"symbol": task, "parameter": value} for task, value in tasks],
            "interrupt_conditions": list(interrupts),
        })
    return {
        "schedule_count": len(verified),
        "total_tasks": sum(s["task_count"] for s in verified),
        "total_interrupt_declarations": sum(s["interrupt_count"] for s in verified),
        "schedule_skeletons": verified,
        "limitations": [
            "Only the original schedule task symbols, scalar parameters and interrupt masks are recovered.",
            "Schedule registration numeric IDs are verified separately in audit_sin_assault_diversion.py.",
            "These declarations do not implement actual Source AI tasks, NPC condition enumeration, movement or animation.",
            "No complete proprietary schedule text or Windows instructions are redistributed.",
        ],
    }


def audit(original_server: bytes) -> dict:
    checksum = hashlib.sha256(original_server).hexdigest()
    if checksum != SERVER_SHA256:
        raise VerificationError("original 2006 server.dll hash mismatch")
    pe = PEImage(original_server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original server.dll image base")
    return {"server_sha256": checksum, **audit_schedules(pe)}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, type=Path)
    parser.add_argument("--output", type=Path, help="create a new JSON file; never overwrite")
    args = parser.parse_args(argv)
    try:
        report = json.dumps(audit(args.server.read_bytes()), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as output:
                output.write(report)
            print("Verified all original assault schedule task and interrupt skeletons")
        else:
            print(report, end="")
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f"Original assault schedule audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
