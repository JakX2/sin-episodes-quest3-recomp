"""Synthetic metadata-only tests for original Ghidra interface factory queue."""
import csv
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from research.tools.build_ghidra_interface_queue import build, va


def csvwrite(path, names, items):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=names)
        writer.writeheader()
        writer.writerows(items)


class InterfaceQueueTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.p = Path(self.temp.name)
        for module, anchor, factory, wrapper in (
            ("client", "0x24164180", "0x24001234", "0x24004321"),
            ("server", "0x221FB5F0", "0x22001234", "0x22004321"),
        ):
            csvwrite(self.p / (module + ".dll_functions.csv"),
                     ["entry", "name", "prototype"], [
                {"entry": anchor[2:], "name": "CreateInterface", "prototype": "void*()"},
                {"entry": factory[2:], "name": "FUN_1", "prototype": "void* factory(void)"},
                {"entry": "0x10000001", "name": "helper", "prototype": "void()"},
            ])
            csvwrite(self.p / (module + ".dll_callers.csv"),
                     ["function_entry", "caller_entry", "kind"], [
                {"function_entry": "0x10000001", "caller_entry": factory[2:],
                 "kind": "UNCONDITIONAL_CALL"},
                {"function_entry": factory[2:], "caller_entry": "",
                 "kind": "UNCONDITIONAL_CALL"},
            ])
        self.manifest = self.p / "manifest.json"
        self.data = {"dlls": {
            "client": {"native_CreateInterface_va": "0x24164180",
                       "static_registration_count": 2,
                       "interface_registrations": [
                           {"interface": "Alpha_IMaterialProxy003",
                            "factory_va": "0x24009999", "registration_va": "0x24009000"},
                           {"interface": "VClient011", "factory_va": "0x24001234",
                            "registration_va": "0x24004321"}]},
            "server": {"native_CreateInterface_va": "0x221FB5F0",
                       "static_registration_count": 1,
                       "interface_registrations": [
                           {"interface": "ServerGameDLL003", "factory_va": "0x22001234",
                            "registration_va": "0x22004321"}]}}}
        self.manifest.write_text(json.dumps(self.data), encoding="utf-8")

    def test_hex_address_formats(self):
        self.assertEqual(va("0x24164180"), va("24164180"))
        self.assertEqual(va(0x24164180), va("24164180"))

    def test_bad_hex_rejected(self):
        for bad in ("", "gggggggg", "000000000", -1, True):
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError):
                    va(bad)

    def test_engine_interfaces_rank_above_material_proxies(self):
        report = build(self.p, self.manifest)
        client = report["modules"]["client"]
        self.assertEqual(client["queue"][0]["interface"], "VClient011")
        self.assertEqual(client["matched_factory_inferred_starts"], 1)
        self.assertEqual(client["queue"][0]["factory_direct_callee_addresses"], ["0x10000001"])
        self.assertFalse(client["queue"][0]["original_function_verified_by_this_tool"])
        self.assertTrue(client["createinterface_ghidra_inferred_start_match"])

    def test_jump_references_not_counted_as_calls(self):
        with (self.p / "client.dll_callers.csv").open("a", encoding="utf-8") as stream:
            stream.write("10000001,24001234,CONDITIONAL_JUMP" + chr(10))
        report = build(self.p, self.manifest)
        target = report["modules"]["client"]["queue"][0]
        self.assertEqual(target["factory_direct_callee_functions"], 1)

    def test_absent_factory_does_not_gain_verification(self):
        item = build(self.p, self.manifest)["modules"]["client"]["queue"][1]
        self.assertFalse(item["factory_ghidra_inferred_start_match"])
        self.assertFalse(item["engine_vtable_verified_by_this_tool"])

    def test_reject_registration_count_mismatch(self):
        self.data["dlls"]["server"]["static_registration_count"] = 100
        self.manifest.write_text(json.dumps(self.data), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "count disagrees"):
            build(self.p, self.manifest)

    def test_reject_duplicate_inferred_start(self):
        with (self.p / "server.dll_functions.csv").open("a", encoding="utf-8") as f:
            f.write('22001234,repeated,void()\n')
        with self.assertRaisesRegex(ValueError, "duplicate"):
            build(self.p, self.manifest)

    def test_seeded_factory_is_not_auto_discovery(self):
        for module, factory, label in (
            ("client", "0x24001234", "VClient011"),
            ("server", "0x22001234", "ServerGameDLL003"),
        ):
            manifest_records = self.data["dlls"][module]["interface_registrations"]
            details = []
            for item in manifest_records:
                details.append({
                    "entry": item["factory_va"],
                    "interface": item["interface"],
                    "outcome": ("seeded_candidate" if item["interface"] == label
                                else "unresolved_candidate"),
                    "result_function_entry": (factory if item["interface"] == label else ""),
                })
            csvwrite(self.p / (module + ".dll_factory_seed_results.csv"),
                     ["entry", "interface", "outcome", "result_function_entry"], details)
        report = build(self.p, self.manifest)
        client = report["modules"]["client"]
        self.assertTrue(client["factory_seed_report_available"])
        self.assertEqual(client["factory_seeds_created_as_candidates"], 1)
        self.assertEqual(client["factory_auto_discovered_prior_to_seeding"], 0)
        self.assertFalse(client["queue"][0]["factory_auto_discovered_before_seeding"])
        self.assertTrue(client["queue"][0]["factory_seeded_candidate_created"])
        self.assertFalse(client["queue"][0]["original_function_verified_by_this_tool"])

    def test_seed_report_requires_all_reviewed_factories(self):
        csvwrite(self.p / "client.dll_factory_seed_results.csv",
                 ["entry", "interface", "outcome", "result_function_entry"], [
                     {"entry": "0x24001234", "interface": "VClient011",
                      "outcome": "seeded_candidate", "result_function_entry": "0x24001234"}])
        with self.assertRaisesRegex(ValueError, "incomplete"):
            build(self.p, self.manifest)

    def test_cli_metadata_output(self):
        out = self.p / "queue.json"
        script = Path(__file__).resolve().parents[1] / "tools" / "build_ghidra_interface_queue.py"
        run = subprocess.run([sys.executable, str(script),
                              "--ghidra-dir", str(self.p),
                              "--manifest", str(self.manifest),
                              "--out", str(out)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        output = json.loads(out.read_text(encoding="utf-8"))
        self.assertEqual(output["new_sha_verified_original_functions"], 0)
        self.assertEqual(output["modules"]["server"]["queue"][0]["interface"],
                         "ServerGameDLL003")


if __name__ == "__main__":
    unittest.main()
