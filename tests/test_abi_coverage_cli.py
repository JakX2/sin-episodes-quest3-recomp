"""End-to-end ABI coverage CLI tests using synthetic metadata only."""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "abi_anchor_coverage.py"


class CoverageCliTests(unittest.TestCase):
    def invoke(self, payload, *args):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "queue.json"
            source.write_text(json.dumps(payload), encoding="utf-8")
            return subprocess.run(
                [sys.executable, str(CLI), str(source), *args],
                capture_output=True, text=True, timeout=20, check=False,
            )

    def test_raw_queue_report(self):
        result = self.invoke({"records": [
            {"module": "client", "interface": "VClient011",
             "rva": "0x0002", "evidence": "candidate"}
        ]})
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["total_records"], 1)
        self.assertEqual(len(report["missing_anchors"]), 5)

    def test_required_anchors_fail_closed(self):
        result = self.invoke({"records": []}, "--require-all-anchors")
        self.assertEqual(result.returncode, 3)
        self.assertIn("missing ABI anchor records", result.stderr)

    def test_tampered_report_is_rejected(self):
        sys.path.insert(0, str(ROOT / "tools"))
        from abi_evidence_queue import report, validate
        payload = report(validate({"records": []}))
        payload["queue_sha256"] = "0" * 64
        result = self.invoke(payload)
        self.assertEqual(result.returncode, 2)
        self.assertIn("rejected", result.stderr)

    def test_private_fields_are_rejected(self):
        result = self.invoke({"records": [
            {"module": "client", "interface": "VClient011",
             "rva": "0x2", "evidence": "candidate",
             "pseudocode": "not allowed"}
        ]})
        self.assertEqual(result.returncode, 2)
        self.assertIn("rejected", result.stderr)


if __name__ == "__main__":
    unittest.main()
