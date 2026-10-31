"""Phase 61 self-contained synthetic instruction fixtures (no game DLL content)."""
from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_hide_input_gate import (
    INPUT_ANCHORS, ACTIVE_ANCHORS, BASE_DISPATCH_ANCHORS,
    SERVER_WEAPON_VTABLE, VIRTUAL_DISPATCH_OFFSET, BASE_DISPATCH_TARGET,
    ACTIVE_HANDLE_GETTER, relative_call_target, validate_anchors, inspect,
)


class Memory:
    def __init__(self):
        self.cells = {}
    def __getitem__(self, v):
        if isinstance(v, slice):
            return bytes(self.cells.get(i, 0) for i in range(v.start, v.stop))
        return self.cells.get(v, 0)
    def write(self, address, data):
        for i, item in enumerate(data):
            self.cells[address+i] = item


class PEFixture:
    def __init__(self):
        self.content = Memory()
    def va_to_offset(self, va):
        return va


def fixture():
    pe = PEFixture()
    for address, instruction, _ in INPUT_ANCHORS + ACTIVE_ANCHORS + BASE_DISPATCH_ANCHORS:
        pe.content.write(address, bytes.fromhex(instruction))
    pe.content.write(SERVER_WEAPON_VTABLE + VIRTUAL_DISPATCH_OFFSET, struct.pack('<I', BASE_DISPATCH_TARGET))
    return pe


class Phase61InputTests(unittest.TestCase):
    def test_valid_fixture_and_guard_semantics(self):
        p = inspect(fixture())
        self.assertTrue(p['dispatch_only_when_selected_weapon_equals_this'])
        self.assertEqual(p['input_dispatch_argument_immediate'], 0)
        self.assertEqual(p['verified_instruction_anchors'], 31)

    def test_owner_handle_sentinel_checked(self):
        p = fixture();p.content.write(0x222AD809, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_stale_owner_handle_branch_mutation(self):
        p = fixture();p.content.write(0x222AD82D, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_removed_owner_virtual_call(self):
        p = fixture();p.content.write(0x222AD837, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_selected_weapon_comparison_mutation(self):
        p = fixture();p.content.write(0x222AD882, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_nonzero_virtual_argument(self):
        p = fixture();p.content.write(0x222AD889, b'\x01')
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_virtual_dispatch_slot_change(self):
        p = fixture();p.content.write(0x222AD88E, b'\x54')
        with self.assertRaises(VerificationError): inspect(p)

    def test_active_weapon_getter_checked(self):
        self.assertEqual(len(validate_anchors(fixture(), ACTIVE_ANCHORS)), 8)
        p = fixture();p.content.write(0x222AA640, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)

    def test_call_target_calculation(self):
        self.assertEqual(relative_call_target(fixture(), 0x222AD87D), ACTIVE_HANDLE_GETTER)

    def test_rejects_unexpected_base_vtable_target(self):
        p = fixture();p.content.write(SERVER_WEAPON_VTABLE + VIRTUAL_DISPATCH_OFFSET, b'\0'*4)
        with self.assertRaises(VerificationError): inspect(p)

    def test_rejects_mutated_base_hide_effect_call(self):
        p = fixture();p.content.write(0x222AEA4D, b'\x90')
        with self.assertRaises(VerificationError): inspect(p)


if __name__ == '__main__':
    unittest.main()
