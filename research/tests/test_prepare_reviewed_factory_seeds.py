"""Synthetic-only reviewed factory seed construction tests."""
import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from research.tools.prepare_reviewed_factory_seeds import seeds, write_seeds


class FactorySeedTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.manifest = self.root / "manifest.json"
        self.output = self.root / "out"
        self.data = {"dlls": {
            "client": {"static_registration_count": 2,
                       "interface_registrations": [
                           {"interface": "VClient011", "factory_va": 0x240FD840},
                           {"interface": "GameClientExports001", "factory_va": 0x240FD810}]},
            "server": {"static_registration_count": 1,
                       "interface_registrations": [
                           {"interface": "ServerGameDLL003", "factory_va": 0x221E7630}]}}}
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps(self.data), encoding="utf-8")

    def test_sorted_and_reviewed(self):
        result = seeds(self.data)
        self.assertEqual(result["client.dll_factory_seeds.csv"][0]["entry"], "240fd810")
        self.assertEqual(result["server.dll_factory_seeds.csv"][0]["interface"],
                         "ServerGameDLL003")
        self.assertEqual(result["client.dll_factory_seeds.csv"][0]["source"],
                         "reviewed_original_static_registration")

    def test_both_modules_required(self):
        del self.data["dlls"]["server"]
        with self.assertRaisesRegex(ValueError, "both"):
            seeds(self.data)

    def test_counts_checked_before_write(self):
        self.data["dlls"]["client"]["static_registration_count"] = 55
        self.save()
        with self.assertRaisesRegex(ValueError, "incomplete"):
            write_seeds(self.manifest, self.output)
        self.assertFalse(self.output.exists())

    def test_duplicate_name_rejected(self):
        self.data["dlls"]["client"]["interface_registrations"][1]["interface"] = "VClient011"
        with self.assertRaisesRegex(ValueError, "duplicate"):
            seeds(self.data)

    def test_bad_va_rejected(self):
        for bad in (-1, 0x100000000, True, "0x22000000"):
            with self.subTest(bad=bad):
                self.data["dlls"]["server"]["interface_registrations"][0]["factory_va"] = bad
                with self.assertRaises(ValueError):
                    seeds(self.data)

    def test_bad_interface_name_rejected(self):
        self.data["dlls"]["server"]["interface_registrations"][0]["interface"] = "A,B"
        with self.assertRaisesRegex(ValueError, "invalid interface"):
            seeds(self.data)

    def test_real_subprocess_outputs_two_files(self):
        script = Path(__file__).resolve().parents[1] / "tools" / "prepare_reviewed_factory_seeds.py"
        run = subprocess.run([sys.executable, str(script), "--manifest", str(self.manifest),
                              "--out-dir", str(self.output)], text=True, capture_output=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        report = json.loads(run.stdout)
        self.assertEqual(report["client.dll_factory_seeds.csv"], 2)
        with (self.output / "client.dll_factory_seeds.csv").open(newline="") as stream:
            rows = list(csv.DictReader(stream))
        self.assertEqual([r["entry"] for r in rows], ["240fd810", "240fd840"])


if __name__ == "__main__":
    unittest.main()
