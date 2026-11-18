"""Synthetic-only tests for public ABI queue comparisons."""
import importlib.util
from pathlib import Path
import tempfile
import json
import unittest

ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT / "tools"))
from abi_evidence_queue import report, validate
from abi_queue_delta import compare, load_queue

BASE = {"module": "client", "interface": "VClient011", "rva": "0x0020",
        "evidence": "candidate"}


class QueueDeltaTests(unittest.TestCase):
    def test_claim_transition_and_stable_fingerprint(self):
        old = validate({"records": [BASE]})
        new = validate({"records": [dict(BASE, evidence="inferred")]})
        delta = compare(old, new)
        self.assertEqual(delta["changed"][0]["tier_direction"], "up")
        self.assertEqual(delta["added"], [])
        self.assertEqual(delta["removed"], [])
        self.assertNotEqual(delta["before_sha256"], delta["after_sha256"])
        self.assertIn("metadata claims", delta["note"])

    def test_add_remove_and_downgrade(self):
        old = validate({"records": [dict(BASE, evidence="inferred")]})
        new = validate({"records": [BASE, dict(BASE, rva="0x30")]})
        delta = compare(old, new)
        self.assertEqual(delta["changed"][0]["tier_direction"], "down")
        self.assertEqual(len(delta["added"]), 1)
        self.assertEqual(len(delta["removed"]), 0)

    def test_report_integrity_gate(self):
        records = validate({"records": [BASE]})
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "queue.json"
            path.write_text(json.dumps(report(records)), encoding="utf-8")
            self.assertEqual(load_queue(path), records)
            corrupted = report(records)
            corrupted["counts"]["engine_tested"] = 100
            path.write_text(json.dumps(corrupted), encoding="utf-8")
            with self.assertRaises(ValueError):
                load_queue(path)

    def test_empty_queue_and_malformed_root(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "empty.json"
            path.write_text('{"records":[]}', encoding="utf-8")
            self.assertEqual(load_queue(path), [])
            path.write_text('[]', encoding="utf-8")
            with self.assertRaises(ValueError):
                load_queue(path)


if __name__ == "__main__":
    unittest.main()
