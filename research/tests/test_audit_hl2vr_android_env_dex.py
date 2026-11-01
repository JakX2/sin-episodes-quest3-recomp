"""Original-free negative tests for pinned Android DEX environment auditor."""
import importlib.util
from pathlib import Path
import struct
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "audit_hl2vr_android_env_dex.py"
SPEC = importlib.util.spec_from_file_location("phase81_dex", SCRIPT)
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


def fixture_dex():
    data = bytearray(256)
    data[:8] = b"dex\n035\0"
    struct.pack_into("<II", data, 56, 2, 112)
    struct.pack_into("<I", data, 112, 144)
    struct.pack_into("<I", data, 116, 150)
    data[144:150] = b"\x01X\x00\x00\x00\x00"
    data[150:154] = b"\x01Y\x00"
    return bytes(data)


class DexParserTests(unittest.TestCase):
    def test_synthetic_strings(self):
        self.assertEqual(module.dex_strings(fixture_dex()), ["X", "Y"])

    def test_wrong_magic(self):
        with self.assertRaises(module.DexAuditError):
            module.dex_strings(b"broken")

    def test_bad_table_bounds(self):
        blob = bytearray(fixture_dex())
        struct.pack_into("<I", blob, 60, len(blob) - 1)
        with self.assertRaises(module.DexAuditError):
            module.dex_strings(bytes(blob))

    def test_bad_string_offset(self):
        blob = bytearray(fixture_dex())
        struct.pack_into("<I", blob, 112, len(blob) + 1)
        with self.assertRaises(module.DexAuditError):
            module.dex_strings(bytes(blob))

    def test_truncated_uleb(self):
        with self.assertRaises(module.DexAuditError):
            module.read_uleb(b"\x80", 0)

    def test_unpinned_dex_rejected(self):
        with self.assertRaisesRegex(module.DexAuditError, "SHA-256 mismatch"):
            module.audit(fixture_dex())


if __name__ == "__main__":
    unittest.main()
