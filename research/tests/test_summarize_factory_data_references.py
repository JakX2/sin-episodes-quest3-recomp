"""Synthetic-only fail-closed tests for factory data xref reconciliation."""
import csv
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from summarize_factory_data_references import summarize

SEED_FIELDS = ["entry", "interface", "source"]
STATUS_FIELDS = ["interface", "entry", "outcome", "inferred_function_bytes", "data_references"]
REF_FIELDS = ["interface", "factory_entry", "instruction_va", "data_target_va",
              "target_block", "reference_kind"]


def write_csv(path, fields, records):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(records)


class FactoryDataReferenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.seeds = self.root / "seeds"
        self.metadata = self.root / "metadata"
        self.seeds.mkdir()
        self.metadata.mkdir()
        self.names = {"client.dll": ("VClient011", "240FD840"),
                      "server.dll": ("ServerGameDLL003", "221E7630")}
        for module, (interface, address) in self.names.items():
            write_csv(self.seeds / f"{module}_factory_seeds.csv", SEED_FIELDS, [
                {"entry": address.lower(), "interface": interface,
                 "source": "reviewed_original_static_registration"}])
            write_csv(self.metadata / f"{module}_factory_data_ref_status.csv", STATUS_FIELDS, [
                {"interface": interface, "entry": address, "outcome": "analyzed_direct_data_references",
                 "inferred_function_bytes": "27", "data_references": "1"}])
            write_csv(self.metadata / f"{module}_factory_direct_data_refs.csv", REF_FIELDS, [
                {"interface": interface, "factory_entry": address,
                 "instruction_va": f"{int(address,16) + 3:08X}",
                 "data_target_va": "24401020" if module == "client.dll" else "22401020",
                 "target_block": ".data", "reference_kind": "READ"}])

    def test_success_conservative_evidence(self):
        report = summarize(self.seeds, self.metadata)
        self.assertEqual(report["modules"]["client.dll"]["direct_nonexecuting_data_references"], 1)
        engine = report["modules"]["client.dll"]["priority_interface_factory_data_refs"]["VClient011"]
        self.assertTrue(engine["data_targets_for_manual_review"][0]["not_verified_as_object_or_vptr"])
        self.assertEqual(report["new_verified_vtables"], 0)
        self.assertEqual(report["new_verified_original_methods"], 0)
        self.assertFalse(report["original_code_or_pseudocode_included"])

    def test_missing_reference_is_detected(self):
        self.replace_refs("client.dll", [])
        with self.assertRaisesRegex(ValueError, "count mismatch"):
            summarize(self.seeds, self.metadata)

    def test_extra_interface_rejected(self):
        self.replace_refs("client.dll", [{
            "interface": "Unreviewed001", "factory_entry": "240FD840",
            "instruction_va": "240FD843", "data_target_va": "24401020",
            "target_block": ".data", "reference_kind": "READ"}])
        with self.assertRaisesRegex(ValueError, "unknown interface"):
            summarize(self.seeds, self.metadata)

    def test_wrong_factory_va_rejected(self):
        self.replace_refs("client.dll", [{
            "interface": "VClient011", "factory_entry": "240FD850",
            "instruction_va": "240FD843", "data_target_va": "24401020",
            "target_block": ".data", "reference_kind": "READ"}])
        with self.assertRaisesRegex(ValueError, "factory entry mismatch"):
            summarize(self.seeds, self.metadata)

    def test_duplicate_reference_rejected(self):
        original = self.get_refs("client.dll")
        self.replace_refs("client.dll", original + original)
        with self.assertRaisesRegex(ValueError, "duplicated direct"):
            summarize(self.seeds, self.metadata)

    def test_missing_factory_status_rejected(self):
        write_csv(self.metadata / "server.dll_factory_data_ref_status.csv", STATUS_FIELDS, [])
        with self.assertRaisesRegex(ValueError, "missing factory"):
            summarize(self.seeds, self.metadata)

    def test_unresolved_factory_with_data_refs_rejected(self):
        name, address = self.names["client.dll"]
        write_csv(self.metadata / "client.dll_factory_data_ref_status.csv", STATUS_FIELDS, [
            {"interface": name, "entry": address, "outcome": "missing_inferred_function",
             "inferred_function_bytes": "", "data_references": "0"}])
        with self.assertRaisesRegex(ValueError, "unresolved factory"):
            summarize(self.seeds, self.metadata)

    def test_malformed_csv_extra_column_rejected(self):
        (self.metadata / "client.dll_factory_direct_data_refs.csv").write_text(
            "interface,factory_entry,instruction_va,data_target_va,target_block,reference_kind\n"
            "VClient011,240FD840,240FD843,24401020,.data,READ,EXTRA\n",
            encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "malformed CSV"):
            summarize(self.seeds, self.metadata)

    def test_unreviewed_seed_source_rejected(self):
        name, address = self.names["client.dll"]
        write_csv(self.seeds / "client.dll_factory_seeds.csv", SEED_FIELDS, [
            {"entry": address, "interface": name, "source": "guessed"}])
        with self.assertRaisesRegex(ValueError, "unreviewed"):
            summarize(self.seeds, self.metadata)

    def test_address_overflow_rejected(self):
        bad = self.get_refs("client.dll")
        bad[0]["data_target_va"] = "100000000"
        self.replace_refs("client.dll", bad)
        with self.assertRaisesRegex(ValueError, "invalid PE32"):
            summarize(self.seeds, self.metadata)

    def test_not_inferred_factory_is_allowed_without_refs(self):
        name, address = self.names["client.dll"]
        write_csv(self.metadata / "client.dll_factory_data_ref_status.csv", STATUS_FIELDS, [
            {"interface": name, "entry": address, "outcome": "missing_inferred_function",
             "inferred_function_bytes": "", "data_references": "0"}])
        self.replace_refs("client.dll", [])
        report = summarize(self.seeds, self.metadata)
        self.assertEqual(report["modules"]["client.dll"]["factories_with_inferred_entries"], 0)

    def get_refs(self, module):
        with (self.metadata / f"{module}_factory_direct_data_refs.csv").open(newline="") as f:
            return list(csv.DictReader(f))

    def replace_refs(self, module, rows):
        write_csv(self.metadata / f"{module}_factory_direct_data_refs.csv", REF_FIELDS, rows)


if __name__ == "__main__":
    unittest.main()
