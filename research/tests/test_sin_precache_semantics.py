"""Synthetic, original-DLL-free evidence tests for SiN precache decoding."""
from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_precache_semantics import (
    FUNCTIONS, HELPERS, immediate, precache_calls, printable_rdata,
)


class FakePE:
    def __init__(self):
        self.data = {
            0x22400010: b"grenade_ar\x00" + b"\x00" * 150,
            0x22400080: b"NPC_BigMutant.Walk\x00" + b"\x00" * 145,
        }

    def section_of(self, va):
        return ".rdata" if va in self.data else ".text"

    def at(self, va, size):
        if va not in self.data or size > len(self.data[va]):
            raise ValueError("not found")
        return self.data[va][:size]


class PrecacheTests(unittest.TestCase):
    def test_precache_other_classname_from_last_immediate_push(self):
        rows = [
            (0x22313781, "push", "0x0"),
            (0x22313783, "push", "0x22400010"),
            (0x22313788, "mov", "esi,ecx"),
            (0x2231378A, "call", "0x22287cb0"),
            (0x22313790, "ret", ""),
        ]
        result = precache_calls(FakePE(), rows)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]["helper_kind"], "entity")
        self.assertEqual(result[0]["literal"], "grenade_ar")
        self.assertEqual(result[0]["callsite_va"], "0x2231378A")
        self.assertEqual(result[0]["argument_status"], "literal")

    def test_never_infer_dynamic_sound_or_model_arguments(self):
        rows = [
            (0x22300000, "push", "eax"),
            (0x22300002, "call", "0x22149b70"),
            (0x22300007, "mov", "eax,DWORD PTR [esi+0x498]"),
            (0x2230000D, "push", "eax"),
            (0x2230000E, "call", "0x221b6170"),
        ]
        result = precache_calls(FakePE(), rows)
        self.assertEqual([x["helper_kind"] for x in result],
                         ["model", "sound_script"])
        self.assertTrue(all(x["argument_status"] == "dynamic_or_unresolved"
                            for x in result))
        self.assertTrue(all(x["literal"] is None for x in result))

    def test_rdata_and_immediate_validation(self):
        pe = FakePE()
        self.assertEqual(printable_rdata(pe, 0x22400080),
                         "NPC_BigMutant.Walk")
        self.assertIsNone(printable_rdata(pe, 0x22310000))
        self.assertEqual(immediate("0x22287cb0"), 0x22287CB0)
        self.assertIsNone(immediate("DWORD PTR [eax+0x10]"))
        self.assertIsNone(immediate("eax"))

    def test_known_original_precache_ranges_do_not_overlap(self):
        assert len(HELPERS) == 3
        ordered = sorted(FUNCTIONS.values())
        for start, stop in ordered:
            self.assertLess(start, stop)
            self.assertLessEqual(stop-start, 0x300)
        for (_, left_stop), (right_start, _) in zip(ordered, ordered[1:]):
            self.assertLessEqual(left_stop, right_start)

    def test_nonprecache_calls_are_never_classified(self):
        rows = [(0x22300001, "push", "0x22400010"),
                (0x22300005, "call", "0x22266b00")]
        self.assertEqual(precache_calls(FakePE(), rows), [])


if __name__ == "__main__":
    unittest.main()
