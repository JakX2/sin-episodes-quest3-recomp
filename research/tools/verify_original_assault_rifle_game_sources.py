#!/usr/bin/env python3
"""Read-only original SDK, shipped game-script and 2006 DLL cross-check.

No original game file bytes are included in this repository. A validated
profile is evidence for a portable C++ config, not a Source/Quest engine gun.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile

DLL_SHA = {
    'client.dll': '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
    'server.dll': '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
}
REQUIRED_FIELDS = ('viewmodel', 'playermodel', 'anim_prefix', 'ZoomFOV',
                   'clip_size', 'clip2_size', 'default_clip', 'default_clip2',
                   'primary_ammo', 'secondary_ammo', 'weight', 'MeleeWeapon',
                   'kickMin', 'kickRampMin', 'kickRampMax', 'kickRampSecs',
                   'kickRampStartScale', 'kickRampEndScale')
SOUND_KEYS = ('reload_one', 'reload_two', 'reload_npc', 'empty', 'single_shot',
              'single_shot_npc', 'double_shot', 'melee_hit', 'melee_hit_world', 'melee_miss')


class VerificationError(ValueError):
    pass


def source_value(source: str, name: str) -> str:
    matches = re.findall(r'^\s*"' + re.escape(name) + r'"\s+"([^"\r\n]*)"', source, re.M)
    if len(matches) != 1:
        raise VerificationError(f'expected one original script key {name!r}; got {len(matches)}')
    return matches[0].strip()


def inspect(fgd: bytes, weapon: bytes, sounds: bytes, server: bytes,
            client: bytes, *, verify_hash: bool = True) -> dict:
    source = weapon.decode('latin1')
    fgd_text = fgd.decode('latin1')
    sound_text = sounds.decode('latin1')
    npc_match = re.search(
        r'@NPCClass\s+base\(BaseGrunt\)\s+studio\("(?P<model>[^"]+)"\)'
        r'\s*=\s*npc_druglab_grunt_assault_rifle\s*:\s*"[^"]*"\s*\[\s*'
        r'additionalequipment\(choices\)\s*:\s*"Weapons"\s*:\s*"(?P<weapon>[^"]+)"',
        fgd_text)
    if not npc_match or npc_match.group('weapon') != 'weapon_assault_rifle':
        raise VerificationError('original FGD does not define expected Druglab rifle grunt')
    values = {key: source_value(source, key) for key in REQUIRED_FIELDS}
    expected = dict(
        viewmodel='models/weapons/v_assault_rifle/v_assault_rifle.mdl',
        playermodel='models/weapons/w_assault_rifle/w_assault_rifle.mdl',
        ZoomFOV='40', clip_size='45', clip2_size='-1',
        default_clip='45', default_clip2='0', primary_ammo='AR',
        secondary_ammo='ARNADE', anim_prefix='smg2', MeleeWeapon='0')
    for key, value in expected.items():
        if values[key] != value:
            raise VerificationError(f'unexpected source value for {key}: {values[key]!r}')
    sounds_found = {}
    for key in SOUND_KEYS:
        name = source_value(source, key)
        if not re.search(r'^\s*"' + re.escape(name) + r'"\s*$', sound_text, re.M):
            raise VerificationError(f'original sound event missing: {name}')
        sounds_found[key] = name
    for name, data in (('client.dll', client), ('server.dll', server)):
        actual = hashlib.sha256(data).hexdigest()
        if verify_hash and actual != DLL_SHA[name]:
            raise VerificationError(f'{name} does not match pinned 2006 SHA-256')
        for marker in (b'CWeaponAssaultRifle', b'weapon_assault_rifle'):
            if marker not in data:
                raise VerificationError(f'{name} lacks original marker {marker!r}')
    if b'CEntityFactory@VCWeaponAssaultRifle' not in server:
        raise VerificationError('server.dll lacks original weapon factory RTTI')
    if b'grenade_ar\0' not in server or b'ARNADE\0' not in server:
        raise VerificationError('server.dll lacks original AR projectile/ammo markers')
    return {
        'sdk_npc_classname': 'npc_druglab_grunt_assault_rifle',
        'sdk_npc_model': npc_match.group('model'),
        'sdk_npc_default_equipment': npc_match.group('weapon'),
        'weapon_script_sha256': hashlib.sha256(weapon).hexdigest(),
        'weapon_script': values,
        'sound_event_references': sounds_found,
        'dlls': {
            name: {
                'sha256': hashlib.sha256(data).hexdigest(),
                'bytes': len(data),
                'pinned_hash_verified': verify_hash,
            }
            for name, data in (('client.dll', client), ('server.dll', server))
        },
        'original_secondary_projectile_classname': 'grenade_ar',
        'limitations': [
            'Static original sources: not native engine entity registration',
            'Viewmodel and NPC model existence requires local asset mount check',
            'Secondary projectile behavior requires separate native implementation',
        ],
    }


def load_sources(sdk_zip: Path, scripts_zip: Path, server: Path, client: Path) -> dict:
    with zipfile.ZipFile(sdk_zip) as z:
        fgd = z.read('SinEpisodesSDK/bin/SinEpisodes.fgd')
        sdk_weapon = z.read('SinEpisodesSDK/SE1/scripts/weapon_assault_rifle.txt')
    with zipfile.ZipFile(scripts_zip) as z:
        game_weapon = z.read('scripts/weapon_assault_rifle.txt')
        game_sounds = z.read('scripts/game_sounds_weapons.txt')
    if sdk_weapon != game_weapon:
        raise VerificationError('SDK and shipped game assault-rifle scripts differ')
    return inspect(fgd, game_weapon, game_sounds, server.read_bytes(), client.read_bytes())


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sdk-zip', required=True, type=Path)
    ap.add_argument('--scripts-zip', required=True, type=Path)
    ap.add_argument('--server', required=True, type=Path)
    ap.add_argument('--client', required=True, type=Path)
    ap.add_argument('--output', type=Path, help='optional NEW output JSON path, never overwrite')
    args = ap.parse_args(argv)
    try:
        info = load_sources(args.sdk_zip, args.scripts_zip, args.server, args.client)
        data = json.dumps(info, indent=2) + '\n'
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            with args.output.open('x', encoding='utf-8') as f:
                f.write(data)
            print('Validated original SiN sources; wrote evidence-only profile')
        else:
            print(data, end='')
    except (OSError, KeyError, ValueError, zipfile.BadZipFile) as exc:
        ap.exit(2, f'Source audit failed: {exc}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
