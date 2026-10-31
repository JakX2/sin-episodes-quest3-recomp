"""No game data: fixtures are synthetic packed descriptors or isolated code bytes."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_descriptor_semantics import (
    FIELD_SAVE, FIELD_INSENDTABLE, FIELD_MODELINDEX, FIELD_KEY,
    check_counter, check_mirror, check_repeated_field, descriptor_flags,
)


class SyntheticPE:
    def __init__(self, raw: bytes):
        self.content = raw
    def va_to_offset(self, va: int) -> int:
        return va


class TestWeaponDescriptorSemantics(unittest.TestCase):
    def test_packed_flag_word(self):
        data = bytearray(52)
        struct.pack_into("<HH", data, 16, 1, FIELD_INSENDTABLE | FIELD_MODELINDEX)
        self.assertEqual(descriptor_flags(SyntheticPE(bytes(data)), 0), {"field_size": 1, "flags": 0x900})

    def test_saved_flag(self):
        data = bytearray(52)
        struct.pack_into("<HH", data, 16, 1, FIELD_SAVE | FIELD_KEY)
        self.assertEqual(descriptor_flags(SyntheticPE(bytes(data)), 0)["flags"], 6)

    def test_wrong_flags_rejected(self):
        with self.assertRaises(VerificationError):
            check_counter([{"field_size": 1, "flags": FIELD_SAVE}], {FIELD_INSENDTABLE: 1}, "test")

    def test_bad_field_size_rejected(self):
        with self.assertRaises(VerificationError):
            check_counter([{"field_size": 2, "flags": FIELD_SAVE}], {FIELD_SAVE: 1}, "test")

    def test_mirrored_fields(self):
        a = {"field": "m_bInReload", "type_id": 6, "flags": 2, "field_size": 1}
        check_mirror([a], [dict(a)])

    def test_mismatch_rejected(self):
        a = {"field": "m_bInReload", "type_id": 6, "flags": 2, "field_size": 1}
        with self.assertRaises(VerificationError):
            check_mirror([a], [{**a, "flags": 0x100}])

    def test_duplicate_idle_descriptors(self):
        check_repeated_field([{"field": "m_flTimeWeaponIdle", "index": 7, "flags": 0x100},
                              {"field": "m_flTimeWeaponIdle", "index": 13, "flags": 0x100}])

    def test_wrong_duplicate_order_rejected(self):
        with self.assertRaises(VerificationError):
            check_repeated_field([{"field": "m_flTimeWeaponIdle", "index": 13, "flags": 0x100},
                                  {"field": "m_flTimeWeaponIdle", "index": 7, "flags": 0x100}])


if __name__ == "__main__":
    unittest.main()
