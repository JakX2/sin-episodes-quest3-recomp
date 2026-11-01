"""Synthetic DEX/ZIP integrity checks only, no Quest/Valve game content."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

SCRIPT = Path(__file__).resolve().parents[2] / 'scripts' / 'verify_sourcevr_launcher_environment.py'
SPEC = importlib.util.spec_from_file_location('sourcevr_dex', SCRIPT)
mod = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(mod)

def fake_dex():
    data = bytearray(0x280)
    data[:8] = b'dex\n035\x00'
    struct.pack_into('<I', data, 32, len(data))
    strings = ['Lfake/Main;', 'literal', 'run', 'other', 'nativeLibraryDir']
    struct.pack_into('<II', data, 56, len(strings), 0xc0)
    struct.pack_into('<II', data, 64, 1, 0x120)
    struct.pack_into('<II', data, 88, 1, 0x124)
    struct.pack_into('<II', data, 96, 1, 0x130)
    off = 0xe0
    for i, s in enumerate(strings):
        struct.pack_into('<I', data, 0xc0+4*i, off)
        encoded = s.encode('ascii')
        data[off] = len(encoded)
        data[off+1:off+1+len(encoded)] = encoded
        data[off+1+len(encoded)] = 0
        off += len(encoded)+2
    struct.pack_into('<I', data, 0x120, 0)
    struct.pack_into('<HHI', data, 0x124, 0, 0, 2)
    struct.pack_into('<8I', data, 0x130, 0, 1, 0, 0, 0, 0, 0x160, 0)
    data[0x160:0x167] = bytes([0, 0, 1, 0, 0, 1, 0x80])
    data[0x167] = 0x03
    struct.pack_into('<HHHHII', data, 0x180, 1, 0, 0, 0, 0, 3)
    struct.pack_into('<HHH', data, 0x190, 0x001a, 1, 0x000e)
    return bytes(data)

class DexTests(unittest.TestCase):
    def test_minimal_dex_and_method_string_reference(self):
        d = mod.Dex(fake_dex())
        method = d.method('Lfake/Main;', 'run')
        d.string_at(method, 0, 'literal')
        self.assertEqual(len(method), 3)

    def test_wrong_string_rejected(self):
        d = mod.Dex(fake_dex())
        with self.assertRaisesRegex(mod.DexError, 'const-string mismatch'):
            d.string_at(d.method('Lfake/Main;', 'run'), 0, 'other')

    def test_wrong_method_rejected(self):
        d = mod.Dex(fake_dex())
        with self.assertRaisesRegex(mod.DexError, 'missing code'):
            d.method('Lfake/Main;', 'missing')

    def test_wrong_opcode_rejected(self):
        d = mod.Dex(fake_dex())
        with self.assertRaisesRegex(mod.DexError, 'missing invoke'):
            d.invoke_at(d.method('Lfake/Main;', 'run'), 0, 'Lfake/Main;', 'run')

    def test_field_opcode_rejected(self):
        d = mod.Dex(fake_dex())
        with self.assertRaisesRegex(mod.DexError, 'missing iget-object'):
            d.field_at(d.method('Lfake/Main;', 'run'), 0, 'nativeLibraryDir')

    def test_corrupt_dex_magic_rejected(self):
        data = b'wrong' + fake_dex()[5:]
        with self.assertRaisesRegex(mod.DexError, 'header'):
            mod.Dex(data)

    def test_corrupt_dex_table_bounds_rejected(self):
        data = bytearray(fake_dex())
        struct.pack_into('<I', data, 60, 0x7fffffff)
        with self.assertRaisesRegex(mod.DexError, 'bounds'):
            mod.Dex(data)

    def test_corrupt_dex_code_bounds_rejected(self):
        data = bytearray(fake_dex())
        struct.pack_into('<I', data, 0x180+12, 0xffffff00)
        with self.assertRaisesRegex(mod.DexError, 'method code'):
            mod.Dex(data)

    def test_missing_string_terminator_rejected(self):
        data = bytearray(fake_dex())
        struct.pack_into('<I', data, 0xc0, 0x270)
        data[0x270] = 1
        data[0x271:] = b'x' * (len(data)-0x271)
        with self.assertRaisesRegex(mod.DexError, 'unterminated'):
            mod.Dex(data)

    def test_uleb128_failure(self):
        with self.assertRaisesRegex(mod.DexError, 'truncated uleb'):
            mod.uleb128(b'\x80', 0)
        with self.assertRaisesRegex(mod.DexError, 'oversized uleb'):
            mod.uleb128(b'\x80\x80\x80\x80\x80', 0)

    def test_wrong_apk_dex_sha_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            apk = Path(tmp) / 'fake.apk'
            with zipfile.ZipFile(apk, 'w') as archive:
                archive.writestr('classes.dex', fake_dex())
            with self.assertRaisesRegex(mod.DexError, 'SHA-256 mismatch'):
                mod.verify_apk(apk, 'portalvr-112')

if __name__=='__main__':
    unittest.main()
