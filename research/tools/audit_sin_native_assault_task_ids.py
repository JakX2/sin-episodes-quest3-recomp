#!/usr/bin/env python3
"""Verify original 2006 SiN assault-specific TASK_* IDs against server.dll.

Read-only PE32 x86 registration and class vtable audit. Do not interpret task
IDs as schedule IDs: the 2006 Source AI registries reuse numeric values.
The original DLL is supplied locally and never copied into this repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# (task ID, symbol, literal VA, immediate name store VA,
#  immediate ID store VA, table register call VA, name stack byte, ID stack byte).
# TASK_WAIT_ASSAULT_DELAY uses a different local stack slot pairing.
IMMEDIATE_TASKS = (
    (0x186A0, "TASK_GET_PATH_TO_RALLY_POINT", 0x223F4FB0,
     0x220D04F5, 0x220D04FD, 0x220D0505, 0x1C, 0x20),
    (0x186A1, "TASK_FACE_RALLY_POINT", 0x223F4F98,
     0x220D0518, 0x220D0520, 0x220D0528, 0x1C, 0x20),
    (0x186A2, "TASK_GET_PATH_TO_ASSAULT_POINT", 0x223F4F78,
     0x220D053B, 0x220D0543, 0x220D054B, 0x1C, 0x20),
    (0x186A3, "TASK_FACE_ASSAULT_POINT", 0x223F4F60,
     0x220D055E, 0x220D0566, 0x220D056E, 0x1C, 0x20),
    (0x186A6, "TASK_AWAIT_CUE", 0x223F4F50,
     0x220D0581, 0x220D0589, 0x220D0591, 0x1C, 0x20),
    (0x186A7, "TASK_AWAIT_ASSAULT_TIMEOUT", 0x223F4F34,
     0x220D05A4, 0x220D05AC, 0x220D05B4, 0x1C, 0x20),
    (0x186A9, "TASK_WAIT_ASSAULT_DELAY", 0x223F4F08,
     0x220D05E5, 0x220D05ED, 0x220D05FB, 0x14, 0x18),
    (0x186AB, "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION", 0x223F4EB0,
     0x220D0656, 0x220D065E, 0x220D0666, 0x1C, 0x20),
)

# Register-borne task ID initialization: (task ID, symbol, name VA,
# name stack store VA, ID load VA, ID load exact bytes, ID stack store VA,
# ID stack store exact bytes, registration call VA).
REGISTER_TASKS = (
    (0x186A4, "TASK_HIT_ASSAULT_POINT", 0x223F4EF0,
     0x220D0613, 0x220D060A, b"\xbf\xa4\x86\x01\x00",
     0x220D061B, b"\x89\x7c\x24\x20", 0x220D061F),
    (0x186A5, "TASK_HIT_RALLY_POINT", 0x223F4ED8,
     0x220D0637, 0x220D062D, b"\xbe\xa5\x86\x01\x00",
     0x220D063F, b"\x89\x74\x24\x20", 0x220D0643),
    (0x186A8, "TASK_ANNOUNCE_CLEAR", 0x223F4F20,
     0x220D05CC, 0x220D05C2, b"\xbd\xa8\x86\x01\x00",
     0x220D05D4, b"\x89\x6c\x24\x20", 0x220D05D8),
)

TASK_REGISTER_FUNCTION_VA = 0x2208C590
ASSAULT_BEHAVIOR_VTABLE_VA = 0x223F5544
TASK_DISPATCH_VIRTUALS = (
    ("first task dispatcher (StartTask candidate)", 0x60, 0x220CF190,
     b"\x83\xec\x60\x53\x56\x8b\xf1\x8b\x4c\x24\x6c\x8b\x01\x3d\xa5\x86\x01\x00"),
    ("second task dispatcher (RunTask candidate)", 0x64, 0x220CF770,
     b"\x83\xec\x48\x53\x8b\x5c\x24\x50\x8b\x03\x3d\xa6\x86\x01\x00"),
)


def require(ok: bool, message: str) -> None:
    if not ok:
        raise VerificationError(message)


def verify_original_call(pe: PEImage, va: int, target: int) -> None:
    data = pe.at(va, 5)
    require(data[0] == 0xE8, f"expected original x86 direct CALL at 0x{va:08X}")
    found = (va + 5 + struct.unpack_from("<i", data, 1)[0]) & 0xFFFFFFFF
    require(found == target,
            f"original x86 CALL 0x{va:08X} resolves 0x{found:08X}, not 0x{target:08X}")


def check_task_name(pe: PEImage, va: int, name: str) -> None:
    require(pe.at(va, len(name) + 1) == name.encode("ascii") + b"\x00",
            f"original TASK literal differs: {name}")


def verify_native_task_registrations(pe: PEImage) -> dict:
    rows = []
    for task_id, symbol, name_va, name_site, id_site, call_site, name_stack, id_stack in IMMEDIATE_TASKS:
        check_task_name(pe, name_va, symbol)
        expected_name = (b"\xc7\x44\x24" + bytes([name_stack]) +
                         struct.pack("<I", name_va))
        expected_id = (b"\xc7\x44\x24" + bytes([id_stack]) +
                       struct.pack("<I", task_id))
        require(pe.at(name_site, 8) == expected_name,
                f"{symbol}: name pointer instruction differs")
        require(pe.at(id_site, 8) == expected_id,
                f"{symbol}: task ID instruction differs")
        verify_original_call(pe, call_site, TASK_REGISTER_FUNCTION_VA)
        rows.append({"symbol": symbol, "original_task_id": f"0x{task_id:05X}",
                     "original_name_va": f"0x{name_va:08X}",
                     "register_call_va": f"0x{call_site:08X}"})

    for task_id, symbol, name_va, name_site, load_site, load_code, store_site, store_code, call_site in REGISTER_TASKS:
        check_task_name(pe, name_va, symbol)
        expected_name = b"\xc7\x44\x24\x1c" + struct.pack("<I", name_va)
        require(pe.at(name_site, 8) == expected_name,
                f"{symbol}: name pointer store differs")
        require(pe.at(load_site, len(load_code)) == load_code,
                f"{symbol}: ID register initialization differs")
        require(pe.at(store_site, len(store_code)) == store_code,
                f"{symbol}: ID register-to-stack store differs")
        verify_original_call(pe, call_site, TASK_REGISTER_FUNCTION_VA)
        rows.append({"symbol": symbol, "original_task_id": f"0x{task_id:05X}",
                     "original_name_va": f"0x{name_va:08X}",
                     "register_call_va": f"0x{call_site:08X}"})

    task_ids = [int(x["original_task_id"], 16) for x in rows]
    require(len(rows) == 11 and len(set(task_ids)) == 11,
            "original task registration size or ID uniqueness differs")
    require(len({x["symbol"] for x in rows}) == 11,
            "original task names not unique")
    rows.sort(key=lambda x: int(x["original_task_id"], 16))
    return {
        "original_registry_va": f"0x{TASK_REGISTER_FUNCTION_VA:08X}",
        "verified_original_specific_task_count": len(rows),
        "original_task_registry_entries": rows,
        "unmapped_original_task_id_in_local_block": "0x186AA",
        "task_vs_schedule_warning":
            "TASK_* and SCHED_* are different registries that reuse numerical IDs",
    }


def verify_original_dispatch_virtuals(pe: PEImage) -> list[dict]:
    dispatch = []
    for label, offset, method, signature in TASK_DISPATCH_VIRTUALS:
        ptr, = struct.unpack("<I", pe.at(ASSAULT_BEHAVIOR_VTABLE_VA + offset, 4))
        require(ptr == method,
                f"CAI_AssaultBehavior vtable +0x{offset:X} no longer points at {label}")
        require(pe.at(method, len(signature)) == signature,
                f"{label}: first x86 dispatch instructions differ")
        dispatch.append({
            "description": label,
            "original_vtable_offset": f"0x{offset:X}",
            "original_method_va": f"0x{method:08X}",
            "what_is_verified": "method reads a task identifier and branches on an original task ID",
        })
    return dispatch


def audit(original_server: bytes) -> dict:
    digest = hashlib.sha256(original_server).hexdigest()
    require(digest == SERVER_SHA256, "not the pinned original 2006 SiN server.dll")
    pe = PEImage(original_server)
    require(pe.base == 0x22000000, "unexpected 2006 server.dll image base")
    return {
        "server_sha256": digest,
        "task_registry": verify_native_task_registrations(pe),
        "native_task_dispatch_virtuals": verify_original_dispatch_virtuals(pe),
        "limits": [
            "Registered original TASK_* IDs verified independently from original schedule IDs.",
            "The +0x60/+0x64 virtuals dispatch on task IDs; their precise inherited C++ method labels are candidates pending ABI proof.",
            "This does not implement any original Source AI task execution, navigation, collision, cue, activity, network or Quest runtime behavior.",
            "No original private DLL instruction bytes or full scripts are emitted.",
        ],
    }


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--output", type=Path, help="new JSON file only; refuses overwrite")
    args = p.parse_args(argv)
    try:
        payload = json.dumps(audit(args.server.read_bytes()), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as output:
                output.write(payload)
            print("Verified 11 original SiN assault-task registrations and two task-dispatch virtuals")
        else:
            print(payload, end="")
        return 0
    except (OSError, ValueError, struct.error) as error:
        p.exit(2, f"Original assault task registry audit failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
