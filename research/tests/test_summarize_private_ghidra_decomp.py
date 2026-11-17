"""Synthetic-only private Ghidra status sanitizer tests."""
import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from research.tools.summarize_private_ghidra_decomp import summarize

HEADERS = {
    "seed": ["entry", "interface", "source"],
    "status": ["interface", "entry", "outcome", "inferred_function_bytes"],
}


def write_rows(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


class DecompStatusTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.seeds = self.root / "seeds"
        self.status = self.root / "status"
        self.seeds.mkdir()
        self.status.mkdir()
        self.interface_names = {
            "client.dll": "VClient011",
            "server.dll": "ServerGameDLL003",
        }
        for module, name in self.interface_names.items():
            write_rows(self.seeds / f"{module}_factory_seeds.csv", HEADERS["seed"], [
                {"entry": "24001234" if module == "client.dll" else "22001234",
                 "interface": name, "source": "reviewed_original_static_registration"},
                {"entry": "22220000", "interface": "MaterialProxy_IMaterialProxy003",
                 "source": "reviewed_original_static_registration"},
            ])
            write_rows(self.status / f"{module}_targeted_decomp_status.csv", HEADERS["status"], [
                {"interface": name,
                 "entry": "0x24001234" if module == "client.dll" else "0x22001234",
                 "outcome": "decompiled_candidate",
                 "inferred_function_bytes": "56"},
            ])

    def status_path(self, module="client.dll"):
        return self.status / f"{module}_targeted_decomp_status.csv"

    def replace(self, module, rows):
        write_rows(self.status_path(module), HEADERS["status"], rows)

    def test_complete_report_never_carries_original_c(self):
        result = summarize(self.seeds, self.status)
        self.assertEqual(result["modules"]["client.dll"]["candidate_c_decompiled"], 1)
        self.assertEqual(result["new_original_whole_body_sha_verified_methods"], 0)
        self.assertFalse(result["original_game_pseudocode_embedded"])
        self.assertNotIn("pseudocode", result["modules"]["client.dll"])

    def test_missing_status_fails_closed(self):
        self.replace("client.dll", [])
        with self.assertRaisesRegex(ValueError, "incomplete"):
            summarize(self.seeds, self.status)

    def test_duplicate_status_fails_closed(self):
        self.replace("client.dll", [
            {"interface": "VClient011", "entry": "0x24001234",
             "outcome": "decompiled_candidate", "inferred_function_bytes": "56"},
            {"interface": "VClient011", "entry": "0x24001234",
             "outcome": "decompiled_candidate", "inferred_function_bytes": "56"},
        ])
        with self.assertRaisesRegex(ValueError, "incomplete|duplicate"):
            summarize(self.seeds, self.status)

    def test_wrong_address_rejected(self):
        self.replace("client.dll", [
            {"interface": "VClient011", "entry": "0x24009999",
             "outcome": "decompiled_candidate", "inferred_function_bytes": "56"}])
        with self.assertRaisesRegex(ValueError, "factory VA"):
            summarize(self.seeds, self.status)

    def test_status_with_pseudocode_instead_of_outcome_rejected(self):
        self.replace("client.dll", [
            {"interface": "VClient011", "entry": "0x24001234",
             "outcome": "int main(void) { return 0; }", "inferred_function_bytes": "56"}])
        with self.assertRaisesRegex(ValueError, "unsafe"):
            summarize(self.seeds, self.status)

    def test_malformed_csv_rejected(self):
        self.status_path().write_text("interface,entry,outcome,inferred_function_bytes\n"
            "VClient011,0x24001234,decompiled_candidate,56,extra\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "malformed"):
            summarize(self.seeds, self.status)

    def test_unverified_source_rejected(self):
        p = self.seeds / "server.dll_factory_seeds.csv"
        write_rows(p, HEADERS["seed"], [
            {"interface": "ServerGameDLL003", "entry": "22001234",
             "source": "untrusted_source"}])
        with self.assertRaisesRegex(ValueError, "unreviewed"):
            summarize(self.seeds, self.status)

    def test_nonempty_length_required_for_success(self):
        self.replace("client.dll", [
            {"interface": "VClient011", "entry": "0x24001234",
             "outcome": "decompiled_candidate", "inferred_function_bytes": ""}])
        with self.assertRaisesRegex(ValueError, "lacks inferred"):
            summarize(self.seeds, self.status)

    def test_unknown_address_and_code_in_length_rejected(self):
        for raw in ("x123", "123456789", "void func()", "-4"):
            self.replace("client.dll", [
                {"interface": "VClient011", "entry": "0x24001234",
                 "outcome": "decompiled_candidate", "inferred_function_bytes": raw}])
            with self.subTest(raw=raw):
                with self.assertRaisesRegex(ValueError, "length"):
                    summarize(self.seeds, self.status)

    def test_cli_result(self):
        out = self.root / "report.json"
        script = Path(__file__).resolve().parents[1] / "tools" / "summarize_private_ghidra_decomp.py"
        run = subprocess.run([sys.executable, str(script),
            "--seed-dir", str(self.seeds), "--status-dir", str(self.status),
            "--out", str(out)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(json.loads(out.read_text(encoding="utf-8"))
                         ["modules"]["server.dll"]["candidate_c_decompiled"], 1)


if __name__ == "__main__":
    unittest.main()
