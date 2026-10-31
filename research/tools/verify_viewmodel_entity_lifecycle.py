#!/usr/bin/env python3
"""Phase 65: inspect owner viewmodel creation, handle assignment and teardown.

User-supplied SHA-256-pinned original SiN PE32 server/client binaries only.
Verifies anchored original instructions and relative calls without publishing
proprietary code, relying on dynamic runtime behavior, or equating x86 ABIs
with an ARM64 SourceVR engine.
"""
from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_reload_constructor_refs import at
from verify_weapon_hide_input_gate import relative_call_target
from verify_weapon_owner_viewmodel_field import verify as verify_phase64

CLASSNAME = 0x2244F5B0
BASE_CLASS_RTTI = 0x224EFB88
FACTORY_RTTI = 0x225183B4
FACTORY_OBJECT = 0x22518098
CREATE_HELPER = 0x2224BEA0
REMOVE_HELPER = 0x2224BF60
HANDLE_SETTER = 0x2224B4B0
CREATOR = 0x22266B00

# Compact byte-level instructions, selected at the start of actual instructions.
# Text interpretation is documented separately from the binary assertions.
ANCHORS = (
    (0x223C1007, '68 b0 f5 44 22', 'factory_register_viewmodel_name'),
    (0x223C100C, '68 98 80 51 22', 'factory_register_entity_factory_object'),
    (0x223C1013, 'ff 12', 'factory_registration_virtual_call'),
    (0x2224BEA0, '53 8b 5c 24 08 57 8b f9', 'creator_receiver_and_index'),
    (0x2224BEA8, '8b 84 9f dc 0b 00 00', 'creator_reads_indexed_owner_handle'),
    (0x2224BEAF, '83 f8 ff', 'creator_checks_missing_handle'),
    (0x2224BECC, '39 41 04', 'creator_checks_generation'),
    (0x2224BED1, '83 39 00', 'creator_reuses_live_model'),
    (0x2224BED9, '68 b0 f5 44 22', 'creator_requests_viewmodel_name'),
    (0x2224BEE3, '8b f0 83 c4 08', 'creator_keeps_factory_return'),
    (0x2224BEE8, '85 f6', 'creator_checks_failed_creation'),
    (0x2224BF00, '8d 8f bc 02 00 00', 'creator_owner_related_setup'),
    (0x2224BF0E, '57 8b ce', 'creator_passes_player_to_setup'),
    (0x2224BF16, '53 8b ce', 'creator_passes_slot_to_setup'),
    (0x2224BF27, '6a 01 57 8b ce', 'creator_post_spawn_setup'),
    (0x2224BF31, '8b 16 8b ce ff 52 08', 'creator_virtual_get_handle'),
    (0x2224BF3F, '53 8d 8f dc 0b 00 00', 'creator_assigns_owner_handle_array'),
    (0x2224BF66, 'be 01 00 00 00', 'cleanup_starts_at_slot_one'),
    (0x2224BF6B, '8d bb e0 0b 00 00', 'cleanup_starts_at_second_slot'),
    (0x2224BF73, '83 f8 ff', 'cleanup_checks_invalid_handle'),
    (0x2224BF90, '39 41 04', 'cleanup_checks_generation'),
    (0x2224BF95, '8b 01 85 c0', 'cleanup_requires_live_entity'),
    (0x2224BFA4, '8d 44 24 0c 50 56', 'cleanup_passes_slot_and_handle'),
    (0x2224BFAA, '8d 8b dc 0b 00 00', 'cleanup_assigns_owner_handle_array'),
    (0x2224BFB0, 'c7 44 24 14 ff ff ff ff', 'cleanup_passes_invalid_handle'),
    (0x2224BFBD, '83 ee 01 83 ef 04', 'cleanup_decrements_slot_and_pointer'),
    (0x2225397C, '89 8e dc 0b 00 00', 'constructor_initializes_slot_zero'),
    (0x22253982, '89 8e e0 0b 00 00', 'constructor_initializes_slot_one'),
)
CALLS = (
    (0x2224BEDE, CREATOR, 'create_named_entity'),
    (0x2224BF09, 0x221438A0, 'created_viewmodel_setup_stage_one'),
    (0x2224BF11, 0x22241250, 'created_viewmodel_setup_stage_two'),
    (0x2224BF19, 0x22241260, 'created_viewmodel_slot_assignment'),
    (0x2224BF1F, 0x22287D80, 'created_viewmodel_additional_setup'),
    (0x2224BF2C, 0x2214D3C0, 'created_viewmodel_owner_related_setup'),
    (0x2224BF4A, HANDLE_SETTER, 'creator_commits_owner_handle'),
    (0x2224BF9C, 0x22286BF0, 'cleanup_removes_viewmodel_entity'),
    (0x2224BFB8, HANDLE_SETTER, 'cleanup_clears_owner_handle'),
)


def inspect(server: PE32) -> dict:
    for va, value, label in (
        (CLASSNAME, b'viewmodel\0', 'entity_classname'),
        (BASE_CLASS_RTTI, b'.?AVCBaseViewModel@@\0', 'viewmodel_rtti'),
        (FACTORY_RTTI, b'.?AV?$CEntityFactory@VCBaseViewModel@@@@\0', 'viewmodel_entity_factory_rtti'),
    ):
        if at(server, va, len(value)) != value:
            raise VerificationError(f'{label}: original server name differs')
    factory_vtable = struct.unpack('<I', at(server, FACTORY_OBJECT, 4))[0]
    if factory_vtable != 0x2244F290:
        raise VerificationError('registered factory object vtable pointer changed')
    complete_object_locator = struct.unpack('<I', at(server, factory_vtable - 4, 4))[0]
    if complete_object_locator != 0x224C7D94:
        raise VerificationError('registered factory vtable RTTI locator changed')
    type_descriptor = struct.unpack('<I', at(server, complete_object_locator + 12, 4))[0]
    if type_descriptor != FACTORY_RTTI - 8:
        raise VerificationError('registered factory RTTI does not identify CBaseViewModel')
    for va, h, name in ANCHORS:
        expected = bytes.fromhex(h)
        if at(server, va, len(expected)) != expected:
            raise VerificationError(f'{name}: instruction at {va:#x} changed')
    for va, target, name in CALLS:
        if relative_call_target(server, va) != target:
            raise VerificationError(f'{name}: target at {va:#x} changed')
    return {
        'classname': 'viewmodel',
        'rtti_class': 'CBaseViewModel',
        'rtti_factory': 'CEntityFactory<CBaseViewModel>',
        'registered_factory_va': hex(FACTORY_OBJECT),
        'factory_type_confirmed_via_vtable_rtti': True,
        'lazy_creation_va': hex(CREATE_HELPER),
        'cleanup_va': hex(REMOVE_HELPER),
        'handle_write_helper_va': hex(HANDLE_SETTER),
        'observed_two_slot_initialization': True,
        'observed_cleanup_order': [1, 0],
        'creation_reuses_valid_existing_entity': True,
        'creation_replaces_missing_or_stale_entity': True,
        'instruction_anchors': len(ANCHORS),
        'direct_call_targets': len(CALLS),
        'limits': 'Original server static analysis. Precise semantics of several setup calls and engine lifecycle ordering need runtime validation.',
    }


def verify(server: Path, client: Path) -> dict:
    prior = verify_phase64(server, client)
    return {'phase': 65, 'result': 'pass', 'binary_sha256': prior['binary_sha256'],
            'viewmodel_lifecycle': inspect(PE32(server.read_bytes()))}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('server', type=Path)
    p.add_argument('client', type=Path)
    a = p.parse_args()
    try:
        print(json.dumps(verify(a.server, a.client), indent=2))
    except (OSError, ValueError, struct.error, VerificationError) as ex:
        p.exit(1, f'PHASE 65 VERIFICATION FAILED: {ex}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
