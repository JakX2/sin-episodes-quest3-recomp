#!/usr/bin/env python3
"""Verify pinned Android DEX launch environment assignments, without executing APKs.

Uses only stdlib. DEX code offsets shown are 16-bit instruction offsets within the
named method (not ELF addresses). Does not prove native linker acceptance.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import zipfile

PROFILES = {
    'hl2vr-0989': {
        'dex': {'classes.dex': '7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95',
                'classes2.dex': '18c5944efa31751e90ee01fe131dab0f91183781545eefea8b5575660ea9b48c'},
        'init': {'gamelibdir': 0x39, 'APP_MOD_LIB': 0xe0, 'APP_LIB_PATH': 0x159},
        'getextra': 0x3b, 'mod_setenv': 0xe2, 'lib_field': 0x15b,
        'activity_init_call': 0x128, 'empty_guard': 0xda, 'guard_branch_before': 0xd8, 'guard_branch_after': 0xde,
        'select_dir': 0x15d, 'lib_setenv': 0x161,
        'turbo_load_path': 0x13, 'turbo_setenv': 0x1a,
    },
    'portalvr-112': {
        'dex': {'classes.dex': 'c876e067cfa06c84efe32747141c2994b66c9150b6d2303956b8491fa991944b'},
        'init': {'gamelibdir': 0x36, 'APP_MOD_LIB': 0xcb, 'APP_LIB_PATH': 0x140},
        'getextra': 0x38, 'mod_setenv': 0xcd, 'lib_field': 0x142,
        'activity_init_call': 0x103, 'empty_guard': 0xc5, 'guard_branch_before': 0xc3, 'guard_branch_after': 0xc9,
        'lib_setenv': 0x144,
    },
}

class DexError(ValueError):
    pass

def read_u16(data, offset):
    if offset < 0 or offset + 2 > len(data):
        raise DexError('truncated DEX u16')
    return struct.unpack_from('<H', data, offset)[0]

def read_u32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise DexError('truncated DEX u32')
    return struct.unpack_from('<I', data, offset)[0]

def uleb128(data, offset):
    number = 0
    for shift in range(0, 35, 7):
        if offset >= len(data):
            raise DexError('truncated uleb128')
        byte = data[offset]
        offset += 1
        number |= (byte & 0x7f) << shift
        if byte < 128:
            return number, offset
    raise DexError('oversized uleb128')

class Dex:
    def __init__(self, data):
        self.data = data
        if len(data) < 112 or not data.startswith(b'dex\n') or read_u32(data, 32) != len(data):
            raise DexError('invalid DEX header or size')
        self.string_count = read_u32(data, 56)
        self.string_offset = read_u32(data, 60)
        self.type_count = read_u32(data, 64)
        self.type_offset = read_u32(data, 68)
        self.method_count = read_u32(data, 88)
        self.method_offset = read_u32(data, 92)
        self.class_count = read_u32(data, 96)
        self.class_offset = read_u32(data, 100)
        for count, offset, width in (
            (self.string_count, self.string_offset, 4),
            (self.type_count, self.type_offset, 4),
            (self.method_count, self.method_offset, 8),
            (self.class_count, self.class_offset, 32),
        ):
            if count > 100000 or offset + count * width > len(data):
                raise DexError('invalid DEX table bounds')
        self.strings = []
        for i in range(self.string_count):
            where = read_u32(data, self.string_offset + i * 4)
            _, start = uleb128(data, where)
            end = data.find(b'\x00', start)
            if end < 0:
                raise DexError('unterminated DEX string')
            self.strings.append(data[start:end].decode('utf-8', 'replace'))
        self.types = [self.strings[read_u32(data, self.type_offset + i * 4)]
                      for i in range(self.type_count)]
        self.methods = []
        for i in range(self.method_count):
            klass, proto, name = struct.unpack_from('<HHI', data, self.method_offset + 8 * i)
            self.methods.append((self.types[klass], self.strings[name]))
        self.definitions = {}
        for i in range(self.class_count):
            class_data_off = read_u32(data, self.class_offset + i * 32 + 24)
            if not class_data_off:
                continue
            where = class_data_off
            sizes = []
            for _ in range(4):
                n, where = uleb128(data, where)
                sizes.append(n)
            for _ in range(sizes[0] + sizes[1]):
                _, where = uleb128(data, where)
                _, where = uleb128(data, where)
            for count in sizes[2:]:
                idx = 0
                for _ in range(count):
                    diff, where = uleb128(data, where)
                    idx += diff
                    _, where = uleb128(data, where)
                    code, where = uleb128(data, where)
                    if idx >= len(self.methods):
                        raise DexError('method index out of range')
                    if code:
                        size = read_u32(data, code + 12)
                        begin = code + 16
                        if size > 4000000 or begin + size * 2 > len(data):
                            raise DexError('truncated method code')
                        self.definitions[self.methods[idx]] = [read_u16(data, begin + 2 * j)
                                                               for j in range(size)]

    def method(self, klass, name):
        found = self.definitions.get((klass, name))
        if found is None:
            raise DexError('missing code for ' + klass + '->' + name)
        return found

    def string_at(self, instructions, index, expected):
        if index + 1 >= len(instructions) or instructions[index] & 255 != 0x1a:
            raise DexError('missing const-string at code-unit offset ' + hex(index))
        value = instructions[index + 1]
        if value >= len(self.strings) or self.strings[value] != expected:
            raise DexError('const-string mismatch at code-unit offset ' + hex(index))

    def invoke_at(self, instructions, index, klass, method):
        if index + 1 >= len(instructions) or instructions[index] & 255 not in (0x6e, 0x70, 0x71, 0x72, 0x74, 0x76, 0x77, 0x78):
            raise DexError('missing invoke at code-unit offset ' + hex(index))
        value = instructions[index + 1]
        if value >= len(self.methods) or self.methods[value] != (klass, method):
            raise DexError('invoke target mismatch at code-unit offset ' + hex(index))

    def field_at(self, instructions, index, fieldname):
        # iget-object vAA,vBB,field@CCCC
        if index + 1 >= len(instructions) or instructions[index] & 255 != 0x54:
            raise DexError('missing iget-object at code-unit offset ' + hex(index))
        field_count = read_u32(self.data, 80)
        field_off = read_u32(self.data, 84)
        idx = instructions[index + 1]
        if idx >= field_count:
            raise DexError('field index out of bounds')
        name_idx = read_u32(self.data, field_off + idx * 8 + 4)
        if self.strings[name_idx] != fieldname:
            raise DexError('field mismatch at code-unit offset ' + hex(index))

def verify_apk(apk, profile):
    expected = PROFILES[profile]
    with zipfile.ZipFile(apk) as archive:
        dexfiles = {}
        for name, digest in expected['dex'].items():
            data = archive.read(name)
            if hashlib.sha256(data).hexdigest() != digest:
                raise DexError('DEX SHA-256 mismatch: ' + name)
            dexfiles[name] = Dex(data)
    main = dexfiles['classes.dex']
    init = main.method('Lcom/valvesoftware/ValveActivity2;', 'initNatives')
    for string, offset in expected['init'].items():
        main.string_at(init, offset, string)
    activity = main.method('Lorg/libsdl/app/SDLActivity;', 'init')
    main.invoke_at(activity, expected['activity_init_call'],
                   'Lcom/valvesoftware/ValveActivity2;', 'initNatives')
    main.invoke_at(init, expected['getextra'], 'Landroid/content/Intent;', 'getStringExtra')
    main.invoke_at(init, expected['empty_guard'], 'Ljava/lang/String;', 'isEmpty')
    if init[expected['guard_branch_before']] & 255 != 0x38 or init[expected['guard_branch_after']] & 255 != 0x39:
        raise DexError('APP_MOD_LIB guard instructions changed')
    main.invoke_at(init, expected['mod_setenv'], 'Lcom/valvesoftware/ValveActivity2;', 'setenv')
    main.field_at(init, expected['lib_field'], 'nativeLibraryDir')
    main.invoke_at(init, expected['lib_setenv'], 'Lcom/valvesoftware/ValveActivity2;', 'setenv')
    if 'select_dir' in expected:
        main.invoke_at(init, expected['select_dir'], 'Lme/nillerusr/VRLauncherTabs;',
                       'selectedNativeLibraryDirectory')
        turbo = dexfiles['classes2.dex']
        runtime_load = turbo.method('Lme/nillerusr/RendererRuntime;', 'load')
        turbo.string_at(runtime_load, expected['turbo_load_path'], 'APP_LIB_PATH')
        turbo.invoke_at(runtime_load, expected['turbo_setenv'], 'Landroid/system/Os;', 'setenv')
        choose_dir = turbo.method('Lme/nillerusr/VRLauncherTabs;', 'selectedNativeLibraryDirectory')
        turbo.invoke_at(choose_dir, 0, 'Lme/nillerusr/RendererRuntime;', 'directory')
        directory = turbo.method('Lme/nillerusr/RendererRuntime;', 'directory')
        turbo.invoke_at(directory, 0, 'Lme/nillerusr/RendererRuntime;', 'selected')
        turbo.invoke_at(directory, 7, 'Lme/nillerusr/RendererRuntime;', 'prepare')
        prepare = turbo.method('Lme/nillerusr/RendererRuntime;', 'prepare')
        turbo.string_at(prepare, 0x3a, 'native-turbo-')
    return {
        'profile': profile, 'dex_sha256_verified': True,
        'jni_environment_initialization': 'ValveActivity2.initNatives',
        'game_activity_calls_initialization': True,
        'mod_directory_guarded_by_nonempty_intent_extra': True,
        'optional_mod_directory_intent_extra': 'gamelibdir',
        'native_module_directory_environment': 'APP_MOD_LIB',
        'app_native_library_environment': 'APP_LIB_PATH',
        'app_native_library_fallback': 'ApplicationInfo.nativeLibraryDir',
        'renderer_specific_app_private_directory': 'native-turbo-*' if 'select_dir' in expected else None,
        'app_mod_lib_set_by_stock_launcher_verified': False,
        'android_linker_namespace_tested': False,
        'external_custom_module_loaded': False,
        'quest_runtime_tested': False,
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True, choices=PROFILES)
    parser.add_argument('--apk', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = verify_apk(args.apk, args.profile)
    except (OSError, KeyError, DexError, IndexError, struct.error, zipfile.BadZipFile) as exc:
        print(json.dumps({'verified': False, 'error': str(exc)}))
        return 1
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
