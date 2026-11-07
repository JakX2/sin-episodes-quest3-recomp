#!/usr/bin/env python3
"""Synthetic fail-closed tests for original assault condition registry audit.

No proprietary DLL required. Synthetic PE fixtures verify the audit's own
instruction/callsite checking, NOT the contents of the 2006 game binary.
"""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "audit_sin_assault_condition_registry",
    TOOLS / "audit_sin_assault_condition_registry.py",
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class SyntheticPE:
    base = 0x22000000

    def __init__(self):
        self.data = {}
        for name, number, symbol, immediate, push, call in audit.SOURCE_CONDITIONS:
            self.put(symbol, name.encode("ascii") + b"\0")
            self.put(immediate - 10, b"\x68" + struct.pack("<I", audit.REGISTRY_SOURCE_FILE_VA)
                     + b"\x68" + struct.pack("<I", audit.REGISTRY_CONTEXT_STRING_VA))
            self.put(immediate, bytes([0x6A, number]))
            self.put(push, b"\x68" + struct.pack("<I", symbol))
            self.put(push + 5, b"\xB9" + struct.pack("<I", audit.REGISTRY_THIS_VA))
            self.call(call, audit.REGISTRY_FUNCTION_VA)
        # The auditor also requires the original received-cue helper witness.
        # Without this, every negative test passes for the wrong reason:
        # an unrelated missing helper instruction rather than its mutation.
        for address, opcode in audit.RECEIVED_CUE_SETTER_WITNESSES:
            self.put(address, opcode)
        self.put(audit.RECEIVED_CUE_TAIL_JUMP_VA, b"\\xE9" + struct.pack(
            "<i", audit.RECEIVED_CUE_TAIL_TARGET_VA -
            audit.RECEIVED_CUE_TAIL_JUMP_VA - 5))
        for _, push, number, call, target in audit.ASSAULT_USE_SITES:
            self.put(push, bytes([0x6A, number]))
            self.call(call, target)

    def put(self, address, content):
        for offset, byte in enumerate(content):
            self.data[address + offset] = byte

    def at(self, address, length):
        return bytes(self.data.get(address + offset, 0) for offset in range(length))

    def call(self, address, target):
        self.put(address, b"\xE8" + struct.pack("<i", target - address - 5))


class RegistryAuditTests(unittest.TestCase):
    def test_valid_synthetic_registration_and_use_sites(self):
        result = audit.audit_original(SyntheticPE())
        self.assertEqual(result["verified_registration_count"], len(audit.SOURCE_CONDITIONS))
        self.assertEqual(result["verified_assault_callsites"], len(audit.ASSAULT_USE_SITES))
        self.assertIn("COND_PROVOKED", result["semantic_milestones"]["BeginAssault"])\n        self.assertEqual(result["original_received_cue_signaling_helper"]["tail_jump_target"], "0x22039760")

    def test_every_registration_rejects_corrupt_id(self):
        for name, number, _, immediate, _, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(condition=name):
                pe = SyntheticPE()
                pe.put(immediate + 1, bytes([number ^ 0x01]))
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_every_registration_rejects_corrupt_symbol(self):
        for name, _, symbol, _, _, _ in audit.SOURCE_CONDITIONS:
            with self.subTest(condition=name):
                pe = SyntheticPE()
                pe.put(symbol, b"X")
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_every_registration_rejects_wrong_target(self):
        for name, _, _, _, _, call in audit.SOURCE_CONDITIONS:
            with self.subTest(condition=name):
                pe = SyntheticPE()
                pe.call(call, audit.REGISTRY_FUNCTION_VA + 4)
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_every_use_site_rejects_wrong_condition(self):
        for role, push, number, _, _ in audit.ASSAULT_USE_SITES:
            with self.subTest(site=role):
                pe = SyntheticPE()
                pe.put(push + 1, bytes([number ^ 0x01]))
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_every_use_site_rejects_wrong_helper(self):
        for role, _, _, call, target in audit.ASSAULT_USE_SITES:
            with self.subTest(site=role):
                pe = SyntheticPE()
                pe.call(call, target + 4)
                with self.assertRaises(audit.VerificationError):
                    audit.audit_original(pe)

    def test_rejects_wrong_registry_receiver_and_prefix(self):
        pe = SyntheticPE()
        _, _, _, immediate, push, _ = audit.SOURCE_CONDITIONS[0]
        pe.put(push + 6, b"\xFF")
        with self.assertRaises(audit.VerificationError):
            audit.audit_original(pe)
        pe = SyntheticPE()
        pe.put(immediate - 10, b"\x90")
        with self.assertRaises(audit.VerificationError):
            audit.audit_original(pe)

    def test_rejects_wrong_direct_call_opcode(self):
        pe = SyntheticPE()
        _, _, _, call, _ = audit.ASSAULT_USE_SITES[0]
        pe.put(call, b"\x90")
        with self.assertRaises(audit.VerificationError):
            audit.audit_original(pe)


if __name__ == "__main__":
    unittest.main()
