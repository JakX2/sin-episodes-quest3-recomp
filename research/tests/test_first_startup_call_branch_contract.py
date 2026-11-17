"""Fail-closed source contract for the owner-private bounded Ghidra review.

These are synthetic/static tests, not original-DLL evidence or Ghidra execution.
"""
from pathlib import Path
import unittest

SCRIPT = (Path(__file__).resolve().parents[1] / "ghidra" /
          "ExportFirstStartupCallBranchCandidates.java")


class FirstStartupCallBranchContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = SCRIPT.read_text(encoding="utf-8")

    def test_only_reviewed_startup_entries(self):
        for text in ("0x240FE0E0L", "0x221E7640L",
                     '"client.dll"', '"server.dll"', '"VClient011"',
                     '"ServerGameDLL003"', "entry != expected"):
            self.assertIn(text, self.code)

    def test_owner_private_gate_and_seed_provenance(self):
        for text in ("SIN_PRIVATE_DECOMP_ACK",
                     "sha256_authenticated_original_pe32_vtable_prefix",
                     "Duplicate startup entry", "Refuse private output overwrite"):
            self.assertIn(text, self.code)

    def test_hard_limits(self):
        for text in ("MAX_BYTES = 65536", "MAX_INSTRUCTIONS = 25000",
                     "MAX_CANDIDATES = 8", "PRECEDING_WINDOW = 32",
                     "FOLLOWING_WINDOW = 16", "monitor.checkCancelled()"):
            self.assertIn(text, self.code)

    def test_not_a_verified_factory_or_branch(self):
        for text in ("ghidra_inferred_nearby_sites_not_verified_factory_or_failure",
                     "verified_factory_calls=0 verified_failure_edges=0"):
            self.assertIn(text, self.code)
        self.assertNotIn("getDecompiledFunction", self.code)
        self.assertNotIn("getBytes(", self.code)


if __name__ == "__main__":
    unittest.main()
