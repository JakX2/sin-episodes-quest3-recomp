"""Synthetic-only regression tests for owner-private vtable pseudocode status."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from summarize_private_vtable_method_decomp import (
    PRIORITY, SEED_HEADER, STATUS_HEADER, SOURCE, summarize,
)


def write(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


class PrivateVtableStatusTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.seeds = self.root / "seeds"
        self.status = self.root / "status"
        self.seeds.mkdir()
        self.status.mkdir()
        self.seed_rows = {}
        self.status_rows = {}
        for module, reviewed in PRIORITY.items():
            source = []
            outcomes = []
            for position, (name, slot) in enumerate(reviewed):
                address = f"{0x22001000 + position * 64:08x}"
                source.append({"entry": address, "interface": name,
                               "slot": str(slot), "source": SOURCE})
                outcomes.append({
                    "interface": name, "slot": str(slot),
                    "entry": address, "outcome": "decompiled_candidate",
                    "inferred_function_bytes": "64",
                })
            self.seed_rows[module] = source
            self.status_rows[module] = outcomes
            self.write_module(module)

    def write_module(self, module):
        write(self.seeds / f"{module}_vtable_method_seeds.csv",
              SEED_HEADER, self.seed_rows[module])
        write(self.status / f"{module}_vtable_targeted_decomp_status.csv",
              STATUS_HEADER, self.status_rows[module])

    def test_bounded_decompilation_does_not_verify_original_abi(self):
        result = summarize(self.seeds, self.status)
        self.assertEqual(result["modules"]["client.dll"]["priority_slots"], 6)
        self.assertEqual(result["modules"]["server.dll"]["priority_slots"], 8)
        self.assertEqual(result["modules"]["server.dll"]["private_pseudocode_candidates"], 8)
        self.assertEqual(result["new_original_whole_body_sha_verified_methods"], 0)
        self.assertFalse(result["original_game_pseudocode_embedded"])

    def test_missing_status_fails(self):
        self.status_rows["client.dll"].pop()
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "incorrect bounded"):
            summarize(self.seeds, self.status)

    def test_duplicate_status_fails(self):
        self.status_rows["client.dll"][-1] = dict(self.status_rows["client.dll"][0])
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "repeated"):
            summarize(self.seeds, self.status)

    def test_unreviewed_interface_rejected(self):
        self.status_rows["server.dll"][0]["interface"] = "Unknown001"
        self.write_module("server.dll")
        with self.assertRaisesRegex(ValueError, "unexpected"):
            summarize(self.seeds, self.status)

    def test_mismatched_method_address_rejected(self):
        self.status_rows["client.dll"][0]["entry"] = "22009999"
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "disagrees"):
            summarize(self.seeds, self.status)

    def test_unreviewed_seed_provenance_rejected(self):
        self.seed_rows["client.dll"][0]["source"] = "arbitrary"
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "unreviewed"):
            summarize(self.seeds, self.status)

    def test_pseudocode_or_instructions_in_status_rejected(self):
        self.status_rows["client.dll"][0]["outcome"] = "void __thiscall foo() {}"
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "unsafe"):
            summarize(self.seeds, self.status)

    def test_outcome_extent_consistency(self):
        row = self.status_rows["client.dll"][0]
        row["outcome"] = "missing_inferred_function"
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "falsely"):
            summarize(self.seeds, self.status)
        row["inferred_function_bytes"] = ""
        self.write_module("client.dll")
        result = summarize(self.seeds, self.status)
        self.assertEqual(result["modules"]["client.dll"]["unavailable_or_failed"], 1)

    def test_non_numeric_extent_rejected(self):
        self.status_rows["client.dll"][0]["inferred_function_bytes"] = "64;\n C source"
        self.write_module("client.dll")
        with self.assertRaisesRegex(ValueError, "non-numeric"):
            summarize(self.seeds, self.status)

    def test_extra_csv_column_rejected(self):
        path = self.status / "client.dll_vtable_targeted_decomp_status.csv"
        source = path.read_text(encoding="utf-8")
        path.write_text(source.replace("\n", ",extra\n", 1), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "unexpected CSV"):
            summarize(self.seeds, self.status)

    def test_missing_priority_seed_rejected(self):
        self.seed_rows["server.dll"].pop()
        self.write_module("server.dll")
        with self.assertRaisesRegex(ValueError, "missing priority"):
            summarize(self.seeds, self.status)

    def test_duplicate_target_address_rejected(self):
        self.seed_rows["server.dll"][1]["entry"] = self.seed_rows["server.dll"][0]["entry"]
        self.write_module("server.dll")
        with self.assertRaisesRegex(ValueError, "duplicate"):
            summarize(self.seeds, self.status)


if __name__ == "__main__":
    unittest.main()
