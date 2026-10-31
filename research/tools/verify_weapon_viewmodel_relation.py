#!/usr/bin/env python3
"""Phase 63: verify original SiN weapon visibility's indexed related-entity lookup.

Requires legally owned SHA256-pinned 2006 server/client DLLs. No DLL bytes
or proprietary functions are distributed. Observations are static only.
"""
from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_reload_constructor_refs import at
from verify_weapon_hide_input_gate import relative_call_target
from verify_weapon_vtable_visibility import verify as verify_phase62

VIEWMODEL_INDEX_NAME = 0x2244F26C
INDEX_DESCRIPTORS = (0x2251FAD4, 0x225203FC)
HELPER = 0x2224AF00
VISIBILITY = 0x222AE960

# Anchors reflect independent control/data flow facts. No function dumps.
ANCHORS = (
    (0x222AE964, '8b 86 94 04 00 00', 'weapon_owner_handle'),
    (0x222AE995, '8b 01', 'owner_vtable'),
    (0x222AE997, 'ff 90 fc 00 00 00', 'owner_conversion_virtual'),
    (0x222AE9A3, '8b 13 8b cb', 'player_gate_virtual_prep'),
    (0x222AE9A7, 'ff 92 20 01 00 00', 'owner_type_gate'),
    (0x222AE9AD, '84 c0', 'gate_boolean_check'),
    (0x222AE9AF, '74 10', 'gate_failure_skips_related_lookup'),
    (0x222AE9B1, '8b 86 98 04 00 00', 'weapon_viewmodel_index_read'),
    (0x222AE9B7, '50 8b cb', 'helper_receives_index_and_owner'),
    (0x222AE9BF, '8b f8', 'related_pointer_saved'),
    (0x222AEA56, '85 ff 74 09', 'null_related_skips_effect'),
    (0x222AEA5A, '6a 20 8b cf', 'related_effect_bit_20'),
    (0x2224AF00, '8b 44 24 04', 'helper_index_argument'),
    (0x2224AF04, '8b 84 81 dc 0b 00 00', 'owner_indexed_handle_array_BDC'),
    (0x2224AF0B, '83 f8 ff', 'invalid_ehandle_sentinel'),
    (0x2224AF16, '8b c8', 'handle_index_source'),
    (0x2224AF18, '81 e1 ff 0f 00 00', 'handle_entity_index_mask'),
    (0x2224AF1E, 'c1 e1 04', 'entity_table_stride_16'),
    (0x2224AF25, 'c1 e8 0c', 'generation_shift'),
    (0x2224AF28, '39 41 04', 'generation_comparison'),
    (0x2224AF2B, '75 05', 'stale_generation_returns_null'),
    (0x2224AF2D, '8b 01 c2 04 00', 'return_valid_related_entity'),
    (0x2224AF32, '33 c0 c2 04 00', 'return_null_from_invalid_handle'),
    (0x222AD7CE, '8b 87 98 04 00 00', 'other_weapon_read_same_model_index'),
    (0x222AD7DC, '85 c0 74 0e', 'other_path_skips_missing_viewmodel'),
    (0x222AD7E0, '8b 40 78 c1 e8 05 f6 d0', 'other_path_tests_related_nodraw'),
)
CALLS = ((0x222AE9BA, HELPER, 'visibility_related_getter'),
         (0x222AD7D7, HELPER, 'is_viewmodel_visible_getter'))


def inspect(pe: PE32) -> dict:
    actual_name = at(pe, VIEWMODEL_INDEX_NAME, len(b'm_nViewModelIndex\0'))
    if actual_name != b'm_nViewModelIndex\0':
        raise VerificationError('m_nViewModelIndex original metadata name mismatch')
    for desc in INDEX_DESCRIPTORS:
        field_type, string_va, offset = struct.unpack('<III', at(pe, desc, 12))
        if (field_type, string_va, offset) != (5, VIEWMODEL_INDEX_NAME, 0x498):
            raise VerificationError(f'viewmodel index descriptor mismatch at {desc:#x}')
    for va, hex_bytes, label in ANCHORS:
        expected = bytes.fromhex(hex_bytes)
        if at(pe, va, len(expected)) != expected:
            raise VerificationError(f'{label}: changed instruction at {va:#x}')
    for call_va, target, name in CALLS:
        if relative_call_target(pe, call_va) != target:
            raise VerificationError(f'{name}: wrong helper target')
    return {'weapon_viewmodel_index_original_offset': '0x498',
            'viewmodel_index_metadata_name': 'm_nViewModelIndex',
            'descriptor_vas': [hex(v) for v in INDEX_DESCRIPTORS],
            'related_getter_va': hex(HELPER),
            'owner_side_indexed_handle_array_offset': '0xbdc',
            'related_handle_checks_generation': True,
            'owner_type_check_virtual_byte_offset': '0x120',
            'related_effect_added_only_when_resolved': True,
            'independent_related_visibility_reader': '0x222ad7ce',
            'original_instruction_anchors_checked': len(ANCHORS),
            'direct_call_targets_checked': len(CALLS),
            'evidence_boundary': 'Owner-side indexed handle is consistent with a player viewmodel; precise 2006 runtime C++ class not independently identified.'}


def verify(server: Path, client: Path) -> dict:
    previous = verify_phase62(server, client)
    return {'phase': 63, 'result': 'pass', 'binary_sha256': previous['binary_sha256'],
            'relation': inspect(PE32(server.read_bytes()))}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('server', type=Path)
    p.add_argument('client', type=Path)
    args = p.parse_args()
    try:
        print(json.dumps(verify(args.server, args.client), indent=2))
    except (OSError, ValueError, struct.error, VerificationError) as exc:
        p.exit(1, f'PHASE 63 VERIFICATION FAILED: {exc}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
