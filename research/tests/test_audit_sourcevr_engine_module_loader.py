"""Synthetic AArch64 game-module loader regression, no original binaries."""
import importlib
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
import audit_sourcevr_engine_module_loader as a


def adrp(pc, target, reg):
    disp = ((target & ~4095) - (pc & ~4095)) // 4096 & 0x1fffff
    return 0x90000000 | ((disp & 3) << 29) | (
        ((disp >> 2) & 0x7ffff) << 5) | reg


def add(reg, target):
    return 0x91000000 | ((target & 4095) << 10) | (reg << 5) | reg


def bl(pc, target):
    return 0x94000000 | (((target - pc) // 4) & 0x3ffffff)


def branch(pc, target):
    return 0x14000000 | (((target - pc) // 4) & 0x3ffffff)


class FakeEngine:
    """Only tiny fabricated words/string/ELF relocation entries."""
    def __init__(self):
        self.server = 0x10000
        self.client = 0x11000
        self.helper = 0x12000
        self.strings = {
            0x21000: b"server.so\x00",
            0x21020: b"GAMEBIN\x00",
            0x21040: b"ServerGameDLL%03d\x00",
            0x21060: b"client\x00",
            0x21080: b"VClient017\x00",
            0x210a0: b"CreateInterface\x00",
        }
        self.words = {}
        self.data = bytearray(0x300)
        # A minimal .dynstr, two dynamic symbols, one .rela.plt and PLT.
        self.data[0:7] = b"\x00dlsym\x00"
        struct.pack_into("<I", self.data, 0x40+24, 1)
        struct.pack_into("<QQq", self.data, 0x100,
                         0x45000, (1 << 32) | 0x402, 0)
        def section(va, off, size, entsize=0):
            return (0, 1, 0, va, off, size, 0, 0, 1, entsize)
        self.sections = {
            ".dynstr": section(0x80000, 0x0, 7),
            ".dynsym": section(0x81000, 0x40, 48, 24),
            ".rela.plt": section(0x82000, 0x100, 24, 24),
            ".plt": section(0x30000, 0x180, 64),
        }
        s, c, h = self.server, self.client, self.helper
        for va, reg, target in (
            (s-0x50, 1, 0x21000),
            (s-0x48, 2, 0x21020),
            (s, 1, 0x21040),
            (c-0x48, 1, 0x21060),
            (c-0x3c, 2, 0x21020),
            (c, 0, 0x21080),
            (h+4, 1, 0x210a0),
        ):
            self.words[va] = adrp(va, target, reg)
            self.words[va+4] = add(reg, target)
        self.words.update({
            s-0x40: 0xf9400008, s-0x38: 0xf9406508,
            s-0x34: 0xd63f0100, s-0x1c: bl(s-0x1c, h),
            s+0x24: 0xd63f0100, s-4: 0x52800148,
            s+0x48: 0x51000502, s+0x50: 0x7100111f,
            s+0x54: 0x5400066d, s+0x70: 0xd63f0100,
            c-0x30: 0xf9400100, c-0x2c: 0xf9400008,
            c-0x28: 0xf9406508, c-0x24: 0xd63f0100,
            c-0x10: bl(c-0x10, h),
            c+0xc: 0xd63f0100,
            h: 0xb4000080,
            h+12: branch(h+12, 0x30020),
        })

    def word(self, va):
        return self.words.get(va, 0)

    def at(self, va, n):
        if va in self.strings:
            return self.strings[va][:n]
        raise a.AuditError(f"missing synthetic string VA {va:#x}")


class ModuleLoaderTest(unittest.TestCase):
    def verify(self, e=None):
        e = e or FakeEngine()
        return a.verify_engine(e, e.server, e.client, e.helper)

    def test_valid_factory_paths(self):
        result = self.verify()
        self.assertEqual(result["server_version_first"], 10)
        self.assertEqual(result["server_version_last"], 4)
        self.assertEqual(result["search_path"], "GAMEBIN")
        self.assertEqual(result["dlsym_plt_va"], "0x30020")

    def test_server_filename_changed(self):
        e=FakeEngine()
        e.strings[0x21000]=b"client.so\x00"
        with self.assertRaisesRegex(a.AuditError, "server.so"):
            self.verify(e)

    def test_client_filename_changed(self):
        e=FakeEngine()
        e.strings[0x21060]=b"server\x00"
        with self.assertRaisesRegex(a.AuditError, "client"):
            self.verify(e)

    def test_search_path_changed(self):
        e=FakeEngine()
        e.strings[0x21020]=b"MODBIN\x00"
        with self.assertRaisesRegex(a.AuditError, "GAMEBIN"):
            self.verify(e)

    def test_client_version_changed(self):
        e=FakeEngine()
        e.strings[0x21080]=b"VClient011\x00"
        with self.assertRaisesRegex(a.AuditError, "VClient017"):
            self.verify(e)

    def test_server_version_format_changed(self):
        e=FakeEngine()
        e.strings[0x21040]=b"ServerGameDLL%02d\x00"
        with self.assertRaisesRegex(a.AuditError, "ServerGameDLL"):
            self.verify(e)

    def test_server_virtual_load_slot_changed(self):
        e=FakeEngine()
        e.words[e.server-0x38]=0xf9406108
        with self.assertRaisesRegex(a.AuditError, "server loader"):
            self.verify(e)

    def test_client_virtual_load_slot_changed(self):
        e=FakeEngine()
        e.words[e.client-0x28]=0xf9406908
        with self.assertRaisesRegex(a.AuditError, "client loader"):
            self.verify(e)

    def test_server_version_floor_was_not_three(self):
        e=FakeEngine()
        e.words[e.server+0x50]=0x71000d1f
        with self.assertRaisesRegex(a.AuditError, "minimum server version"):
            self.verify(e)

    def test_server_version_not_initially_ten(self):
        e=FakeEngine()
        e.words[e.server-4]=0x52800128
        with self.assertRaisesRegex(a.AuditError, "first server DLL version"):
            self.verify(e)

    def test_dlsym_helper_changes(self):
        e=FakeEngine()
        e.words[e.helper+12]=branch(e.helper+12, 0x30030)
        with self.assertRaisesRegex(a.AuditError, "dlsym PLT"):
            self.verify(e)

    def test_dlsym_relocation_must_be_jump_slot(self):
        e=FakeEngine()
        struct.pack_into("<QQq",e.data,0x100,0x45000,(1<<32)|0x401,0)
        with self.assertRaisesRegex(a.AuditError, "JUMP_SLOT"):
            self.verify(e)

    def test_dlsym_import_absent(self):
        e=FakeEngine()
        e.data[1:6]=b"other"
        with self.assertRaisesRegex(a.AuditError, "dlsym"):
            self.verify(e)

    def test_server_and_client_factory_helper_same(self):
        e=FakeEngine()
        e.words[e.client-0x10]=bl(e.client-0x10,e.helper+4)
        with self.assertRaisesRegex(a.AuditError, "client factory"):
            self.verify(e)

    def test_profile_hash_coverage(self):
        self.assertEqual(len(a.ENGINE_WITNESSES),7)
        self.assertEqual(len({v[0] for v in a.ENGINE_WITNESSES.values()}),7)
        self.assertEqual(set(a.ENGINE_WITNESSES),
                         set(a.PROFILE_SHAS))

    def test_negative_relative_branch(self):
        va=0x12000
        self.assertEqual(a.decode_b(va, branch(va,va-40)),va-40)
        with self.assertRaisesRegex(a.AuditError,"unconditional B"):
            a.decode_b(va,0)

if __name__=="__main__":
    unittest.main()
