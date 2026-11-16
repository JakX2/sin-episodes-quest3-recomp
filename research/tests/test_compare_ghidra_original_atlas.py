"""Synthetic-only Ghidra atlas comparison tests, no retail binaries."""
from __future__ import annotations

import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import compare_ghidra_original_atlas as atlas


class SyntheticGhidraAtlasTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.entries = {
            "methods": [
                {"dll": "client", "entry_va": "0x24126C40",
                 "label": "synthetic client float proxy"},
                {"dll": "client", "entry_va": "0x24126C90",
                 "label": "synthetic client int32 proxy"},
                {"dll": "server", "entry_va": "0x221FB5F0",
                 "label": "synthetic server export"},
            ]
        }
        self.catalog = self.root / "verified.json"
        self.catalog.write_text(json.dumps(self.entries), encoding="utf-8")

    def file(self, name: str, rows: list[str],
             headings: tuple[str, ...] = ("entry", "name")) -> Path:
        path = self.root / (name + "_functions.csv")
        with path.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(headings)
            for addr in rows:
                writer.writerow([addr, "Ghidra synthetic name"])
        return path

    def test_overlap_and_missing_are_not_method_credit(self):
        self.file("client.dll", ["24126c40", "0x24126A20", "241281d0"])
        self.file("server.dll", ["0x221FB5F0", "2230f4e0"])
        report = atlas.analyze(self.root, self.catalog)
        self.assertEqual(report["total_new_verified_methods"], 0)
        client = report["modules"]["client.dll"]
        self.assertEqual(client["ghidra_inferred_function_entries"], 3)
        self.assertEqual(client["already_SHA_reviewed_catalog_entries"], 2)
        self.assertEqual(client["sha_reviewed_entries_matched_by_ghidra"], 1)
        self.assertEqual(client["sha_reviewed_entries_not_identified_by_ghidra"], 1)
        self.assertEqual(client["inferred_entries_not_SHA_reviewed"], 2)
        self.assertEqual(client["catalog_entries_missing_in_ghidra"],
                         ["0x24126C90"])
        self.assertEqual(client["new_original_SHA_verified_methods"], 0)
        server = report["modules"]["server.dll"]
        self.assertEqual(server["sha_reviewed_entries_matched_by_ghidra"], 1)
        self.assertEqual(server["inferred_entries_not_SHA_reviewed"], 1)

    def test_duplicate_entries_refused(self):
        self.file("client.dll", ["24126C40", "0x24126c40"])
        with self.assertRaisesRegex(atlas.AtlasError, "duplicate"):
            atlas.module_atlas(self.root / "client.dll_functions.csv",
                               self.entries["methods"], "client.dll")

    def test_missing_entry_column_is_rejected(self):
        self.file("client.dll", [], ("location", "name"))
        with self.assertRaisesRegex(atlas.AtlasError, "missing entry"):
            atlas.module_atlas(self.root / "client.dll_functions.csv",
                               self.entries["methods"], "client.dll")

    def test_bad_address_is_rejected(self):
        for bad in ("", "missing", "garbage123", "0x1x2"):
            with self.subTest(input=bad):
                with self.assertRaises(atlas.AtlasError):
                    atlas.parse_entry(bad)

    def test_valid_address_parsing_includes_zero_prefixes(self):
        self.assertEqual(atlas.parse_entry("24126C40"), 0x24126C40)
        self.assertEqual(atlas.parse_entry("0x24126C40"), 0x24126C40)
        self.assertEqual(atlas.parse_entry(" 0X24126c40 "), 0x24126C40)


if __name__ == "__main__":
    unittest.main()
