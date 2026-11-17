"""Synthetic PE32-only tests. No commercial executable bytes are used."""
import hashlib
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from recover_original_vtable_prefixes import verify_and_recover, PE32


def fake_original():
    data = bytearray(0x1000)
    def w16(p, x): struct.pack_into("<H", data, p, x)
    def w32(p, x): struct.pack_into("<I", data, p, x)
    data[:2] = b"MZ"
    w32(0x3c, 0x80)
    data[0x80:0x84] = b"PE\\x00\\x00".decode("unicode_escape").encode("latin1")
    w16(0x84, 0x14c)
    w16(0x86, 3)
    w16(0x94, 0xe0)
    w16(0x96, 0x2002)
    w16(0x98, 0x10b)
    w32(0x98+28, 0x10000000)
    for i, (name, rva, flags) in enumerate((
        (b".text", 0x1000, 0x60000020),
        (b".rdata", 0x2000, 0x40000040),
        (b".data", 0x3000, 0xc0000040))):
        p = 0x178 + i*40
        data[p:p+len(name)] = name
        w32(p+8, 0x200)
        w32(p+12, rva)
        w32(p+16, 0x200)
        w32(p+20, 0x400+i*0x200)
        w32(p+36, flags)
    # .data singleton, .rdata vtable and MSVC COL, .text targets.
    w32(0x800, 0x10002020)
    w32(0x620, 0x10001010)
    w32(0x624, 0x10001030)
    w32(0x61c, 0x10002080)
    w32(0x680, 0)
    w32(0x684, 0)
    w32(0x68c, 0x10003040)
    w32(0x690, 0x100020a0)
    return data


def spec(data):
    return {"size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
            "base": 0x10000000,
            "pins": (("Synthetic001", 0x10003000, 0x10002020,
                      0x10001010, 2),)}


class OriginalVtableSyntheticTests(unittest.TestCase):
    def test_two_slots_and_atlas_match(self):
        blob = fake_original()
        result = verify_and_recover(bytes(blob), spec(blob), {0x10001010})
        self.assertEqual(result["observed_vtable_slots"], 2)
        self.assertEqual(result["ghidra_exact_function_entries"], 1)
        self.assertEqual(result["interfaces"][0]["slots"][1]["ghidra_exact_function_entry"], False)
        self.assertFalse(result["interfaces"][0]["slots"][0]["newly_verified_method_body"])

    def test_hash_gate_precedes_pe_parse(self):
        blob = fake_original()
        expected = spec(blob)
        blob[0x450] ^= 1
        with self.assertRaisesRegex(ValueError, "SHA256"):
            verify_and_recover(bytes(blob), expected)

    def test_wrong_vptr_rejected(self):
        blob = fake_original()
        expected = spec(blob)
        struct.pack_into("<I", blob, 0x800, 0x10002024)
        expected["sha256"] = hashlib.sha256(blob).hexdigest()
        with self.assertRaisesRegex(ValueError, "vptr"):
            verify_and_recover(bytes(blob), expected)

    def test_non_executable_target_rejected(self):
        blob = fake_original()
        expected = spec(blob)
        struct.pack_into("<I", blob, 0x624, 0x10003040)
        expected["sha256"] = hashlib.sha256(blob).hexdigest()
        with self.assertRaisesRegex(ValueError, "executable"):
            verify_and_recover(bytes(blob), expected)

    def test_wrong_rtti_section_rejected(self):
        blob = fake_original()
        expected = spec(blob)
        struct.pack_into("<I", blob, 0x61c, 0x10001010)
        expected["sha256"] = hashlib.sha256(blob).hexdigest()
        with self.assertRaisesRegex(ValueError, "RTTI"):
            verify_and_recover(bytes(blob), expected)

    def test_truncated_pe_rejected(self):
        with self.assertRaises(ValueError):
            PE32(b"MZ", 0x10000000)

    def test_duplicate_slot_target_rejected(self):
        blob = fake_original()
        expected = spec(blob)
        struct.pack_into("<I", blob, 0x624, 0x10001010)
        expected["sha256"] = hashlib.sha256(blob).hexdigest()
        with self.assertRaisesRegex(ValueError, "duplicate"):
            verify_and_recover(bytes(blob), expected)


if __name__ == "__main__":
    unittest.main()
