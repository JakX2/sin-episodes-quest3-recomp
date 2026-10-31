#!/usr/bin/env python3
"""Phase 62: verify concrete SiN firearm vtables and HideWeapon visibility branch.

For original, SHA256-pinned 2006 server/client PE32 DLLs (not bundled).
Static evidence only; not executed Source engine or Android VR behavior.
"""
from __future__ import annotations

import argparse
import json
import struct
from dataclasses import dataclass
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_reload_constructor_refs import at
from verify_weapon_hide_input_gate import verify as verify_phase61, relative_call_target

DISPATCH_SLOT_BYTES = 0x350
EFFECT_BIT = 0x20
TARGET = 0x222AE960
ADD_EFFECTS = 0x2214C840


@dataclass(frozen=True)
class WeaponVTable:
    name: str
    typedesc: int
    col: int
    vtable: int
    constructor_store: int


WEAPONS = (
    WeaponVTable('CBaseCombatWeapon', 0x224DE300, 0x224CA34C, 0x2245A554, 0x222AE399),
    WeaponVTable('CWeaponMagnum', 0x2252DDF4, 0x224CF560, 0x2247875C, 0x223168CC),
    WeaponVTable('CWeaponAssaultRifle', 0x2252D58C, 0x224CF3A8, 0x224776AC, 0x22313C18),
    WeaponVTable('CWeaponScatterGun', 0x2252E8A8, 0x224CF8B4, 0x2247A1E4, 0x2231BACD),
)

# Pinned, small independent instructions, not proprietary function dumps.
EFFECT_ANCHORS = (
    (0x222AE9C1, '80 7c 24 10 00', 'argument_boolean_branch'),
    (0x222AE9D1, '24 df', 'clear_effect_20_on_weapon'),
    (0x222AEA0C, '24 df', 'clear_effect_20_on_related_entity'),
    (0x222AEA4D, '6a 20 8b ce', 'set_effect_20_on_weapon'),
    (0x222AEA5A, '6a 20 8b cf', 'set_effect_20_on_related_entity'),
    (0x2214C848, '8b 4e 78 8b c1 0b c3', 'effect_helper_ors_flag_into_state'),
    (0x2214C872, '89 46 78', 'effect_helper_stores_state'),
    (0x2214C885, 'f6 c3 20', 'effect_helper_tests_20'),
)


def read_u32(pe, va: int) -> int:
    return struct.unpack('<I', at(pe, va, 4))[0]


def verify_vtable(pe, spec: WeaponVTable) -> dict:
    name = b'.?AV' + spec.name.encode('ascii') + b'@@\0'
    if at(pe, spec.typedesc + 8, len(name)) != name:
        raise VerificationError(f'{spec.name}: RTTI class name does not match')
    signature, offset, cd_offset, desc, hierarchy = struct.unpack('<IIIII', at(pe, spec.col, 20))
    if (signature, offset, cd_offset, desc) != (0, 0, 0, spec.typedesc):
        raise VerificationError(f'{spec.name}: RTTI complete object locator disagrees')
    if hierarchy < pe.base:
        raise VerificationError(f'{spec.name}: hierarchy pointer is not a PE VA')
    if read_u32(pe, spec.vtable - 4) != spec.col:
        raise VerificationError(f'{spec.name}: RTTI/vtable boundary mismatch')
    if at(pe, spec.constructor_store, 2) != b'\xc7\x06':
        raise VerificationError(f'{spec.name}: constructor does not write vptr')
    if read_u32(pe, spec.constructor_store + 2) != spec.vtable:
        raise VerificationError(f'{spec.name}: constructor installs another vtable')
    target = read_u32(pe, spec.vtable + DISPATCH_SLOT_BYTES)
    if target != TARGET:
        raise VerificationError(f'{spec.name}: virtual +0x350 points to {target:#x} instead of visibility path')
    return {'name': spec.name, 'rtti_type_descriptor_va': hex(spec.typedesc),
            'constructor_vptr_store_va': hex(spec.constructor_store), 'primary_vtable_va': hex(spec.vtable),
            'slot_212_va': hex(spec.vtable + DISPATCH_SLOT_BYTES), 'slot_212_target_va': hex(target)}


def inspect(pe: PE32) -> dict:
    result = [verify_vtable(pe, spec) for spec in WEAPONS]
    # Earlier Phase61 erroneously used base vtable +0x24 as the start: ensure
    # the shifted pointer still demonstrates why the prior claim was false.
    mistaken_vtable = WEAPONS[0].vtable + 0x24
    if read_u32(pe, mistaken_vtable + DISPATCH_SLOT_BYTES) != 0x222AF450:
        raise VerificationError('expected to reproduce the prior nine-slot error')
    for va, hexstr, label in EFFECT_ANCHORS:
        if at(pe, va, len(bytes.fromhex(hexstr))) != bytes.fromhex(hexstr):
            raise VerificationError(f'visibility path changed: {label} at {va:#x}')
    if relative_call_target(pe, 0x222AEA51) != ADD_EFFECTS:
        raise VerificationError('primary AddEffects-like call target changed')
    if relative_call_target(pe, 0x222AEA5E) != ADD_EFFECTS:
        raise VerificationError('related-entity AddEffects-like call target changed')
    return {'weapons': result, 'vtable_slot_byte_offset': hex(DISPATCH_SLOT_BYTES),
            'common_virtual_target': hex(TARGET), 'effective_hide_input_argument': 0,
            'zero_argument_branch_adds_effect_bit': hex(EFFECT_BIT),
            'other_branch_clears_effect_bit': hex(EFFECT_BIT),
            'add_effects_like_helper_va': hex(ADD_EFFECTS),
            'flag_field_offset_in_original_server_entity': '0x78',
            'related_entity_effect_path': True,
            'prior_phase61_shifted_lookup_reproduced': True,
            'effect_instruction_anchors': len(EFFECT_ANCHORS),
            'warning': "The function effect-bit behavior is statically demonstrated. No real SourceVR integration or observed live weapon visibility is claimed."}


def verify(server: Path, client: Path) -> dict:
    earlier = verify_phase61(server, client)
    return {'phase': 62, 'result': 'pass', 'binary_sha256': earlier['binary_sha256'],
            'visibility': inspect(PE32(server.read_bytes()))}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('server', type=Path)
    p.add_argument('client', type=Path)
    a = p.parse_args()
    try:
        print(json.dumps(verify(a.server, a.client), indent=2))
    except (OSError, ValueError, struct.error, VerificationError) as e:
        p.exit(1, f'PHASE 62 VERIFICATION FAILED: {e}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
