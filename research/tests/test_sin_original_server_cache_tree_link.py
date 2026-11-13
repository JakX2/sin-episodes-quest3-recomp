#!/usr/bin/env python3
"""Original-binary-free adversarial regression tests for Phase196 link audit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_server_cache_tree_link import (
    ORIGINALS, INCOMING, OUTGOING_CALL_OFFSETS, WITNESSES,
    relative_call, verify_native, audit, VerificationError,
)

class SyntheticPE:
    base = 0x22000000
    def __init__(self) -> None:
        self.mem: dict[int, int] = {}
        for spec in ORIGINALS.values():
            va = spec["va"]
            for offset, witness_hex in WITNESSES.items():
                self.put(va + offset, bytes.fromhex(witness_hex))
            for offset in OUTGOING_CALL_OFFSETS:
                self.put(va + offset, relative_call(va + offset, spec["balance"]))
            self.put(va + spec["length"], b"\xcc" * 3)
        for at, target in INCOMING.items():
            self.put(at, relative_call(at, target))

    def put(self, address: int, payload: bytes) -> None:
        for i, byte in enumerate(payload):
            pos = address + i
            if pos in self.mem and self.mem[pos] != byte:
                raise AssertionError("overlapping inconsistent synthetic opcode %#x" % pos)
            self.mem[pos] = byte

    def at(self, address: int, size: int) -> bytes:
        return bytes(self.mem.get(address + i, 0) for i in range(size))

    def section(self, address: int) -> str:
        return ".text"

    def corrupt(self, address: int) -> None:
        self.mem[address] = self.mem.get(address, 0) ^ 1

class Tests(unittest.TestCase):
    def test_synthetic_witnesses_never_claim_original_hashes(self):
        report = verify_native(SyntheticPE(), check_hash=False)
        self.assertEqual(report["sha_verified_original_functions"], 0)
        self.assertEqual(report["sha_verified_native_code_bytes"], 0)
        self.assertEqual(report["sparse_opcode_witnesses"], 36)
        self.assertEqual(report["exact_outgoing_balance_calls"], 6)
        self.assertEqual(report["exact_incoming_insert_calls"], 2)

    def test_all_36_independent_native_instruction_witnesses(self):
        for name, spec in ORIGINALS.items():
            for offset in WITNESSES:
                with self.subTest(name=name, offset=offset):
                    fake = SyntheticPE()
                    fake.corrupt(spec["va"] + offset)
                    with self.assertRaises(VerificationError):
                        verify_native(fake, check_hash=False)

    def test_all_six_balance_call_targets(self):
        for name, spec in ORIGINALS.items():
            for offset in OUTGOING_CALL_OFFSETS:
                with self.subTest(name=name, offset=offset):
                    fake = SyntheticPE()
                    fake.corrupt(spec["va"] + offset + 1)
                    with self.assertRaises(VerificationError):
                        verify_native(fake, check_hash=False)

    def test_both_incoming_phase193_calls(self):
        for site in INCOMING:
            with self.subTest(source=site):
                fake = SyntheticPE()
                fake.corrupt(site + 1)
                with self.assertRaises(VerificationError):
                    verify_native(fake, check_hash=False)

    def test_method_terminator_and_alignment(self):
        for spec in ORIGINALS.values():
            fake = SyntheticPE()
            fake.corrupt(spec["va"] + spec["length"])
            with self.assertRaises(VerificationError):
                verify_native(fake, check_hash=False)

    def test_unwitnessed_difference_breaks_whole_body_twin(self):
        fake = SyntheticPE()
        fake.corrupt(ORIGINALS["scene"]["va"] + 0x3A)
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=False)

    def test_wrong_original_image_base_or_section(self):
        fake = SyntheticPE()
        fake.base = 0x24000000
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=False)
        fake = SyntheticPE()
        fake.section = lambda _: ".rdata"
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=False)

    def test_synthetic_function_or_dll_cannot_claim_original_hash(self):
        with self.assertRaises(VerificationError):
            verify_native(SyntheticPE(), check_hash=True)
        with self.assertRaises(VerificationError):
            audit(b"MZ", b"MZ")

if __name__ == "__main__":
    unittest.main()
