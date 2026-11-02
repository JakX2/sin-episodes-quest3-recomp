import unittest

from tools.sourcevr_java_env_audit import (
    Dex, audit, decode_uleb, require, EXPECTED_SHA256,
)


class SourceVRDexTests(unittest.TestCase):
    def test_decode_uleb(self):
        self.assertEqual(decode_uleb(b"\x7f", 0), (127, 1))
        self.assertEqual(decode_uleb(b"\xac\x02", 0), (300, 2))
        with self.assertRaises(ValueError):
            decode_uleb(b"\x80", 0)
        with self.assertRaises(ValueError):
            decode_uleb(b"\x80" * 5, 0)

    def test_fail_closed_on_unknown_revision(self):
        with self.assertRaisesRegex(ValueError, "unsupported SourceVR classes.dex"):
            audit(b"not a DEX")
        with self.assertRaisesRegex(ValueError, "invalid DEX"):
            Dex(b"not a DEX")
        with self.assertRaises(ValueError):
            require(False, "synthetic mismatch")
        self.assertEqual(len(EXPECTED_SHA256), 64)

    def test_header_boundaries(self):
        bad = bytearray(112)
        bad[:8] = b"dex\n039\0"
        # Legitimate size but bogus string table.
        import struct
        struct.pack_into("<I", bad, 32, len(bad))
        struct.pack_into("<II", bad, 56, 1, 2000)
        with self.assertRaises(ValueError):
            Dex(bytes(bad))


if __name__ == "__main__":
    unittest.main()
