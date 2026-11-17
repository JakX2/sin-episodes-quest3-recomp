"""Synthetic-only private startup metadata gate regressions."""
import csv
import importlib.util
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "validate_private_first_startup_review.py"
spec = importlib.util.spec_from_file_location("startup_gate", SCRIPT)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class StartupReviewGate(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "private.csv"
        self.row = [
            "VClient011", "0", "0x240FE0E0", "1", "128",
            "VEngineClient012", "112", "140", "3", gate.TIER,
        ]

    def write(self, rows):
        with self.path.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(gate.HEADER)
            writer.writerows(rows)

    def test_valid_private_review_aggregate_only(self):
        self.write([self.row])
        result = gate.validate(self.path, "client.dll")
        self.assertEqual(result["candidate_count"], 1)
        self.assertEqual(result["candidates_with_prior_literal"], 1)
        self.assertEqual(result["candidates_with_nearby_conditional"], 1)
        self.assertEqual(result["verified_factory_calls"], 0)
        self.assertEqual(result["verified_failure_edges"], 0)
        self.assertNotIn("VEngineClient012", str(result))
        self.assertNotIn("128", str(result))

    def test_valid_empty_and_server_review(self):
        self.write([])
        self.assertEqual(gate.validate(self.path, "client.dll")["candidate_count"], 0)
        row = self.row.copy()
        row[0], row[2], row[5] = "ServerGameDLL003", "0x221E7640", "VEngineServer021"
        self.write([row])
        self.assertEqual(gate.validate(self.path, "server.dll")["candidate_count"], 1)

    def test_reject_unreviewed_identity_and_tier(self):
        for field, value in [(0, "VClientPrediction001"), (2, "0xDEADBEEF"),
                             (9, "verified_factory")]:
            row = self.row.copy()
            row[field] = value
            self.write([row])
            with self.assertRaises(ValueError):
                gate.validate(self.path, "client.dll")

    def test_reject_order_and_incomplete_evidence(self):
        for field, value in [(3, "2"), (4, "-1"), (5, "SECRET_PAYLOAD"),
                             (6, "129"), (7, "127"), (8, "17")]:
            row = self.row.copy()
            row[field] = value
            self.write([row])
            with self.assertRaises(ValueError):
                gate.validate(self.path, "client.dll")
        row = self.row.copy()
        row[5] = ""
        self.write([row])
        with self.assertRaises(ValueError):
            gate.validate(self.path, "client.dll")

    def test_reject_extra_columns_rows_and_large_files(self):
        self.write([self.row + ["untrusted"]])
        with self.assertRaises(ValueError):
            gate.validate(self.path, "client.dll")
        self.write([self.row] * 9)
        with self.assertRaises(ValueError):
            gate.validate(self.path, "client.dll")
        self.path.write_text("x" * 8193, encoding="utf-8")
        with self.assertRaises(ValueError):
            gate.validate(self.path, "client.dll")


if __name__ == "__main__":
    unittest.main()
