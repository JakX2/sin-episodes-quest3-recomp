#!/usr/bin/env python3
"""Mutation diagnostics for the synthetic original assault registry audit.

These tests use synthetic instruction bytes only. They do not inspect or
authenticate the proprietary 2006 server.dll.
"""
import struct
import unittest

from test_sin_assault_condition_registry import SyntheticPE, audit


class AssaultAuditDiagnosticTests(unittest.TestCase):
    def test_positive_fixture_reaches_all_audit_sections(self):
        report = audit.audit_original(SyntheticPE())
        self.assertEqual(report["verified_registration_count"], 9)
        self.assertEqual(report["verified_assault_callsites"], 7)
        self.assertEqual(
            report["original_received_cue_signaling_helper"]["tail_jump_target"],
            "0x22039760",
        )

    def test_registration_mutations_fail_at_the_intended_witness(self):
        for name, number, symbol, immediate, push, call in audit.SOURCE_CONDITIONS:
            with self.subTest(name=name, witness="numeric"):
                pe = SyntheticPE()
                pe.put(immediate + 1, bytes((number ^ 1,)))
                with self.assertRaisesRegex(
                    audit.VerificationError, "original ID instruction mismatch"
                ):
                    audit.audit_original(pe)
            with self.subTest(name=name, witness="symbol"):
                pe = SyntheticPE()
                pe.put(symbol, b"X")
                with self.assertRaisesRegex(
                    audit.VerificationError, "original string literal mismatch"
                ):
                    audit.audit_original(pe)
            with self.subTest(name=name, witness="call"):
                pe = SyntheticPE()
                pe.call(call, audit.REGISTRY_FUNCTION_VA + 4)
                with self.assertRaisesRegex(
                    audit.VerificationError, "call at .* changed to"
                ):
                    audit.audit_original(pe)

    def test_use_site_mutations_fail_at_the_intended_witness(self):
        for role, push, number, call, target in audit.ASSAULT_USE_SITES:
            with self.subTest(role=role, witness="condition"):
                pe = SyntheticPE()
                pe.put(push + 1, bytes((number ^ 1,)))
                with self.assertRaisesRegex(
                    audit.VerificationError, "original condition argument changed"
                ):
                    audit.audit_original(pe)
            with self.subTest(role=role, witness="target"):
                pe = SyntheticPE()
                pe.call(call, target + 4)
                with self.assertRaisesRegex(
                    audit.VerificationError, "call at .* changed to"
                ):
                    audit.audit_original(pe)

    def test_cue_helper_mutations_fail_at_the_intended_witness(self):
        for address, opcode in audit.RECEIVED_CUE_SETTER_WITNESSES:
            with self.subTest(address=hex(address)):
                pe = SyntheticPE()
                pe.put(address, bytes((opcode[0] ^ 1,)))
                with self.assertRaisesRegex(
                    audit.VerificationError,
                    "generic assault received-cue helper changed at",
                ):
                    audit.audit_original(pe)
        pe = SyntheticPE()
        pe.put(audit.RECEIVED_CUE_TAIL_JUMP_VA, b"\x90")
        with self.assertRaisesRegex(
            audit.VerificationError, "generic cue helper no longer tail-jumps"
        ):
            audit.audit_original(pe)
        pe = SyntheticPE()
        offset = (
            audit.RECEIVED_CUE_TAIL_TARGET_VA + 4
            - audit.RECEIVED_CUE_TAIL_JUMP_VA - 5
        )
        pe.put(
            audit.RECEIVED_CUE_TAIL_JUMP_VA,
            b"\xE9" + struct.pack("<i", offset),
        )
        with self.assertRaisesRegex(
            audit.VerificationError, "generic cue helper no longer signals COND_PROVOKED"
        ):
            audit.audit_original(pe)


if __name__ == "__main__":
    unittest.main()
