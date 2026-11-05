#!/usr/bin/env python3
"""Read-only original SiN MobGrunt rally-point and conditional timing audit.

Input is hash-pinned owner's 2006 server.dll, no proprietary bytes emitted.
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


def same(pe, address, expected, label):
    if pe.at(address, len(expected)) != expected:
        raise VerificationError(f'{label}: mismatch at 0x{address:08X}')


def call_to(pe, address, expected):
    data = pe.at(address, 5)
    if data[0] != 0xE8 or address + 5 + struct.unpack_from('<i', data, 1)[0] != expected:
        raise VerificationError(f'call at 0x{address:08X} not to 0x{expected:08X}')


def original_float(pe, address, expected):
    actual, = struct.unpack('<f', pe.at(address, 4))
    if abs(actual - expected) > 1e-7:
        raise VerificationError(f'floating-point constant mismatch at 0x{address:08X}')
    return actual


# Original 2006 server.dll static registry thunks: (class, entry VA, name VA, factory VA).
ASSAULT_REGISTRATIONS = (
    ('assault_rallypoint', 0x223B9190, 0x223F5640, 0x224E4BB8),
    ('assault_assaultpoint', 0x223B91B0, 0x223F5654, 0x224E4BBC),
    ('ai_goal_assault', 0x223B91F0, 0x223F566C, 0x224E4BC0),
)


def verify_assault_class_registration(pe):
    registrations = []
    for name, entry, name_va, factory_va in ASSAULT_REGISTRATIONS:
        chunk = pe.at(entry, 22)
        if not (chunk[:1] == b'\xe8' and chunk[5:8] == b'\x8b\x10\x68' and
                chunk[12] == 0x68 and chunk[17:] == b'\x8b\xc8\xff\x12\xc3'):
            raise VerificationError(f'{name}: original registry thunk shape differs')
        call_to(pe, entry, 0x2228B110)
        if (struct.unpack_from('<I', chunk, 8)[0] != name_va or
                struct.unpack_from('<I', chunk, 13)[0] != factory_va):
            raise VerificationError(f'{name}: original name/factory pointer differs')
        same(pe, name_va, name.encode('ascii') + b'\0', name + ' original class literal')
        registrations.append({
            'classname': name, 'registration_va': f'0x{entry:08X}',
            'factory_object_va': f'0x{factory_va:08X}',
            'registry_getter_va': '0x2228B110',
        })
    return registrations


def audit_code(pe):
    # InputAssault turns the Source variant into a string; an assault-behavior
    # subobject at this+0xDB8 takes that cue/rally point and an integer 3.
    same(pe, 0x22357E50, b'\x56\x8b\xf1\x8b\x4c\x24\x08', 'InputAssault entry')
    same(pe, 0x22357E5A, b'\x83\x79\x10\x02', 'variant string type check')
    call_to(pe, 0x22357E6D, 0x221561D0)
    call_to(pe, 0x22357E78, 0x221F3DE0)
    same(pe, 0x22357E82, b'\x6a\x03\x51\x8d\x8e\xb8\x0d\x00\x00',
         'Assault parameter + subobject offset')
    call_to(pe, 0x22357E8B, 0x220CFC00)
    same(pe, 0x220CFC1A, b'\x68\x7c\x3c\x3f\x22', 'SetParameters diagnostic xref')
    same(pe, 0x223F3C7C, b'CAI_AssaultBehavior::SetParameters\0',
         'original assault routine name')
    same(pe, 0x223F3CA0, b"**ERROR: Can't find a rally point named '%s'\n\0",
         'missing rally-point diagnostic')

    # ThrowGrenades input (variant bool) resets grenade height to 0 and writes
    # boolean byte at +0xC6C, or clears both fields on a type mismatch.
    same(pe, 0x22357EA4, b'\x83\x78\x18\x06', 'grenade input variant check')
    same(pe, 0x22357EAD, b'\xc7\x81\x70\x0c\x00\x00\x00\x00\x00\x00',
         'grenade height reset')
    same(pe, 0x22357EB7, b'\x88\x81\x6c\x0c\x00\x00', 'grenade toggle write')
    same(pe, 0x22357EC0, b'\x33\xc0\x89\x81\x70\x0c\x00\x00\x88\x81\x6c\x0c\x00\x00',
         'invalid-type clears both fields')

    # Two independent original float-return functions select based on +0xC6C.
    same(pe, 0x22357FA0, b'\x80\xb9\x6c\x0c\x00\x00\x00\x74\x07',
         'first flag-dependent accessor')
    same(pe, 0x22357FA9, b'\xd9\x05\xd0\x99\x3d\x22\xc3', 'first active value')
    same(pe, 0x22357FB0, b'\xd9\x05\xf8\x22\x3d\x22\xc3', 'first inactive value')
    same(pe, 0x22357FC0, b'\x80\xb9\x6c\x0c\x00\x00\x00\x74\x07',
         'second flag-dependent accessor')
    same(pe, 0x22357FC9, b'\xd9\x05\xac\x7c\x40\x22\xc3', 'second active value')
    same(pe, 0x22357FD0, b'\xd9\x05\x80\xe1\x3d\x22\xc3', 'second inactive value')
    active = [original_float(pe, 0x223D99D0, 0.25),
              original_float(pe, 0x22407CAC, 2.5)]
    inactive = [original_float(pe, 0x223D22F8, 1.0),
                original_float(pe, 0x223DE180, 6.0)]

    # An NPC setup path uses the same flag to choose the first duration,
    # then adds it to a global game-time value and writes NPC +0xC80.
    same(pe, 0x2235F3DA, b'\x38\x9d\x6c\x0c\x00\x00', 'NPC flag comparison')
    same(pe, 0x2235F3F2, b'\xd9\x05\xd0\x99\x3d\x22', 'true initial duration')
    same(pe, 0x2235F3FA, b'\xd9\x05\xf8\x22\x3d\x22', 'false initial duration')
    same(pe, 0x2235F406, b'\xd8\x42\x0c', 'game time addition')
    same(pe, 0x2235F410, b'\xd9\x9d\x80\x0c\x00\x00', 'initial timestamp store')
    same(pe, 0x2235F461, b'\x88\x9d\x6c\x0c\x00\x00', 'flag reset')

    return {
        'registered_assault_entities': verify_assault_class_registration(pe),
        'assault_input': {
            'handler_va': '0x22357E50',
            'string_variant_type_comparison': 2,
            'behavior_object_offset': '0xDB8',
            'callee_va': '0x220CFC00',
            'callee_identified_by_original_diagnostic': 'CAI_AssaultBehavior::SetParameters',
            'passed_integer': 3,
            'original_error_mentions_missing_named_rally_point': True,
        },
        'throw_grenades_input': {
            'handler_va': '0x22357EA0',
            'boolean_variant_type_comparison': 6,
            'toggle_byte_offset': '0xC6C',
            'reset_grenade_height_offset': '0xC70',
        },
        'conditional_values': {
            'first_accessor_va': '0x22357FA0',
            'second_accessor_va': '0x22357FC0',
            'enabled': active,
            'disabled': inactive,
            'initialization_timestamp_offset': '0xC80',
        },
        'limits': [
            'Original input/branch and time data flow verified, but semantic names for both float-return accessors remain unknown.',
            'Integer argument 3 is not proven to be a specific AI schedule enum.',
            'Values 0.25, 1.0, 2.5, 6.0 must NOT be claimed as verified grenade cooldown or damage.',
            'Original 32-bit object offsets are not transferable to ARM64 game classes.',
        ],
    }


def check_sdk_fgd(base_fgd: bytes, game_fgd: bytes):
    """Cross-correlate the original SDK FGD contract with machine-code semantics."""
    base = base_fgd.decode('latin1')
    game = game_fgd.decode('latin1')
    exact_assault = (r'input\s+Assault\(string\)\s*:\s*'
                     r'"Start an assault\. Parameter passed in should be the name of the rally point\."')
    if len(re.findall(exact_assault, base, flags=re.I)) != 1:
        raise VerificationError('SDK base.fgd Assault(string) rally-point contract differs')
    cue = re.findall(
        r'AssaultCue\(choices\)\s*:\s*"Assault Cue"\s*:\s*1\s*=\s*\[([^\]]+)\]',
        game, flags=re.S)
    if len(cue) != 1 or not re.search(r'3\s*:\s*"Don.t wait for a cue\."', cue[0]):
        raise VerificationError('SDK AssaultCue=3 contract differs')
    kinds = ['assault_rallypoint', 'assault_assaultpoint', 'ai_goal_assault']
    for name in kinds:
        if len(re.findall(r'=\s*' + re.escape(name) + r'\s*:', game)) != 1:
            raise VerificationError(f'SDK entity definition missing: {name}')
    return {
        'assault_input': 'Assault(string) expects the name of a rally point',
        'assault_cue_value_3': 'Don\'t wait for a cue.',
        'assault_entity_classnames': kinds,
        'interpretation': (
            'SDK AssaultCue 3 matches original machine-code parameter 3; '
            'relationship is corroboration, not a full reconstructed enum/implementation.'
        ),
    }


def audit(server, verify_hash=True):
    digest = hashlib.sha256(server).hexdigest()
    if verify_hash and digest != SERVER_SHA256:
        raise VerificationError('expected original 2006 server.dll SHA256')
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError('unexpected original PE base')
    return {'original_server_sha256': digest, **audit_code(pe)}


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server', type=Path, required=True)
    p.add_argument('--sdk-zip', type=Path,
                   help='optional original SinEpisodesSDK.zip, to verify FGD semantics')
    p.add_argument('--output', type=Path)
    args = p.parse_args(argv)
    try:
        value = audit(args.server.read_bytes())
        if args.sdk_zip:
            with zipfile.ZipFile(args.sdk_zip) as sdk:
                value['original_sdk_fgd'] = check_sdk_fgd(
                    sdk.read('SinEpisodesSDK/bin/base.fgd'),
                    sdk.read('SinEpisodesSDK/bin/SinEpisodes.fgd'))
        data = json.dumps(value, indent=2) + '\n'
        if args.output:
            with args.output.open('x', encoding='utf8') as f:
                f.write(data)
        else:
            print(data, end='')
        return 0
    except (OSError, ValueError, struct.error, KeyError, zipfile.BadZipFile) as err:
        p.exit(2, 'Original assault behavior audit failed: ' + str(err) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
