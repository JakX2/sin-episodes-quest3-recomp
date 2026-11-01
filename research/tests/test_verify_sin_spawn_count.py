"""Synthetic original-free PE32 verifier regressions (no SiN DLL in repo)."""
import hashlib
import struct
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import verify_sin_spawn_count as v


def synthetic_pe():
    blob = bytearray(2048)
    blob[:2] = b"MZ"
    struct.pack_into("<I", blob, 0x3c, 0x80)
    blob[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HHIIIHH", blob, 0x84, 0x14c, 1, 0, 0, 0, 224, 0)
    opt = 0x98
    struct.pack_into("<H", blob, opt, 0x10b)
    struct.pack_into("<I", blob, opt + 28, 0x22000000)
    sh = opt + 224
    blob[sh:sh+8] = b".text\x00\x00\x00"
    struct.pack_into("<IIII", blob, sh+8, 0x400, 0x1000, 0x400, 0x400)
    blob[0x408:0x40a] = b"\xab\xcd"
    return bytes(blob)


class VerifyOriginalDLLTests(unittest.TestCase):
    def test_map_section_rva(self):
        self.assertEqual(v.read_pe_va(synthetic_pe(), 0x22001008, 2), b"\xab\xcd")

    def test_bad_magic_rejected(self):
        with self.assertRaises(v.PEVerificationError):
            v.read_pe_va(b"bad", 0x22001008, 2)

    def test_bad_arch_rejected(self):
        blob = bytearray(synthetic_pe())
        struct.pack_into("<H", blob, 0x84, 0x8664)
        with self.assertRaisesRegex(v.PEVerificationError, "i386"):
            v.read_pe_va(bytes(blob), 0x22001008, 2)

    def test_bad_rva_rejected(self):
        with self.assertRaisesRegex(v.PEVerificationError, "address"):
            v.read_pe_va(synthetic_pe(), 0x21000000, 2)

    def test_truncated_section_data(self):
        with self.assertRaisesRegex(v.PEVerificationError, "truncated"):
            v.read_pe_va(synthetic_pe()[:0x450], 0x22001100, 4)

    def test_unknown_address(self):
        with self.assertRaises(v.PEVerificationError):
            v.read_pe_va(synthetic_pe(), 0x22004000, 2)

    def test_unpinned_dll_rejected(self):
        with self.assertRaisesRegex(v.PEVerificationError, "SHA-256"):
            v.verify(synthetic_pe())

    def test_full_synthetic_branch_report(self):
        blob = synthetic_pe()
        original = (v.EXPECTED_SHA256, v.ZERO_VA, v.SIGNATURES)
        v.EXPECTED_SHA256 = hashlib.sha256(blob).hexdigest()
        v.ZERO_VA = 0x22001004
        v.SIGNATURES = {0x22001008: b"\xab\xcd"}
        try:
            result = v.verify(blob)
            self.assertTrue(result["original_server_dll_sha256_verified"])
            self.assertFalse(result["original_code_executed"])
        finally:
            v.EXPECTED_SHA256, v.ZERO_VA, v.SIGNATURES = original


if __name__ == "__main__":
    unittest.main()
