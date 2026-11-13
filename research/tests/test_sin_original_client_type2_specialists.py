#!/usr/bin/env python3
"""Synthetic-only tests for Phase215 original client snapshot type-2 specialists."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_client_type2_specialists import (
    METHODS, NESTED_CALLS, STRINGS, BASE, VerificationError,
    rel_call, verify_native, audit,
)


class FakeOriginalPE:
    def __init__(self):
        self.base = BASE["client"]
        self.memory = {}
        self.strings = {
            address + i for address, value in STRINGS.items()
            for i in range(len(value) + 1)
        }
        for va, size, _digest, pad, router, ret_pop, witnesses in METHODS.values():
            for off, hx in witnesses.items():
                self.write(va + off, bytes.fromhex(hx))
            self.write(va + size - 3, b"\xc2" + ret_pop.to_bytes(2, "little"))
            self.write(va + size, b"\xcc" * pad)
            self.write(router, rel_call(router, va))
        for caller, callee in NESTED_CALLS:
            self.write(caller, rel_call(caller, callee))
        for va, value in STRINGS.items():
            self.write(va, value.encode("ascii") + b"\0")

    def write(self, va: int, data: bytes):
        for i, item in enumerate(data):
            previous = self.memory.get(va + i, item)
            if previous != item:
                raise AssertionError("inconsistent synthetic native witness")
            self.memory[va + i] = item

    def at(self, va: int, size: int) -> bytes:
        return bytes(self.memory.get(va + i, 0) for i in range(size))

    def section_of(self, va: int) -> str:
        return ".rdata" if va in self.strings else ".text"

    def corrupt(self, va: int):
        self.memory[va] = self.memory.get(va, 0) ^ 1


class Phase215Tests(unittest.TestCase):
    def test_original_free_fixture_never_claims_sha_verified_recovery(self):
        fake = FakeOriginalPE()
        result = verify_native(fake, check_hash=False)
        self.assertEqual(result["new_original_SHA_verified_native_functions"], 0)
        self.assertEqual(result["new_complete_original_x86_bytes"], 0)
        self.assertEqual(result["direct_router_calls"], 3)
        self.assertEqual(result["nested_diagnostic_calls"], 3)
        self.assertEqual(result["sparse_instruction_witnesses"], 23)
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=True)
        with self.assertRaises(VerificationError):
            audit(b"MZ", b"MZ")

    def test_corrupt_every_sparse_x86_witness(self):
        for name, (va, _, _, _, _, _, witnesses) in METHODS.items():
            for off in witnesses:
                with self.subTest(name=name, offset=hex(off)):
                    fake = FakeOriginalPE()
                    fake.corrupt(va + off)
                    with self.assertRaises(VerificationError):
                        verify_native(fake, check_hash=False)

    def test_corrupt_each_native_caller_and_diagnostic_call(self):
        callers = [(spec[4], spec[0]) for spec in METHODS.values()]
        callers += list(NESTED_CALLS)
        for caller, callee in callers:
            with self.subTest(callsite=hex(caller), callee=hex(callee)):
                fake = FakeOriginalPE()
                fake.corrupt(caller + 1)
                with self.assertRaises(VerificationError):
                    verify_native(fake, check_hash=False)

    def test_corrupt_each_ret_padding_and_ret_arity(self):
        for name, (va, size, _, pad, _, ret_pop, _) in METHODS.items():
            with self.subTest(method=name):
                fake = FakeOriginalPE()
                fake.corrupt(va + size - 2)
                with self.assertRaises(VerificationError):
                    verify_native(fake, check_hash=False)
            for i in range(pad):
                with self.subTest(method=name, padding=i):
                    fake = FakeOriginalPE()
                    fake.corrupt(va + size + i)
                    with self.assertRaises(VerificationError):
                        verify_native(fake, check_hash=False)

    def test_corrupt_original_diagnostic_strings(self):
        for va, value in STRINGS.items():
            for i in (0, len(value)):
                with self.subTest(va=hex(va), byte=i):
                    fake = FakeOriginalPE()
                    fake.corrupt(va + i)
                    with self.assertRaises(VerificationError):
                        verify_native(fake, check_hash=False)

    def test_reject_bad_code_or_literal_sections_and_image_base(self):
        fake = FakeOriginalPE()
        fake.base += 0x1000
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=False)
        fake = FakeOriginalPE()
        fake.section_of = lambda _va: ".data"
        with self.assertRaises(VerificationError):
            verify_native(fake, check_hash=False)

    def test_original_type2_complete_native_sizes_are_distinct(self):
        self.assertEqual(sum(x[1] for x in METHODS.values()), 178)
        self.assertEqual({x[5] for x in METHODS.values()}, {8, 12})
        self.assertEqual(
            {x[0] for x in METHODS.values()},
            {0x24198B80, 0x24197D00, 0x241984A0},
        )


if __name__ == "__main__":
    unittest.main()
