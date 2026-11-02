import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from sourcevr_mod_dispatch_audit import (
    ELF64, adrp, add, audit, bl, cbnz, parse_load,
)

def fake_elf():
    data = bytearray(0x400)
    data[:16] = b"\x7fELF\x02\x01\x01" + b"\0" * 9
    struct.pack_into("<HHIQQQIHHHHHH", data, 16,
                     3, 183, 1, 0, 0, 0x300, 0, 64, 0, 0, 64, 4, 1)
    names = b"\0.shstrtab\0.text\0.rodata\0"
    data[0x280:0x280+len(names)] = names
    fmt = "<IIQQQQIIQQ"
    struct.pack_into(fmt, data, 0x300+64, 1, 3, 0, 0, 0x280, len(names), 0, 0, 1, 0)
    struct.pack_into(fmt, data, 0x300+128, 11, 1, 6, 0x1000, 0x100, 0x50, 0, 0, 4, 0)
    struct.pack_into(fmt, data, 0x300+192, 17, 1, 2, 0x3000, 0x200, 0x80, 0, 0, 1, 0)
    struct.pack_into("<II", data, 0x100, 0xd0000000, 0x91000000)
    data[0x200:0x200+len(b"APP_MOD_LIB\0")] = b"APP_MOD_LIB\0"
    return bytes(data)

def load_fixture():
    names = ("libclient.so", "libengine.so", "libserver.so", "liblauncher.so")
    return "\n".join(f"sourcevr/v1/native/blobs/{i:064x}/{name}"
                     for i, name in enumerate(names)) + "\n"

class DispatchTests(unittest.TestCase):
    def test_arm64_instruction_decoders(self):
        self.assertEqual(adrp(0xd0000000, 0x1000), (0, 0x3000))
        self.assertEqual(add(0x91000000), (0, 0, 0))
        self.assertEqual(bl(0x94000006, 0x1008), 0x1020)
        self.assertEqual(cbnz(0xb5000080, 0x1000), 0x1010)
        self.assertIsNone(cbnz(0xd65f03c0, 0x1000))

    def test_elf_address_reference_and_bounds(self):
        elf = ELF64(fake_elf())
        self.assertEqual(elf.reference(0x1000, 0x1004), "APP_MOD_LIB")
        with self.assertRaises(ValueError):
            elf.read(0x9999, 4)
        with self.assertRaises(ValueError):
            ELF64(b"invalid ELF")

    def test_manifest_order_and_validation(self):
        result = parse_load(load_fixture(), "hl2")
        self.assertEqual(result["staged_positions"]["libclient.so"], 0)
        self.assertTrue(result["client_server_before_launcher"])
        self.assertFalse(result["server_before_engine"])
        with self.assertRaises(ValueError):
            parse_load(load_fixture().replace("libserver.so", "libengine.so"), "hl2")
        with self.assertRaises(ValueError):
            parse_load(load_fixture().replace("liblauncher.so", "libvphysics.so"), "hl2")
        with self.assertRaises(ValueError):
            parse_load(load_fixture() + "../unexpected.so\n", "hl2")

    def test_unknown_binary_versions_fail_closed(self):
        with self.assertRaisesRegex(ValueError, "unsupported binary revisions"):
            audit("hl2", fake_elf(), fake_elf(), load_fixture())

if __name__ == "__main__":
    unittest.main()
