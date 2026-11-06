#!/usr/bin/env python3
"""Audit original SiN 2006 assault rallypoint ownership and assaultpoint linkage.

Read-only instruction-level verification against owner's hash-pinned server.dll.
No game code is executed, redistributed or translated from x86 to ARM64.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError


# Exact instruction witnesses from original Windows x86 server.dll.
# Each short witness is a precondition for the semantic notes below.
PATTERNS = (
    ("lock reads existing handle +0x3BC", 0x220CE433, b"\x8b\x86\xbc\x03\x00\x00"),
    ("lock checks live owner entity", 0x220CE45B, b"\x83\x39\x00"),
    ("lock refuses occupied point", 0x220CE460, b"\x32\xc0"),
    ("lock reads requested owner", 0x220CE466, b"\x8b\x4c\x24\x08"),
    ("lock obtains owner handle via virtual slot", 0x220CE46E, b"\x8b\x01\xff\x50\x08"),
    ("lock stores requested owner handle", 0x220CE475, b"\x89\x8e\xbc\x03\x00\x00"),
    ("lock null owner clears handle", 0x220CE481, b"\xc7\x86\xbc\x03\x00\x00\xff\xff\xff\xff"),
    ("release reads current owner handle", 0x220CE4A0, b"\x8b\x81\xbc\x03\x00\x00"),
    ("release validates live entity pointer", 0x220CE4C9, b"\x8b\x12\x85\xd2"),
    ("release compares current owner with requester", 0x220CE4F7, b"\x3b\x54\x24\x08"),
    ("release refuses another owner", 0x220CE4FD, b"\x32\xc0"),
    ("release clears ownership", 0x220CE503, b"\xc7\x81\xbc\x03\x00\x00\xff\xff\xff\xff"),
    ("SetParameters passes NPC to lock function", 0x220CFE43, b"\x8b\x4f\x04\x51"),
    ("SetParameters caches selected rallypoint handle", 0x220CFE5A, b"\x89\x47\x18"),
    ("assault advance resets arrival flag 0", 0x220CE966, b"\x88\x5e\x24"),
    ("assault advance resets arrival flag 1", 0x220CE969, b"\x88\x5e\x25"),
    ("assault advance clears assaultpoint handle", 0x220CE96C, b"\xc7\x46\x14\xff\xff\xff\xff"),
    ("assault advance resets flag 2", 0x220CE973, b"\x88\x5e\x26"),
    ("assault advance resets timestamp field +0x28", 0x220CE976, b"\x89\x5e\x28"),
    ("assault advance resets state field +0x20", 0x220CE979, b"\x89\x5e\x20"),
    ("assault advance reads stored rallypoint handle", 0x220CE97C, b"\x8b\x46\x18"),
    ("assault advance reads rally assaultpoint name +0x394", 0x220CE9A7,
     b"\x8b\x80\x94\x03\x00\x00"),
    ("assault advance stores resolved assaultpoint handle", 0x220CE9E6, b"\x89\x46\x14"),
    ("missing assaultpoint diagnostics xref", 0x220CEA4A, b"\x68\xc0\x3b\x3f\x22"),
    ("chain advance reads active assaultpoint handle", 0x220CE5B9, b"\x8b\x46\x14"),
    ("chain advance checks next-assaultpoint name", 0x220CE5DE,
     b"\x83\xb9\x98\x03\x00\x00\x00"),
    ("chain advance reads next-assaultpoint name", 0x220CE610,
     b"\x8b\x89\x98\x03\x00\x00"),
    ("chain advance stores new assaultpoint handle", 0x220CE654, b"\x89\x46\x14"),
    ("chain advance resets point-arrival flag", 0x220CE657, b"\xc6\x46\x25\x00"),
    ("missing next point diagnostics xref", 0x220CE68F, b"\x68\x8c\x3b\x3f\x22"),
    ("terminal point clears assault state", 0x220CE6DA,
     b"\xc7\x46\x1c\x00\x00\x00\x00"),
    ("rallypoint arrival sets behavior flag +0x24", 0x220CF539,
     b"\xc6\x46\x24\x01"),
    ("rally OnArrival output address +0x3A4", 0x220CF54E,
     b"\x8b\xc8\x81\xc1\xa4\x03\x00\x00"),
    ("case progression checks never-timeout flag", 0x220CF5CA,
     b"\x80\xb8\xba\x03\x00\x00\x00"),
    ("case progression checks nonempty next point", 0x220CF5DA,
     b"\x83\xb8\x98\x03\x00\x00\x00"),
    ("contact progression checks clear-on-contact flag", 0x220CF960,
     b"\x80\xb8\xb8\x03\x00\x00\x00"),
    ("contact progression tests condition integer 10", 0x220CF970,
     b"\x6a\x0a"),
)


CALL_EDGES = (
    ("selected point acquire owner lock", 0x220CFE49, 0x220CE430),
    ("after successful selection reset/link state", 0x220CFEDD, 0x220CE960),
    ("resolve assaultpoint by linked map name", 0x220CE9D1, 0x223A5BAB),
    ("resolve next assaultpoint by map name", 0x220CE63F, 0x223A5BAB),
    ("release rally ownership at end of assault chain", 0x220CE70F, 0x220CE4A0),
    ("rallypoint OnArrival output method", 0x220CF556, 0x22156900),
    ("case-triggered point chain advance", 0x220CF5E9, 0x220CE5B0),
    ("clear-on-contact NPC condition query", 0x220CF972, 0x220397C0),
    ("contact-triggered point chain advance", 0x220CF981, 0x220CE5B0),
)

ERROR_LITERAL_VA = 0x223F3BC0
ERROR_LITERAL = b"**ERROR: Can't find assault point named: %s\n\x00"
NEXT_ERROR_LITERAL_VA = 0x223F3B8C
NEXT_ERROR_LITERAL = b"**ERROR: Can't find next assault point: %s\n\x00"


def verify_call(pe: PEImage, call_va: int, target_va: int) -> None:
    code = pe.at(call_va, 5)
    if code[0] != 0xE8:
        raise VerificationError(f"expected x86 direct CALL at 0x{call_va:08X}")
    actual = (call_va + 5 + struct.unpack_from("<i", code, 1)[0]) & 0xFFFFFFFF
    if actual != target_va:
        raise VerificationError(f"call at 0x{call_va:08X} unexpectedly targets 0x{actual:08X}")


def audit_instruction_evidence(pe: PEImage) -> dict:
    for label, address, expected in PATTERNS:
        if pe.at(address, len(expected)) != expected:
            raise VerificationError(f"{label}: instruction changed at 0x{address:08X}")
    for _, address, target in CALL_EDGES:
        verify_call(pe, address, target)
    if pe.at(ERROR_LITERAL_VA, len(ERROR_LITERAL)) != ERROR_LITERAL:
        raise VerificationError("original missing-assaultpoint diagnostic differs")
    if pe.at(NEXT_ERROR_LITERAL_VA, len(NEXT_ERROR_LITERAL)) != NEXT_ERROR_LITERAL:
        raise VerificationError("original missing-next-assaultpoint diagnostic differs")
    # The observed Source handle index uses 12 low bits and the serial in the
    # remaining upper bits. This is evidence only, not a reusable ABI.
    return {
        "server_class": "CAssaultRallyPoint / CAI_AssaultBehavior",
        "rallypoint_lock": {
            "acquire_function_va": "0x220CE430",
            "release_function_va": "0x220CE4A0",
            "locked_by_x86_member_offset": "0x3BC",
            "empty_source_handle": "0xFFFFFFFF",
            "acquire_when_live_owner_exists": "reject, including same-owner retry",
            "acquire_when_handle_empty_or_stale": "store new owner handle or empty when null",
            "release_when_live_owner_differs": "reject, leave old owner",
            "release_when_same_owner_or_expired": "clear to 0xFFFFFFFF",
        },
        "assault_state_transition": {
            "input_selection_method_va": "0x220CFC00",
            "selected_rallypoint_owning_behavior_offset": "0x18",
            "acquire_rallypoint_call_va": "0x220CFE49",
            "next_state_init_call_va": "0x220CFEDD",
            "next_state_init_function_va": "0x220CE960",
            "rallypoint_assault_name_original_offset": "0x394",
            "linked_assaultpoint_handle_original_offset": "0x14",
            "assaultpoint_by_name_lookup_call_va": "0x220CE9D1",
            "missing_name_diagnostic": "Can't find assault point named",
        },
        "next_assaultpoint_chain": {
            "advance_function_va": "0x220CE5B0",
            "current_point_handle_original_offset": "0x14",
            "next_assaultpoint_key_original_offset": "0x398",
            "lookup_by_next_point_name_call_va": "0x220CE63F",
            "found_point_handle_store_va": "0x220CE654",
            "reset_arrival_flag_original_offset": "0x25",
            "missing_target_diagnostic_va": "0x223F3B8C",
            "no_next_point_termination_branch_va": "0x220CE6D7",
            "terminal_rally_lock_release_call_va": "0x220CE70F",
            "behavior_state_reset_offset": "0x1C",
        },
        "progression_trigger_sites": {
            "rally_arrival": {
                "flag_set_va": "0x220CF539",
                "OnArrival_event_member_offset": "0x3A4",
                "event_output_call_va": "0x220CF556",
            },
            "case_triggered": {
                "advance_call_va": "0x220CF5E9",
                "never_timeout_member_offset": "0x3BA",
                "next_point_name_member_offset": "0x398",
                "observed_conditional": "when neverTimeout and no next name, bypass this advance path",
            },
            "clear_on_contact": {
                "advance_call_va": "0x220CF981",
                "enabled_flag_offset": "0x3B8",
                "npc_condition_query_va": "0x220CF972",
                "queried_condition_numeric_id": 10,
                "interpretation": "both clear-on-contact flag and NPC condition query gate advancement",
            },
        },
        "evidence": {
            "verified_instruction_witnesses": len(PATTERNS),
            "verified_call_edges": len(CALL_EDGES),
            "verified_error_literal_va": "0x223F3BC0",
        },
        "limits": [
            "Original named next-point lookup and terminal lock release are verified, "
            "but the complete conditions that trigger progression and AI schedules remain unresolved.",
            "Acquisition/release calls and comparisons are original x86 evidence, "
            "not a field-level copy of the original 2006 C++ source.",
            "A missing or stale entity handle is safely modeled by host liveness; "
            "original Source 32-bit serial/index handle encoding is not a Quest ABI.",
            "The original x86 object offsets are not valid native ARM64 offsets.",
            "NPC condition ID 10 and the case-triggered event enumeration are observed numbers, not yet fully named.",
        ],
    }


def audit(original: bytes) -> dict:
    digest = hashlib.sha256(original).hexdigest()
    if digest != SERVER_SHA256:
        raise VerificationError("original 2006 SiN server.dll hash mismatch")
    pe = PEImage(original)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original PE image base")
    return {"server_sha256": digest, **audit_instruction_evidence(pe)}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, type=Path)
    parser.add_argument("--output", type=Path, help="write a new JSON file only")
    args = parser.parse_args(argv)
    try:
        value = audit(args.server.read_bytes())
        content = json.dumps(value, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open("x", encoding="utf8") as output:
                output.write(content)
            print("Verified original rally lock/unlock and assault-point link flow")
        else:
            print(content, end="")
        return 0
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(2, "SiN rallypoint lock audit failed: " + str(exc) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
