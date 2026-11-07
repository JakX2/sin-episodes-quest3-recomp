"""Synthetic x86 regression tests. Never bundles a commercial SiN DLL."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "setparameters", TOOLS / "audit_sin_assault_setparameters_outcomes.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    base = 0x22000000

    def __init__(self):
        self.memory = {va: bytes.fromhex(sig) for va, sig, _ in a.WITNESSES}
        self.memory[0x223F3C7C] = b"CAI_AssaultBehavior::SetParameters\0"
        for va, target, _ in a.CALLS:
            self.memory[va] = b"\xe8" + struct.pack("<i", target - va - 5)

    def at(self, va, n):
        for start, data in self.memory.items():
            if start <= va and va + n <= start + len(data):
                return data[va - start:va - start + n]
        raise a.VerificationError(f"missing fixture instruction at {va:#x}")


class SetParametersOutcomeTests(unittest.TestCase):
    def test_valid_native_path_fixture(self):
        r = a.verify_original(FakePE())
        self.assertEqual((r["verified_witnesses"], r["verified_branches"], r["verified_calls"]),
                         (26, 5, 3))
        self.assertIn("received cue +0x20 = 0", r["outcomes"]["candidate_resolved"])

    def test_branches_resolve_as_expected(self):
        pe = FakePE()
        for va, target, _ in a.BRANCHES:
            self.assertEqual(a.branch_destination(pe, va), target)

    def test_missing_candidate_guard_corruption(self):
        pe = FakePE()
        pe.memory[0x220CFE22] = bytes.fromhex("85 db 75 1e")
        with self.assertRaisesRegex(a.VerificationError, "no best rally candidate"):
            a.verify_original(pe)

    def test_missing_candidate_exit_corruption(self):
        pe = FakePE()
        pe.memory[0x220CFE3E] = b"\xe9" + struct.pack("<i", 0x220CFED4 - 0x220CFE43)
        with self.assertRaisesRegex(a.VerificationError, "missing-candidate exit jump"):
            a.verify_original(pe)

    def test_failed_link_mode_zero_corruption(self):
        pe = FakePE()
        pe.memory[0x220CFEA0] = bytes.fromhex("c7 47 1c 01 00 00 00")
        with self.assertRaisesRegex(a.VerificationError, "invalid handle clears"):
            a.verify_original(pe)

    def test_success_mode_field_corruption(self):
        pe = FakePE()
        pe.memory[0x220CFED8] = bytes.fromhex("89 4f 20")
        with self.assertRaisesRegex(a.VerificationError, "valid point assigns"):
            a.verify_original(pe)

    def test_success_mode_argument_corruption(self):
        pe = FakePE()
        pe.memory[0x220CFED4] = bytes.fromhex("8b 4c 24 58")
        with self.assertRaisesRegex(a.VerificationError, "requested cue mode"):
            a.verify_original(pe)

    def test_reset_cue_offset_corruption(self):
        pe = FakePE()
        pe.memory[0x220CE979] = bytes.fromhex("89 5e 1c")
        with self.assertRaisesRegex(a.VerificationError, "resets received"):
            a.verify_original(pe)

    def test_success_reset_call_wrong_target(self):
        pe = FakePE()
        va = 0x220CFEDD
        pe.memory[va] = b"\xe8" + struct.pack("<i", 0x220CE520 - va - 5)
        with self.assertRaisesRegex(a.VerificationError, "initialization/reset helper"):
            a.verify_original(pe)

    def test_goal_call_wrong_target(self):
        pe = FakePE()
        va = 0x220CFF27
        pe.memory[va] = b"\xe8" + struct.pack("<i", 0x220CE520 - va - 5)
        with self.assertRaisesRegex(a.VerificationError, "goal setup"):
            a.verify_original(pe)

    def test_diagnostic_class_name_corruption(self):
        pe = FakePE()
        pe.memory[0x223F3C7C] = b"X" + pe.memory[0x223F3C7C][1:]
        with self.assertRaisesRegex(a.VerificationError, "diagnostic name"):
            a.verify_original(pe)

    def test_binary_hash_failure_precedes_pe_parser(self):
        with self.assertRaisesRegex(a.VerificationError, "SHA-256 mismatch"):
            a.audit(b"not the original 2006 server.dll")


if __name__ == "__main__":
    unittest.main()
