#!/usr/bin/env python3
"""Pinned 2006 SiN grunt input-handler execution audit, no original DLL published.

Uses the independent Phase84 PE RVA reader. All addresses refer to the exact
SHA-pinned owner-held Windows i386 server.dll. Tests verify registration
pointers and *specific instructions*, not an entire NPC AI call graph.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from verify_sin_spawn_count import read_pe_va, PEVerificationError

SERVER_SHA256 = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"

# Registry descriptors: source input name pointer is at the pinned descriptor
# address; the registered member-function entry is +8 bytes.
REGISTERED_HANDLERS = {
    "LookOn": (0x22535520, 0x22490f20, 0x2235ddc0),
    "LookOff": (0x225354ec, 0x22490f34, 0x2235f260),
    "StartPatrolling": (0x22535554, 0x22485b74, 0x22357e30),
    "StopPatrolling": (0x22535588, 0x22485b50, 0x22357e40),
    "ThrowGrenades": (0x225355f0, 0x22490eec, 0x22357ea0),
}
# Short independent instruction *checks*, not disassembled game source.
# They pin the flags word at +0xb4, patrol byte +0xc65, grenade aggression
# byte +0xc6c, and auxiliary grenade word +0xc70.
INSTRUCTIONS = {
    0x2235ddc0: "8b91b4000000",       # mov edx,[ecx+b4]
    0x2235ddc8: "25fffffeff",         # AND: clear 0x10000
    0x2235dde1: "8981b4000000",       # flag write
    0x2235ddf4: "830a01",             # conditional AI state bit update
    0x2235f260: "8b91b4000000",
    0x2235f268: "0d00000100",         # OR: set 0x10000
    0x2235f281: "8981b4000000",
    0x2235f294: "830a01",
    0x22357e30: "c681650c000001",     # patrol true
    0x22357e37: "c20400",             # ret 4
    0x22357e40: "c681650c000000",     # patrol false
    0x22357e47: "c20400",
    0x22357ea4: "83781806",           # typed value field: 6
    0x22357ea8: "7516",               # branch if not type 6
    0x22357ead: "c781700c000000000000", # reset auxiliary 32-bit word
    0x22357eb7: "88816c0c0000",       # bool variant byte -> aggression byte
    0x22357ec2: "8981700c0000",       # reset auxiliary word on wrong type
    0x22357ec8: "88816c0c0000",       # clear aggression byte on wrong type
}

class GruntHandlerError(ValueError):
    pass


def verify(original: bytes, *, sha256=SERVER_SHA256, registrations=None,
           signatures=None):
    if hashlib.sha256(original).hexdigest() != sha256:
        raise GruntHandlerError("original server.dll SHA-256 mismatch")
    registration = REGISTERED_HANDLERS if registrations is None else registrations
    signatures = INSTRUCTIONS if signatures is None else signatures
    for name, (descriptor, name_va, handler_va) in registration.items():
        if read_pe_va(original, name_va, len(name) + 1) != name.encode("ascii") + b"\0":
            raise GruntHandlerError("input name mismatch: " + name)
        found_name = struct.unpack("<I", read_pe_va(original, descriptor, 4))[0]
        found_handler = struct.unpack("<I", read_pe_va(original, descriptor + 8, 4))[0]
        if found_name != name_va or found_handler != handler_va:
            raise GruntHandlerError("datamap handler pointer mismatch: " + name)
    for address, signature in signatures.items():
        expected = bytes.fromhex(signature)
        if read_pe_va(original, address, len(expected)) != expected:
            raise GruntHandlerError("x86 handler instruction mismatch at " + hex(address))
    return {
        "input_handler_registry_verified": sorted(registration),
        "instruction_signatures_verified": len(signatures),
        "original_dll_hash_verified": True,
        "look_flag_mask": "0x00010000",
        "runtime_spawnflag_field_offset": "0xb4",
        "patrol_flag_field_offset": "0xc65",
        "grenade_aggression_field_offset": "0xc6c",
        "grenade_auxiliary_zero_field_offset": "0xc70",
        "typed_boolean_variant_value": 6,
        "invalid_variant_fallback": "zero aggression and auxiliary field",
        "original_ai_schedule_reconstructed": False,
        "sourcevr_game_module_loaded": False,
        "quest_headset_tested": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_server_dll", type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.original_server_dll.read_bytes())
        print(json.dumps(result, indent=2, sort_keys=True))
    except (OSError, ValueError, PEVerificationError) as exc:
        parser.exit(1, "grunt handler audit failed: " + str(exc) + "\n")

if __name__ == "__main__":
    main()
