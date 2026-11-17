"""SDK 2004 name crosswalk is explicitly a *hypothesis*, never an ABI proof."""
import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build_source_sdk_2004_crosswalk import (
    build, EXPECTED_PREFIXES, EXPECTED_SHA256, SDK_METHODS,
)


def sample():
    modules = {}
    for mod, mapping in EXPECTED_PREFIXES.items():
        interfaces = []
        for index, (name, count) in enumerate(mapping.items()):
            base = (0x24000000 if mod == "client.dll" else 0x22000000)
            table = base + 0x10000 + 0x1000 * index
            slots = [
                {"slot": i, "pointer_entry_va": f"0x{table+4*i:08X}",
                 "target_va": f"0x{base+0x40000+0x1000*index+i*16:08X}"}
                for i in range(count)
            ]
            interfaces.append({
                "interface": name, "prefix_slots": count,
                "original_vtable_va": f"0x{table:08X}", "slots": slots,
            })
        modules[mod] = {
            "full_file_sha256": EXPECTED_SHA256[mod], "interfaces": interfaces,
        }
    return {
        "schema": "sin-owner-original-interface-vtable-prefix-v1",
        "evidence_tier": "SHA256_authenticated_original_PE32_vtable_pointer_values_only",
        "original_code_or_pseudocode_embedded": False,
        "modules": modules,
    }


class SDK2004CrosswalkTests(unittest.TestCase):
    def test_all_five_authentic_interface_families_are_named(self):
        report = build(sample())
        self.assertEqual(sum(len(v) for v in report["modules"].values()), 5)
        self.assertEqual(sum(len(entry["slots"]) for m in report["modules"].values()
                             for entry in m.values()), 105)
        self.assertEqual(report["verified_original_method_signatures_added"], 0)
        self.assertFalse(report["source_sdk_code_embedded"])

    def test_matching_counts_only_for_three_interface_families(self):
        report = build(sample())
        matches = {name: data["slot_count_matches"]
                   for m in report["modules"].values() for name, data in m.items()}
        self.assertTrue(matches["GameClientExports001"])
        self.assertTrue(matches["ServerGameClients003"])
        self.assertTrue(matches["ServerGameEnts001"])
        self.assertFalse(matches["VClient011"])
        self.assertFalse(matches["ServerGameDLL003"])

    def test_unmatched_extra_slots_never_get_invented_sdk_names(self):
        report = build(sample())
        client = report["modules"]["client.dll"]["VClient011"]
        server = report["modules"]["server.dll"]["ServerGameDLL003"]
        self.assertEqual(client["extra_original_prefix_entries_beyond_sdk"], 1)
        self.assertEqual(server["extra_original_prefix_entries_beyond_sdk"], 2)
        self.assertIsNone(client["slots"][-1]["source_sdk_2004_method_name_hypothesis"])
        self.assertIsNone(server["slots"][-1]["source_sdk_2004_method_name_hypothesis"])

    def test_early_hypotheses_are_from_2004_not_2013(self):
        report = build(sample())
        client = report["modules"]["client.dll"]["VClient011"]["slots"]
        server = report["modules"]["server.dll"]["ServerGameDLL003"]["slots"]
        self.assertEqual(client[0]["source_sdk_2004_method_name_hypothesis"], "Init")
        self.assertEqual(client[1]["source_sdk_2004_method_name_hypothesis"], "Shutdown")
        self.assertEqual(server[0]["source_sdk_2004_method_name_hypothesis"], "DLLInit")
        self.assertEqual(server[1]["source_sdk_2004_method_name_hypothesis"], "GameInit")
        self.assertTrue(all(not item["sdk_symbol_name_verified_as_sin_method"]
                            for item in client + server))
        self.assertEqual(len(SDK_METHODS["VClient011"]), 47)

    def test_inherited_and_explicit_virtual_destructors_accounted(self):
        report = build(sample())
        client = report["modules"]["client.dll"]["GameClientExports001"]["slots"]
        server = report["modules"]["server.dll"]["ServerGameEnts001"]["slots"]
        self.assertEqual(client[0]["source_sdk_2004_method_name_hypothesis"],
                         "~IBaseInterface")
        self.assertEqual(client[1]["source_sdk_2004_method_name_hypothesis"],
                         "IsPlayerGameVoiceMuted")
        self.assertEqual(server[0]["source_sdk_2004_method_name_hypothesis"],
                         "~IServerGameEnts")

    def test_rejects_modified_original_digest(self):
        evidence = sample()
        evidence["modules"]["server.dll"]["full_file_sha256"] = "bad"
        with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
            build(evidence)

    def test_rejects_reordered_vtable_slots(self):
        evidence = sample()
        slots = evidence["modules"]["client.dll"]["interfaces"][0]["slots"]
        slots[0], slots[1] = slots[1], slots[0]
        with self.assertRaisesRegex(ValueError, "noncontiguous"):
            build(evidence)

    def test_rejects_fabricated_method_pointer(self):
        evidence = sample()
        evidence["modules"]["server.dll"]["interfaces"][0]["slots"][0]["target_va"] = (
            "0x00000000"
        )
        with self.assertRaisesRegex(ValueError, "Null original PE32 VA"):
            build(evidence)

    def test_rejects_source_code_or_wrong_evidence_tier(self):
        evidence = sample()
        evidence["original_code_or_pseudocode_embedded"] = True
        with self.assertRaisesRegex(ValueError, "Unexpected or unauthenticated"):
            build(evidence)

    def test_rejects_shortened_original_prefix(self):
        evidence = sample()
        evidence["modules"]["server.dll"]["interfaces"][0]["slots"].pop()
        with self.assertRaisesRegex(ValueError, "incomplete"):
            build(evidence)


if __name__ == "__main__":
    unittest.main()
