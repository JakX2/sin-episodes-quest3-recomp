#!/usr/bin/env python3
"""Additional synthetic x86 registry witness corruption tests.

Exercises the exact argument pointer and opcode witnesses, not the original
proprietary DLL. Run with Python unittest discovery in research/tools.
"""
import unittest

from test_sin_assault_condition_registry import SyntheticPE, audit


class RegistrationArgumentWitnessTests(unittest.TestCase):
    def test_each_symbol_pointer_must_match_registered_string(self):
        for name, _, _, _, push, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(name=name):
                pe = SyntheticPE()
                pe.put(push + 1, b"\\x00")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_each_symbol_push_opcode_must_be_push_imm32(self):
        for name, _, _, _, push, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(name=name):
                pe = SyntheticPE()
                pe.put(push, b"\\x90")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_each_numeric_condition_push_opcode_must_be_push_imm8(self):
        for name, _, _, immediate, _, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(name=name):
                pe = SyntheticPE()
                pe.put(immediate, b"\\x90")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_each_assault_use_site_push_opcode_must_match(self):
        for role, push, _, _, _ in audit.ASSAULT_USE_SITES:
            with self.subTest(site=role):
                pe = SyntheticPE()
                pe.put(push, b"\\x90")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_each_registry_context_pointer_must_match(self):
        for name, _, _, immediate, _, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(name=name):
                pe = SyntheticPE()
                pe.put(immediate - 5 + 1, b"\\x00")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)


if __name__ == "__main__":
    unittest.main()
