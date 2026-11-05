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
