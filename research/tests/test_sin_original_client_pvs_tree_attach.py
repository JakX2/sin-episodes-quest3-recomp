#!/usr/bin/env python3
"""Phase239 synthetic-only mutation tests; no artificial original-code credit."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import audit_sin_original_client_pvs_tree_attach as native


class SyntheticPE:
    def __init__(self):
        self.base = native.ORIGINALS["client"]["base"]
        self.memory = {}
        self.text = set()
        va = native.METHOD["entry"]
        size = native.METHOD["length"]
        self.write(va, bytes(size))
        self.write(va + size, b"\xCC" * native.METHOD["trailing_int3"])
        self.text.update(range(va, va + size))
        for off in native.METHOD["ret12_offsets"]:
            self.write(va + off, b"\xC2\x0C\x00")
        for off, hx in native.METHOD["sparse_witnesses"].items():
            self.write(va + off, bytes.fromhex(hx))
        for site, dest in native.CALLS:
            self.write(site, native.e8_call(site, dest))
            self.text.update(range(site, site + 5))

    def write(self, va, content):
        for i, value in enumerate(content):
            self.memory[va + i] = value

    def at(self, va, size):
        return bytes(self.memory.get(va + i, 0) for i in range(size))

    def section_of(self, va):
        return ".text" if va in self.text else ""

    def flip(self, va):
        self.memory[va] = self.memory.get(va, 0) ^ 1


class Phase239SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def check(pe):
        return native.verify_native(pe, check_hash=False,
                                    authentic_provenance=False)

    def test_synthetic_fixture_cannot_claim_original_function(self):
        result = self.check(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"], 0)
        self.assertEqual(result["new_original_native_code_bytes"], 0)
        self.assertEqual(result["original_method_native_bytes"], 106)
        self.assertEqual(result["checked_sparse_original_witnesses"], 19)
        self.assertEqual(result["checked_direct_original_E8_edges"], 4)
        self.assertEqual(result["original_ret12_exits"], 3)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(), check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ", b"MZ")

    def test_all_sparse_native_instruction_witnesses_are_pinned(self):
        for off in native.METHOD["sparse_witnesses"]:
            with self.subTest(offset=hex(off)):
                fake = SyntheticPE()
                fake.flip(native.METHOD["entry"] + off)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_all_original_RET12_exits_and_INT3_padding(self):
        va = native.METHOD["entry"]
        for off in native.METHOD["ret12_offsets"]:
            for byte in range(3):
                with self.subTest(ret=hex(off), byte=byte):
                    fake = SyntheticPE()
                    fake.flip(va + off + byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)
        for byte in range(native.METHOD["trailing_int3"]):
            with self.subTest(pad=byte):
                fake = SyntheticPE()
                fake.flip(va + native.METHOD["length"] + byte)
                with self.assertRaises(native.VerificationError):
                    self.check(fake)

    def test_three_balancer_calls_and_original_caller_E8(self):
        self.assertEqual(native.CALLS, (
            (0x24107724, 0x24106E30),
            (0x24107733, 0x24106E30),
            (0x24107741, 0x24106E30),
            (0x24108786, 0x241076E0)
        ))
        for site, dest in native.CALLS:
            for byte in range(5):
                with self.subTest(site=hex(site), byte=byte):
                    fake = SyntheticPE()
                    fake.flip(site + byte)
                    with self.assertRaises(native.VerificationError):
                        self.check(fake)

    def test_original_PE_base_and_complete_text_boundaries(self):
        fake = SyntheticPE()
        fake.base += 0x1000
        with self.assertRaises(native.VerificationError):
            self.check(fake)
        for point in (native.METHOD["entry"],
                      native.METHOD["entry"] + native.METHOD["length"] - 1):
            fake = SyntheticPE()
            fake.text.remove(point)
            with self.assertRaises(native.VerificationError):
                self.check(fake)

    def test_real_review_credit_requires_real_both_DLL_SHA256s(self):
        self.assertEqual(native.METHOD["length"], 106)
        self.assertEqual(native.METHOD["ret12_offsets"], (0x4A, 0x59, 0x67))
        self.assertEqual(native.METHOD["trailing_int3"], 6)
        self.assertEqual(len(native.METHOD["sparse_witnesses"]), 19)
        self.assertEqual(native.METHOD["sha256"],
                         "fd141c9025d7975a43a24b398a6b5bee1f34cead227171e5319c269e78c8df9f")


if __name__ == "__main__":
    unittest.main()
