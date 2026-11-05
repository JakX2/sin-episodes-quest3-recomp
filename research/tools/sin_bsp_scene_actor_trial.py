#!/usr/bin/env python3
"""Safely produce an experimental *copy* of a SiN Source BSP with actor proxies.

This is a temporary stock-HL2 scene test, not a restoration of Ritual classes.
Never rewrites the input, never redistributes the game map. Usage:
  python sin_bsp_scene_actor_trial.py map.bsp --report
  python sin_bsp_scene_actor_trial.py map.bsp --write map_actor_trial.bsp
  python sin_bsp_scene_actor_trial.py map.bsp --verify map_actor_trial.bsp
  python sin_bsp_scene_actor_trial.py map.bsp --generic-target heli_name --write heli_trial.bsp
  python sin_bsp_scene_actor_trial.py map.bsp --pickup-proxies --write pickup_trial.bsp
  python sin_bsp_scene_actor_trial.py map.bsp --grunt-target roof_grunt_1_seq1 --write grunt_trial.bsp

Use only on lawfully obtained local BSPs; test the output in a separate mod
folder, with the correct original assets and scene cache mounted.
"""
import argparse
import pathlib
import re
import struct
import sys

HEADER = 8 + 64 * 16 + 4
ALIASES = {
    "npc_jc": "npc_citizen",        # tentative Source actor substitute
    "npc_jessica": "npc_alyx",       # tentative companion substitute
}
GENERIC = {"npc_generic_actor": "npc_citizen"}
# Temporary stock-HL2 pickup equivalents, NOT Ritual weapon implementations.
# Opt-in only: effects, ammo counts, fire modes and I/O may differ from SiN.
PICKUP_PROXIES = {
    "weapon_magnum": "weapon_357",
    "weapon_assault_rifle": "weapon_ar2",
    "weapon_scattergun": "weapon_shotgun",
    "weapon_grenade": "weapon_frag",
    "item_ammo_magnum": "item_ammo_357",
}
CLASSNAME_RE = re.compile(rb'(?m)(^\s*"classname"\s*")([^"\r\n]+)(")')
ENTITY_BLOCK_RE = re.compile(rb'\{[^{}]*\}')
KV_PAIR_RE = re.compile(rb'"([^"\r\n]+)"\s*"([^"\r\n]*)"')



class BspError(ValueError):
    pass


def read_bsp(blob: bytes):
    if len(blob) < HEADER or blob[:4] != b'VBSP':
        raise BspError("Not a valid little-endian VBSP header")
    version = struct.unpack_from('<i', blob, 4)[0]
    if not 17 <= version <= 22:
        raise BspError(f"Unsupported BSP version {version} (expected 17..22)")
    ofs, size, lumpversion, fourcc = struct.unpack_from('<iii4s', blob, 8)
    if ofs < HEADER or size <= 0 or ofs + size > len(blob):
        raise BspError("Invalid/overlapping entity-lump offset or length")
    if fourcc != b'\0\0\0\0':
        raise BspError("Compressed entity lumps are unsupported: refusing to guess")
    data = blob[ofs:ofs+size]
    if not data.endswith(b'\0'):
        raise BspError("Original entity lump lacks terminator")
    if b'{' not in data or b'"classname"' not in data:
        raise BspError("No standard entity text in lump 0")
    return version, ofs, size, lumpversion, data


def patch_targeted_generic_actor(data: bytes, name: str):
    """Replace one named SiN generic actor with Source's generic_actor.

    Scope is a single entity's classname, not a global classname replace.
    Fail closed if the name is absent, ambiguous, or belongs to another class.
    """
    if not name or not name.isascii() or any(ch in name for ch in '"{}\\'):
        raise BspError('Invalid --generic-target; expected a plain ASCII targetname')

    wanted = name.encode('ascii').lower()
    matches = []

    for match in ENTITY_BLOCK_RE.finditer(data):
        block = match.group(0)
        pairs = [(key.lower(), value) for key, value in KV_PAIR_RE.findall(block)]
        names = [value for key, value in pairs if key == b'targetname']
        if any(value.lower() == wanted for value in names):
            classes = [value.lower() for key, value in pairs if key == b'classname']
            if names != [name.encode('ascii')] and not (
                len(names) == 1 and names[0].lower() == wanted
            ):
                raise BspError('Ambiguous or duplicate targetname keys')
            if classes != [b'npc_generic_actor']:
                raise BspError(f'Target {name} must be exactly one npc_generic_actor')
            result, count = CLASSNAME_RE.subn(
                lambda m: m.group(1) + b'generic_actor' + m.group(3), block
            )
            if count != 1:
                raise BspError(f'Could not rewrite classname for target {name}')
            matches.append((match.span(), result))

    if len(matches) != 1:
        raise BspError(f'Expected exactly one npc_generic_actor named {name}; found {len(matches)}')
    (start, stop), replacement = matches[0]
    return data[:start] + replacement + data[stop:]



def patch_targeted_grunt(data: bytes, name: str, override_equipment=False):
    """Replace EXACTLY one uniquely named SiN grunt with armed HL2 npc_combine_s.

    Preserve original keys including targetname, squadname, and spawnflags.
    Add AR2 equipment only when absent. Fail closed on duplicate targetnames,
    unexpected classes, or incompatible existing equipment. This is a desktop
    test proxy, not a faithful Ritual enemy implementation.
    """
    if not name or not name.isascii() or any(ch in name for ch in '"{}\\'):
        raise BspError('Invalid --grunt-target; expected a plain ASCII targetname')

    wanted = name.encode('ascii').lower()
    matches = []
    for match in ENTITY_BLOCK_RE.finditer(data):
        block = match.group(0)
        pairs = [(key.lower(), value) for key, value in KV_PAIR_RE.findall(block)]
        names = [v for k, v in pairs if k == b'targetname']
        if not any(v.lower() == wanted for v in names):
            continue

        classes = [v.lower() for k, v in pairs if k == b'classname']
        if len(names) != 1 or classes not in (
            [b'npc_druglab_grunt_assault_rifle'], [b'npc_druglab_grunt_elite']
        ):
            raise BspError(f'Target {name} must be exactly one supported SiN grunt')

        equipment = [v for k, v in pairs if k == b'additionalequipment']
        if len(equipment) > 1:
            raise BspError(f'Target {name} has duplicate equipment keys')
        if equipment and equipment[0].lower() != b'weapon_ar2' and not override_equipment:
            actual = equipment[0].decode('utf-8', errors='replace')
            raise BspError(
                f'Target {name} already has equipment {actual!r}; '
                'pass --grunt-equipment-override for the experimental copy'
            )

        changed, count = CLASSNAME_RE.subn(
            lambda m: m.group(1) + b'npc_combine_s' + m.group(3), block
        )
        if count != 1:
            raise BspError(f'Could not rewrite classname for target {name}')
        if not equipment:
            changed = changed[:-1] + b'\n"additionalequipment" "weapon_ar2"\n}'
        elif equipment[0].lower() != b'weapon_ar2':
            changed, replaced = re.subn(
                rb'("additionalequipment"\s*")[^"\r\n]+(")',
                lambda m: m.group(1) + b'weapon_ar2' + m.group(2),
                changed, count=1, flags=re.I
            )
            if replaced != 1:
                raise BspError(f'Cannot rewrite equipment for {name}')
        matches.append((match.span(), changed))

    if len(matches) != 1:
        raise BspError(f'Expected exactly one uniquely named SiN grunt {name}; found {len(matches)}')
    (start, stop), replacement = matches[0]
    return data[:start] + replacement + data[stop:]

def patch_entities(data: bytes, include_generic=False, generic_targets=(), pickup_proxies=False, grunt_targets=(), grunt_equipment_override=False):
    replacements = dict(ALIASES)
    if include_generic:
        replacements.update(GENERIC)
    if pickup_proxies:
        replacements.update(PICKUP_PROXIES)
    count = {k: 0 for k in replacements}
    def sub(m):
        name = m.group(2).decode('ascii', errors='ignore')
        if name not in replacements:
            return m.group(0)
        count[name] += 1
        return m.group(1) + replacements[name].encode() + m.group(3)
    patched = CLASSNAME_RE.sub(sub, data)
    if include_generic and generic_targets:
        raise BspError('--include-generic cannot be combined with --generic-target')
    for target in generic_targets:
        patched = patch_targeted_generic_actor(patched, target)
        count[f'npc_generic_actor:{target}'] = 1
    for target in grunt_targets:
        patched = patch_targeted_grunt(patched, target, override_equipment=grunt_equipment_override)
        count[f'grunt:{target}'] = 1
    return patched, count


def make_trial(blob: bytes, include_generic=False, generic_targets=(), pickup_proxies=False, grunt_targets=(), grunt_equipment_override=False):
    version, offset, size, lumpversion, data = read_bsp(blob)
    changed, counts = patch_entities(data, include_generic, generic_targets, pickup_proxies, grunt_targets, grunt_equipment_override)
    if not any(counts.values()):
        return None, counts
    # Appending a replacement lump avoids relocating any existing maps/lighting
    # lumps or recompressing unrecognized BSP payloads. Old lump is orphaned.
    padding = (-len(blob)) % 4
    position = len(blob) + padding
    out = bytearray(blob)
    out.extend(b'\0' * padding)
    out.extend(changed)
    struct.pack_into('<iii4s', out, 8, position, len(changed), lumpversion, b'\0'*4)
    # Structural safety checks only. Real Source-engine acceptance is untested.
    _, off2, size2, _, readback = read_bsp(bytes(out))
    assert off2 == position and size2 == len(changed) and readback == changed
    assert blob[8+16:HEADER] == out[8+16:HEADER]  # all non-entity lump headers and revision
    assert out[offset:offset+size] == data       # original entity payload untouched
    return bytes(out), counts


def main(argv=None):
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('bsp',type=pathlib.Path)
    action=ap.add_mutually_exclusive_group(required=True)
    action.add_argument('--report', action='store_true')
    action.add_argument('--write',type=pathlib.Path)
    action.add_argument('--verify',type=pathlib.Path,
                        help='check an existing trial copy byte-for-byte against the expected rewrite')
    ap.add_argument('--include-generic',action='store_true',help='also try ALL npc_generic_actor -> npc_citizen (old experiment)')
    ap.add_argument('--pickup-proxies',action='store_true',
                    help='opt-in SiN weapon and ammo pickups -> stock HL2 counterparts; not original mechanics')
    ap.add_argument('--grunt-equipment-override',action='store_true',
                    help='opt-in override selected SiN grunt equipment to weapon_ar2 in test COPY')
    ap.add_argument('--grunt-target',action='append',default=[],metavar='TARGETNAME',
                    help='only replace this uniquely named SiN grunt with armed npc_combine_s (repeatable)')
    ap.add_argument('--generic-target',action='append',default=[],metavar='TARGETNAME',
                    help='only rewrite this named npc_generic_actor to generic_actor (repeatable)')
    args=ap.parse_args(argv)
    source=args.bsp.resolve()
    try:
        data=source.read_bytes()
        if args.grunt_equipment_override and not args.grunt_target:
            raise BspError('--grunt-equipment-override requires --grunt-target')
        result, counts=make_trial(data,args.include_generic,args.generic_target,args.pickup_proxies,args.grunt_target,args.grunt_equipment_override)
        print(f"Source {source.name}: class rewrites {counts}")
        if result is None:
            if args.verify:
                raise BspError('No matching original actor classes; cannot verify a trial rewrite')
            print('No matching actor classnames; input unchanged')
            return 0
        if args.verify:
            target = args.verify.resolve()
            if target == source:
                raise BspError('Verification requires a separate experimental BSP copy')
            # Reconstruct the complete expected BSP from the untouched original.
            # This verifies the entity rewrite AND that all other bytes match.
            if target.read_bytes() != result:
                raise BspError('Trial BSP does not match the expected copy-only actor rewrite')
            print(f'VERIFIED experimental BSP copy: {target.name} (all {len(result)} bytes)')
            print('Structural verification only; HL2/SourceVR loading and scenes remain untested.')
            return 0
        if args.write:
            target=args.write.resolve()
            if target == source:
                raise BspError('Will not overwrite input BSP')
            # The --write request explicitly authorizes this destination. The
            # original input is never touched; parent directories may be new.
            target.parent.mkdir(parents=True, exist_ok=True)
            try:
                # Atomic existence check: no race between exists() and write().
                # 'xb' also refuses to follow an existing destination symlink.
                with target.open('xb') as output:
                    output.write(result)
            except FileExistsError:
                raise BspError('Will not overwrite existing output BSP')
            print(f"Created experimental map COPY: {target} ({len(result)} bytes)")
            print('Not validated against Source runtime; original SiN AI and scenes remain unresolved.')
        else:
            print('Dry-run only. Use --write OUTPUT.bsp to create an isolated copy.')
        return 0
    except (OSError, BspError) as e:
        print('Error:',e,file=sys.stderr)
        return 2


if __name__=='__main__':
    sys.exit(main())