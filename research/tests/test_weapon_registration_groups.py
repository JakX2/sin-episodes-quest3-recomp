"""Synthetic checks for Phase 57 helpers; no game files needed."""
import sys
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(ROOT))
import verify_weapon_registration_groups as phase57
from verify_weapon_field_metadata import VerificationError


class TestRegistrationGroups(unittest.TestCase):
    def test_encoded_immediate_store(self):
        self.assertEqual(phase57.immediate_store(0x2251F7F8, 29),
                         bytes.fromhex("c7 05 f8 f7 51 22 1d 00 00 00"))

    def test_mirrored_fields_with_single_exception(self):
        server = [{"field": "m_nNextThinkTick", "type_id": 5, "object_offset": 0x74}]
        client = [{"field": "m_nNextThinkTick", "type_id": 5, "object_offset": 0x84}]
        for i in range(1, 29):
            server.append({"field": f"synthetic_{i}", "type_id": 6, "object_offset": 0x490 + i})
            client.append({"field": f"synthetic_{i}", "type_id": 6, "object_offset": 0x490 + i + 0x2F4})
        result = phase57.assert_mirror(server, client)
        self.assertEqual(result["offset_deltas"], {"0x10": 1, "0x2f4": 28})

    def test_rejects_mismatched_sequence(self):
        with self.assertRaises(VerificationError):
            phase57.assert_mirror([{"field": "a", "type_id": 5, "object_offset": 12}],
                                  [{"field": "b", "type_id": 5, "object_offset": 12}])

    def test_rejects_uniform_object_delta(self):
        with self.assertRaises(VerificationError):
            phase57.assert_mirror([{"field": "m_nNextThinkTick", "type_id": 5, "object_offset": 0x74}],
                                  [{"field": "m_nNextThinkTick", "type_id": 5, "object_offset": 0x368}])

    def test_registration_rejects_wrong_count_store(self):
        class PEStub:
            content = phase57.immediate_store(0x4000, 28) + phase57.immediate_store(0x5000, 0x6000) + bytes(20)
            @staticmethod
            def va_to_offset(x):
                return 0
        with self.assertRaises(VerificationError):
            phase57.verify_registration(PEStub(), "synthetic", 0x6000, 0x1000, 29, 0x4000, 0x5000)

    def test_registration_accepts_correct_count_and_base(self):
        class PEStub:
            content = phase57.immediate_store(0x4000, 29) + phase57.immediate_store(0x5000, 0x6000) + bytes(20)
            @staticmethod
            def va_to_offset(x):
                return 0
        phase57.verify_registration(PEStub(), "synthetic", 0x6000, 0x1000, 29, 0x4000, 0x5000)


if __name__ == "__main__":
    unittest.main()
