"""Synthetic-only regression tests for public ABI evidence queue."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("abi_evidence_queue", Path(__file__).resolve().parents[1] / "tools" / "abi_evidence_queue.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

BASE = {"module": "client", "interface": "VClient011", "rva": "0x1234",
        "evidence": "candidate", "body_sha256": None}


class EvidenceQueueTests(unittest.TestCase):
    def test_deterministic_order(self):
        a = dict(BASE)
        b = dict(BASE, rva="0x0002", evidence="inferred")
        first = module.report(module.validate({"records": [a, b]}))
        second = module.report(module.validate({"records": [b, a]}))
        self.assertEqual(first, second)
        self.assertEqual(first["counts"]["engine_tested"], 0)
        self.assertEqual(first["records"][0]["rva"], "0x0002")

    def test_duplicate_rejected(self):
        with self.assertRaises(ValueError):
            module.validate({"records": [BASE, dict(BASE)]})

    def test_unreviewed_cannot_claim_reviewed(self):
        with self.assertRaises(ValueError):
            module.validate({"records": [dict(BASE, evidence="sha_reviewed")]})

    def test_engine_test_requires_both_evidence_fields(self):
        digest = "a" * 64
        with self.assertRaises(ValueError):
            module.validate({"records": [dict(BASE, evidence="engine_tested", body_sha256=digest)]})
        reviewed = dict(BASE, evidence="engine_tested", body_sha256=digest,
                        test_id="retail/owner-test-001")
        self.assertEqual(module.report(module.validate({"records": [reviewed]}))["counts"]["engine_tested"], 1)

    def test_rejects_private_material_fields(self):
        for field in ("disassembly", "pseudocode", "binary_bytes", "original_body"):
            with self.subTest(field=field), self.assertRaises(ValueError):
                module.validate({"records": [dict(BASE, **{field: "sensitive"})]})

    def test_rejects_unknown_anchor_and_malformed_rva(self):
        for mutation in ({"interface": "Unknown001"}, {"rva": "1234"}, {"rva": "0xZZ"}):
            with self.assertRaises(ValueError):
                module.validate({"records": [dict(BASE, **mutation)]})


if __name__ == "__main__":
    unittest.main()
