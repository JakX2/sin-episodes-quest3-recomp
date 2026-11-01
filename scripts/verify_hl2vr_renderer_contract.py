#!/usr/bin/env python3
"""Read-only evidence: HL2VR 0.989 Android launcher native directory contract.

Accepts an owner-held APK, pins both DEX blobs by SHA-256, checks method bounds
and selected exact bytecode references. Does NOT execute an APK or prove custom
native module loading on Android / Quest. No original DEX bytes are distributed.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

PINNED = {
    'classes.dex': '7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95',
    'classes2.dex': '18c5944efa31751e90ee01fe131dab0f91183781545eefea8b5575660ea9b48c',
}

class DexError(ValueError):
    pass


def uleb(data, pos):
    value = 0
    for index in range(5):
        if pos >= len(data):
            raise DexError('truncated ULEB128')
        byte = data[pos]
        pos += 1
        value |= (byte & 127) << (7 * index)
        if byte < 128:
            if index == 4 and byte > 15:
                raise DexError('ULEB128 exceeds 32 bits')
            return value, pos
    raise DexError('ULEB128 too long')


class Dex:
    def __init__(self, raw):
        self.raw = raw
        if len(raw) < 112 or raw[:4] != b'dex\n':
            raise DexError('invalid DEX header')
        size = struct.unpack_from('<I', raw, 32)[0]
        if size != len(raw):
            raise DexError('DEX file size mismatch')
        self.strings = self._strings()
        self.types = self._types()
        self.methods = self._methods()
        self.code = self._codes()

    def _ids(self, offset, stride, limit=200000):
        count, start = struct.unpack_from('<II', self.raw, offset)
        if count > limit or start > len(self.raw) or count * stride > len(self.raw) - start:
            raise DexError('DEX ID table out of bounds')
        return count, start

    def _strings(self):
        count, start = self._ids(56, 4)
        result = []
        for i in range(count):
            pos = struct.unpack_from('<I', self.raw, start + 4*i)[0]
            if pos >= len(self.raw):
                raise DexError('DEX string offset out of range')
            _, pos = uleb(self.raw, pos)
            end = self.raw.find(b'\0', pos)
            if end < 0:
                raise DexError('unterminated DEX string')
            result.append(self.raw[pos:end].decode('utf8', errors='replace'))
        return result

    def _types(self):
        count, start = self._ids(64, 4)
        try:
            return [self.strings[struct.unpack_from('<I', self.raw, start + 4*i)[0]]
                    for i in range(count)]
        except IndexError as exc:
            raise DexError('invalid DEX type reference') from exc

    def _methods(self):
        count, start = self._ids(88, 8)
        try:
            return [(self.types[cls], self.strings[name]) for cls, proto, name in
                    (struct.unpack_from('<HHI', self.raw, start + 8*i) for i in range(count))]
        except IndexError as exc:
            raise DexError('invalid DEX method reference') from exc

    def _codes(self):
        count, start = self._ids(96, 32)
        result = {}
        for i in range(count):
            classdata = struct.unpack_from('<I', self.raw, start + i*32 + 24)[0]
            if not classdata:
                continue
            if classdata >= len(self.raw):
                raise DexError('classdata out of range')
            pos = classdata
            counts = []
            for _ in range(4):
                value, pos = uleb(self.raw, pos)
                counts.append(value)
            if any(c > len(self.methods) + 10000 for c in counts):
                raise DexError('invalid classdata member count')
            for count_fields in counts[:2]:
                for _ in range(count_fields):
                    _, pos = uleb(self.raw, pos)
                    _, pos = uleb(self.raw, pos)
            for count_methods in counts[2:]:
                method_idx = 0
                for _ in range(count_methods):
                    delta, pos = uleb(self.raw, pos)
                    method_idx += delta
                    _, pos = uleb(self.raw, pos)
                    co, pos = uleb(self.raw, pos)
                    if method_idx >= len(self.methods):
                        raise DexError('invalid classdata method index')
                    if co:
                        if co + 16 > len(self.raw):
                            raise DexError('truncated code item')
                        insns = struct.unpack_from('<I', self.raw, co + 12)[0]
                        if insns > (len(self.raw) - co - 16) // 2:
                            raise DexError('code item instruction range out of bounds')
                        if method_idx in result:
                            raise DexError('duplicate code item')
                        result[method_idx] = (co + 16, co + 16 + insns*2)
        return result

    def method_bounds(self, classname, method):
        candidates = [(i, self.code[i]) for i, item in enumerate(self.methods)
                      if item == (classname, method) and i in self.code]
        if len(candidates) != 1:
            raise DexError('missing or ambiguous method code: ' + classname + '.' + method)
        return candidates[0][1]

    def require(self, method, at, op, arg=None):
        start, end = method
        if at < start or at + 4 > end or at % 2:
            raise DexError('opcode outside expected method code')
        if self.raw[at] != op:
            raise DexError('wrong DEX opcode at ' + hex(at))
        index = struct.unpack_from('<H', self.raw, at + 2)[0]
        if arg is not None:
            if op == 0x1a:
                actual = self.strings[index] if index < len(self.strings) else None
            elif op in (0x6e, 0x71, 0x72):
                actual = self.methods[index] if index < len(self.methods) else None
            else:
                actual = index
            if actual != arg:
                raise DexError('DEX reference mismatch at ' + hex(at))
        return index


def verify(primary, renderer):
    if hashlib.sha256(primary).hexdigest() != PINNED['classes.dex']:
        raise DexError('primary classes.dex SHA-256 mismatch')
    if hashlib.sha256(renderer).hexdigest() != PINNED['classes2.dex']:
        raise DexError('renderer classes2.dex SHA-256 mismatch')
    a, b = Dex(primary), Dex(renderer)
    maincls = 'Lcom/valvesoftware/ValveActivity2;'
    runtime = 'Lme/nillerusr/RendererRuntime;'
    tabs = 'Lme/nillerusr/VRLauncherTabs;'
    init = a.method_bounds(maincls, 'initNatives')
    selecteddir = b.method_bounds(tabs, 'selectedNativeLibraryDirectory')
    directory = b.method_bounds(runtime, 'directory')
    selected = b.method_bounds(runtime, 'selected')
    prepare = b.method_bounds(runtime, 'prepare')
    load = b.method_bounds(runtime, 'load')

    # gamelibdir Intent string is assigned to v4, then both null and empty
    # guards branch past APP_MOD_LIB's conditional setenv().
    a.require(init, 0xd0ca, 0x1a, 'gamelibdir')
    a.require(init, 0xd0ce, 0x6e, ('Landroid/content/Intent;', 'getStringExtra'))
    if primary[0xd0d4:0xd0d6] != b'\x0c\x04':
        raise DexError('gamelibdir result register changed')
    if primary[0xd208:0xd20c] != bytes.fromhex('38 04 0d 00'):
        raise DexError('null guard changed')
    a.require(init, 0xd20c, 0x6e, ('Ljava/lang/String;', 'isEmpty'))
    if primary[0xd214:0xd218] != bytes.fromhex('39 07 07 00'):
        raise DexError('empty-string guard changed')
    a.require(init, 0xd218, 0x1a, 'APP_MOD_LIB')
    a.require(init, 0xd21c, 0x71, (maincls, 'setenv'))

    a.require(init, 0xd30a, 0x1a, 'APP_LIB_PATH')
    if primary[0xd30e:0xd312] != bytes.fromhex('54 08 07 00'):
        raise DexError('nativeLibraryDir fallback field changed')
    a.require(init, 0xd312, 0x71, (tabs, 'selectedNativeLibraryDirectory'))
    a.require(init, 0xd31a, 0x71, (maincls, 'setenv'))
    b.require(selecteddir, 0xe4c8, 0x71, (runtime, 'directory'))
    b.require(directory, 0x65f8, 0x71, (runtime, 'selected'))
    if renderer[0x6600:0x6606] != bytes.fromhex('39 00 03 00 11 02'):
        raise DexError('non-Turbo fallback branch changed')
    b.require(directory, 0x6606, 0x71, (runtime, 'prepare'))
    b.require(directory, 0x660e, 0x6e, ('Ljava/io/File;', 'getAbsolutePath'))

    for address, value in ((0x5f6e, 'portal'), (0x5f7e, 'portal2'),
                           (0x5f98, 'launcher_release_legacy_gles_backend'),
                           (0x5fb4, 'legacy_gles'),
                           (0x5ff6, 'hl2quest_turbo/manifest.tsv'),
                           (0x604c, 'native-turbo-'),
                           (0x5da6, 'APP_LIB_PATH'),
                           (0x5dba, 'HL2QUEST_GLES_LEGACY'),
                           (0x5dc8, 'HL2QUEST_DXVK_RENDERER')):
        meth = selected if 0x5f2c <= address < 0x5fc8 else (
            prepare if 0x5fc8 <= address <= 0x64ac else load)
        b.require(meth, address, 0x1a, value)
    b.require(load, 0x5d9e, 0x71, (runtime, 'prepare'))
    b.require(load, 0x5db4, 0x71, ('Landroid/system/Os;', 'setenv'))
    b.require(load, 0x5e24, 0x71, ('Ljava/lang/System;', 'load'))

    return {
        'build': 'HL2VR Standalone 0.989',
        'primary_dex_sha256_verified': True,
        'renderer_dex_sha256_verified': True,
        'app_mod_lib': 'optional nonempty Intent extra: gamelibdir',
        'app_lib_path': 'installed nativeLibraryDir unless Turbo is selected; then RendererRuntime.prepare() directory',
        'turbo_selection': 'non-Portal game, shared preference launcher_release_legacy_gles_backend; possible legacy_gles Intent override',
        'turbo_storage': 'app getFilesDir()/native-turbo-<manifest-digest> from packaged hl2quest_turbo/manifest.tsv',
        'turbo_load': 'RendererRuntime.load calls prepare, sets APP_LIB_PATH and loads packaged libraries',
        'paths_observed_in_running_android_process': False,
        'custom_native_module_loaded': False,
        'quest_headset_tested': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    args = parser.parse_args()
    try:
        with zipfile.ZipFile(args.apk) as z:
            out = verify(z.read('classes.dex'), z.read('classes2.dex'))
    except (OSError, KeyError, ValueError, zipfile.BadZipFile) as exc:
        parser.exit(1, 'Phase82 DEX verification failed: ' + str(exc) + '\n')
    print(json.dumps(out, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
