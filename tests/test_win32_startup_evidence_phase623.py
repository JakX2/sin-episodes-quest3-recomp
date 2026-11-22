"""Synthetic-only tests; never require retail DLLs or assets."""
import importlib.util
import json
from pathlib import Path
import unittest

PATH = Path(__file__).resolve().parents[1] / "research/tools/validate_win32_startup_evidence_phase623.py"
spec = importlib.util.spec_from_file_location("startup_gate", PATH)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def trace():
    items = []
    def add(event, **kw):
        items.append({"seq": len(items), "event": event, "origin": "retail_process",
                      "pid": 4321, **kw})
    add("process_start")
    for module in ("server", "client"):
        add("module_load", module=module, pe32=True, result="ok")
        for interface in gate.INTERFACES[module]:
            add("factory", module=module, interface=interface, result="ok")
    for module, interface, method in gate.LIFECYCLE:
        add("lifecycle_enter", module=module, interface=interface, method=method)
        add("lifecycle_return", module=module, interface=interface, method=method, result="ok")
    add("entity_registered", classname="npc_druglab_grunt")
    add("entity_spawned", classname="npc_druglab_grunt", result="ok")
    add("map_active", map="synthetic_map", result="ok")
    return items


def check(items):
    return gate.validate(json.dumps(item) + "\n" for item in items)


class StartupEvidenceTests(unittest.TestCase):
    def test_complete_trace_is_consistent_not_proof(self):
        result = check(trace())
        self.assertTrue(result["valid"], result["errors"])
        self.assertFalse(result["retail_gameplay_boot_proven"])
        self.assertEqual(result["spawned_npcs"], ["npc_druglab_grunt"])

    def test_missing_factory_is_rejected(self):
        self.assertFalse(check([x for x in trace() if not (
            x["event"] == "factory" and x.get("interface") == "VClientPrediction001"
        )])["valid"])

    def test_no_init_return_is_rejected(self):
        self.assertFalse(check([x for x in trace() if not (
            x["event"] == "lifecycle_return" and x.get("method") == "DLLInit"
        )])["valid"])

    def test_failed_module_load_is_rejected(self):
        items = trace()
        next(x for x in items if x["event"] == "module_load")["pe32"] = False
        self.assertFalse(check(items)["valid"])

    def test_cross_process_splice_is_rejected(self):
        items = trace()
        items[-1]["pid"] = 9999
        self.assertFalse(check(items)["valid"])

    def test_duplicate_sequence_is_rejected(self):
        items = trace()
        items[-1]["seq"] = items[-2]["seq"]
        self.assertFalse(check(items)["valid"])

    def test_spawn_before_registration_is_rejected(self):
        items = trace()
        a = next(i for i, x in enumerate(items) if x["event"] == "entity_registered")
        b = next(i for i, x in enumerate(items) if x["event"] == "entity_spawned")
        items[a], items[b] = items[b], items[a]
        self.assertFalse(check(items)["valid"])

    def test_untrusted_origin_is_rejected(self):
        items = trace()
        items[0]["origin"] = "synthetic"
        self.assertFalse(check(items)["valid"])

    def test_malformed_input_is_rejected(self):
        self.assertFalse(gate.validate(["{broken"])["valid"])


if __name__ == "__main__":
    unittest.main()
