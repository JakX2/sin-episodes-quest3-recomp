"""Synthetic vtable arrays only, no proprietary DLL bytes."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    'original_druglab_diff', TOOLS / 'audit_sin_original_druglab_vtable_diff.py'
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class FakePE:
    def __init__(self, mob, elite):
        self.blocks = {
            audit.MOB_VTABLE: struct.pack('<%dI' % (len(mob)+1), *(mob+[0])),
            audit.ELITE_VTABLE: struct.pack('<%dI' % (len(elite)+1), *(elite+[0])),
        }

    def at(self, va, size):
        for base, data in self.blocks.items():
            if base <= va and va + size <= base + len(data):
                return data[va-base:va-base+size]
        raise audit.VerificationError(f'unmapped test address 0x{va:08X}')

    def section_of(self, va):
        return '.text' if 0x22000000 <= va < 0x22400000 else ''


def original_shape():
    mob = [0x22002000 + 8*i for i in range(532)]
    elite = list(mob) + [0x2201F000]
    for index in audit.EXPECTED_DIFF_SLOTS:
        elite[index] += 0x100000
    return mob, elite


class DruglabPrimaryVtableDiffTests(unittest.TestCase):
    def test_verified_original_shape_532_entries_and_23_overrides(self):
        mob, elite = original_shape()
        result = audit.compare(FakePE(mob, elite), require_original=True)
        self.assertEqual(result['compared_slots'], 532)
        self.assertEqual(result['shared_slot_count'], 509)
        self.assertEqual(result['differing_slot_count'], 23)
        self.assertEqual(result['elite_contiguous_executable_slots'], 533)
        self.assertEqual(
            [row['slot_index'] for row in result['differing_slots']],
            list(audit.EXPECTED_DIFF_SLOTS))

    def test_fail_closed_if_original_override_disappears(self):
        mob, elite = original_shape()
        elite[10] = mob[10]
        with self.assertRaises(audit.VerificationError):
            audit.compare(FakePE(mob, elite), require_original=True)

    def test_fail_closed_when_original_boundary_changes(self):
        mob, elite = original_shape()
        with self.assertRaises(audit.VerificationError):
            audit.compare(FakePE(mob[:-1], elite), require_original=True)

    def test_generic_short_arrays_use_same_comparison_logic(self):
        mob = [0x22002000, 0x22002020, 0x22002040]
        elite = [0x22002000, 0x22002030, 0x22002040]
        result = audit.compare(FakePE(mob, elite), require_original=False)
        self.assertEqual(result['shared_slot_count'], 2)
        self.assertEqual(result['differing_slot_count'], 1)
        self.assertEqual(result['differing_slots'][0]['slot_index'], 1)


if __name__ == '__main__':
    unittest.main()
