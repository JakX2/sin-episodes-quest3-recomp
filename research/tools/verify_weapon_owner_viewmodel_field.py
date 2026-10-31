#!/usr/bin/env python3
"""Phase 64: prove SiN owner.m_hViewModel is the array consumed by HideWeapon.

Checks exact code/data anchors against independently SHA-256-pinned original 2006
SiN client/server PE32 DLLs. No copyrighted DLL data is redistributed.
Dynamic server metadata is verified as initialization INSTRUCTIONS, not as a
populated static data image or a runtime-observed object.
"""
from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_reload_constructor_refs import at
from verify_weapon_hide_input_gate import relative_call_target
from verify_weapon_viewmodel_relation import verify as verify_phase63

SERVER_NAME_VA = 0x22450B88
CLIENT_NAME_VA = 0x2428A43C
SERVER_DYNAMIC_DESCRIPTOR_VA = 0x225195B0
CLIENT_STATIC_DESCRIPTOR_VA = 0x242F9290
OWNER_VIEWMODEL_OFFSET_SERVER = 0xBDC
OWNER_VIEWMODEL_OFFSET_CLIENT = 0x1058
OBSERVED_VIEWMODEL_SLOTS = 2

# The server descriptor starts as zeros on disk. These are distinct static
# instructions that build its type, name, offset, and overall field bytes.
SERVER_INITIALIZERS = (
    (0x22251BFC, 0x225195B0, 13, 'server_field_type_13'),
    (0x22251C06, 0x225195B4, SERVER_NAME_VA, 'server_field_name'),
    (0x22251C10, 0x225195B8, OWNER_VIEWMODEL_OFFSET_SERVER, 'server_field_offset'),
    (0x22251C44, 0x225195D4, 8, 'server_array_bytes_8'),
)
# Server and client register the array with a literal length of two.
CODE_ANCHORS = (
    (0x22249C14, '68 dc 0b 00 00', 'server_register_offset'),
    (0x22249C19, '68 88 0b 45 22', 'server_register_name'),
    (0x22249C2A, '68 88 0b 45 22', 'server_array_register_name'),
    (0x22249C2F, '6a 04', 'server_element_bytes_4'),
    (0x22249C31, '6a 02', 'server_element_count_2'),
    (0x240D3AE7, '68 58 10 00 00', 'client_register_offset'),
    (0x240D3AEC, '68 f8 a8 28 24', 'client_other_field_is_not_same_name'),
    (0x240D3AFD, '68 3c a4 28 24', 'client_register_array_name'),
    (0x240D3B02, '6a 04', 'client_element_bytes_4'),
    (0x240D3B04, '6a 02', 'client_element_count_2'),
    (0x2224AF04, '8b 84 81 dc 0b 00 00', 'related_lookup_owner_array'),
)
CALL_ANCHORS = (
    (0x22249C23, 0x2227EB70, 'server_field_registration'),
    (0x22249C38, 0x2225EE70, 'server_array_registration'),
    (0x240D3B0B, 0x24126BD0, 'client_array_registration'),
)


def inspect(server: PE32, client: PE32) -> dict:
    for pe, va, role in ((server, SERVER_NAME_VA, 'server'),
                         (client, CLIENT_NAME_VA, 'client')):
        if at(pe, va, len(b'm_hViewModel\0')) != b'm_hViewModel\0':
            raise VerificationError(f'{role}: wrong owner field name')

    # The server descriptor lives in .data but begins uninitialized.
    if at(server, SERVER_DYNAMIC_DESCRIPTOR_VA, 52) != b'\x00' * 52:
        raise VerificationError('server runtime descriptor expected zero-initialized on disk')
    for va, destination, value, label in SERVER_INITIALIZERS:
        expected = b'\xC7\x05' + struct.pack('<II', destination, value)
        if at(server, va, len(expected)) != expected:
            raise VerificationError(f'{label}: wrong dynamic registration instruction')

    # The client has a fully static typedescription_t of type 13 (EHANDLE in
    # corresponding Source field-type definitions), 2 elements, 8 bytes total.
    if struct.unpack('<III', at(client, CLIENT_STATIC_DESCRIPTOR_VA, 12)) != (
            13, CLIENT_NAME_VA, OWNER_VIEWMODEL_OFFSET_CLIENT):
        raise VerificationError('client m_hViewModel field type/name/offset mismatch')
    if struct.unpack('<H', at(client, CLIENT_STATIC_DESCRIPTOR_VA+16, 2))[0] != 2:
        raise VerificationError('client m_hViewModel descriptor count is not two')
    if struct.unpack('<I', at(client, CLIENT_STATIC_DESCRIPTOR_VA+36, 4))[0] != 8:
        raise VerificationError('client m_hViewModel descriptor byte width is not eight')

    for va, expected_hex, label in CODE_ANCHORS:
        pe = client if va >= client.base and va < client.base+0x400000 else server
        expected = bytes.fromhex(expected_hex)
        if at(pe, va, len(expected)) != expected:
            raise VerificationError(f'{label}: modified code bytes at {va:#x}')
    for va, target, label in CALL_ANCHORS:
        pe = client if va >= client.base and va < client.base+0x400000 else server
        if relative_call_target(pe, va) != target:
            raise VerificationError(f'{label}: unexpected call destination')

    return {
        'field_name': 'm_hViewModel',
        'original_2006_server_offset': hex(OWNER_VIEWMODEL_OFFSET_SERVER),
        'original_2006_client_offset': hex(OWNER_VIEWMODEL_OFFSET_CLIENT),
        'array_elements': OBSERVED_VIEWMODEL_SLOTS,
        'element_size': 4,
        'server_registration': 'runtime-initialized type 13 descriptor and array registration',
        'client_registration': 'static type 13 descriptor and array registration',
        'same_server_owner_array_used_by_visibility_helper': True,
        'initializers_checked': len(SERVER_INITIALIZERS),
        'instruction_anchors_checked': len(CODE_ANCHORS),
        'registration_call_targets_checked': len(CALL_ANCHORS),
        'scope': 'Specific original build; object dynamic type of each handle target unproven; not native Android layout',
    }


def verify(server_path: Path, client_path: Path) -> dict:
    previous = verify_phase63(server_path, client_path)
    s = PE32(server_path.read_bytes())
    c = PE32(client_path.read_bytes())
    return {'phase': 64, 'result': 'pass', 'binary_sha256': previous['binary_sha256'],
            'viewmodel_owner_field': inspect(s, c)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('server', type=Path)
    p.add_argument('client', type=Path)
    a = p.parse_args()
    try:
        print(json.dumps(verify(a.server, a.client), indent=2))
    except (OSError, ValueError, struct.error, VerificationError) as exc:
        p.exit(1, f'PHASE 64 VERIFICATION FAILED: {exc}\n')

if __name__ == '__main__':
    main()
