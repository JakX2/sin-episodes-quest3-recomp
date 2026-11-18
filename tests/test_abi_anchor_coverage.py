"""Synthetic batch coverage regression tests; no original DLL content."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from abi_evidence_queue import ANCHORS, validate
from abi_anchor_coverage import summarize


class AnchorCoverageTests(unittest.TestCase):
    def test_empty_queue_identifies_all_six_missing(self):
        result = summarize(validate({"records": []}))
        self.assertEqual(result["missing_anchors"], list(ANCHORS))
        self.assertEqual(result["total_records"], 0)

    def test_coverage_is_per_interface_not_per_module(self):
        records = validate({"records": [
            {"module": "client", "interface": "VClient011", "rva": "0x0002", "evidence": "inferred"},
            {"module": "server", "interface": "ServerGameDLL003", "rva": "0x1", "evidence": "candidate"},
        ]})
        result = summarize(records)
        self.assertEqual(result["total_records"], 2)
        self.assertEqual(len(result["missing_anchors"]), 4)
        self.assertEqual(result["anchors"][0]["counts"]["inferred"], 1)
        self.assertEqual(result["anchors"][1]["module"], "server")
        self.assertEqual(len(result["without_engine_test_claim"]), 6)

    def test_engine_test_claim_does_not_imply_verification(self):
        records = validate({"records": [{
            "module": "client", "interface": "VClient011", "rva": "0x10",
            "evidence": "engine_tested", "body_sha256": "a" * 64,
            "test_id": "synthetic:test"
        }]})
        result = summarize(records)
        self.assertTrue(result["anchors"][0]["has_engine_test_claim"])
        self.assertIn("not independent", result["note"])


if __name__ == "__main__":
    unittest.main()
