"""SourceVR filesystem loader verifier: synthetic ELF and instruction tests only."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

SCRIPTS = Path(__file__).resolve().parents[2] / 'scripts'
sys.path.insert(0, str(SCRIPTS))
SPEC = importlib.util.spec_from_file_location('module_search', SCRIPTS / 'verify_sourcevr_module_search.py')
mod = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(mod)


def adrp(pc, target, register=0):
    imm = ((target >> 12) - (pc >> 12)) & ((1 << 21) - 1)
    return 0x90000000 | ((imm & 3) << 29) | (((imm >> 2) & 0x7ffff) << 5) | register


def add(low, register=0):
    return 0x91000000 | ((low & 0xfff) << 10) | (register << 5) | register


def bl(pc, target):
    return 0x94000000 | (((target - pc) // 4) & 0x3ffffff)


def make_elf(broken_branch=False, broken_ref=False, bad_arch=False):
    rodata = b''
    addresses = {}
    for value in ('APP_LIB_PATH', 'APP_MOD_LIB', '%s/lib%s', '%s/%s',
                  'dlopen', 'libdl.so'):
        addresses[value] = 0x20000 + len(rodata)
        rodata += value.encode() + b'\x00'
    code = bytearray(0x300)
    refs = {'APP_LIB_PATH': 0x10020, 'APP_MOD_LIB': 0x10030,
            '%s/lib%s': 0x10040, '%s/%s': 0x10050}
    targets = [(0x10100, 0x10140), (0x10104, 0x10150), (0x10108, 0x10160)]

    def put(pc, op):
        struct.pack_into('<I', code, pc - 0x10000, op)

    for key, pc in refs.items():
        put(pc, adrp(pc, addresses[key]))
        put(pc + 4, add((addresses[key] & 0xfff) + (4 if broken_ref and key == 'APP_MOD_LIB' else 0)))
    for pc, target in targets:
        put(pc, bl(pc, target + (4 if broken_branch and pc == 0x10104 else 0)))
    names = b'\0.text\0.rodata\0.shstrtab\0'
    blob = bytearray(0x5000)
    blob[:6] = b'\x7fELF\x02\x01'
    struct.pack_into('<H', blob, 18, 62 if bad_arch else 183)
    struct.pack_into('<Q', blob, 40, 0x4000)
    struct.pack_into('<HHH', blob, 58, 64, 4, 3)
    blob[0x1000:0x1000+len(code)] = code
    blob[0x2000:0x2000+len(rodata)] = rodata
    blob[0x3000:0x3000+len(names)] = names
    fmt = '<IIQQQQIIQQ'
    for i, (noff, addr, pos, size) in enumerate([
        (0, 0, 0, 0), (1, 0x10000, 0x1000, len(code)),
        (7, 0x20000, 0x2000, len(rodata)), (15, 0, 0x3000, len(names)),
    ]):
        struct.pack_into(fmt, blob, 0x4000 + i*64,
                         noff, 1, 0, addr, pos, size, 0, 0, 1, 0)
    return bytes(blob)


def fixture_profile(blob):
    return {
        'sha256': hashlib.sha256(blob).hexdigest(),
        'env_refs': {'APP_LIB_PATH': 0x10020, 'APP_MOD_LIB': 0x10030},
        'formats': {'%s/lib%s': 0x10040, '%s/%s': 0x10050},
        'loader_to_search': (0x10100, 0x10140),
        'search_to_dlopen_wrapper': (0x10104, 0x10150),
        'wrapper_to_dlopen_plt': (0x10108, 0x10160),
    }


class ModuleSearchTests(unittest.TestCase):
    def run_fixture(self, blob, expected=None):
        mod.PROFILES['fixture'] = fixture_profile(blob)
        try:
            if expected:
                with self.assertRaisesRegex(mod.VerificationError, expected):
                    mod.verify_bytes(blob, 'fixture')
            else:
                return mod.verify_bytes(blob, 'fixture')
        finally:
            del mod.PROFILES['fixture']

    def test_bl_decoder_and_rejection(self):
        self.assertEqual(mod.decode_bl(bl(0x10100, 0x10140), 0x10100), 0x10140)
        self.assertIsNone(mod.decode_bl(0xd65f03c0, 0x10100))

    def test_synthetic_elf(self):
        output = self.run_fixture(make_elf())
        self.assertTrue(output['sha256_verified'])
        self.assertTrue(output['native_dlopen_import_present'])
        self.assertFalse(output['external_sin_game_module_loaded'])
        self.assertFalse(output['quest_runtime_verified'])

    def test_modified_branch_rejected(self):
        self.run_fixture(make_elf(broken_branch=True), 'BL target mismatch')

    def test_wrong_environment_reference_rejected(self):
        self.run_fixture(make_elf(broken_ref=True), 'instruction reference mismatch')

    def test_wrong_sha_rejected(self):
        blob = make_elf()
        mod.PROFILES['fixture'] = {**fixture_profile(blob), 'sha256': 'f'*64}
        try:
            with self.assertRaisesRegex(mod.VerificationError, 'SHA-256 mismatch'):
                mod.verify_bytes(blob, 'fixture')
        finally:
            del mod.PROFILES['fixture']

    def test_not_arm64_rejected(self):
        self.run_fixture(make_elf(bad_arch=True), 'AArch64')

    def test_missing_import_rejected(self):
        blob = make_elf().replace(b'libdl.so\x00', b'libdx.so\x00')
        self.run_fixture(blob, 'libdl dependency')

    def test_truncated_section_table_rejected(self):
        blob = bytearray(make_elf())
        struct.pack_into('<Q', blob, 40, len(blob)-2)
        self.run_fixture(bytes(blob), 'section table')


if __name__ == '__main__':
    unittest.main()
