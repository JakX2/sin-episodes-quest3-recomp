"""Synthetic-only exact overlap tests; no original SiN binaries or bytes."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from join_factory_pointer_candidates import join


def write(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


class JoinFactoryPointerCandidatesTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.seeds = self.root / "seeds"
        self.data = self.root / "data"
        self.seeds.mkdir()
        self.data.mkdir()
        self.modules = {"client.dll": ("VClient011", "24001000"),
                        "server.dll": ("ServerGameDLL003", "22001000")}
        for module, (name, factory) in self.modules.items():
            write(self.seeds / f"{module}_factory_seeds.csv",
                  ["entry", "interface", "source"],
                  [{"entry": factory, "interface": name,
                    "source": "reviewed_original_static_registration"}])
            write(self.data / f"{module}_factory_data_ref_status.csv",
                  ["interface", "entry", "outcome", "inferred_function_bytes", "data_references"],
                  [{"interface": name, "entry": factory,
                    "outcome": "analyzed_direct_data_references",
                    "inferred_function_bytes": "64", "data_references": "1"}])
            write(self.data / f"{module}_factory_direct_data_refs.csv",
                  ["interface", "factory_entry", "instruction_va", "data_target_va",
                   "target_block", "reference_kind"],
                  [{"interface": name, "factory_entry": factory,
                    "instruction_va": f"{int(factory, 16)+4:08X}",
                    "data_target_va": "24002008" if module == "client.dll" else "22002008",
                    "target_block": ".rdata", "reference_kind": "DATA"}])
            base = 0x24002000 if module == "client.dll" else 0x22002000
            write(self.data / f"{module}_relocated_code_pointers.csv",
                  ["source", "target", "target_function", "source_block"],
                  [{"source": f"{base+4*i:08X}", "target": f"{base+0x1000+16*i:08X}",
                    "target_function": f"{base+0x1000+16*i:08X}",
                    "source_block": ".rdata"} for i in range(5)])

    def test_exact_slot_overlap(self):
        result = join(self.seeds, self.data)
        self.assertEqual(result["new_verified_vtables"], 0)
        self.assertFalse(result["original_code_or_pseudocode_included"])
        for module in self.modules:
            row = result["modules"][module]["review_queue"][0]
            self.assertEqual(row["candidate_slot_index"], 2)
            self.assertEqual(row["candidate_slot_count"], 5)
            self.assertTrue(row["not_verified_as_vptr_or_vtable"])

    def test_unrelated_address_no_match(self):
        module = "client.dll"
        path = self.data / f"{module}_factory_direct_data_refs.csv"
        rows = list(csv.DictReader(path.open()))
        rows[0]["data_target_va"] = "24003000"
        write(path, list(rows[0]), rows)
        result = join(self.seeds, self.data)
        self.assertEqual(result["modules"][module]["exact_address_overlaps"], 0)

    def test_invalid_factory_rejected(self):
        module = "client.dll"
        path = self.data / f"{module}_factory_direct_data_refs.csv"
        rows = list(csv.DictReader(path.open()))
        rows[0]["factory_entry"] = "24001100"
        write(path, list(rows[0]), rows)
        with self.assertRaisesRegex(ValueError, "unreviewed factory"):
            join(self.seeds, self.data)

    def test_reference_count_mismatch_rejected(self):
        module = "server.dll"
        path = self.data / f"{module}_factory_data_ref_status.csv"
        rows = list(csv.DictReader(path.open()))
        rows[0]["data_references"] = "2"
        write(path, list(rows[0]), rows)
        with self.assertRaisesRegex(ValueError, "count mismatch"):
            join(self.seeds, self.data)

    def test_pointer_conflict_rejected(self):
        module = "server.dll"
        path = self.data / f"{module}_relocated_code_pointers.csv"
        rows = list(csv.DictReader(path.open()))
        duplicate = dict(rows[0])
        duplicate["target"] = "22005000"
        duplicate["target_function"] = "22005000"
        write(path, list(rows[0]), rows + [duplicate])
        with self.assertRaisesRegex(ValueError, "conflicting pointer"):
            join(self.seeds, self.data)

    def test_missing_status_rejected(self):
        module = "client.dll"
        write(self.data / f"{module}_factory_data_ref_status.csv",
              ["interface", "entry", "outcome", "inferred_function_bytes", "data_references"], [])
        with self.assertRaisesRegex(ValueError, "incomplete factory"):
            join(self.seeds, self.data)


if __name__ == "__main__":
    unittest.main()
