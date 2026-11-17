"""Synthetic-only tests for bounded private startup census metadata."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from summarize_startup_factory_census import HEADER, summarize


def make_row(module):
    client = module == "client.dll"
    return ["VClient011" if client else "ServerGameDLL003", "0",
            "0x240FE0E0" if client else "0x221E7640",
            "Init" if client else "DLLInit", "bounded_inferred_function",
            "240", "45", "3", "2", "2", "VEngineClient013;VFileSystem017"]


class StartupCensusTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for module in ("client.dll", "server.dll"):
            self.write(module, make_row(module))

    def write(self, module, row, header=HEADER):
        with (self.root / (module + "_startup_factory_census.csv")).open(
                "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(header)
            writer.writerow(row)

    def test_valid_bounded_summary(self):
        report = summarize(self.root)
        self.assertFalse(report["independently_tested_retail_startup"])
        self.assertEqual(report["modules"]["client.dll"]["ghidra_indirect_call_site_count"], 2)
        self.assertFalse(report["modules"]["server.dll"]["original_first_failure_branch_verified"])

    def test_wrong_entry_rejected(self):
        row = make_row("client.dll")
        row[2] = "0x240FE0E1"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "entry"):
            summarize(self.root)

    def test_wrong_slot_rejected(self):
        row = make_row("client.dll")
        row[1] = "1"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "slot"):
            summarize(self.root)

    def test_arbitrary_string_rejected(self):
        row = make_row("client.dll")
        row[-1] = "secret_path_to_original_code;VFileSystem017"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "interface literal"):
            summarize(self.root)

    def test_unbounded_counter_rejected(self):
        row = make_row("client.dll")
        row[7] = "999999999"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "counter"):
            summarize(self.root)

    def test_missing_method_cannot_claim_calls(self):
        row = make_row("client.dll")
        row[4] = "missing_exact_ghidra_function"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "Missing method"):
            summarize(self.root)

    def test_duplicate_literals_rejected(self):
        row = make_row("client.dll")
        row[-1] = "VFileSystem017;VFileSystem017"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "interface literal"):
            summarize(self.root)

    def test_wrong_header_rejected(self):
        self.write("client.dll", make_row("client.dll"), HEADER[:-1])
        with self.assertRaisesRegex(ValueError, "fields"):
            summarize(self.root)

    def test_more_calls_than_instructions_rejected(self):
        row = make_row("client.dll")
        row[6] = "4"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "More calls"):
            summarize(self.root)

    def test_unbounded_status_must_not_report_calls(self):
        row = make_row("client.dll")
        row[4] = "unbounded_inferred_function"
        self.write("client.dll", row)
        with self.assertRaisesRegex(ValueError, "Unbounded method"):
            summarize(self.root)


if __name__ == "__main__":
    unittest.main()
