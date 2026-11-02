"""Independent instruction-graph regression tests; no commercial DLLs needed."""
from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_gameplay_control_flow import (
    direct_target, traverse, rdata_string, PEImage,
)
from test_sin_rtti_vtables import make_rtti


class OriginalCfgTests(unittest.TestCase):
    def test_follows_both_branches_and_direct_calls(self):
        code = {
            0x1000: ("push", "0x22410000"),
            0x1005: ("jne", "0x00001014"),
            0x1007: ("call", "0x00001100"),
            0x100c: ("ret", ""),
            0x1014: ("call", "DWORD PTR [eax+0x1c]"),
            0x1017: ("ret", ""),
            0x1100: ("ret", ""),
        }
        result = traverse(0x1000, code, sorted(code), {0x1000, 0x1100})
        self.assertEqual(result["reachable_instruction_count"], 6)
        self.assertEqual(result["conditional_branches"], 1)
        self.assertEqual(result["direct_callees"], [
            {"va": "0x00001100", "count": 1},
        ])
        self.assertEqual(result["indirect_call_count"], 1)
        self.assertFalse(result["possibly_truncated"])
        self.assertEqual(result["exit_kinds"]["ret"], 2)

    def test_does_not_walk_into_a_different_known_vtable_entry(self):
        code = {
            0x1000: ("jmp", "0x00001100"),
            0x1100: ("ret", ""),
        }
        result = traverse(0x1000, code, sorted(code), {0x1000, 0x1100})
        self.assertEqual(result["reachable_instruction_count"], 1)
        self.assertIn("0x00001100", result["external_or_other_vtable_branch_targets"])

    def test_records_unresolved_jump_table(self):
        code = {
            0x1000: ("jmp", "DWORD PTR [eax*4+0x22400000]"),
            0x1007: ("ret", ""),
        }
        result = traverse(0x1000, code, sorted(code), {0x1000})
        self.assertEqual(result["reachable_instruction_count"], 1)
        self.assertTrue(result["unresolved_branches"])

    def test_budget_expiry_explicit(self):
        code = {0x1000+i: ("nop", "") for i in range(30)}
        result = traverse(0x1000, code, sorted(code), {0x1000},
                          instruction_limit=5)
        self.assertEqual(result["reachable_instruction_count"], 5)
        self.assertTrue(result["possibly_truncated"])

    def test_pe32_rdata_string_in_original_free_fixture(self):
        image, _, _ = make_rtti()
        pe = PEImage(image)
        self.assertIsNone(rdata_string(pe, 0x22001000))
        self.assertIsNone(rdata_string(pe, 0x22500000))

    def test_target_pattern_never_accepts_register_indirect_call(self):
        self.assertEqual(direct_target("0x2235de00"), 0x2235de00)
        self.assertIsNone(direct_target("DWORD PTR [eax+0x1c]"))
        self.assertIsNone(direct_target("eax"))


if __name__ == "__main__":
    unittest.main()
