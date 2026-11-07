#!/usr/bin/env python3
"""Verify original 2006 SiN AssaultBehavior::SetParameters cue-state outcomes.

Read-only SHA256-pinned x86 instruction witnesses, no original binary emitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

SET_PARAMETERS_VA = 0x220CFC00
INITIALIZE_ASSAULT_VA = 0x220CE960

# Original VA, exact bytes, diagnostic label.
WITNESSES = (
    (0x220CFC00, "83 ec 44 53 55 56 57", "SetParameters entry"),
    (0x220CFC67, "0f 84 b9 01 00 00", "candidate lookup null to missing branch"),
    (0x220CFE22, "85 db 75 1d", "no best rally candidate branch"),
    (0x220CFE26, "8b 47 04 8b 80 c0 00 00 00 50", "missing-candidate message setup"),
    (0x220CFE30, "68 d0 3c 3f 22", "missing-candidate diagnostic format"),
    (0x220CFE3E, "e9 9f 00 00 00", "missing-candidate exit jump"),
    (0x220CFE49, "e8 e2 e5 ff ff", "selected point ownership helper call"),
    (0x220CFE55, "8b 00 83 f8 ff 89 47 18", "stores selected rally EHANDLE"),
    (0x220CFE5D, "74 22", "unresolved/invalid handle jumps to failure"),
    (0x220CFE7C, "83 39 00", "resolved handle pointer test"),
    (0x220CFE7F, "75 53", "valid linked rally point goes to success"),
    (0x220CFE81, "8b 44 24 58 85 c0", "invalid handle diagnostic path"),
    (0x220CFE8F, "68 a0 3c 3f 22", "invalid rally-name diagnostic format"),
    (0x220CFEA0, "c7 47 1c 00 00 00 00", "invalid handle clears configured cue mode"),
    (0x220CFED4, "8b 4c 24 5c", "valid point obtains requested cue mode arg"),
    (0x220CFED8, "89 4f 1c", "valid point assigns configured cue mode"),
    (0x220CFEDB, "8b cf", "valid point passes behavior this to reset helper"),
    (0x220CFEDD, "e8 7e ea ff ff", "valid point invokes initialization/reset helper"),
    (0x220CE960, "53 33 db 56 8b f1", "assault initialize/reset entry"),
    (0x220CE966, "88 5e 24", "reset boolean state at +0x24"),
    (0x220CE969, "88 5e 25", "reset boolean state at +0x25"),
    (0x220CE96C, "c7 46 14 ff ff ff ff", "clears active assault point handle"),
    (0x220CE973, "88 5e 26", "reset boolean state at +0x26"),
    (0x220CE976, "89 5e 28", "reset timing/state field at +0x28"),
    (0x220CE979, "89 5e 20", "resets received assault cue to zero"),
    (0x220CFEF5, "c2 08 00", "SetParameters returns with two arguments"),
)

BRANCHES = (
    (0x220CFC67, 0x220CFE26, "missing candidate through null result"),
    (0x220CFE24, 0x220CFE43, "found candidate proceeds to linking"),
    (0x220CFE3E, 0x220CFEE2, "missing candidate skips mode and cue writes"),
    (0x220CFE5D, 0x220CFE81, "invalid handle reaches mode-clear branch"),
    (0x220CFE7F, 0x220CFED4, "valid handle reaches success branch"),
)

CALLS = (
    (0x220CFF27, SET_PARAMETERS_VA, "goal setup calls original SetParameters"),
    (0x22357E8B, SET_PARAMETERS_VA, "MobGrunt Assault calls original SetParameters"),
    (0x220CFEDD, INITIALIZE_ASSAULT_VA, "successful setup calls reset helper"),
)


def require(test, message):
    if not test:
        raise VerificationError(message)


def branch_destination(pe, va):
    op = pe.at(va, 1)[0]
    if op in (0x74, 0x75):
        return (va + 2 + struct.unpack("<b", pe.at(va + 1, 1))[0]) & 0xffffffff
    if op == 0xE9:
        return (va + 5 + struct.unpack("<i", pe.at(va + 1, 4))[0]) & 0xffffffff
    if op == 0x0F and pe.at(va + 1, 1)[0] in (0x84, 0x85):
        return (va + 6 + struct.unpack("<i", pe.at(va + 2, 4))[0]) & 0xffffffff
    raise VerificationError(f"unexpected x86 relative branch opcode at {va:#x}")


def verify_call(pe, va, target, label):
    data = pe.at(va, 5)
    require(data[0] == 0xE8, f"{label}: direct CALL opcode changed")
    actual = (va + 5 + struct.unpack_from("<i", data, 1)[0]) & 0xffffffff
    require(actual == target, f"{label}: CALL now targets {actual:#x}")


def verify_original(pe):
    require(pe.base == 0x22000000, "unexpected original server.dll image base")
    for va, signature, label in WITNESSES:
        expected = bytes.fromhex(signature)
        require(pe.at(va, len(expected)) == expected,
                f"{label}: original bytes changed at {va:#x}")
    for va, target, label in BRANCHES:
        require(branch_destination(pe, va) == target, f"{label}: changed branch target")
    for va, target, label in CALLS:
        verify_call(pe, va, target, label)
    name = b"CAI_AssaultBehavior::SetParameters\0"
    require(pe.at(0x223F3C7C, len(name)) == name, "native diagnostic name changed")
    return {
        "function_va": "0x220CFC00",
        "verified_witnesses": len(WITNESSES),
        "verified_branches": len(BRANCHES),
        "verified_calls": len(CALLS),
        "outcomes": {
            "no_candidate": "No cue mode or received-cue assignment on this exit path",
            "candidate_unresolved": "Writes configured cue +0x1C = 0 without calling reset helper",
            "candidate_resolved": "Writes requested mode +0x1C then calls 0x220CE960, which writes received cue +0x20 = 0",
        },
        "limits": [
            "These are selected verified x86 paths; not full entity lookup or exact original source.",
            "Invalid rally handle may also trigger base NPC fallback, not modeled here.",
            "Offsets are from the original 32-bit DLL only, not SourceVR ARM64 ABI.",
            "Host callback result enum is a new port-side design, not original return value.",
        ],
    }


def audit(data):
    sha = hashlib.sha256(data).hexdigest()
    require(sha == SERVER_SHA256, "original server.dll SHA-256 mismatch")
    return {"server_sha256": sha, **verify_original(PEImage(data))}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        print(json.dumps(audit(args.server.read_bytes()), indent=2))
        return 0
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(2, f"Original SetParameters verification failed: {exc}\n")


if __name__ == "__main__":
    raise SystemExit(main())
