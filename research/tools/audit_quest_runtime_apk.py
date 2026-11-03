#!/usr/bin/env python3
"""Read-only structural audit of a user-provided HL2VR Quest APK.

Accepts an APK or a ZIP containing one APK. Does NOT run, extract or
redistribute proprietary native modules. Uses only Python stdlib.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import zipfile

REQUIRED = (
    'libengine.so', 'libclient.so', 'libserver.so',
    'libsourcevr.so', 'libopenxr_loader.so', 'libSDL2.so',
)
FACTORY_MODULES = ('libengine.so', 'libclient.so', 'libserver.so', 'libsourcevr.so')
INSTALL_PREFIX = 'lib/arm64-v8a/'
TURBO_PREFIX = 'assets/hl2quest_turbo/arm64-v8a/'
INTERFACE_MARKERS = (
    'VClient017', 'ServerGameDLL010', 'VEngineClient014', 'VEngineServer023',
    'VClientEntityList003', 'VClientPrediction001', 'VEngineCvar004',
    'VFileSystem022', 'VMaterialSystem081'
)

class AuditError(ValueError):
    pass

def _cstr(buf: bytes, start: int) -> str:
    if start < 0 or start >= len(buf):
        return ''
    end = buf.find(b'\0', start)
    if end < 0: end = len(buf)
    return buf[start:end].decode('utf8', 'replace')

def inspect_elf(data: bytes) -> dict:
    """Inspect ELF64 little-endian AArch64 .so without external readelf."""
    if len(data) < 64 or data[:4] != b'\x7fELF' or data[4] != 2 or data[5] != 1:
        raise AuditError('not ELF64 little-endian')
    if struct.unpack_from('<H', data, 18)[0] != 183:
        raise AuditError('not AArch64 ELF (e_machine != 183)')
    if struct.unpack_from('<H', data, 16)[0] != 3:
        raise AuditError('not an ET_DYN shared object')
    shoff = struct.unpack_from('<Q', data, 40)[0]
    shentsize = struct.unpack_from('<H', data, 58)[0]
    shnum = struct.unpack_from('<H', data, 60)[0]
    shstrndx = struct.unpack_from('<H', data, 62)[0]
    if shnum == 0 or shentsize < 64 or shstrndx >= shnum or shoff + shentsize * shnum > len(data):
        raise AuditError('missing or invalid ELF section headers')
    headers = []
    for i in range(shnum):
        values = struct.unpack_from('<IIQQQQIIQQ', data, shoff + i * shentsize)
        _, typ, _flags, _addr, offset, size, link, _info, _align, entsize = values
        if offset + size > len(data) and typ != 8:
            raise AuditError('ELF section out of range')
        headers.append({'name_index': values[0], 'type': typ, 'offset': offset,
                        'size': size, 'link': link, 'entsize': entsize})
    shstr = headers[shstrndx]
    names = data[shstr['offset']:shstr['offset'] + shstr['size']]
    byname = {_cstr(names, h['name_index']): h for h in headers}
    dynsym = byname.get('.dynsym')
    dynstr = byname.get('.dynstr')
    if not dynsym or not dynstr or dynsym['entsize'] < 24:
        raise AuditError('missing valid ELF .dynsym/.dynstr')
    strings = data[dynstr['offset']:dynstr['offset'] + dynstr['size']]
    exports = []
    for pos in range(dynsym['offset'], dynsym['offset'] + dynsym['size'], dynsym['entsize']):
        if pos + 24 > len(data): break
        name, info, _other, section, _value, _size = struct.unpack_from('<IBBHQQ', data, pos)
        # Defined GLOBAL or WEAK symbols only: imports are NOT exports.
        if section != 0 and (info >> 4) in (1, 2):
            exports.append(_cstr(strings, name))
    dynamic = byname.get('.dynamic')
    needed = []
    soname = None
    if dynamic and dynamic['entsize'] >= 16:
        for pos in range(dynamic['offset'], dynamic['offset'] + dynamic['size'], dynamic['entsize']):
            if pos + 16 > len(data): break
            tag, value = struct.unpack_from('<qQ', data, pos)
            if tag == 0: break
            if tag == 1: needed.append(_cstr(strings, value))
            if tag == 14: soname = _cstr(strings, value)
    build_id = None
    note = byname.get('.note.gnu.build-id')
    if note and note['size'] >= 12:
        b = data[note['offset']:note['offset'] + note['size']]
        namesz, descsz, ntype = struct.unpack_from('<III', b, 0)
        offset = 12 + ((namesz + 3) // 4) * 4
        if ntype == 3 and b[12:12 + namesz].rstrip(b'\0') == b'GNU' and offset + descsz <= len(b):
            build_id = b[offset:offset + descsz].hex()
    # These ASCII strings are candidates, NOT verified factory registration.
    advertised = [marker for marker in INTERFACE_MARKERS
                  if re.search(rb'(?<![a-zA-Z0-9_])' + marker.encode() + rb'\x00', data)]
    return {'architecture': 'ELF64 little-endian AArch64', 'soname': soname,
            'needed': needed, 'build_id': build_id,
            'create_interface_exported': 'CreateInterface' in exports,
            'interface_strings_only': advertised, 'export_count': len(exports)}

def inspect_apk(apk: zipfile.ZipFile) -> dict:
    members = set(apk.namelist())
    def inspect_group(prefix):
        found = {}
        for name in sorted(members):
            if not name.startswith(prefix) or not name.endswith('.so'):
                continue
            short = name[len(prefix):]
            if '/' in short: continue
            content = apk.read(name)
            found[short] = {'sha256': hashlib.sha256(content).hexdigest(),
                            'size': len(content), **inspect_elf(content)}
        return found
    installed = inspect_group(INSTALL_PREFIX)
    turbo = inspect_group(TURBO_PREFIX)
    missing = [name for name in REQUIRED if name not in installed]
    if missing: raise AuditError('missing required app-native modules: ' + ', '.join(missing))
    factory_missing = [name for name in FACTORY_MODULES
                       if not installed[name]['create_interface_exported']]
    if factory_missing: raise AuditError('expected CreateInterface export absent: ' + ', '.join(factory_missing))
    both = sorted(set(installed) & set(turbo))
    diff = [name for name in both if installed[name]['sha256'] != turbo[name]['sha256']]
    match = [name for name in both if installed[name]['sha256'] == turbo[name]['sha256']]
    return {'installed_count': len(installed), 'turbo_count': len(turbo),
            'installed_only': sorted(set(installed) - set(turbo)),
            'turbo_only': sorted(set(turbo) - set(installed)),
            'matching_variants': match, 'different_variants': diff,
            'no_sin_named_modules': not any('sin' in n.lower() for n in installed),
            'libraries': installed, 'turbo_libraries': turbo,
            'warning': 'Factory export and ELF format are necessary, not sufficient, for SiN/Source engine ABI compatibility.'}

def analyze_path(path: Path) -> dict:
    with zipfile.ZipFile(path) as outer:
        if any(n.startswith(INSTALL_PREFIX) for n in outer.namelist()):
            report = inspect_apk(outer)
            report['container'] = 'APK'
        else:
            apk_names = [n for n in outer.namelist() if n.lower().endswith('.apk')]
            if len(apk_names) != 1:
                raise AuditError('ZIP must contain exactly one APK')
            # Nested APK may be >250 MiB; avoid holding it in process RAM.
            with tempfile.TemporaryFile() as temp:
                with outer.open(apk_names[0]) as source:
                    import shutil
                    shutil.copyfileobj(source, temp, 8 * 1024 * 1024)
                temp.seek(0)
                with zipfile.ZipFile(temp) as apk:
                    report = inspect_apk(apk)
            report['container'] = 'ZIP containing APK'
            report['inner_apk_name'] = apk_names[0]
        report['source_size_bytes'] = path.stat().st_size
        with path.open('rb') as f:
            report['source_sha256'] = hashlib.file_digest(f, 'sha256').hexdigest()
    return report

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--apk-or-zip', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    result = analyze_path(args.apk_or_zip)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items()
                      if k not in ('libraries', 'turbo_libraries')}, indent=2))
if __name__ == '__main__':
    main()
