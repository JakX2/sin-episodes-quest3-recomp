"""Synthetic ARM64/ELF regression for real native filesystem module-load path."""
from pathlib import Path
import importlib.util
import struct
import sys
import unittest
from unittest import mock

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
import audit_sourcevr_filesystem_gamebin_route as a


def adrp(pc, dest, reg):
    diff = ((dest & ~4095) - (pc & ~4095)) // 4096 & 0x1fffff
    return 0x90000000 | ((diff & 3) << 29) | ((diff >> 2 & 0x7ffff) << 5) | reg


def add(reg, dest):
    return 0x91000000 | ((dest & 4095) << 10) | (reg << 5) | reg


def bl(pc, dest):
    return 0x94000000 | (((dest-pc)//4) & 0x3ffffff)


class FakeELF:
    """Only locally synthesized words and strings, not a game binary."""
    def __init__(self, launcher=False):
        self.words = {}
        self.is_launcher = launcher
        self.data = bytearray()
        self.sections = {}
        self.strings = {}
        self.qwords = {}
        if launcher:
            for pc, reg, dest, string in (
                (0x37308, 0, 0xde53, "APP_LIB_PATH"),
                (0x37368, 0, 0xde53, "APP_LIB_PATH"),
                (0x3737c, 0, 0x1149e, "APP_MOD_LIB"),
            ):
                self.make_string(pc, reg, dest, string)
            for site, target in (
                (0x37310, 0x7a700), (0x37328, 0x7aab0),
                (0x37378, 0x7a810), (0x3738c, 0x7a810),
            ):
                self.words[site] = bl(site, target)
            self.words[0x37314] = 0xb50003e0
        else:
            self.qwords = {
                a.FS_TYPEINFO+8: a.FS_TYPE_NAME,
                a.FS_VTABLE-8: a.FS_TYPEINFO,
                a.FS_VTABLE+0xc8: a.FS_LOADER,
            }
            self.strings[a.FS_TYPE_NAME] = b"15CBaseFileSystem\x00"
            for pc, reg, dest, string in (
                (0x6f0c0, 0, 0x7cf8, "APP_LIB_PATH"),
                (0x6f0d0, 0, 0x8ae1, "APP_MOD_LIB"),
                (0x6f014, 1, 0x92e9, ".so"),
                (0x6f1a0, 2, 0x8fd5, "lib%s"),
            ):
                self.make_string(pc, reg, dest, string)
            for site, target in (
                (0x2dec8, a.FS_LOAD_BY_NAME),
                (0x6f108, a.FS_DLOPEN_WRAPPER),
                (0x6f1d0, a.FS_DLOPEN_WRAPPER),
                (0x6ee2c, 0x94c30),
                (0x6f0c8, 0x945b0),
                (0x6f0d8, 0x945b0),
            ):
                self.words[site] = bl(site, target)
            self.words.update({
                0x6f184: 0x794803e8,
                0x6f188: 0x528d2d8a,
                0x6f190: 0x39500be9,
                0x6f194: 0x52800c48,
                0x6f19c: 0x54000160,
            })

    def make_string(self, pc, reg, dest, string):
        self.words[pc] = adrp(pc, dest, reg)
        self.words[pc+4] = add(reg, dest)
        self.strings[dest] = string.encode() + b"\x00"

    def word(self, va):
        return self.words.get(va, 0)

    def at(self, va, length):
        if va in self.qwords and length == 8:
            return struct.pack("<Q", self.qwords[va])
        if va in self.strings:
            return self.strings[va][:length]
        raise a.AuditError(f"invalid synthetic VA {va:#x}")


def fake_plt(elf, symbol):
    # The filesystem and launcher are distinct ELF builds, each with its own
    # import order and therefore a different getenv@PLT address.
    if elf.is_launcher:
        return {"getenv": 0x7a700, "setenv": 0x7a810,
                "dladdr": 0x7aab0}[symbol]
    return {"dlopen": 0x94c30, "getenv": 0x945b0}[symbol]


class LoaderTests(unittest.TestCase):
    def test_complete_filesystem_fixture(self):
        with mock.patch.object(a, "imported_plt", side_effect=fake_plt):
            result = a.verify_filesystem(FakeELF())
        self.assertEqual(result["class_rtti"], "CBaseFileSystem")
        self.assertEqual(result["class_module_load_virtual_slot_offset"], "0xc8")
        self.assertEqual(result["real_dlopen_plt_va"], "0x94c30")

    def test_complete_launcher_fixture(self):
        with mock.patch.object(a, "imported_plt", side_effect=fake_plt):
            result = a.verify_launcher(FakeELF(launcher=True))
        self.assertIn("APP_MOD_LIB", result["bootstrap_env"])

    def fs_error(self, change, pattern):
        fixture = FakeELF()
        change(fixture)
        with mock.patch.object(a, "imported_plt", side_effect=fake_plt):
            with self.assertRaisesRegex(a.AuditError, pattern):
                a.verify_filesystem(fixture)

    def launcher_error(self, change, pattern):
        fixture = FakeELF(launcher=True)
        change(fixture)
        with mock.patch.object(a, "imported_plt", side_effect=fake_plt):
            with self.assertRaisesRegex(a.AuditError, pattern):
                a.verify_launcher(fixture)

    def test_rtti_changed(self):
        self.fs_error(lambda e: e.qwords.__setitem__(a.FS_TYPEINFO+8, 5),
                      "RTTI name pointer")

    def test_class_rtti_name_changed(self):
        self.fs_error(lambda e: e.strings.__setitem__(
            a.FS_TYPE_NAME, b"15CUnknownSystem\x00"), "RTTI class")

    def test_vtable_typeinfo_changed(self):
        self.fs_error(lambda e: e.qwords.__setitem__(a.FS_VTABLE-8, 5),
                      "vtable")

    def test_virtual_loader_target_changed(self):
        self.fs_error(lambda e: e.qwords.__setitem__(
            a.FS_VTABLE+0xc8, 0x2dd94), "virtual slot")

    def test_virtual_method_does_not_route_to_module_loader(self):
        self.fs_error(lambda e: e.words.__setitem__(
            0x2dec8, bl(0x2dec8, 0x6efc4)), "virtual .*handler")

    def test_wrong_dlopen_wrapper_call(self):
        self.fs_error(lambda e: e.words.__setitem__(
            0x6ee2c, bl(0x6ee2c, 0x94c40)), "real dlopen")

    def test_missing_mod_lib_environment(self):
        self.fs_error(lambda e: e.strings.__setitem__(
            0x8ae1, b"APP_MOD_BAD\x00"), "APP_MOD_LIB")

    def test_missing_lib_path_environment(self):
        self.fs_error(lambda e: e.strings.__setitem__(
            0x7cf8, b"APP_BAD_PATH\x00"), "APP_LIB_PATH")

    def test_bad_module_prefix(self):
        self.fs_error(lambda e: e.strings.__setitem__(
            0x8fd5, b"bin%s\x00"), "lib%s")

    def test_bad_so_extension(self):
        self.fs_error(lambda e: e.strings.__setitem__(
            0x92e9, b".dll\x00"), ".so")

    def test_wrong_launcher_setenv(self):
        self.launcher_error(lambda e: e.words.__setitem__(
            0x3738c, bl(0x3738c, 0x7a700)), "module path setenv")

    def test_wrong_launcher_dladdr(self):
        self.launcher_error(lambda e: e.words.__setitem__(
            0x37328, bl(0x37328, 0x7a810)), "dladdr")

    def test_launcher_missing_early_exit(self):
        self.launcher_error(lambda e: e.words.__setitem__(
            0x37314, 0), "bootstrap")

    def test_plt_symbol_resolution_from_synthetic_elf_relocation(self):
        fixture = FakeELF()
        data = bytearray(300)
        data[0:8] = b"\x00dlopen\x00"
        struct.pack_into("<I", data, 64+24, 1)
        struct.pack_into("<QQq", data, 180, 0x40000,
                         (1 << 32) | 0x402, 0)
        fixture.data = data
        def section(va, off, length, entry=0):
            return (0,1,0,va,off,length,0,0,1,entry)
        fixture.sections = {
            ".dynstr": section(0x10000, 0, 8),
            ".dynsym": section(0x20000, 64, 48, 24),
            ".rela.plt": section(0x30000, 180, 24, 24),
            ".plt": section(0x40000, 240, 64),
        }
        self.assertEqual(a.imported_plt(fixture, "dlopen"), 0x40020)
        struct.pack_into("<QQq", fixture.data, 180, 0x40000,
                         (1 << 32) | 0x401, 0)
        with self.assertRaisesRegex(a.AuditError, "JUMP_SLOT"):
            a.imported_plt(fixture, "dlopen")

    def test_all_profiles_share_same_pinned_filesystem(self):
        self.assertEqual(len(a.ENGINE_WITNESSES), 7)
        self.assertEqual(len(a.FILESYSTEM_SHA256), 64)
        self.assertEqual(len(a.HL2_LAUNCHER_SHA256), 64)


if __name__ == "__main__":
    unittest.main()
