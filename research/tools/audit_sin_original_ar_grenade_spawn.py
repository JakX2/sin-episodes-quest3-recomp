#!/usr/bin/env python3
"""Verify original SiN 2006 Assault Rifle's actual grenade projectile path.

Read-only exact-VA PE32 instruction references; no original executable bytes
are emitted and no original machine code runs on Quest 3. This demonstrates
actual entity creation separate from the gun's earlier PrecacheOther call.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

SPAWN_CALL_EDGES = {
    # Original server CWeaponAssaultRifle game routine:
    0x22313E2A: 0x22146C40,  # spawn helper, following push "grenade_ar"
    0x22313E46: 0x22143610,  # state/action setter on created entity
    0x22313E55: 0x222B3430,  # additional setter on created entity
    0x22313FBB: 0x22173260,  # timestamp/state update on weapon+0x49C
    0x22313FDE: 0x221732A0,  # timestamp/state update on weapon+0x4A0
    # Shared helper invoked by this firing path:
    0x22146C48: 0x22266B00,  # CreateEntityByName
    0x22266B34: 0x2228B110,  # original server entity factory registry
}
FLOAT_LITERALS = {
    0x224771F0: 2000.0,
    0x223E97F0: 1000.0,
    0x223D9778: 0.5,
    0x223D22F8: 1.0,
}


def decode_call_target(va: int, encoded: bytes) -> int:
    if len(encoded) != 5 or encoded[0] != 0xE8:
        raise VerificationError(f"expected x86 relative CALL at 0x{va:08X}")
    return va + 5 + struct.unpack_from("<i", encoded, 1)[0]


def audit(original: bytes, *, check_hash: bool = True) -> dict:
    digest = hashlib.sha256(original).hexdigest()
    if check_hash and digest != SERVER_SHA256:
        raise VerificationError("original SiN 2006 server.dll SHA-256 mismatch")
    pe = PEImage(original)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original SiN server PE image base")
    if pe.at(0x22313E25, 5) != b"\x68" + struct.pack("<I", 0x22477230):
        raise VerificationError("AR grenade classname push instruction mismatch")
    if pe.at(0x22477230, len(b"grenade_ar\x00")) != b"grenade_ar\x00":
        raise VerificationError("grenade_ar literal mismatch")
    if pe.at(0x2245F43C, len(b"ARNADE\x00")) != b"ARNADE\x00":
        raise VerificationError("ARNADE ammo literal mismatch")
    edges = []
    for callsite, expected in SPAWN_CALL_EDGES.items():
        actual = decode_call_target(callsite, pe.at(callsite, 5))
        if actual != expected:
            raise VerificationError(
                f"AR grenade code edge mismatch: 0x{callsite:08X} => 0x{actual:08X}")
        edges.append({"callsite_va": f"0x{callsite:08X}",
                      "actual_target_va": f"0x{actual:08X}"})
    numeric = {}
    for address, expected in FLOAT_LITERALS.items():
        actual = struct.unpack("<f", pe.at(address, 4))[0]
        if actual != expected:
            raise VerificationError(f"original numeric literal mismatch at {address:#x}")
        numeric[f"0x{address:08X}"] = actual
    return {
        "dll": "original 2006 server.dll",
        "sha256": digest,
        "actual_projectile_path": {
            "original_weapon_routine_va": "0x22313C60",
            "classname_push_va": "0x22313E25",
            "classname": "grenade_ar",
            "entity_factory_helper_va": "0x22146C40",
            "CreateEntityByName_va": "0x22266B00",
            "original_entity_registry_getter_va": "0x2228B110",
            "related_ammo_literal": "ARNADE",
            "verified_direct_call_edges": edges,
            "original_float_constants": numeric,
            "original_weapon_state_offsets_seen": ["0x49C", "0x4A0"],
        },
        "evidence_limitations": [
            "The actual projectile creation path is distinct from the grenade_ar PrecacheOther site at 0x22313780.",
            "1000.0 and 2000.0 are two branch-selected vector multipliers; their physical units and branch meaning remain to confirm.",
            "0.5 and 1.0 are added to global game-time in timestamp paths; full attack timing semantics need more verification.",
            "Virtual calls on the projectile, owner/target identity and damage calculation remain unresolved.",
            "This is original Windows x86 static evidence, not an executable ARM64 game module.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    try:
        result = audit(args.server.read_bytes())
        args.output.write_text(json.dumps(result, indent=2) + "\n",
                               encoding="utf-8")
    except (OSError, ValueError, struct.error) as exc:
        p.exit(1, "SIN AR GRENADE AUDIT FAILED: " + str(exc) + "\n")
    print("Verified original assault-rifle grenade creation edges and timed states")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
