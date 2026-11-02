"""Synthetic tests for original SiN dual-DLL code census. No proprietary data."""
from __future__ import annotations
import pathlib
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_entire_dll_code_census import direct,inspect_lines,rdata_string

class FakePE:
    def in_section(self,va,name):
        return name==".rdata" and 0x223D2000<=va<0x223D2080
    def section(self,name):
        return (0x223D2000,b"grenade_ar\x00"+bytes(128))

class OriginalDllCensusTests(unittest.TestCase):
    def test_direct_call_vs_virtual_indirect(self):
        self.assertEqual(direct("0x22287cb0"),0x22287CB0)
        self.assertIsNone(direct("DWORD PTR [eax+0x50]"))
        self.assertIsNone(direct("eax"))

    def test_union_of_virtual_and_nonvirtual_targets(self):
        code=[
            "22200010: call 0x22200100",
            "22200015: call DWORD PTR [eax+0x4]",
            "22200020: jmp 0x22200100",
            "22200040: mov eax,0x223d2000",
            "22200100: ret",
            "22200200: ret",
            "22200300: call 0x22200400",
            "22200400: ret",
        ]
        counts,rows=inspect_lines(code,0x22200000,0x600,
                        {0x22200100:["CGrenadeAR"],0x22200200:["CBaseGrenade"]},
                        FakePE())
        self.assertEqual(counts["direct_call_instructions_to_text"],2)
        self.assertEqual(counts["indirect_call_instructions"],1)
        self.assertEqual(counts["direct_unconditional_jumps_to_text"],1)
        self.assertEqual(counts["vtable_seed_unique"],2)
        self.assertEqual(counts["direct_call_target_unique"],2)
        self.assertEqual(counts["union_entrypoint_candidates"],3)
        self.assertEqual(counts["vtable_direct_call_overlap"],1)
        by_va={x["entry_va"]:x for x in rows}
        self.assertEqual(by_va["0x22200100"]["seed_evidence"],"vtable+direct_call")
        self.assertEqual(by_va["0x22200400"]["seed_evidence"],"direct_call")
        self.assertEqual(by_va["0x22200100"]["direct_jump_to_entry_count"],1)
        self.assertEqual(by_va["0x22200200"]["recovery_status"],
                         "candidate_only_no_function_boundary_or_C++_semantics")
        self.assertIn("grenade_ar",by_va["0x22200100"]["nearby_string_examples"])

    def test_branches_alone_are_not_function_candidates(self):
        counts,rows=inspect_lines(["22200010: jne 0x22200070",
                                   "22200070: ret"],0x22200000,0x100,{},FakePE())
        self.assertEqual(counts["branch_instructions"],1)
        self.assertEqual(counts["union_entrypoint_candidates"],0)
        self.assertFalse(rows)

    def test_only_real_null_terminated_rdata_literals_count(self):
        pe=FakePE()
        self.assertEqual(rdata_string(pe,0x223D2000),"grenade_ar")
        self.assertIsNone(rdata_string(pe,0x223D2010))
        self.assertIsNone(rdata_string(pe,0x22200010))

    def test_full_json_requires_original_schema_and_sha(self):
        # Purely synthetic tests above exercise the decoded evidence path.
        # Original source verification happens during the CLI's SHA-locked
        # inspect_original entry before spawning objdump.
        from audit_sin_entire_dll_code_census import inspect_original,CensusError
        with self.assertRaisesRegex(CensusError,"SHA256"):
            import tempfile
            with tempfile.TemporaryDirectory() as td:
                p=pathlib.Path(td)/"synthetic.dll"
                p.write_bytes(b"not original SiN")
                inspect_original(p,"server",{"sha256":"forged"})

if __name__=="__main__":
    unittest.main()
