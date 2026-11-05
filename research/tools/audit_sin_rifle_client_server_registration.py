#!/usr/bin/env python3
"""Verify original SiN Episodes rifle client/server code links (read-only PE32).

Does not execute, extract or redistribute proprietary DLL machine code.
Original DLLs are supplied as local paths and must match their pinned hashes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, PEImage, VerificationError,
)

SERVER_REGISTRATIONS = (
    ('weapon_assault_rifle', 0x223C4F60, 0x22477C30, 0x2252D4FC),
    ('grenade_ar', 0x223C4FC0, 0x22477230, 0x2252D500),
    ('npc_druglab_grunt_assault_rifle', 0x223C7020, 0x22490EA0, 0x22536848),
    ('npc_druglab_grunt_elite', 0x223C6940, 0x2248E760, 0x225346A0),
)


def require(ok: bool, message: str) -> None:
    if not ok:
        raise VerificationError(message)


def call_target(va: int, encoded: bytes) -> int:
    if len(encoded) != 5 or encoded[0] != 0xE8:
        raise VerificationError(f'expected direct x86 call at 0x{va:08X}')
    return (va + 5 + struct.unpack_from('<i', encoded, 1)[0]) & 0xFFFFFFFF


def original_server_registration(pe: PEImage, name: str, entry: int,
                                 name_va: int, factory_va: int) -> dict:
    code = pe.at(entry, 22)
    require(code[0] == 0xE8 and code[5:8] == b'\x8b\x10\x68' and
            code[12] == 0x68 and code[17:] == b'\x8b\xc8\xff\x12\xc3',
            f'{name}: server registration thunk shape mismatch')
    require(call_target(entry, code[:5]) == 0x2228B110,
            f'{name}: server registry getter mismatch')
    require(struct.unpack_from('<I', code, 8)[0] == name_va and
            struct.unpack_from('<I', code, 13)[0] == factory_va,
            f'{name}: server classname/factory argument mismatch')
    require(pe.at(name_va, len(name)+1) == name.encode('ascii') + b'\0',
            f'{name}: original server classname mismatch')
    return {'classname': name, 'registration_va': f'0x{entry:08X}',
            'factory_object_va': f'0x{factory_va:08X}',
            'classname_va': f'0x{name_va:08X}',
            'registry_getter_va': '0x2228B110'}


def original_client_registration(pe: PEImage) -> dict:
    entry = 0x24270B80
    code = pe.at(entry, 32)
    require(code[0] == 0xE8 and code[5:8] == b'\x8b\x10\x68' and
            code[12] == 0x68 and code[17] == 0x68 and code[22] == 0x68 and
            code[27:] == b'\x8b\xc8\xff\x12\xc3',
            'client weapon registration thunk shape mismatch')
    require(call_target(entry, code[:5]) == 0x24103BF0,
            'client registry getter mismatch')
    observed = tuple(struct.unpack_from('<I', code, ofs)[0]
                     for ofs in (8, 13, 18, 23))
    expected = (0x2422B5B0, 0x830, 0x242C2954, 0x2429B020)
    require(observed == expected, 'client registration arguments differ')
    require(pe.at(0x2429B020, len(b'weapon_assault_rifle\0')) ==
            b'weapon_assault_rifle\0', 'client classname mismatch')
    require(pe.at(0x242C2954, len(b'C_WeaponAssaultRifle\0')) ==
            b'C_WeaponAssaultRifle\0',
            'client registration class-name argument mismatch')
    require(pe.at(0x24320A60, len(b'.?AVC_WeaponAssaultRifle@@\0')) ==
            b'.?AVC_WeaponAssaultRifle@@\0', 'client C_WeaponAssaultRifle RTTI mismatch')
    # The registration argument at 0x242C2954 is C_WeaponAssaultRifle,
    # NOT the RecvTable. Distinct DT_WeaponAssaultRifle is constructed below.
    require(pe.at(0x242C298C, len(b'DT_WeaponAssaultRifle\0')) ==
            b'DT_WeaponAssaultRifle\0', 'missing genuine weapon RecvTable label')
    require(pe.at(0x2422A6A8, 5) == b'\x68\x8c\x29\x2c\x24' and
            pe.at(0x2422A6AD, 2) == b'\x6a\x01' and
            pe.at(0x2422A6AF, 5) == b'\x68\x34\xf0\x37\x24' and
            pe.at(0x2422A6B4, 5) == b'\xb9\x98\xf5\x37\x24',
            'original client DT_WeaponAssaultRifle construction differs')
    require(call_target(0x2422A6B9, pe.at(0x2422A6B9, 5)) == 0x24126A30,
            'original client receive table constructor call differs')
    require(pe.at(0x2427643C, len(b'baseclass\0')) == b'baseclass\0',
            'original client receive-table baseclass label missing')
    require(pe.at(0x2422A696, 5) == b'\x68\x3c\x64\x27\x24',
            'baseclass receive property reference missing')
    return {'classname': 'weapon_assault_rifle', 'registration_va': f'0x{entry:08X}',
            'registry_getter_va': '0x24103BF0',
            'registration_callback_va': '0x2422B5B0',
            'size_argument': 0x830,
            'client_class_name_va': '0x242C2954',
            'client_class_name': 'C_WeaponAssaultRifle',
            'recv_table_name': 'DT_WeaponAssaultRifle',
            'recv_table_name_va': '0x242C298C',
            'recv_table_constructor_va': '0x2422A660',
            'recv_table_constructor_call_va': '0x24126A30',
            'recv_table_declared_direct_props': 1,
            'direct_prop_name_observed': 'baseclass',
            'rtti_class': 'C_WeaponAssaultRifle'}


def original_projectile_virtual_path(pe: PEImage) -> dict:
    # MSVC RTTI Complete Object Locator at vtable[-1], TypeDescriptor at COL+12.
    require(pe.at(0x224776A8, 4) == struct.pack('<I', 0x224CF3A8),
            'CWeaponAssaultRifle vtable locator mismatch')
    require(pe.at(0x224CF3B4, 4) == struct.pack('<I', 0x2252D58C),
            'CWeaponAssaultRifle RTTI descriptor pointer mismatch')
    require(pe.at(0x2252D594, len(b'.?AVCWeaponAssaultRifle@@\0')) ==
            b'.?AVCWeaponAssaultRifle@@\0', 'server rifle RTTI name mismatch')
    require(pe.at(0x22313C18, 6) == b'\xc7\x06\xac\x76\x47\x22',
            'constructor vtable install mismatch')
    require(struct.unpack('<I', pe.at(0x224776AC + 0x388, 4))[0] == 0x2230BA30,
            'neighbor rifle virtual method changed')
    require(struct.unpack('<I', pe.at(0x224776AC + 0x38C, 4))[0] == 0x22313C60,
            'grenade launch function not present in original weapon vtable')
    require(pe.at(0x22313E25, 5) == b'\x68\x30\x72\x47\x22',
            'no verified grenade_ar classname push in rifle method')
    require(call_target(0x22313E2A, pe.at(0x22313E2A, 5)) == 0x22146C40,
            'projectile spawn helper mismatch')
    require(call_target(0x22146C48, pe.at(0x22146C48, 5)) == 0x22266B00,
            'spawn helper CreateEntityByName call mismatch')
    require(pe.at(0x2245F43C, 7) == b'ARNADE\0' and
            pe.at(0x22313E5A, 5) == b'\x68\x3c\xf4\x45\x22',
            'ARNADE literal or weapon code reference mismatch')
    for call_va, target in ((0x22313FBB, 0x22173260),
                            (0x22313FDE, 0x221732A0)):
        require(call_target(call_va, pe.at(call_va, 5)) == target,
                'original weapon cooldown/state call edge differs')
    nums = {}
    for address, value in ((0x224771F0, 2000.0), (0x223E97F0, 1000.0),
                           (0x223D9778, 0.5), (0x223D22F8, 1.0)):
        found, = struct.unpack('<f', pe.at(address, 4))
        require(found == value, f'original float literal mismatch at 0x{address:08X}')
        nums[f'0x{address:08X}'] = found
    return {
        'server_class': 'CWeaponAssaultRifle',
        'vtable_va': '0x224776AC', 'rtti_col_va': '0x224CF3A8',
        'neighbor_virtual_offset': '0x388', 'neighbor_virtual_va': '0x2230BA30',
        'projectile_launch_virtual_offset': '0x38C', 'projectile_launch_va': '0x22313C60',
        'projectile_spawn_helper_va': '0x22146C40',
        'CreateEntityByName_va': '0x22266B00', 'projectile_classname': 'grenade_ar',
        'secondary_ammo_marker': 'ARNADE',
        'original_code_float_literals': nums,
        'weapon_state_call_sites': ['0x22313FBB', '0x22313FDE'],
    }


def original_primary_shared_fire_path(pe: PEImage) -> dict:
    """Trace rifle vtable -> shared firing state machine -> bullet/clip path."""
    require(struct.unpack('<I', pe.at(0x224776AC + 0x388, 4))[0] == 0x2230BA30,
            'rifle shared primary dispatch vtable slot mismatch')
    # The weapon-specific 0x3AC virtual returns a constant 0.075 float.
    require(struct.unpack('<I', pe.at(0x224776AC + 0x3AC, 4))[0] == 0x22313850,
            'rifle fire interval virtual slot mismatch')
    require(pe.at(0x22313850, 7) == b'\xd9\x05\x04\x72\x47\x22\xc3',
            'rifle interval method shape differs')
    fire_interval, = struct.unpack('<f', pe.at(0x22477204, 4))
    require(abs(fire_interval - 0.075) < 1e-6,
            'original rifle fire interval constant differs')
    require(pe.at(0x2230B4C8, 6) == b'\xff\x90\xac\x03\x00\x00',
            'shared firing routine lacks interval virtual dispatch')
    require(call_target(0x2230BA7D, pe.at(0x2230BA7D, 5)) == 0x2230B430,
            'single-fire branch no longer calls shared shot routine')
    require(pe.at(0x2230BA76, 5) == b'\xe9\x75\xfd\xff\xff',
            'shared fire mode/burst branch changed')
    require(call_target(0x2230B84E, pe.at(0x2230B84E, 5)) == 0x2230B430,
            'multi-shot path no longer calls shared shot routine')
    require(pe.at(0x2230B584, 6) == b'\x8b\x86\xec\x04\x00\x00',
            'shared shot routine no longer reads clip at +0x4EC')
    require(call_target(0x2230B59F, pe.at(0x2230B59F, 5)) == 0x222A8210,
            'shared bullet count to clip subtract helper call differs')
    require(pe.at(0x222A821B, 2) == b'\x2b\x0e',
            'clip helper no longer subtracts count')
    return {
        'rifle_fire_mode_dispatch_vtable_offset': '0x388',
        'rifle_fire_mode_dispatch_va': '0x2230BA30',
        'weapon_specific_interval_vtable_offset': '0x3AC',
        'weapon_specific_interval_va': '0x22313850',
        'verified_interval_seconds': fire_interval,
        'shared_shot_routine_va': '0x2230B430',
        'single_branch_call_va': '0x2230BA7D',
        'burst_branch_call_va': '0x2230B84E',
        'shot_count_subtract_call_va': '0x2230B59F',
        'clip_integer_field_offset': '0x4EC',
        'subtract_helper_va': '0x222A8210',
        'interpretation_limits': [
            'The +0x4EC field is used as a magazine/clip counter by the shared shot routine.',
            'The +0x584 field is a separate burst/queued-shot counter; do not confuse it with clip ammo.',
            '0.075 seconds is the verified rifle virtual fire interval; mode and timer branch semantics require further work.',
            'This remains Windows x86 code and is not a Quest runtime implementation.'
        ],
    }


def audit(server: bytes, client: bytes, verify_hash: bool = True) -> dict:
    server_sha, client_sha = (hashlib.sha256(server).hexdigest(),
                              hashlib.sha256(client).hexdigest())
    if verify_hash:
        require(server_sha == SERVER_SHA256 and client_sha == CLIENT_SHA256,
                'original 2006 server/client DLL hashes differ')
    sp, cp = PEImage(server), PEImage(client)
    require(sp.base == 0x22000000 and cp.base == 0x24000000,
            'unexpected original DLL load image base')
    return {
        'server_sha256': server_sha,
        'client_sha256': client_sha,
        'hash_verified': verify_hash,
        'server_entity_factories': [
            original_server_registration(sp, *args) for args in SERVER_REGISTRATIONS
        ],
        'client_class_registration': original_client_registration(cp),
        'server_weapon_virtual_projectile_path': original_projectile_virtual_path(sp),
        'server_primary_shared_fire_path': original_primary_shared_fire_path(sp),
        'limitations': [
            'These are original x86 instruction and RTTI witnesses, not recovered C++ source.',
            'The client registration arg 0x242C2954 is a class-name string, not a receive-table pointer.',
            'A distinct DT_WeaponAssaultRifle receive table is registered with 1 direct baseclass prop; inherited field layout remains to decode.',
            'The verified 1000 and 2000 multipliers occur in branch-specific projectile calculations; units and branch conditions need further analysis.',
            'The verified 0.5 and 1.0 constants occur in state/timing paths; exact firing cooldown semantics need further analysis.',
            'Automatic primary fire, NPC attack behavior, prediction and networking still need native reconstruction.',
            'No original binary data or executable instructions are emitted.'
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server', type=Path, required=True)
    p.add_argument('--client', type=Path, required=True)
    p.add_argument('--output', type=Path, default=None)
    a = p.parse_args()
    try:
        result = audit(a.server.read_bytes(), a.client.read_bytes())
        payload = json.dumps(result, indent=2) + '\n'
        if a.output:
            with a.output.open('x', encoding='utf-8') as f:
                f.write(payload)
            print('Verified original client/server rifle and grenade class registration')
        else:
            print(payload, end='')
        return 0
    except (OSError, ValueError, struct.error) as error:
        p.exit(2, f'Original rifle decomp audit failed: {error}\n')


if __name__ == '__main__':
    raise SystemExit(main())
