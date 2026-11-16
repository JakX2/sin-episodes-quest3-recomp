"""Synthetic-only ABI atlas triage regressions; no commercial bytes."""
import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from research.tools.triage_original_abi_atlas import build, check_provenance, EXPECTED_SHA256

def write(path, columns, rows):
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)

class TriageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.d = Path(self.tmp.name)
        write(self.d / "client_function_candidates.csv",
              ["address", "tier", "call_sites", "data_pointer_references"], [
              {"address": "0x10001000", "tier": "strong_anchor",
               "call_sites": "1", "data_pointer_references": "1"},
              {"address": "0x10002000", "tier": "call_target",
               "call_sites": "8", "data_pointer_references": "0"}])
        write(self.d / "client_exports.csv", ["address", "name"],
              [{"address": "0x10001000", "name": "CreateInterface"}])
        write(self.d / "client_direct_edges.csv",
              ["caller", "target", "kind"], [
              {"caller": "0x10003000", "target": "0x10002000", "kind": "call"}])

    def test_factory_priority(self):
        report = build(self.d, "client")
        self.assertEqual(report["candidate_count"], 2)
        self.assertEqual(report["ranked_candidates"][0]["address"], "0x10001000")
        self.assertEqual(report["new_verified_original_functions"], 0)

    def test_ghidra_does_not_verify(self):
        write(self.d / "client.dll_functions.csv", ["entry"],
              [{"entry": "0x10002000"}])
        report = build(self.d, "client", self.d)
        self.assertEqual(report["ghidra_inferred_starts_matched"], 1)
        self.assertFalse(report["ranked_candidates"][1]["whole_body_sha_verified_by_this_tool"])

    def test_duplicate_rejected(self):
        row = {"address": "0x10001000", "tier": "strong_anchor",
               "call_sites": "1", "data_pointer_references": "0"}
        write(self.d / "client_function_candidates.csv",
              list(row), [row, row])
        with self.assertRaises(ValueError):
            build(self.d, "client")

    def test_bad_tier_rejected(self):
        row = {"address": "0x10001000", "tier": "verified",
               "call_sites": "1", "data_pointer_references": "0"}
        write(self.d / "client_function_candidates.csv", list(row), [row])
        with self.assertRaises(ValueError):
            build(self.d, "client")

    def test_missing_schema_rejected(self):
        write(self.d / "client_exports.csv", ["name"],
              [{"name": "CreateInterface"}])
        with self.assertRaises(ValueError):
            build(self.d, "client")

    def test_summary_sha_gate(self):
        (self.d / "summary.json").write_text(json.dumps({"modules": {
            name: {"sha256": digest} for name, digest in EXPECTED_SHA256.items()}}))
        check_provenance(self.d)
        (self.d / "summary.json").write_text(json.dumps({"modules": {
            "client.dll": {"sha256": "wrong"}}}))
        with self.assertRaises(ValueError):
            check_provenance(self.d)

    def test_real_ghidra_java_bare_hex_address_matches(self):
        # Java ExportOriginalFunctionAtlas uses Address.toString(), without 0x.
        write(self.d / "client.dll_functions.csv", ["entry"],
              [{"entry": "10002000"}])
        report = build(self.d, "client", self.d)
        self.assertEqual(report["ghidra_inferred_starts_matched"], 1)
        self.assertTrue(report["ranked_candidates"][1]["ghidra_inferred_start"])
        self.assertFalse(report["ranked_candidates"][1]["whole_body_sha_verified_by_this_tool"])

    def test_ghidra_rejects_oversize_pe32_va(self):
        write(self.d / "client.dll_functions.csv", ["entry"],
              [{"entry": "100000000"}])
        with self.assertRaisesRegex(ValueError, "Ghidra PE32"):
            build(self.d, "client", self.d)

    def test_ghidra_rejects_malformed_va(self):
        write(self.d / "client.dll_functions.csv", ["entry"],
              [{"entry": "004020GG"}])
        with self.assertRaisesRegex(ValueError, "Ghidra PE32"):
            build(self.d, "client", self.d)

    def test_inventory_stays_strictly_prefixed(self):
        write(self.d / "client_function_candidates.csv",
              ["address", "tier", "call_sites", "data_pointer_references"],
              [{"address": "10001000", "tier": "strong_anchor",
                "call_sites": "1", "data_pointer_references": "0"}])
        with self.assertRaisesRegex(ValueError, "0x prefix"):
            build(self.d, "client")

    def test_full_cli_reports_two_modules_and_auth_metadata(self):
        # Exercises the real main(), not only build(). Older code crashed
        # while summarizing the _authentication section as a DLL.
        write(self.d / "server_function_candidates.csv",
              ["address", "tier", "call_sites", "data_pointer_references"],
              [{"address": "0x20001000", "tier": "call_target",
                "call_sites": "1", "data_pointer_references": "0"}])
        write(self.d / "server_exports.csv", ["address", "name"], [])
        write(self.d / "server_direct_edges.csv",
              ["caller", "target", "kind"], [])
        write(self.d / "client.dll_functions.csv", ["entry"],
              [{"entry": "10002000"}])
        write(self.d / "server.dll_functions.csv", ["entry"],
              [{"entry": "20001000"}])
        (self.d / "summary.json").write_text(json.dumps({"modules": {
            name: {"sha256": digest} for name, digest in EXPECTED_SHA256.items()}}))
        output = self.d / "abi.json"
        script = Path(__file__).resolve().parents[1] / "tools" / "triage_original_abi_atlas.py"
        run = subprocess.run(
            [sys.executable, str(script), "--inventory", str(self.d),
             "--ghidra", str(self.d), "--out", str(output)],
            capture_output=True, text=True, check=False)
        self.assertEqual(run.returncode, 0, run.stderr)
        stdout = json.loads(run.stdout)
        self.assertEqual(set(stdout), {"client", "server"})
        self.assertEqual(stdout["client"]["ghidra_inferred_starts_matched"], 1)
        data = json.loads(output.read_text(encoding="utf-8"))
        self.assertEqual(data["_authentication"]["both_originals_verified"], False)
        self.assertFalse(data["client"]["ranked_candidates"][1]["whole_body_sha_verified_by_this_tool"])

if __name__ == "__main__":
    unittest.main()
