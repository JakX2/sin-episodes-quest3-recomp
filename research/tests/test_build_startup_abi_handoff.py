"""Synthetic-only fail-closed tests for runtime ABI research handoff."""
import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build_startup_abi_handoff import CONTRACTS, handoff


def fixture():
    modules = {}
    for name, item in CONTRACTS.items():
        modules[name] = {
            "interface": item["interface"], "slot": 0,
            "original_entry_va": item["original_entry_va"],
            "sdk2004_method_name_hypothesis":
                item["sdk2004_method_name_hypothesis"],
            "ghidra_inferred_method_status": "bounded_inferred_function",
            "direct_string_reference_interface_version_candidates":
                [item["sdk2004_first_engine_factory_query_hypothesis"]],
            "original_factory_argument_types_verified": False,
            "original_first_failure_branch_verified": False,
            "original_method_body_verified": False,
        }
    return {
        "schema": "sin-private-startup-factory-census-v1",
        "original_instruction_bytes_or_pseudocode_embedded": False,
        "independently_tested_retail_startup": False,
        "modules": modules,
    }


class StartupAbiHandoffTests(unittest.TestCase):
    def test_handoff_preserves_evidence_tiers(self):
        result = handoff(fixture())
        self.assertFalse(result["retail_engine_startup_verified"])
        self.assertEqual(result["modules"]["client.dll"]["reviewed_original_ret_imm_bytes"], 12)
        self.assertEqual(result["modules"]["server.dll"]["reviewed_original_ret_imm_bytes"], 16)
        self.assertEqual(result["modules"]["server.dll"]["reviewed_original_stack_argument_count"], 4)
        self.assertFalse(result["modules"]["client.dll"]["original_factory_request_order_verified"])
        self.assertTrue(result["modules"]["client.dll"]["first_query_literal_observed_as_direct_reference"])

    def test_missing_literal_not_promoted_to_original_fact(self):
        data = fixture()
        data["modules"]["client.dll"]["direct_string_reference_interface_version_candidates"] = []
        result = handoff(data)
        self.assertFalse(result["modules"]["client.dll"]["first_query_literal_observed_as_direct_reference"])
        self.assertFalse(result["modules"]["client.dll"]["original_factory_argument_types_verified"])

    def test_reject_wrong_entry(self):
        data = fixture()
        data["modules"]["server.dll"]["original_entry_va"] = "0x221E7641"
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_original_claims(self):
        for key in ("original_factory_argument_types_verified",
                    "original_first_failure_branch_verified",
                    "original_method_body_verified"):
            with self.subTest(key=key):
                data = fixture()
                data["modules"]["client.dll"][key] = True
                with self.assertRaises(ValueError):
                    handoff(data)

    def test_reject_proprietary_payload_claim(self):
        data = fixture()
        data["original_instruction_bytes_or_pseudocode_embedded"] = True
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_retail_claim(self):
        data = fixture()
        data["independently_tested_retail_startup"] = True
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_extra_module(self):
        data = fixture()
        data["modules"]["unexpected.dll"] = copy.deepcopy(data["modules"]["client.dll"])
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_arbitrary_literal(self):
        data = fixture()
        data["modules"]["client.dll"]["direct_string_reference_interface_version_candidates"] = ["private-original-C-text"]
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_duplicate_literals(self):
        data = fixture()
        data["modules"]["client.dll"]["direct_string_reference_interface_version_candidates"] = ["VEngineClient012"] * 2
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_wrong_schema(self):
        data = fixture()
        data["schema"] = "unknown"
        with self.assertRaises(ValueError):
            handoff(data)


    def test_reject_unexpected_private_metadata_at_both_levels(self):
        for level in ("top", "module"):
            with self.subTest(level=level):
                data = fixture()
                target = data if level == "top" else data["modules"]["client.dll"]
                target["private_original_pseudocode"] = "unreviewed proprietary data"
                with self.assertRaises(ValueError):
                    handoff(data)

    def test_reject_wrong_evidence_tier(self):
        data = fixture()
        data["evidence_tier"] = "engine_tested_complete_method"
        with self.assertRaises(ValueError):
            handoff(data)

    def test_reject_inferred_count_overflow_and_boolean(self):
        for value in (True, -1, 25002, "2"):
            with self.subTest(value=value):
                data = fixture()
                data["modules"]["client.dll"].update({
                    "ghidra_inferred_function_bytes": 20,
                    "ghidra_inferred_instruction_count": value,
                    "ghidra_direct_call_site_count": 0,
                    "ghidra_indirect_call_site_count": 0,
                })
                with self.assertRaises(ValueError):
                    handoff(data)

    def test_reject_inconsistent_missing_function_observations(self):
        data = fixture()
        row = data["modules"]["client.dll"]
        row["ghidra_inferred_method_status"] = "missing_exact_ghidra_function"
        with self.assertRaises(ValueError):
            handoff(data)  # A missing function cannot contain version literals.

    def test_reject_partial_and_impossible_call_counts(self):
        data = fixture()
        row = data["modules"]["server.dll"]
        row["ghidra_inferred_instruction_count"] = 3
        with self.assertRaises(ValueError):
            handoff(data)
        row.update({
            "ghidra_inferred_function_bytes": 100,
            "ghidra_direct_call_site_count": 2,
            "ghidra_indirect_call_site_count": 2,
        })
        with self.assertRaises(ValueError):
            handoff(data)

    def test_accept_bounded_consistent_counters_without_promotion(self):
        data = fixture()
        data["evidence_tier"] = "sha_reviewed_method_entries_ghidra_inferred_callsite_counts"
        data["modules"]["client.dll"].update({
            "ghidra_inferred_function_bytes": 64,
            "ghidra_inferred_instruction_count": 12,
            "ghidra_direct_call_site_count": 1,
            "ghidra_indirect_call_site_count": 2,
        })
        result = handoff(data)
        self.assertFalse(result["modules"]["client.dll"]["original_first_failure_branch_verified"])
        self.assertFalse(result["retail_engine_startup_verified"])


if __name__ == "__main__":
    unittest.main()
