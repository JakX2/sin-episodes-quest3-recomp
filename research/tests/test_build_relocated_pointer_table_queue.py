"""Synthetic relocation-atlas regressions, no original DLL bytes."""
import csv
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build_relocated_pointer_table_queue import analyze_module, build

FIELDS = ("source", "target", "target_function", "source_block")


class PointerRunsTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.rows = [
            {"source": f"{0x2000+4*i:08x}", "target": f"{0x5000+4*i:08x}",
             "target_function": f"{0x5000+4*i:08x}" if i % 2 == 0 else "",
             "source_block": ".rdata"} for i in range(6)
        ] + [
            {"source": f"{0x3000+4*i:08x}", "target": f"{0x6000+4*i:08x}",
             "target_function": "", "source_block": ".data"} for i in range(10)
        ]
        self.write("client.dll", self.rows)
        self.write("server.dll", self.rows)

    def write(self, module, rows):
        with (self.root / f"{module}_relocated_code_pointers.csv").open("w", newline="") as out:
            writer = csv.DictWriter(out, fieldnames=FIELDS)
            writer.writeheader()
            writer.writerows(rows)

    def candidate_count(self):
        return analyze_module(self.root / "client.dll_relocated_code_pointers.csv")[
            "rdata_pointer_runs_4plus_distinct_targets"]

    def test_real_candidate_properties(self):
        result = build(self.root)
        client = result["modules"]["client.dll"]
        self.assertEqual(client["relocated_data_code_pointers_unique_sources"], 16)
        self.assertEqual(client["rdata_pointer_runs_4plus_distinct_targets"], 1)
        self.assertEqual(client["candidate_runs_by_size_bucket"]["04-16"], 1)
        self.assertEqual(client["review_queue"][0]["table_candidate_va"], "0x00002000")
        self.assertEqual(client["review_queue"][0]["ghidra_exact_function_start_matches"], 3)
        self.assertTrue(client["review_queue"][0]["not_verified_as_vtable"])
        self.assertEqual(result["new_verified_original_methods"], 0)
        self.assertFalse(result["original_code_or_pseudocode_included"])

    def test_gap_splits_sequences(self):
        rows = self.rows[:3] + [{**self.rows[3], "source": "00002400"}] + self.rows[4:]
        self.write("client.dll", rows)
        self.assertEqual(self.candidate_count(), 0)

    def test_only_distinct_targets(self):
        rows = [{**x, "target": "00005000", "target_function": "00005000"} for x in self.rows[:6]]
        self.write("client.dll", rows)
        self.assertEqual(self.candidate_count(), 0)

    def test_data_pointer_arrays_are_excluded(self):
        self.write("client.dll", self.rows[6:])
        self.assertEqual(self.candidate_count(), 0)

    def test_identical_relocation_duplicates_collapse(self):
        self.write("client.dll", self.rows + [self.rows[0]])
        report = analyze_module(self.root / "client.dll_relocated_code_pointers.csv")
        self.assertEqual(report["relocated_data_code_pointers_unique_sources"], 16)
        self.assertEqual(report["relocated_data_code_pointer_records_total"], 17)
        self.assertEqual(report["deduplicated_identical_relocation_records"], 1)

    def test_conflicting_source_fails(self):
        self.write("client.dll", self.rows + [{**self.rows[0], "target": "00008000",
                                               "target_function": ""}])
        with self.assertRaisesRegex(ValueError, "conflicting pointer"):
            self.candidate_count()

    def test_mismatched_function_start_fails(self):
        rows = list(self.rows)
        rows[0] = {**rows[0], "target_function": "00007000"}
        self.write("client.dll", rows)
        with self.assertRaisesRegex(ValueError, "mismatches target"):
            self.candidate_count()

    def test_invalid_hex_and_alignment_fail(self):
        for invalid in ("g123", "100000000", "00002001"):
            rows = list(self.rows)
            rows[0] = {**rows[0], "source": invalid}
            self.write("client.dll", rows)
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                self.candidate_count()

    def test_strict_csv_schema_and_malformed_row(self):
        self.root.joinpath("client.dll_relocated_code_pointers.csv").write_text(
            "source,target,target_function,source_block\n2000,5000,5000,.rdata,extra\n")
        with self.assertRaisesRegex(ValueError, "malformed CSV"):
            self.candidate_count()

    def test_candidate_buckets_sample_across_lengths(self):
        rows = []
        for start, length in ((0x2000, 4), (0x3000, 18), (0x4000, 67),
                              (0x5000, 130), (0x6000, 260)):
            rows.extend({"source": f"{start+4*i:08x}",
                         "target": f"{0x8000+start+4*i:08x}",
                         "target_function": "", "source_block": ".rdata"}
                        for i in range(length))
        self.write("client.dll", rows)
        report = analyze_module(self.root / "client.dll_relocated_code_pointers.csv")
        self.assertEqual(len(report["review_queue"]), 5)
        self.assertTrue(all(item["not_verified_as_vtable"] for item in report["review_queue"]))
        self.assertEqual(set(report["candidate_runs_by_size_bucket"].values()), {1})

    def test_x86_packed_unaligned_data_is_not_vtable(self):
        rows = [{"source": f"{0x3002+4*i:08x}",
                 "target": f"{0x7000+4*i:08x}",
                 "target_function": "", "source_block": ".data"} for i in range(6)]
        self.write("client.dll", rows)
        report = analyze_module(self.root / "client.dll_relocated_code_pointers.csv")
        self.assertEqual(report["relocated_data_code_pointers_unique_sources"], 6)
        self.assertEqual(report["rdata_pointer_runs_4plus_distinct_targets"], 0)

    def test_zero_or_bad_bounds_fail(self):
        with self.assertRaisesRegex(ValueError, "bounds"):
            analyze_module(self.root / "client.dll_relocated_code_pointers.csv",
                           max_per_bucket=0)
        with self.assertRaisesRegex(ValueError, "bounds"):
            analyze_module(self.root / "client.dll_relocated_code_pointers.csv",
                           min_slots=3)


if __name__ == "__main__":
    unittest.main()
