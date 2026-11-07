#!/usr/bin/env python3
"""Synthetic CLI success-path tests for the hash-pinned assault evidence report.

The audit itself is stubbed: these tests prove report I/O semantics only,
not the authenticity of any original DLL or the accuracy of its disassembly.
"""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "audit_sin_assault_condition_registry", TOOLS / "audit_sin_assault_condition_registry.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class AssaultAuditReportSuccessTests(unittest.TestCase):
    def test_successful_report_is_json_and_exclusive_on_second_run(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "synthetic.dll"
            report = root / "nested" / "assault-report.json"
            binary.write_bytes(b"synthetic-fixture")
            payload = {"server_sha256": "synthetic-only", "conditions": [0x18, 0x33]}
            with patch.object(audit, "audit", return_value=payload) as fake_audit:
                self.assertEqual(audit.main(["--server", str(binary), "--output", str(report)]), 0)
                self.assertEqual(json.loads(report.read_text(encoding="utf-8")), payload)
                original = report.read_bytes()
                with self.assertRaises(SystemExit) as caught:
                    audit.main(["--server", str(binary), "--output", str(report)])
                self.assertEqual(caught.exception.code, 2)
                self.assertEqual(report.read_bytes(), original)
                self.assertEqual(fake_audit.call_count, 2)
                fake_audit.assert_any_call(b"synthetic-fixture")

    def test_bad_sha_does_not_create_output_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "synthetic.dll"
            report = root / "missing-parent" / "report.json"
            binary.write_bytes(b"not-original")
            with self.assertRaises(SystemExit) as caught:
                audit.main(["--server", str(binary), "--output", str(report)])
            self.assertEqual(caught.exception.code, 2)
            self.assertFalse(report.parent.exists())


if __name__ == "__main__":
    unittest.main()
