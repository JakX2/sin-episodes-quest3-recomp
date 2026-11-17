import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from abi_vtable_evidence import reconcile

BASE = {"schema": "sin-vtable-evidence-v1", "observations": [
    {"module": "client.dll", "interface": "VClient011", "slot": 0,
     "target_va": "0x10001234", "provenance": "ghidra-vtable-pointer",
     "signature_reviewed": False, "signature": None}]}

class EvidenceTests(unittest.TestCase):
    def change(self, **changes):
        d = copy.deepcopy(BASE)
        d["observations"][0].update(changes)
        return d

    def test_candidate_not_verified(self):
        r = reconcile(BASE)
        self.assertEqual(r["interfaces"][0]["candidate_slots"], 1)
        self.assertEqual(r["verified_original_methods_added"], 0)

    def test_reviewed_signature(self):
        r = reconcile(self.change(signature_reviewed=True, signature="reviewed ABI"))
        self.assertEqual(r["interfaces"][0]["reviewed_signatures"], 1)

    def test_duplicate(self):
        d = copy.deepcopy(BASE)
        d["observations"] *= 2
        with self.assertRaises(ValueError): reconcile(d)

    def test_null_address(self):
        with self.assertRaises(ValueError): reconcile(self.change(target_va="0x00000000"))

    def test_unreviewed_signature(self):
        with self.assertRaises(ValueError): reconcile(self.change(signature="guess"))

    def test_bool_slot(self):
        with self.assertRaises(ValueError): reconcile(self.change(slot=True))

    def test_noncanonical_address(self):
        with self.assertRaises(ValueError): reconcile(self.change(target_va="10001234"))

    def test_unknown_interface(self):
        with self.assertRaises(ValueError): reconcile(self.change(interface="VClient999"))

if __name__ == "__main__":
    unittest.main()
