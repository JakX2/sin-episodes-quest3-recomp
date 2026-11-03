"""Small, synthetic AArch64 APK fixtures. Never require commercial files."""
from __future__ import annotations
import io
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
from audit_quest_runtime_apk import AuditError, REQUIRED, inspect_elf, analyze_path

def fake_elf(*, machine=183, factory=True):
    shstr = b'\0.shstrtab\0.dynstr\0.dynsym\0.dynamic\0'
    strings = b'\0CreateInterface\0liblog.so\0libmock.so\0'
    indexes = [shstr.index(n) for n in (b'.shstrtab', b'.dynstr', b'.dynsym', b'.dynamic')]
    null = b'\0' * 24
    symbol = struct.pack('<IBBHQQ', 1, 0x12, 0, 1 if factory else 0, 0, 120)
    dynsym = null + symbol
    dynamic = (struct.pack('<qQ', 1, strings.index(b'liblog.so')) +
               struct.pack('<qQ', 14, strings.index(b'libmock.so')) +
               struct.pack('<qQ', 0, 0))
    chunks = [shstr, strings, dynsym, dynamic]
    data = bytearray(64)
    headers = [(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    pos = 64
    for idx, chunk in enumerate(chunks):
        align = 8
        pad = (-pos) % align
        data += bytes(pad)
        pos += pad
        off = pos
        data += chunk
        pos += len(chunk)
        typ = [3, 3, 11, 6][idx]
        ent = [0, 0, 24, 16][idx]
        link = 2 if idx in (2, 3) else 0
        headers.append((indexes[idx], typ, 0, 0, off, len(chunk), link, 0, align, ent))
    data += bytes((-pos) % 8)
    shoff = len(data)
    for h in headers:
        data += struct.pack('<IIQQQQIIQQ', *h)
    ident = b'\x7fELF' + bytes([2, 1, 1]) + bytes(9)
    header = struct.pack('<16sHHIQQQIHHHHHH', ident, 3, machine, 1, 0, 0,
                         shoff, 0, 64, 0, 0, 64, 5, 1)
    data[:64] = header
    return bytes(data)

def make_apk(*, missing=None, missing_factory=None):
    out = io.BytesIO()
    with zipfile.ZipFile(out, 'w') as z:
        for name in REQUIRED:
            if name == missing: continue
            z.writestr('lib/arm64-v8a/' + name, fake_elf(factory=name != missing_factory))
        z.writestr('assets/hl2quest_turbo/arm64-v8a/libclient.so', fake_elf())
        z.writestr('assets/hl2quest_turbo/arm64-v8a/libengine.so', fake_elf())
    return out.getvalue()

class QuestRuntimeAuditTests(unittest.TestCase):
    def test_parse_fake_arm64_dynamic_factory(self):
        data = inspect_elf(fake_elf())
        self.assertEqual(data['architecture'], 'ELF64 little-endian AArch64')
        self.assertEqual(data['soname'], 'libmock.so')
        self.assertIn('liblog.so', data['needed'])
        self.assertTrue(data['create_interface_exported'])
        self.assertGreaterEqual(data['export_count'], 1)

    def test_reject_other_architecture_and_bad_header(self):
        with self.assertRaisesRegex(AuditError, 'AArch64'):
            inspect_elf(fake_elf(machine=62))
        with self.assertRaisesRegex(AuditError, 'ELF64'):
            inspect_elf(b'garbage')

    def test_read_raw_apk_without_extraction(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'sample.apk'
            p.write_bytes(make_apk())
            result = analyze_path(p)
        self.assertEqual(result['container'], 'APK')
        self.assertEqual(result['installed_count'], len(REQUIRED))
        self.assertTrue(result['no_sin_named_modules'])
        self.assertIn('libclient.so', result['matching_variants'])

    def test_read_zip_wrapping_apk(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'sample.zip'
            with zipfile.ZipFile(p, 'w') as z:
                z.writestr('HL2VR/example.apk', make_apk())
            result = analyze_path(p)
        self.assertEqual(result['container'], 'ZIP containing APK')
        self.assertEqual(result['inner_apk_name'], 'HL2VR/example.apk')

    def test_missing_required_module_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'sample.apk'
            p.write_bytes(make_apk(missing='libserver.so'))
            with self.assertRaisesRegex(AuditError, 'libserver.so'):
                analyze_path(p)

    def test_missing_factory_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'sample.apk'
            p.write_bytes(make_apk(missing_factory='libserver.so'))
            with self.assertRaisesRegex(AuditError, 'CreateInterface'):
                analyze_path(p)

if __name__ == '__main__':
    unittest.main()
