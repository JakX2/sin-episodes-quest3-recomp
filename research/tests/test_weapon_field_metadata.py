"""Tests the standalone PE/descriptor parser; no proprietary binaries required."""
import importlib.util
from pathlib import Path
import struct
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "verify_weapon_field_metadata.py"
spec = importlib.util.spec_from_file_location("field_verifier", SCRIPT)
module = importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def synthetic_pe():
    b = bytearray(0xA00)
    b[:2] = b"MZ"
    struct.pack_into("<I", b, 0x3C, 0x80)
    b[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", b, 0x84, 0x14C, 2)
    struct.pack_into("<H", b, 0x94, 0xE0)
    struct.pack_into("<H", b, 0x98, 0x10B)
    struct.pack_into("<I", b, 0x98 + 28, 0x22000000)
    off = 0x178
    for i, (name, rva, fileoff) in enumerate(((b".rdata", 0x1000, 0x200), (b".data", 0x2000, 0x600))):
        hdr = off + i * 40
        b[hdr:hdr + 8] = name.ljust(8, b"\0")
        struct.pack_into("<IIII", b, hdr + 8, 0x200, rva, 0x200, fileoff)
    b[0x230:0x230 + len(b"m_bReloadsSingly\0")] = b"m_bReloadsSingly\0"
    struct.pack_into("<III", b, 0x620, 6, 0x22001030, 0x508)
    return bytes(b)


class TestPE(unittest.TestCase):
    def test_find_descriptor(self):
        pe = module.PE32(synthetic_pe())
        self.assertEqual(pe.va_to_offset(0x22001030), 0x230)
        self.assertEqual(pe.descriptors("m_bReloadsSingly"), [
            {"descriptor_va": 0x22002020, "type_id": 6, "object_offset": 0x508}
        ])

    def test_reject_corrupted_headers(self):
        b = bytearray(synthetic_pe())
        b[:2] = b"ZZ"
        with self.assertRaises(module.VerificationError):
            module.PE32(bytes(b))

    def test_reject_corrupted_descriptor(self):
        b = bytearray(synthetic_pe())
        struct.pack_into("<I", b, 0x628, 0x509)
        self.assertEqual(module.PE32(bytes(b)).descriptors("m_bReloadsSingly")[0]["object_offset"], 0x509)

    def test_reject_missing_name(self):
        with self.assertRaises(module.VerificationError):
            module.PE32(synthetic_pe()).descriptors("m_bInReload")

    def test_reject_truncated_section(self):
        with self.assertRaises(module.VerificationError):
            module.PE32(synthetic_pe()[:0x700])


if __name__ == "__main__":
    unittest.main()
