"""Phase 62 synthetic tests, no original game code or DLL data included."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_vtable_visibility import (
    ADD_EFFECTS, EFFECT_ANCHORS, TARGET, WEAPONS, DISPATCH_SLOT_BYTES,
    inspect, verify_vtable,
)


class Memory:
    def __init__(self): self.cells = {}
    def __getitem__(self, s):
        if isinstance(s, slice): return bytes(self.cells.get(a, 0) for a in range(s.start, s.stop))
        return self.cells.get(s, 0)
    def write(self, addr, data):
        for i, byte in enumerate(data): self.cells[addr+i] = byte


class PEFixture:
    def __init__(self): self.base = 0x22000000; self.content = Memory()
    def va_to_offset(self, address): return address


def fixture():
    p = PEFixture()
    for spec in WEAPONS:
        name = ('.?AV'+spec.name+'@@\0').encode()
        p.content.write(spec.typedesc+8, name)
        p.content.write(spec.col, struct.pack('<IIIII', 0,0,0,spec.typedesc,0x22400000))
        p.content.write(spec.vtable-4, struct.pack('<I', spec.col))
        p.content.write(spec.constructor_store, b'\xc7\x06'+struct.pack('<I',spec.vtable))
        p.content.write(spec.vtable+DISPATCH_SLOT_BYTES, struct.pack('<I',TARGET))
    p.content.write(WEAPONS[0].vtable+0x24+DISPATCH_SLOT_BYTES, struct.pack('<I',0x222AF450))
    for va,hexstr,_ in EFFECT_ANCHORS: p.content.write(va,bytes.fromhex(hexstr))
    for va in (0x222AEA51,0x222AEA5E):
        p.content.write(va,b'\xe8'+struct.pack('<i',ADD_EFFECTS-(va+5)))
    return p


class Phase62VisibilityTests(unittest.TestCase):
    def test_all_four_real_class_name_locations_and_target(self):
        result=inspect(fixture());self.assertEqual(len(result['weapons']),4)
        self.assertTrue(all(w['slot_212_target_va']==hex(TARGET) for w in result['weapons']))

    def test_constructor_vptr_mutation_detected(self):
        p=fixture();p.content.write(WEAPONS[1].constructor_store+2,b'\0'*4)
        with self.assertRaises(VerificationError): inspect(p)

    def test_mangled_class_name_detected(self):
        p=fixture();p.content.write(WEAPONS[2].typedesc+8,b'\x90')
        with self.assertRaises(VerificationError):inspect(p)

    def test_rtti_col_type_mutation_detected(self):
        p=fixture();p.content.write(WEAPONS[3].col+12,b'\0'*4)
        with self.assertRaises(VerificationError):inspect(p)

    def test_rtti_vtable_boundary_detected(self):
        p=fixture();p.content.write(WEAPONS[0].vtable-4,b'\0'*4)
        with self.assertRaises(VerificationError):inspect(p)

    def test_one_derived_override_detected(self):
        p=fixture();p.content.write(WEAPONS[3].vtable+DISPATCH_SLOT_BYTES,b'\0'*4)
        with self.assertRaises(VerificationError):inspect(p)

    def test_original_nine_slot_error_reproducible(self):
        p=fixture();p.content.write(WEAPONS[0].vtable+0x24+DISPATCH_SLOT_BYTES,b'\0'*4)
        with self.assertRaises(VerificationError):inspect(p)

    def test_hide_effect_mask_mutation_detected(self):
        p=fixture();p.content.write(0x222AEA4E,b'\x10')
        with self.assertRaises(VerificationError):inspect(p)

    def test_show_effect_mask_mutation_detected(self):
        p=fixture();p.content.write(0x222AE9D1,b'\x25')
        with self.assertRaises(VerificationError):inspect(p)

    def test_add_effects_helper_or_mutation_detected(self):
        p=fixture();p.content.write(0x2214C84D,b'\x2b')
        with self.assertRaises(VerificationError):inspect(p)

    def test_add_effects_call_redirection_detected(self):
        p=fixture();p.content.write(0x222AEA52,b'\0')
        with self.assertRaises(VerificationError):inspect(p)

    def test_related_entity_call_redirection_detected(self):
        p=fixture();p.content.write(0x222AEA5F,b'\0')
        with self.assertRaises(VerificationError):inspect(p)

    def test_vtable_checker_is_per_class(self):
        p=fixture();r=verify_vtable(p,WEAPONS[0]);self.assertEqual(r['name'],'CBaseCombatWeapon')


if __name__=='__main__': unittest.main()
