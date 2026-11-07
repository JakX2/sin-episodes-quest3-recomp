#!/usr/bin/env python3
"""Fail-closed integrity tests for the original-DLL assault registry audit CLI.

Synthetic inputs only: these tests do not establish any original DLL bytes.
"""
import importlib.util
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


class AssaultAuditIntegrityTests(unittest.TestCase):
    def test_rejects_nonmatching_sha_before_parsing_pe(self):
        # A tampered or synthetic DLL must not be interpreted as original evidence.
        with patch.object(audit, "PEImage") as parser:
            with self.assertRaisesRegex(audit.VerificationError, "SHA-256 mismatch"):
                audit.audit(b"synthetic-not-the-original-server-dll")
            parser.assert_not_called()

    def test_cli_does_not_overwrite_existing_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "synthetic.dll"
            report = root / "existing-report.json"
            binary.write_bytes(b"synthetic")
            report.write_text('{"preserve": true}\n', encoding="utf-8")
            # Stub only the original binary audit: exercise actual CLI output handling.
            with patch.object(audit, "audit", return_value={"synthetic": True}):
                with self.assertRaises(SystemExit) as caught:
                    audit.main(["--server", str(binary), "--output", str(report)])
            self.assertEqual(caught.exception.code, 2)
            self.assertEqual(report.read_text(encoding="utf-8"), '{"preserve": true}\n')

    def test_cli_rejects_bad_sha_without_creating_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "synthetic.dll"
            report = root / "should-not-exist.json"
            binary.write_bytes(b"synthetic")
            with self.assertRaises(SystemExit) as caught:
                audit.main(["--server", str(binary), "--output", str(report)])
            self.assertEqual(caught.exception.code, 2)
            self.assertFalse(report.exists())


if __name__ == "__main__":
    unittest.main()
