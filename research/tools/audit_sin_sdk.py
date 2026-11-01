#!/usr/bin/env python3
"""Private SDK ZIP audit: aggregate original WeaponData and Hammer VMF entities.

No SDK bytes or per-entity targets are emitted. This is NOT gameplay execution.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct
import sys
import zipfile
import zlib

ZIP_ROOT = 'SinEpisodesSDK/'
VPK_DIR = ZIP_ROOT + 'vpks/depot_1318_dir.vpk'
VPK_DATA = ZIP_ROOT + 'vpks/depot_1318_000.vpk'
MAP_PATHS = ('sesdk_content/mapsrc/se1_docks04.vmf',
             'sesdk_content/mapsrc/se1_highrise01.vmf',
             'sesdk_content/mapsrc/arena_office.vmf')
WEAPONS = ('weapon_magnum', 'weapon_assault_rifle', 'weapon_scattergun')
MAP_MAX = 32 * 1024 * 1024

class SDKFormatError(ValueError):
    pass


def read_vpk_directory(data):
    if len(data) < 28:
        raise SDKFormatError('truncated VPK directory')
    signature, version, tree_size = struct.unpack_from('<III', data)
    if signature != 0x55aa1234 or version != 2 or tree_size > len(data) - 28:
        raise SDKFormatError('expected VPK v2 directory')
    off = 28
    limit = 28 + tree_size
    result = {}

    def read_name():
        nonlocal off
        end = data.find(b'\x00', off, limit)
        if end < 0 or end - off > 512:
            raise SDKFormatError('invalid VPK tree entry name')
        name = data[off:end].decode('ascii')
        off = end + 1
        return name

    while True:
        extension = read_name()
        if not extension:
            break
        while True:
            folder = read_name()
            if not folder:
                break
            while True:
                basename = read_name()
                if not basename:
                    break
                if off + 18 > limit:
                    raise SDKFormatError('truncated VPK file record')
                crc, preload, archive, position, length, terminator = struct.unpack_from('<IHHIIH', data, off)
                off += 18
                if terminator != 0xffff or preload > limit - off:
                    raise SDKFormatError('corrupt VPK file record')
                header_data = data[off:off+preload]
                off += preload
                name = f'{folder}/{basename}.{extension}'.lower()
                if name in result:
                    raise SDKFormatError('duplicate VPK entry')
                result[name] = (crc, archive, position, length, header_data)
                if len(result) > 100000:
                    raise SDKFormatError('too many VPK entries')
    if off != limit:
        raise SDKFormatError('VPK tree size mismatch')
    return result


def vpk_file(entries, packed, filename):
    if filename not in entries:
        raise SDKFormatError('required VMF missing: ' + filename)
    crc, archive, position, length, preloaded = entries[filename]
    if archive != 0 or length > MAP_MAX or position > len(packed) or length > len(packed) - position:
        raise SDKFormatError('VMF has unsupported archive or invalid bounds')
    payload = preloaded + packed[position:position+length]
    if zlib.crc32(payload) & 0xffffffff != crc:
        raise SDKFormatError('VMF CRC mismatch: ' + filename)
    return payload


def audit_vmf(payload):
    if len(payload) > MAP_MAX:
        raise SDKFormatError('VMF exceeds maximum size')
    text = payload.decode('latin-1')
    blocks = re.findall(r'(?m)^entity\s*\r?\n\{\r?\n(.*?)^\}\r?$', text, flags=re.S)
    if len(blocks) != len(re.findall(r'(?m)^entity\s*$', text)):
        raise SDKFormatError('cannot safely identify all top-level VMF entities')
    counts = Counter()
    output_count = 0
    bad_outputs = 0
    difficulty = Counter()
    for block in blocks:
        cls = re.search(r'(?m)^\t"classname"\s+"([^"]+)"', block)
        if not cls:
            raise SDKFormatError('VMF entity missing classname')
        name = cls.group(1)
        counts[name] += 1
        if name == 'trigger_dynamic_difficulty':
            diff = re.search(r'(?m)^\t"EstimatedDifficulty"\s+"([^"]+)"', block)
            difficulty[diff.group(1) if diff else 'not-set'] += 1
        connection = re.search(r'(?m)^\tconnections\r?\n\t\{\r?\n(.*?)^\t\}\r?$', block, re.S)
        if connection:
            fields = re.findall(r'(?m)^\t\t"([^"]+)"\s+"([^"]*)"\s*$', connection.group(1))
            for output, value in fields:
                output_count += 1
                if len(value.split('\x1b' if '\x1b' in value else ',')) != 5:
                    bad_outputs += 1
    selected = ('logic_auto', 'logic_relay', 'logic_timer', 'trigger_dynamic_difficulty',
                'npc_sintek_grunt', 'npc_sintek_jetpack', 'npc_template_maker',
                'trigger_changelevel', 'info_landmark', 'func_door', 'func_button',
                'prop_vehicle_sin', 'scripted_sequence', 'trigger_once')
    return {'entity_count': len(blocks),
            'output_connection_count': output_count,
            'output_shapes_needing_review': bad_outputs,
            'selected_class_counts': {c: counts[c] for c in selected if counts[c]},
            'dynamic_difficulty_tier_counts': dict(sorted(difficulty.items()))}


def extract_weapon_keys(text):
    # Only fields needed for inventory; this does not execute Source KeyValues.
    fields = ('viewmodel', 'ZoomFOV', 'clip_size', 'clip2_size', 'default_clip',
              'default_clip2', 'primary_ammo', 'secondary_ammo')
    values = {}
    for field in fields:
        match = re.search(r'(?m)^\s*"' + re.escape(field) + r'"\s+"([^"]*)"', text)
        if not match:
            raise SDKFormatError('WeaponData missing required field: ' + field)
        values[field] = match.group(1)
    for key in ('ZoomFOV', 'clip_size', 'clip2_size', 'default_clip', 'default_clip2'):
        try:
            values[key] = int(values[key])
        except ValueError as exc:
            raise SDKFormatError('invalid integer WeaponData field') from exc
    return values


def audit_sdk(archive):
    with zipfile.ZipFile(archive) as z:
        entries = read_vpk_directory(z.read(VPK_DIR))
        packed = z.read(VPK_DATA)
        maps = {name.rsplit('/', 1)[-1]: audit_vmf(vpk_file(entries, packed, name))
                for name in MAP_PATHS}
        weapons = {w: extract_weapon_keys(z.read(ZIP_ROOT + 'SE1/scripts/' + w + '.txt').decode('latin-1'))
                   for w in WEAPONS}
        return {'scope': 'static-owner-sdk-source-inventory-not-gameplay-execution',
                'map_source': 'official SDK VPK v2 VMF, CRC-validated',
                'maps': maps, 'weapons': weapons,
                'limitations': ['Original VMF keyvalues and entity targetnames are not emitted.',
                                'VMF entity/output counts are not compiled BSP execution tests.',
                                'WeaponData scripts define settings, not animation events or game code.',
                                'No SourceVR, module loader, or Quest runtime tested.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk_zip', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        output = json.dumps(audit_sdk(args.sdk_zip), indent=2) + '\n'
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as exc:
        parser.exit(1, 'SDK AUDIT FAILED: ' + str(exc) + '\n')
    if args.output:
        args.output.write_text(output, encoding='utf-8')
    else:
        print(output, end='')


if __name__ == '__main__':
    sys.exit(main())
