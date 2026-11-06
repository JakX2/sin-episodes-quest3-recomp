#!/usr/bin/env python3
"""Read-only audit: how original SiN ai_goal_assault BeginAssault signals NPCs.

Reads the owner's SHA256-pinned original 2006 server.dll. Verifies native
typedescription_t registration, actor iteration, assault-behavior resolution,
received-cue writes, and native NPC condition signaling. Optionally validates
the corresponding original SinEpisodes.fgd contract. Does not rewrite DLLs.
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

# Original CAI_GoalEntity derived goal layout and typedescription_t entries:
# (field, descriptor VA, name VA, type, offset, flag, external-name VA, external, handler).
GOAL_DATAMAP = (
    ("m_RallyPoint", 0x224E5344, 0x223F44F4, 2, 0x3C8, 6,
     0x223F44E8, "rallypoint", 0),
    ("m_AssaultCue", 0x224E5378, 0x223F4488, 5, 0x3CC, 6,
     0x223F44DC, "AssaultCue", 0),
    ("InputBeginAssault", 0x224E53AC, 0x223F44C8, 0, 0, 8,
     0x223F44B8, "BeginAssault", 0x220CFF30),
)

GOAL_ACTOR_SETUP_VTABLE_SLOT_VA = 0x223F5280
GOAL_ACTOR_SETUP_VA = 0x220CFF00
BEGIN_ASSAULT_HANDLER_VA = 0x220CFF30

# Short exact original x86 instruction/dataflow witnesses. Each verified byte
# span is deliberately kept small; full original machine code is not emitted.
WITNESSES = (
    (0x220CFF00, "56 8d 44 24 08", "actor setup helper entry"),
    (0x220CFF15, "8b 8e cc 03 00 00", "reads goal cue mode +0x3CC"),
    (0x220CFF1B, "8b 96 c8 03 00 00", "reads goal rallypoint name +0x3C8"),
    (0x220CFF21, "51", "pushes configured cue mode"),
    (0x220CFF26, "52", "pushes configured rallypoint name"),
    (0x220CFF30, "51 55 56 57 33 ed 8b f1 33 ff",
     "BeginAssault handler saves registers and initializes actor loop"),
    (0x220CFF40, "8b 86 c4 03 00 00", "reads goal actor list state bits"),
    (0x220CFF65, "3b be b8 03 00 00", "compares actor index with actor count +0x3B8"),
    (0x220CFF6D, "57", "pushes actor index to resolver"),
    (0x220CFF79, "8d 4c 24 0c", "prepares out param for assault behavior resolver"),
    (0x220CFF89, "8b 4c 24 0c", "loads resolved assault behavior pointer"),
    (0x220CFF8D, "8b 41 04", "reads original owning NPC pointer"),
    (0x220CFF94, "89 a8 64 08 00 00", "clears owner NPC state field +0x864"),
    (0x220CFF9A, "89 a8 5c 08 00 00", "clears owner NPC state field +0x85C"),
    (0x220CFFA0, "c7 41 20 01 00 00 00",
     "sets CAI_AssaultBehavior.m_ReceivedAssaultCue +0x20 to 1"),
    (0x220CFFA7, "8b 49 04", "reloads behavior owning NPC pointer"),
    (0x220CFFAA, "6a 18", "pushes native NPC condition ID 0x18"),
    (0x220CFFB1, "83 c7 01", "increments goal actor iteration index"),
    (0x220CF120, "56 8b f1 8b 86 c4 03 00 00",
     "per-index actor handle resolver starts with actor list flags"),
    (0x220CF146, "8b 44 24 08", "reads requested actor index"),
    (0x220CF14A, "39 86 b8 03 00 00", "bounds checks goal actor count"),
    (0x220CF152, "8b 8e ac 03 00 00", "reads goal actor list base +0x3AC"),
    (0x220CF15D, "83 f8 ff", "rejects empty Source entity handle"),
    (0x22357E82, "6a 03 51 8d 8e b8 0d 00 00",
     "direct MobGrunt Assault input independently passes cue mode 3"),
)

CALL_EDGES = (
    (0x220CFF0C, 0x220CDC80, "actor setup resolves assault behavior"),
    (0x220CFF27, 0x220CFC00, "actor setup invokes SetParameters with goal cue mode"),
    (0x220CFF50, 0x220613A0, "BeginAssault refreshes existing actor list"),
    (0x220CFF59, 0x22063320, "BeginAssault gathers actor list when needed"),
    (0x220CFF70, 0x220CF120, "BeginAssault fetches goal actor by index"),
    (0x220CFF80, 0x220CDC80, "BeginAssault resolves actor assault behavior"),
    (0x220CFFAC, 0x22039760, "BeginAssault sets NPC condition ID 0x18"),
    (0x22357E8B, 0x220CFC00, "direct MobGrunt Assault SetParameters call"),
)


def same(pe: PEImage, address: int, expected: bytes, reason: str):
    if pe.at(address, len(expected)) != expected:
        raise VerificationError(f"{reason}: unexpected original bytes at 0x{address:08X}")


def direct_call(pe: PEImage, callsite: int, target: int, reason: str):
    data = pe.at(callsite, 5)
    if data[0] != 0xE8:
        raise VerificationError(f"{reason}: missing original relative x86 CALL")
    actual = (callsite + 5 + struct.unpack_from("<i", data, 1)[0]) & 0xFFFFFFFF
    if actual != target:
        raise VerificationError(
            f"{reason}: CALL at 0x{callsite:08X} points to 0x{actual:08X}")


def verify_original_datamap(pe: PEImage):
    rows = []
    for name, desc, name_va, kind, offset, flags, external_va, external, handler in GOAL_DATAMAP:
        words = struct.unpack("<13I", pe.at(desc, 52))
        expected = (kind, name_va, offset, 0, (flags << 16) | 1,
                    external_va, 0, handler, 0, 0 if kind == 0 else 4, 0, 0, 0)
        if words != expected:
            raise VerificationError(f"ai_goal_assault datamap field {name} differs")
        same(pe, name_va, name.encode("ascii") + b"\0", "datamap native field name")
        same(pe, external_va, external.encode("ascii") + b"\0",
             "datamap external Hammer key/input")
        rows.append({
            "member": name, "x86_offset": f"0x{offset:X}",
            "hammer_name": external,
            "handler_va": f"0x{handler:08X}" if handler else None,
        })
    return rows


def verify_original_x86(pe: PEImage):
    fields = verify_original_datamap(pe)
    actual = struct.unpack("<I", pe.at(GOAL_ACTOR_SETUP_VTABLE_SLOT_VA, 4))[0]
    if actual != GOAL_ACTOR_SETUP_VA:
        raise VerificationError("goal actor setup vtable hook changed")
    for address, signature, label in WITNESSES:
        same(pe, address, bytes.fromhex(signature), label)
    for callsite, target, label in CALL_EDGES:
        direct_call(pe, callsite, target, label)
    return {
        "original_entity": "ai_goal_assault",
        "original_goal_datamap": fields,
        "goal_actor_setup_hook": {
            "original_vtable_slot_va": "0x223F5280",
            "function_va": "0x220CFF00",
            "configures": "per-actor CAI_AssaultBehavior::SetParameters",
            "rallypoint_name_field": "goal +0x3C8",
            "cue_mode_field": "goal +0x3CC",
        },
        "BeginAssault_input": {
            "registered_input": "BeginAssault",
            "original_native_handler_va": "0x220CFF30",
            "actor_count_original_offset": "0x3B8",
            "actor_handle_list_original_offset": "0x3AC",
            "actor_by_index_resolver_va": "0x220CF120",
            "per_actor_behavior_lookup_va": "0x220CDC80",
            "effect_if_resolved": [
                "if owning NPC pointer exists: clear original NPC +0x864 and +0x85C",
                "write CAI_AssaultBehavior.m_ReceivedAssaultCue +0x20 = 1",
                "invoke owner NPC condition setter with numeric 0x18",
            ],
            "condition_setter_va": "0x22039760",
            "invalid_actor_or_behavior": "skipped; next actor still examined",
        },
        "MobGrunt_direct_Assault_input": {
            "original_handler": "0x22357E50",
            "behavior_set_parameters": "0x220CFC00",
            "cue_mode": 3,
            "distinction":
                "direct NPC Assault supplies an immediate cue mode of 3; "
                "goal setup uses configurable AssaultCue and BeginAssault "
                "later sets received cue=1 for each resolved actor",
        },
        "verified_instruction_witnesses": len(WITNESSES),
        "verified_direct_call_edges": len(CALL_EDGES),
        "constraints": [
            "This is the original 2006 Windows x86 layout; do not copy offsets to Quest ARM64.",
            "Goal activation hook is identified by vtable pointer and call graph, not a proven public C++ method name.",
            "The exact enum name of NPC condition 0x18 remains unproven.",
            "NPC +0x864/+0x85C fields are reset, but their member names are not established.",
            "No actual original SiN game entity or SourceVR AI scheduler is implemented.",
        ],
    }


def validate_sdk_fgd(data: bytes):
    text = data.decode("latin1")
    for pattern in (
        r'@PointClass\s+base\(Targetname\)\s*=\s*ai_goal_assault',
        r'AssaultCue\(choices\)\s*:\s*"Assault Cue"\s*:\s*1',
        r'1\s*:\s*"Entity System Input"',
        r'2\s*:\s*"Gunfire"',
        r'3\s*:\s*"Don.t wait for a cue\."',
        r'input\s+BeginAssault\(\s*void\s*\)\s*:\s*"Begin assault phase"',
        r'input\s+Activate\(\s*void\s*\)',
        r'input\s+Deactivate\(\s*void\s*\)',
    ):
        if not re.search(pattern, text, re.I):
            raise VerificationError(f"original SDK assault goal contract missing: {pattern}")
    return {
        "source": "SinEpisodesSDK/bin/SinEpisodes.fgd",
        "input": "BeginAssault(void)",
        "rallypoint_key": "rallypoint",
        "cue_modes": {
            1: "Entity System Input",
            2: "Gunfire",
            3: "Don't wait for a cue.",
        },
    }


def audit(server: bytes, sdk_fgd: bytes | None = None):
    digest = hashlib.sha256(server).hexdigest()
    if digest != SERVER_SHA256:
        raise VerificationError("not owner's SHA256-pinned original 2006 server.dll")
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original PE32 image base")
    result = {"original_server_sha256": digest, **verify_original_x86(pe)}
    if sdk_fgd is not None:
        result["original_sdk"] = validate_sdk_fgd(sdk_fgd)
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--sdk-zip", type=Path)
    parser.add_argument("--output", type=Path, help="create new report only; no overwrite")
    args = parser.parse_args(argv)
    try:
        game_fgd = None
        if args.sdk_zip:
            with zipfile.ZipFile(args.sdk_zip) as sdk:
                game_fgd = sdk.read("SinEpisodesSDK/bin/SinEpisodes.fgd")
        report = json.dumps(audit(args.server.read_bytes(), game_fgd), indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as destination:
                destination.write(report)
            print("Verified original ai_goal_assault BeginAssault and setup paths")
        else:
            print(report, end="")
        return 0
    except (OSError, ValueError, struct.error, KeyError, zipfile.BadZipFile) as ex:
        parser.exit(2, "Original SiN goal assault input audit failed: " + str(ex) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
