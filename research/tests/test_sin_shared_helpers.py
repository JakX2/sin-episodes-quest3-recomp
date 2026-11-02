"""Synthetic and ownership-aware shared gameplay helper tests."""
from __future__ import annotations
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_shared_helpers import shared_helpers
from verify_gameplay_entity_registry import SERVER_SHA256, CLIENT_SHA256


def report(caller: str, callee: str, *, side="server.dll", digest=SERVER_SHA256):
    return {
        "dll": side,
        "sha256": digest,
        "layers": [
            {"depth": 0, "functions": [{
                "start_va": caller,
                "vtable_origin": [{"virtual_slot": 25}],
                "direct_callees": [{"va": callee, "count": 2}],
            }]},
            {"depth": 1, "functions": [{
                "start_va": callee,
                "direct_callees": [{"va": "0x22111111", "count": 1}],
            }]},
        ],
    }


class SharedHelperTests(unittest.TestCase):
    def test_two_independent_game_classes_share_a_real_callee(self):
        data = shared_helpers({
            "grunt": report("0x22350000", "0x22287CB0"),
            "phone": report("0x22302000", "0x22287CB0"),
            "assault": report("0x22313000", "0x22287CB0"),
        })
        self.assertEqual(data["distinct_shared_direct_callees"], 1)
        found = data["shared_helpers"][0]
        self.assertEqual(found["distinct_class_groups"], 3)
        self.assertEqual(found["total_call_instructions"], 6)
        self.assertNotIn("0x22111111", [x["callee_va"] for x in data["shared_helpers"]])

    def test_same_class_two_calls_do_not_fake_shared_class_evidence(self):
        d = report("0x22350000", "0x22287CB0")
        d["layers"][0]["functions"].append({
            "start_va": "0x22352000",
            "direct_callees": [{"va": "0x22287CB0", "count": 10}],
        })
        data = shared_helpers({
            "grunt": d,
            "phone": report("0x22302000", "0x22265000"),
        })
        self.assertEqual(data["distinct_shared_direct_callees"], 0)

    def test_cross_dll_is_rejected_even_with_matching_address(self):
        with self.assertRaisesRegex(ValueError, "different original"):
            shared_helpers({
                "grunt": report("0x22350000", "0x22287CB0"),
                "client": report("0x24220000", "0x22287CB0",
                                 side="client.dll", digest=CLIENT_SHA256),
            })

    def test_sha_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "fingerprint"):
            shared_helpers({
                "a": report("0x22350000", "0x22287CB0"),
                "b": report("0x22320000", "0x22287CB0", digest="wrong"),
            })

    def test_requires_separate_groups_and_depth_zero(self):
        with self.assertRaisesRegex(ValueError, "at least two"):
            shared_helpers({"only_one": report("0x1", "0x2")})
        bad = report("0x22350000", "0x22287CB0")
        bad["layers"][0]["depth"] = 1
        with self.assertRaisesRegex(ValueError, "depth-zero"):
            shared_helpers({"a": bad, "b": report("0x22300000", "0x22287CB0")})


if __name__ == "__main__":
    unittest.main()
