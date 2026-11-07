#!/usr/bin/env python3
"""Synthetic tests of atomic and exclusive evidence-report publication.

The original DLL audit is mocked. No original-binary claims are made.
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


class AtomicReportTests(unittest.TestCase):
    def test_failed_publish_leaves_no_final_or_staged_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "synthetic.dll"
            source.write_bytes(b"fixture")
            output = root / "out" / "evidence.json"
            with patch.object(audit, "audit", return_value={"synthetic": True}):
                with patch.object(audit.os, "link", side_effect=OSError("simulated publish failure")):
                    with self.assertRaises(SystemExit) as caught:
                        audit.main(["--server", str(source), "--output", str(output)])
            self.assertEqual(caught.exception.code, 2)
            self.assertFalse(output.exists())
            self.assertEqual(list(output.parent.iterdir()), [])

    def test_exclusive_publish_preserves_existing_report_and_cleans_stage(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "synthetic.dll"
            source.write_bytes(b"fixture")
            output = root / "evidence.json"
            output.write_bytes(b"existing evidence")
            with patch.object(audit, "audit", return_value={"synthetic": True}):
                with self.assertRaises(SystemExit) as caught:
                    audit.main(["--server", str(source), "--output", str(output)])
            self.assertEqual(caught.exception.code, 2)
            self.assertEqual(output.read_bytes(), b"existing evidence")
            self.assertEqual(sorted(p.name for p in root.iterdir()), ["evidence.json", "synthetic.dll"])

    def test_successful_publish_leaves_only_final_report(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "synthetic.dll"
            source.write_bytes(b"fixture")
            output = root / "evidence.json"
            with patch.object(audit, "audit", return_value={"synthetic": True}):
                self.assertEqual(audit.main(["--server", str(source), "--output", str(output)]), 0)
            self.assertEqual(output.read_text(encoding="utf-8"), '{\n  "synthetic": true\n}\n')
            self.assertEqual(sorted(p.name for p in root.iterdir()), ["evidence.json", "synthetic.dll"])


if __name__ == "__main__":
    unittest.main()
