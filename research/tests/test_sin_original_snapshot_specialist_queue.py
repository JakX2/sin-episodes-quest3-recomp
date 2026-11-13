#!/usr/bin/env python3
"""Synthetic metadata tests for Phase212/213 snapshot call target triage."""
from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import build_sin_original_snapshot_specialist_queue as queue


class QueueTests(unittest.TestCase):
    @staticmethod
    def ledger():
        rows = [{
            "dll": "client", "entry_va": f"0x{queue.ENTRY:08X}",
            "original_code_length": queue.SIZE,
            "original_code_sha256": queue.SHA,
        }]
        rows.extend({
            "dll": "client", "entry_va": f"0x{data[0]:08X}",
            "original_code_length": data[1],
            "original_code_sha256": data[2],
        } for data in queue.METHODS.values())
        return {
            "schema_version": 1,
            "dll_sha256": {side: queue.ORIGINALS[side]["sha256"]
                           for side in ("server", "client")},
            "methods": rows,
        }

    def test_exact_phase212_213_graph_without_original_dlls(self):
        result = queue.build_queue(self.ledger())
        self.assertFalse(result["new_original_binary_validation_performed"])
        self.assertEqual(result["counts"], {
            "native_call_sites": 39, "distinct_native_destinations": 36,
            "reviewed_destinations_in_ledger": 5,
            "unreviewed_destinations_in_ledger": 31,
            "unreviewed_call_sites": 34,
        })
        self.assertEqual([r["target_va"] for r in result["already_reviewed_targets"]], [
            "0x24198A10", "0x24198A40", "0x24198A70", "0x24198B40",
            "0x241996F0",
        ])
        self.assertEqual(result["cases"][10]["field_types"], [10])
        self.assertEqual(result["cases"][10]["direct_calls"], 1)
        self.assertEqual(result["cases"][10]["unreviewed_target_calls"], 0)
        self.assertEqual(result["cases"][12]["direct_calls"], 0)
        self.assertEqual(result["cases"][11]["field_types"], [13])
        self.assertEqual(result["cases"][1]["unreviewed_target_calls"], 3)
        self.assertEqual(result["cases"][2]["unreviewed_target_calls"], 3)
        priorities = [item["priority"] for item in result["unreviewed_target_queue"]]
        self.assertEqual([priorities.count(p) for p in ("P0", "P1", "P2")], [6, 3, 22])
        self.assertEqual(len({x["target_va"] for x in result["unreviewed_target_queue"]}), 31)

    def test_shared_callee_groups_and_priority(self):
        result = queue.build_queue(self.ledger())
        rows = {r["target_va"]: r for r in result["unreviewed_target_queue"]}
        self.assertEqual(rows["0x24197A90"]["case_indices"], [8, 9])
        self.assertEqual(rows["0x24197A90"]["priority"], "P1")
        self.assertEqual(rows["0x24198AA0"]["priority"], "P0")
        self.assertEqual(rows["0x24198B80"]["native_call_sites"], ["0x24199883"])
        self.assertEqual(rows["0x24198B80"]["field_types"], [2])

    def test_newly_reviewed_synthetic_entry_reduces_queue(self):
        data = self.ledger()
        data["methods"].append({
            "dll": "client", "entry_va": "0x24198AA0",
            "original_code_length": 1,
            "original_code_sha256": "0" * 64,
        })
        result = queue.build_queue(data)
        self.assertEqual(result["counts"]["unreviewed_destinations_in_ledger"], 30)
        self.assertEqual(result["counts"]["unreviewed_call_sites"], 33)

    def test_reject_wrong_original_file_provenance(self):
        data = self.ledger(); data["dll_sha256"]["client"] = "0" * 64
        with self.assertRaises(queue.QueueError): queue.build_queue(data)
        data = self.ledger(); data["schema_version"] = 2
        with self.assertRaises(queue.QueueError): queue.build_queue(data)

    def test_reject_missing_and_mutated_verified_methods(self):
        base = self.ledger()
        for row in base["methods"]:
            with self.subTest(method=row["entry_va"]):
                data = copy.deepcopy(base)
                match = next(x for x in data["methods"] if x["entry_va"] == row["entry_va"])
                match["original_code_sha256"] = "f" * 64
                with self.assertRaises(queue.QueueError): queue.build_queue(data)
        data = self.ledger(); data["methods"].pop()
        with self.assertRaises(queue.QueueError): queue.build_queue(data)

    def test_reject_duplicate_and_invalid_ledger_entries(self):
        data = self.ledger(); data["methods"].append(copy.deepcopy(data["methods"][0]))
        with self.assertRaises(queue.QueueError): queue.build_queue(data)
        data = self.ledger(); data["methods"][0]["entry_va"] = "0xZZZ"
        with self.assertRaises(queue.QueueError): queue.build_queue(data)

    def test_refuse_drifted_call_metadata(self):
        with patch.object(queue, "CALL_EDGES", queue.CALL_EDGES[:-1]):
            with self.assertRaises(queue.QueueError): queue.build_queue(self.ledger())
        with patch.object(queue, "CASE_DESTINATIONS", queue.CASE_DESTINATIONS[:-1]):
            with self.assertRaises(queue.QueueError): queue.build_queue(self.ledger())
        with patch.object(queue, "CALL_EDGES", queue.CALL_EDGES[:-1] + (queue.CALL_EDGES[0],)):
            with self.assertRaises(queue.QueueError): queue.build_queue(self.ledger())


if __name__ == "__main__":
    unittest.main()
