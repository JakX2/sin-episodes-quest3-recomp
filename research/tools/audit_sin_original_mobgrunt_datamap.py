#!/usr/bin/env python3
"""Read-only pinned original 2006 SiN MobGrunt datamap + input dispatch audit.

This is original x86 Windows PE structural evidence, not recovered C++ code.
The private original server DLL stays on the owner's machine.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# (C++ member, descriptor VA, string VA, Source FieldType ordinal,
#  original object-relative member offset, flags, external KV name VA)
# Original 2006 32-bit Source typedescription_t is 0x34 bytes on these witnesses.
MEMBERS = (
    ('m_iNumGrenades', 0x225351CC, 0x22491030, 5, 0xC38, 6, 0x22491024),
    ('m_flGrenadeHeight', 0x225353A0, 0x22490F9C, 1, 0xC70, 6, 0x22490F8C),
    ('m_flShotDelay', 0x22535198, 0x22491040, 1, 0xC9C, 2, 0),
    ('m_nShots', 0x22535164, 0x22491050, 5, 0xC98, 2, 0),
    ('m_flNextAltFireTime', 0x225350FC, 0x22491070, 16, 0xC90, 2, 0),
    ('m_vecAltFireTarget', 0x22535644, 0x22490EC0, 3, 0xFCC, 2, 0),
    ('m_iLastAnimEventHandled', 0x22535610, 0x22490ED4, 5, 0xFC8, 2, 0),
    ('m_bCanCrouch', 0x22534F5C, 0x22491108, 6, 0xC66, 2, 0),
    ('m_nKickDamage', 0x22534E8C, 0x22491124, 5, 0xC54, 2, 0),
    # Remaining 24 descriptor entries recovered from the contiguous 0x34-byte table.
    ('m_vecTossVelocity', 0x22534EC0, 0x22477BC0, 3, 0xC58, 2, 0),
    ('m_bStanding', 0x22534EF4, 0x22491118, 6, 0xC64, 2, 0),
    ('m_bShouldPatrol', 0x22534F28, 0x22485CA4, 6, 0xC65, 2, 0),
    ('m_bFirstEncounter', 0x22534F90, 0x224910F4, 6, 0xC67, 2, 0),
    ('m_flNextPainSoundTime', 0x22534FC4, 0x224910DC, 16, 0xC78, 2, 0),
    ('m_flNextAlertSoundTime', 0x22534FF8, 0x224910C4, 16, 0xC7C, 2, 0),
    ('m_flNextGrenadeCheck', 0x2253502C, 0x22477BA8, 16, 0xC80, 2, 0),
    ('m_flAltFireCheck', 0x22535060, 0x224910B0, 16, 0xC84, 2, 0),
    ('m_flNextLostSoundTime', 0x22535094, 0x22491098, 16, 0xC88, 2, 0),
    ('m_flAlertPatrolTime', 0x225350C8, 0x22491084, 16, 0xC8C, 2, 0),
    ('m_flNextCoughTime', 0x22535130, 0x2249105C, 16, 0xC94, 2, 0),
    ('m_Sentences', 0x22535200, 0x22491018, 10, 0xCC0, 2, 0),
    ('m_bWounded', 0x22535234, 0x2249100C, 6, 0xCA0, 2, 0),
    ('m_pWounded', 0x22535268, 0x22491000, 12, 0xCA4, 2, 0),
    ('m_pHelper', 0x2253529C, 0x22490FF4, 12, 0xCA8, 2, 0),
    ('m_fKneeDamage', 0x225352D0, 0x22490FE4, 1, 0xCAC, 2, 0),
    ('m_bHaveSat', 0x22535304, 0x22490FD8, 6, 0xCB0, 2, 0),
    ('m_bGrenadeHappy', 0x22535338, 0x22490FC8, 6, 0xC6C, 2, 0),
    ('m_shouldPlayDeathSound', 0x2253536C, 0x22490FB0, 6, 0xCB1, 2, 0),
    ('m_hPhysicsEnt', 0x225353D4, 0x22490F7C, 13, 0xCB4, 2, 0),
    ('m_hCurrentGrenade', 0x22535408, 0x22490F68, 13, 0xCB8, 2, 0),
    ('m_hTargetBarrel', 0x2253543C, 0x22490F58, 13, 0xCBC, 2, 0),
    ('m_hIgnitor', 0x22535470, 0x22490F4C, 13, 0xC3C, 2, 0),
    ('m_szFlameInflictor', 0x225354A4, 0x2241221C, 2, 0xC40, 2, 0),
)

# (handler field name, descriptor VA, field name string VA, type ordinal,
#  external Hammer I/O name, external name VA, original member-function VA)
INPUTS = (
    ('InputAssault', 0x225355A8, 0x22490F10, 2, 'Assault',
     0x223F5624, 0x22357E50),
    ('InputThrowGrenades', 0x225355DC, 0x22490EFC, 6, 'ThrowGrenades',
     0x22490EEC, 0x22357EA0),
    ('InputLookOn', 0x2253550C, 0x22490F28, 0, 'LookOn',
     0x22490F20, 0x2235DDC0),
    ('InputLookOff', 0x225354D8, 0x22490F3C, 0, 'LookOff',
     0x22490F34, 0x2235F260),
    ('InputStartPatrolling', 0x22535540, 0x22485CB4, 0, 'StartPatrolling',
     0x22477C70, 0x22357E30),
    ('InputStopPatrolling', 0x22535574, 0x22485CC4, 0, 'StopPatrolling',
     0x22477C80, 0x22357E40),
)


def check_label(pe: PEImage, address: int, expected: str) -> None:
    encoded = expected.encode('ascii') + b'\0'
    if pe.at(address, len(encoded)) != encoded:
        raise VerificationError(
            f'SiN NPC datamap string mismatch at 0x{address:08X}: {expected}')


def read_descriptor(pe: PEImage, desc: int) -> dict:
    raw = pe.at(desc, 52)
    dtype, field_name_va, offset0, offset1, size_flags, external_name_va, \
        _reserved, handler_va = struct.unpack_from('<8I', raw)
    return dict(type_id=dtype, name_va=field_name_va,
                offset=offset0, secondary_offset=offset1,
                size=size_flags & 0xFFFF, flags=size_flags >> 16,
                external_name_va=external_name_va, input_handler_va=handler_va)


def audit_datamap(pe: PEImage) -> dict:
    fields, inputs = [], []
    for name, desc, string_va, dtype, offset, flags, external_va in MEMBERS:
        d = read_descriptor(pe, desc)
        if not (d['type_id'] == dtype and d['name_va'] == string_va and
                d['offset'] == offset and d['secondary_offset'] == 0 and
                d['size'] == 1 and d['flags'] == flags and
                d['external_name_va'] == external_va):
            raise VerificationError(f'{name}: original MobGrunt datamap differs')
        check_label(pe, string_va, name)
        external = None
        if external_va:
            external = {0x22491024: 'NumGrenades',
                        0x22490F8C: 'GrenadeHeight'}[external_va]
            check_label(pe, external_va, external)
        fields.append(dict(member=name, offset_hex=f'0x{offset:X}',
                           descriptor_va=f'0x{desc:08X}', field_type_id=dtype,
                           flags=flags, hammer_key=external))

    for label, desc, string_va, dtype, input_name, external_va, handler in INPUTS:
        d = read_descriptor(pe, desc)
        if not (d['type_id'] == dtype and d['name_va'] == string_va and
                d['offset'] == 0 and d['size'] == 1 and d['flags'] == 8 and
                d['external_name_va'] == external_va and
                d['input_handler_va'] == handler):
            raise VerificationError(f'{label}: original input dispatch datamap differs')
        check_label(pe, string_va, label)
        check_label(pe, external_va, input_name)
        inputs.append(dict(member_handler=label, external_input=input_name,
                           descriptor_va=f'0x{desc:08X}',
                           function_va=f'0x{handler:08X}', field_type_id=dtype))
    return dict(
        original_server_class='CNPC_MobGrunt',
        original_npc_classname='npc_druglab_grunt_assault_rifle',
        member_descriptors=fields,
        input_descriptors=inputs,
        limitations=[
            'These are original x86 datamap member offsets for a specific 2006 build.',
            'Offsets are not transferable to another compiler or 64-bit ARM64 class.',
            'Field type ordinals/flag values are original binary values; more semantics require matching SDK headers.',
            'The Assault input has a known original handler, but full AI schedule behavior is not yet recovered.',
        ],
    )


def audit_handler_code(pe: PEImage) -> dict:
    # Actual InputAssault: push state 3 and call a handler on object+0xDB8.
    if pe.at(0x22357E50, 3) != b'\x56\x8b\xf1':
        raise VerificationError('InputAssault handler prologue differs')
    if pe.at(0x22357E82, 2) != b'\x6a\x03':
        raise VerificationError('InputAssault state argument differs')
    if pe.at(0x22357E85, 6) != b'\x8d\x8e\xb8\x0d\x00\x00':
        raise VerificationError('InputAssault state object offset differs')
    call = pe.at(0x22357E8B, 5)
    if call[0] != 0xE8 or 0x22357E90 + struct.unpack_from('<i', call, 1)[0] != 0x220CFC00:
        raise VerificationError('InputAssault state call target differs')
    # InputAssault also receives a Source input variant. If variant type is
    # 2 (string), it uses that pointer or a shared empty string; otherwise
    # invokes the original string conversion helper. The value is then
    # interned before dispatch to the assault behavior object at +0xDB8.
    if pe.at(0x22357E5A, 4) != b'\x83\x79\x10\x02':
        raise VerificationError('InputAssault variant string type branch differs')
    if pe.at(0x22357E66, 5) != b'\xb8\xa8\x47\x3d\x22':
        raise VerificationError('InputAssault empty string fallback differs')
    for va, target in ((0x22357E6D, 0x221561D0),
                       (0x22357E78, 0x221F3DE0)):
        code = pe.at(va, 5)
        if code[0] != 0xE8 or va + 5 + struct.unpack_from('<i', code, 1)[0] != target:
            raise VerificationError('InputAssault string conversion/intern edge differs')
    # InputThrowGrenades: reset m_flGrenadeHeight at +0xC70,
    # set 0xC6C byte from accepted bool value or reset on wrong input type.
    if pe.at(0x22357EA4, 4) != b'\x83\x78\x18\x06':
        raise VerificationError('InputThrowGrenades value type check differs')
    if pe.at(0x22357EAD, 10) != b'\xc7\x81\x70\x0c\x00\x00\x00\x00\x00\x00':
        raise VerificationError('InputThrowGrenades height reset differs')
    if pe.at(0x22357EB7, 6) != b'\x88\x81\x6c\x0c\x00\x00':
        raise VerificationError('InputThrowGrenades bool write differs')
    return dict(
        assault_input=dict(handler_va='0x22357E50',
                           relative_state_object_offset='0xDB8',
                           submitted_state_argument=3,
                           target_call_va='0x220CFC00',
                           string_variant_type_id=2,
                           empty_string_fallback_va='0x223D47A8',
                           non_string_conversion_va='0x221561D0',
                           string_pool_intern_va='0x221F3DE0'),
        grenade_input=dict(handler_va='0x22357EA0',
                           expected_variant_type_id=6,
                           reset_grenade_height_offset='0xC70',
                           bool_field_offset='0xC6C'),
        limitations=[
            'The state argument 3 is observed, but its AI meaning is not yet decoded.',
            'The boolean field at +0xC6C must be linked to its actual name and update cycle.',
        ],
    )


def audit_look_target_flag(pe: PEImage) -> dict:
    """The two real map input handlers toggle a Source entity flag bit.

    On clears 0x10000, Off sets it. The class object's +0xB4 field is a
    flags word in this original x86 build. Source singleplayer variants
    commonly name bit 0x10000 FL_NOTARGET; confirm the branch before
    assigning that symbol to this exact executable.
    """
    evidence = (
        (0x2235DDC0, b'\x8b\x91\xb4\x00\x00\x00', 'LookOn flags read'),
        (0x2235DDC8, b'\x25\xff\xff\xfe\xff', 'LookOn bit clear'),
        (0x2235DDD1, b'\x80\xb9\xf0\x01\x00\x00\x00', 'LookOn notify conditional'),
        (0x2235DDDA, b'\xc6\x81\x00\x02\x00\x00\x01', 'LookOn deferred dirty flag'),
        (0x2235DDF4, b'\x83\x0a\x01', 'LookOn change notifier'),
        (0x2235F260, b'\x8b\x91\xb4\x00\x00\x00', 'LookOff flags read'),
        (0x2235F268, b'\x0d\x00\x00\x01\x00', 'LookOff bit set'),
        (0x2235F271, b'\x80\xb9\xf0\x01\x00\x00\x00', 'LookOff notify conditional'),
        (0x2235F27A, b'\xc6\x81\x00\x02\x00\x00\x01', 'LookOff deferred dirty flag'),
        (0x2235F294, b'\x83\x0a\x01', 'LookOff change notifier'),
    )
    for va, expected, desc in evidence:
        if pe.at(va, len(expected)) != expected:
            raise VerificationError(f'original MobGrunt {desc} changed at 0x{va:08X}')
    return dict(
        handlers=dict(LookOn='0x2235DDC0', LookOff='0x2235F260'),
        original_flags_field_offset='0xB4',
        bit_mask='0x00010000',
        look_on_operation='flags &= ~0x00010000',
        look_off_operation='flags |= 0x00010000',
        change_notification_observed=True,
        meaning='Likely AI targetability/no-target switch, not a look animation',
        semantic_caveat='Exact Source branch flag naming must be verified separately',
    )


def audit(server: bytes, *, hash_check: bool = True) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    if hash_check and digest != SERVER_SHA256:
        raise VerificationError('expected original 2006 SiN server.dll SHA-256')
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError('unexpected SiN server PE image base')
    return dict(dll_sha256=digest,
                mobgrunt_datamap=audit_datamap(pe),
                original_input_handlers=audit_handler_code(pe),
                look_on_off_flag_behavior=audit_look_target_flag(pe))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        info = audit(args.server.read_bytes())
        payload = json.dumps(info, indent=2) + '\n'
        if args.output:
            with args.output.open('x', encoding='utf8') as output:
                output.write(payload)
        else:
            print(payload, end='')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(2, f'Original MobGrunt datamap audit failed: {exc}\n')


if __name__ == '__main__':
    raise SystemExit(main())
