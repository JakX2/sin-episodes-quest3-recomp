"""Synthetic-only tests for audited original-vtable Ghidra seed results."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_original_vtable_seed_results import (
    audit, PROVENANCE, SEED_HEADER, RESULT_HEADER,
)


def write(path, header, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=header)
        writer.writeheader()
        writer.writerows(rows)


class VtableSeedAuditTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.seeds = self.root / "seeds"
        self.results = self.root / "results"
        self.seeds.mkdir()
        self.results.mkdir()
        self.names = {"client.dll": "VClient011", "server.dll": "ServerGameDLL003"}
        self.expected = {module: {name: 2} for module, name in self.names.items()}
        self.seed_map = {}
        self.result_map = {}
        for module, name in self.names.items():
            seeds = [
                {"entry": f"{0x22001000+i*16:08x}", "interface": name,
                 "slot": str(i), "source": PROVENANCE} for i in range(2)]
            results = [
                {"entry": f"{0x22001000+i*16:08x}", "interface": name,
                 "slot": str(i),
                 "outcome": "seeded_candidate" if i else "already_inferred",
                 "result_function_entry": f"{0x22001000+i*16:08x}"} for i in range(2)]
            self.seed_map[module] = seeds
            self.result_map[module] = results
            write(self.seeds / f"{module}_vtable_method_seeds.csv", SEED_HEADER, seeds)
            write(self.results / f"{module}_vtable_seed_results.csv", RESULT_HEADER, results)

    def replace(self, module, kind, rows):
        if kind == "seeds":
            write(self.seeds / f"{module}_vtable_method_seeds.csv", SEED_HEADER, rows)
        else:
            write(self.results / f"{module}_vtable_seed_results.csv", RESULT_HEADER, rows)

    def run_audit(self):
        return audit(self.seeds, self.results, self.expected)

    def test_existing_and_seeded_results_are_candidates_not_verified_methods(self):
        result = self.run_audit()
        client = result["modules"]["client.dll"]
        self.assertEqual(client["newly_seeded_ghidra_candidates"], 1)
        self.assertEqual(client["existing_ghidra_starts"], 1)
        self.assertEqual(result["newly_verified_original_method_bodies"], 0)
        self.assertFalse(result["original_code_or_pseudocode_embedded"])

    def test_missing_result_fails_closed(self):
        self.replace("client.dll", "results", self.result_map["client.dll"][:1])
        with self.assertRaisesRegex(ValueError, "incomplete"):
            self.run_audit()

    def test_duplicate_result_fails_closed(self):
        self.replace("client.dll", "results", self.result_map["client.dll"][:1]*2)
        with self.assertRaisesRegex(ValueError, "duplicate"):
            self.run_audit()

    def test_wrong_address_rejected(self):
        rows = [dict(x) for x in self.result_map["client.dll"]]
        rows[0]["entry"] = "2200beef"
        self.replace("client.dll", "results", rows)
        with self.assertRaisesRegex(ValueError, "VA mismatch"):
            self.run_audit()

    def test_unreviewed_source_rejected(self):
        rows = [dict(x) for x in self.seed_map["client.dll"]]
        rows[0]["source"] = "guessed"
        self.replace("client.dll", "seeds", rows)
        with self.assertRaisesRegex(ValueError, "unreviewed"):
            self.run_audit()

    def test_unexpected_status_rejected(self):
        rows = [dict(x) for x in self.result_map["client.dll"]]
        rows[0]["outcome"] = "int foo() { return 0; }"
        self.replace("client.dll", "results", rows)
        with self.assertRaisesRegex(ValueError, "unexpected"):
            self.run_audit()

    def test_unresolved_seed_rejects_function_address(self):
        rows = [dict(x) for x in self.result_map["client.dll"]]
        rows[1]["outcome"] = "overlaps_another_inferred_function"
        self.replace("client.dll", "results", rows)
        with self.assertRaisesRegex(ValueError, "unresolved seed"):
            self.run_audit()

    def test_unresolved_seed_is_review_queue_not_verified_method(self):
        rows = [dict(x) for x in self.result_map["client.dll"]]
        rows[1]["outcome"] = "overlaps_another_inferred_function"
        rows[1]["result_function_entry"] = ""
        self.replace("client.dll", "results", rows)
        result = self.run_audit()
        self.assertEqual(result["modules"]["client.dll"]["unresolved_candidates"], 1)
        self.assertEqual(result["newly_verified_original_method_bodies"], 0)


if __name__ == "__main__":
    unittest.main()
