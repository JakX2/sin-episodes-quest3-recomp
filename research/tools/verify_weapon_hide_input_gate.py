#!/usr/bin/env python3
"""Phase 61: verify the original SiN CBaseCombatWeapon HideWeapon input gate.

Requires original SHA-256 pinned server.dll + client.dll (Phase 60 chain).
This is static x86 evidence, NOT original source, runtime execution, or
identification of every dynamic override of the virtual dispatch.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_reload_constructor_refs import at, verify as verify_phase60

INPUT_VA = 0x222AD800
ACTIVE_HANDLE_GETTER = 0x222AA640
SERVER_WEAPON_VTABLE = 0x2245A554
VIRTUAL_DISPATCH_OFFSET = 0x350
BASE_DISPATCH_TARGET = 0x222AE960

# Exact opcode anchors observed in the original 2006 x86 PE image. Each
# carries an independent control-flow fact. No 32-bit offsets are to be
# reused as offsets inside a native Android ARM64 C++ object.
INPUT_ANCHORS = (
    (0x222AD800, '56 8b f1', 'save_this'),
    (0x222AD803, '8b 86 94 04 00 00', 'owner_handle_at_weapon_494'),
    (0x222AD809, '83 f8 ff', 'owner_handle_invalid_sentinel'),
    (0x222AD80C, '0f 84 80 00 00 00', 'return_if_owner_invalid'),
    (0x222AD812, '8b 15 20 56 4f 22', 'entity_handle_table'),
    (0x222AD81A, '81 e1 ff 0f 00 00', 'handle_index_mask'),
    (0x222AD827, 'c1 e8 0c', 'handle_serial_shift'),
    (0x222AD82A, '39 41 04', 'handle_serial_match'),
    (0x222AD82D, '75 63', 'return_if_stale'),
    (0x222AD831, '85 c9', 'resolved_owner_null_test'),
    (0x222AD835, '8b 01', 'first_owner_virtual_lookup'),
    (0x222AD837, 'ff 90 fc 00 00 00', 'owner_interface_call_fc'),
    (0x222AD841, '8b 86 94 04 00 00', 'owner_handle_checked_again'),
    (0x222AD875, 'ff 90 fc 00 00 00', 'second_owner_interface_call_fc'),
    (0x222AD87D, 'e8 be cd ff ff', 'selected_weapon_resolver_call'),
    (0x222AD882, '3b c6', 'selected_weapon_equals_this'),
    (0x222AD884, '75 0c', 'return_if_not_selected'),
    (0x222AD888, '6a 00', 'dispatch_zero_argument'),
    (0x222AD88C, 'ff 92 50 03 00 00', 'virtual_dispatch_350'),
    (0x222AD893, 'c2 04 00', 'input_handler_ret4'),
)
ACTIVE_ANCHORS = (
    (0x222AA640, '8b 81 8c 07 00 00', 'active_weapon_handle_at_actor_78c'),
    (0x222AA646, '83 f8 ff', 'active_handle_invalid'),
    (0x222AA64B, '8b 15 20 56 4f 22', 'active_handle_uses_same_table'),
    (0x222AA653, '81 e1 ff 0f 00 00', 'active_handle_index_mask'),
    (0x222AA660, 'c1 e8 0c', 'active_handle_serial_shift'),
    (0x222AA663, '39 41 04', 'active_handle_serial_check'),
    (0x222AA668, '8b 01', 'return_resolved_selected_weapon'),
    (0x222AA66B, '33 c0', 'return_null_on_invalid'),
)
BASE_DISPATCH_ANCHORS = (
    (0x222AE960, '53 56 8b f1 8b 86 94 04 00 00', 'base_virtual_target_prologue_and_owner'),
    (0x222AE9C1, '80 7c 24 10 00', 'base_virtual_target_switch_argument'),
    (0x222AEA4D, '6a 20 8b ce e8 ea dd e9 ff', 'base_virtual_target_hide_effect_20'),
)


def validate_anchors(pe: PE32, anchors: tuple[tuple[int, str, str], ...]) -> list[str]:
    checked = []
    for va, hexdump, label in anchors:
        expected = bytes.fromhex(hexdump)
        if at(pe, va, len(expected)) != expected:
            raise VerificationError(f'{label} mismatch at {va:#x}')
        checked.append(label)
    return checked


def relative_call_target(pe: PE32, va: int) -> int:
    instruction = at(pe, va, 5)
    if instruction[0] != 0xE8:
        raise VerificationError(f'expected direct call at {va:#x}')
    return va + 5 + struct.unpack('<i', instruction[1:])[0]


def inspect(pe: PE32) -> dict:
    input_checked = validate_anchors(pe, INPUT_ANCHORS)
    getter_checked = validate_anchors(pe, ACTIVE_ANCHORS)
    base_checked = validate_anchors(pe, BASE_DISPATCH_ANCHORS)
    if relative_call_target(pe, 0x222AD87D) != ACTIVE_HANDLE_GETTER:
        raise VerificationError('InputHideWeapon resolver points to unexpected target')
    base_target = struct.unpack('<I', at(pe, SERVER_WEAPON_VTABLE + VIRTUAL_DISPATCH_OFFSET, 4))[0]
    if base_target != BASE_DISPATCH_TARGET:
        raise VerificationError('CBaseCombatWeapon vtable +0x350 target mismatch')
    return {
        'input_handler_va': hex(INPUT_VA),
        'source_owner_handle_offset': '0x494',
        'source_active_weapon_handle_offset': '0x78c',
        'serial_checked_entity_handle': True,
        'owner_accessor_virtual_byte_offset': '0xfc',
        'selected_weapon_resolver_va': hex(ACTIVE_HANDLE_GETTER),
        'dispatch_only_when_selected_weapon_equals_this': True,
        'input_dispatch_virtual_byte_offset': hex(VIRTUAL_DISPATCH_OFFSET),
        'input_dispatch_argument_immediate': 0,
        'base_weapon_vtable_va': hex(SERVER_WEAPON_VTABLE),
        'base_weapon_virtual_target_va': hex(base_target),
        'base_virtual_target_reads_reload_flags': False,
        'base_virtual_target_effect_bit_20': True,
        'verified_instruction_anchors': len(input_checked)+len(getter_checked)+len(base_checked),
        'warning': 'Corrected primary constructor-installed vtable start; the target branches on argument and handles effects bit 0x20. Derived overrides and live effects remain separately testable.'
    }


def verify(server: Path, client: Path) -> dict:
    prior = verify_phase60(server, client)
    return {'phase': 61, 'result': 'pass', 'binary_sha256': prior['sha256'],
            'input_hide_weapon': inspect(PE32(server.read_bytes()))}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('server', type=Path)
    parser.add_argument('client', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify(args.server, args.client), indent=2))
    except (OSError, ValueError, struct.error, VerificationError) as exc:
        parser.exit(1, f'PHASE 61 VERIFICATION FAILED: {exc}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
