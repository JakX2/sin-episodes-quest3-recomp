#!/usr/bin/env python3
"""Offline, fail-closed audit of HL2VR 0.989 Android launch entrypoint.

Source APK is owner-held; this script contains no original Android bytecode.
It does not install or launch the APK or test JNI/Android linker behavior.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from verify_hl2vr_renderer_contract import Dex, DexError

APK_SHA256 = '5ce5c4c16fd2aa934db45ed51de32878a561ed3dfe5405ee502ef199238a785c'
MANIFEST_SHA256 = '0fb8c391729ec37666039b5131de84eed4cd5293a6cb5ca3cbda57c1c62f8c52'
DEX_SHA256 = '7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95'


class ManifestError(ValueError):
    pass


def _u16(data, pos):
    if pos < 0 or pos + 2 > len(data):
        raise ManifestError('truncated uint16')
    return struct.unpack_from('<H', data, pos)[0]


def _u32(data, pos):
    if pos < 0 or pos + 4 > len(data):
        raise ManifestError('truncated uint32')
    return struct.unpack_from('<I', data, pos)[0]


def _length8(blob, pos):
    if pos >= len(blob):
        raise ManifestError('truncated UTF8 length')
    val = blob[pos]
    pos += 1
    if val & 128:
        if pos >= len(blob):
            raise ManifestError('truncated UTF8 length')
        val = ((val & 127) << 8) | blob[pos]
        pos += 1
    return val, pos


def _length16(blob, pos):
    val = _u16(blob, pos)
    pos += 2
    if val & 0x8000:
        val = ((val & 0x7fff) << 16) | _u16(blob, pos)
        pos += 2
    return val, pos


def _strings(blob, start, end):
    if start + 28 > end or _u16(blob, start) != 1:
        raise ManifestError('missing Android string pool')
    header = _u16(blob, start + 2)
    count, styles, flags, delta, styles_start = struct.unpack_from('<IIIII', blob, start + 8)
    if header < 28 or count > 100000 or start + header + 4 * count > end:
        raise ManifestError('string-pool table bounds')
    output = []
    for i in range(count):
        pos = start + delta + _u32(blob, start + header + 4 * i)
        if pos >= end:
            raise ManifestError('string data out of bounds')
        if flags & 0x100:
            chars, pos = _length8(blob, pos)
            size, pos = _length8(blob, pos)
            if size > end - pos or pos + size >= end or blob[pos + size] != 0:
                raise ManifestError('invalid UTF8 string')
            decoded = blob[pos:pos+size].decode('utf8')
            if len(decoded.encode('utf-16-le')) // 2 != chars:
                raise ManifestError('UTF8 string length mismatch')
        else:
            size, pos = _length16(blob, pos)
            if size > (end - pos - 2) // 2 or _u16(blob, pos + size * 2) != 0:
                raise ManifestError('invalid UTF16 string')
            decoded = blob[pos:pos+size*2].decode('utf-16-le')
        output.append(decoded)
    return output


def parse_axml(blob):
    if len(blob) < 8 or _u16(blob, 0) != 3 or _u32(blob, 4) != len(blob):
        raise ManifestError('expected Android binary XML document')
    strings = None
    roots = []
    stack = []
    pos = 8

    def get(index):
        if index == 0xffffffff:
            return None
        if strings is None or index >= len(strings):
            raise ManifestError('invalid Android XML string index')
        return strings[index]

    while pos < len(blob):
        if pos + 8 > len(blob):
            raise ManifestError('truncated XML chunk')
        typ, header, length = struct.unpack_from('<HHI', blob, pos)
        if header < 8 or length < header or pos + length > len(blob):
            raise ManifestError('invalid Android XML chunk bounds')
        if typ == 1:
            if strings is not None:
                raise ManifestError('duplicate XML string pool')
            strings = _strings(blob, pos, pos + length)
        elif typ == 0x102:
            if header < 16 or pos + 36 > pos + length:
                raise ManifestError('truncated XML start tag')
            name = get(_u32(blob, pos + 20))
            astart, asize, acount = struct.unpack_from('<HHH', blob, pos + 24)
            if asize != 20 or acount > 1000 or pos + 16 + astart + asize*acount > pos + length:
                raise ManifestError('invalid XML attributes')
            attrs = {}
            for i in range(acount):
                a = pos + 16 + astart + asize*i
                ns, label, raw = struct.unpack_from('<III', blob, a)
                tvsize, unused, dtype, dvalue = struct.unpack_from('<HBBI', blob, a + 12)
                if tvsize != 8:
                    raise ManifestError('invalid XML typed attribute')
                key = get(label)
                if key in attrs:
                    raise ManifestError('duplicate XML attribute')
                if raw != 0xffffffff:
                    val = get(raw)
                elif dtype == 3:
                    val = get(dvalue)
                elif dtype == 0x12:
                    val = bool(dvalue)
                elif dtype in (0x10, 0x11):
                    val = dvalue
                else:
                    val = None
                attrs[key] = val
            item = {'tag': name, 'attrs': attrs, 'children': []}
            if stack:
                stack[-1]['children'].append(item)
            else:
                roots.append(item)
            stack.append(item)
        elif typ == 0x103:
            if not stack or get(_u32(blob, pos+20)) != stack[-1]['tag']:
                raise ManifestError('unbalanced XML end tag')
            stack.pop()
        pos += length
    if stack or len(roots) != 1 or roots[0]['tag'] != 'manifest':
        raise ManifestError('not one complete Android manifest')
    return roots[0]


def find_activity(root, name):
    app = [e for e in root['children'] if e['tag'] == 'application']
    if len(app) != 1:
        raise ManifestError('expected one application')
    activities = [e for e in app[0]['children'] if e['tag'] == 'activity' and e['attrs'].get('name') == name]
    if len(activities) != 1:
        raise ManifestError('missing or ambiguous game activity')
    return activities[0]


def validate_manifest_entry(root):
    if root['attrs'].get('package') != 'com.valvesoftware.source.vrtest':
        raise ManifestError('unexpected Android package')
    activity = find_activity(root, 'org.libsdl.app.SDLActivity')
    if activity['attrs'].get('exported') not in (True, 'true') or activity['attrs'].get('process') != ':game':
        raise ManifestError('SDLActivity must be explicitly exported in :game')
    if activity['attrs'].get('permission'):
        raise ManifestError('SDLActivity has explicit access permission')
    filters = [x for x in activity['children'] if x['tag'] == 'intent-filter']
    if not any('org.libsdl.app.SDLACTIVITY' in [v['attrs'].get('name') for v in f['children'] if v['tag'] == 'action'] for f in filters):
        raise ManifestError('SDL launch intent action missing')
    sdk_nodes = [x for x in root['children'] if x['tag'] == 'uses-sdk']
    if len(sdk_nodes) != 1 or str(sdk_nodes[0]['attrs'].get('targetSdkVersion')) != '24':
        raise ManifestError('unexpected target SDK policy')
    return activity


def verify_manifest_and_dex(manifest, primary, *, strict=True):
    if strict and hashlib.sha256(manifest).hexdigest() != MANIFEST_SHA256:
        raise ManifestError('HL2VR 0.989 manifest SHA mismatch')
    if strict and hashlib.sha256(primary).hexdigest() != DEX_SHA256:
        raise ManifestError('HL2VR 0.989 primary DEX SHA mismatch')
    root = parse_axml(manifest)
    validate_manifest_entry(root)
    dex = Dex(primary)
    sdl = 'Lorg/libsdl/app/SDLActivity;'
    valve = 'Lcom/valvesoftware/ValveActivity2;'
    init = dex.method_bounds(sdl, 'init')
    create = dex.method_bounds(sdl, 'onCreate')
    native = dex.method_bounds(valve, 'initNatives')
    dex.require(create, 0x1107e, 0x6e, (sdl, 'init'))
    dex.require(init, 0x10978, 0x6e, (sdl, 'getIntent'))
    dex.require(init, 0x10980, 0x71, (valve, 'initNatives'))
    if primary[0x1097e:0x10980] != bytes.fromhex('0c 07') or primary[0x10984:0x10986] != bytes.fromhex('7c 00'):
        raise ManifestError('getIntent result not passed into initNatives as expected')
    dex.require(native, 0xd0ca, 0x1a, 'gamelibdir')
    dex.require(native, 0xd0ce, 0x6e, ('Landroid/content/Intent;', 'getStringExtra'))
    dex.require(native, 0xd218, 0x1a, 'APP_MOD_LIB')
    dex.require(native, 0xd21c, 0x71, (valve, 'setenv'))
    return {
        'source': 'original_owner_held_HL2VR_Standalone_0.989_APK',
        'sha256_pinned': strict,
        'application_package': root['attrs']['package'],
        'component': 'org.libsdl.app.SDLActivity',
        'component_exported': True,
        'component_process': ':game',
        'target_sdk': 24,
        'action': 'org.libsdl.app.SDLACTIVITY',
        'intent_to_initnatives_verified': True,
        'nonempty_gamelibdir_sets_APP_MOD_LIB': True,
        'example_adb_manual_launch_not_executed': 'adb shell am start -n com.valvesoftware.source.vrtest/org.libsdl.app.SDLActivity -a org.libsdl.app.SDLACTIVITY --es gamelibdir /PATH/TO/AUTHORIZED/NATIVE/LIB/DIR',
        'launch_from_external_caller_tested': False,
        'external_native_module_loaded': False,
        'real_quest_device_tested': False,
        'sourcevr_custom_game_api_verified': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    args = parser.parse_args()
    try:
        if hashlib.sha256(args.apk.read_bytes()).hexdigest() != APK_SHA256:
            raise ManifestError('owner APK SHA mismatch')
        with zipfile.ZipFile(args.apk) as z:
            report = verify_manifest_and_dex(z.read('AndroidManifest.xml'), z.read('classes.dex'))
    except (OSError, zipfile.BadZipFile, KeyError, ManifestError, DexError) as exc:
        parser.exit(1, 'Phase83 activity audit failed: ' + str(exc) + '\n')
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
