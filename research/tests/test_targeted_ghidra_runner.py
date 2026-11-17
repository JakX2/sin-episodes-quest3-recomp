"""Synthetic-only fail-closed tests for opt-in local target-only Ghidra mode."""
from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

RUNNER = Path(__file__).resolve().parents[1] / "tools" / "run_ghidra_original.sh"


class TargetedModeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.ghidra = self.root / "fake-ghidra"
        (self.ghidra / "support").mkdir(parents=True)
        self.launched = self.root / "headless-was-run"
        headless = self.ghidra / "support" / "analyzeHeadless"
        headless.write_text(
            "#!/bin/sh\nprintf invoked > \"{}\"\n".format(self.launched),
            encoding="utf-8",
        )
        headless.chmod(0o755)
        self.originals = self.root / "bin"
        self.originals.mkdir()
        self.output = self.root / "owner-private-output"

    def run_script(self, mode="targeted-c"):
        return subprocess.run(
            ["bash", str(RUNNER), str(self.ghidra),
             str(self.originals), str(self.output), mode],
            text=True, capture_output=True, check=False,
            env={**os.environ, "SIN_GHIDRA_SEED_DIRECTORY": ""},
        )

    def test_wrong_originals_never_reach_decompiler(self):
        (self.originals / "client.dll").write_bytes(b"synthetic client only")
        (self.originals / "server.dll").write_bytes(b"synthetic server only")
        result = self.run_script()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SHA256 mismatch", result.stderr)
        self.assertFalse(self.launched.exists())
        self.assertFalse((self.output / "client.dll_PRIVATE_TARGETED_DECOMPILATION").exists())

    def test_missing_originals_fail_before_any_analysis(self):
        result = self.run_script()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing owner-original", result.stderr)
        self.assertFalse(self.launched.exists())

    def test_unknown_mode_fails_closed(self):
        result = self.run_script("unsafe-public-upload")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Mode must be", result.stderr)
        self.assertFalse(self.launched.exists())

    def test_runner_explicitly_guards_output_and_runs_sha_gate(self):
        source = RUNNER.read_text(encoding="utf-8")
        self.assertIn('if [[ "$mode" == "targeted-c" ]]', source)
        self.assertIn("verify_original_batch.py", source)
        self.assertIn("ExportTargetedInterfacePseudocode.java", source)
        self.assertIn("Refusing to generate game pseudocode inside the public checkout", source)


if __name__ == "__main__":
    unittest.main()
