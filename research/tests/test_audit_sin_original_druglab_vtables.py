"""Synthetic original SiN Druglab RTTI, complete-object-locator and vtables."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    'druglab_vtables', TOOLS / 'audit_sin_original_druglab_vtables.py'
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class FakePE:
    def __init__(self, regions):
        self.regions = regions

    def at(self, va, size):
        for start, value in self.regions:
            if start <= va and va + size <= start + len(value):
                return value[va-start:va-start+size]
        raise audit.VerificationError(f'unmapped synthetic original VA 0x{va:08X}')


def witness(name, type_descriptor, constructor, entries):
    hierarchy = {'CNPC_MobGrunt': 0x224D2490,
                 'CNPC_EliteGrunt': 0x224D17B0}[name]
    regions = [(type_descriptor+8, ('.?AV'+name+'@@\0').encode('ascii'))]
    constructor_code = b'\xc7\x06' + struct.pack('<I', entries[0][2])
    for offset, col, vtable, first_method in entries[1:]:
        constructor_code += b'\xc7\x86' + struct.pack('<I', offset)
        constructor_code += struct.pack('<I', vtable)
    regions.append((constructor, constructor_code))
    for offset, col, vtable, first_method in entries:
        regions.append((col, struct.pack('<5I', 0, offset, 0, type_descriptor, hierarchy)))
        regions.append((vtable - 4, struct.pack('<2I', col, first_method)))
    return regions


def hierarchy_witness(name, hierarchy, array):
    td = {item[0]: item[1] for item in audit.CLASSES}[name]
    slots = [0] * 17
    regs = []
    all_types = [(0, name, td, 0)] + list(audit.BASE_WITNESSES)
    for index, base_name, type_va, disp in all_types:
        bcd = 0x13000000 + 0x40*index
        slots[index] = bcd
        regs.append((bcd, struct.pack('<3I', type_va, 0, disp)))
        regs.append((type_va+8, ('.?AV'+base_name+'@@\0').encode('ascii')))
    regs.append((hierarchy, struct.pack('<4I', 0, 1, 17, array)))
    regs.append((array, struct.pack('<17I', *slots)))
    return regs


class DruglabVtablesTests(unittest.TestCase):
    def test_both_rtti_classes_and_three_constructor_vptrs(self):
        for name, td, constructor, entries in audit.CLASSES:
            info = audit.verify_class(
                FakePE(witness(name, td, constructor, entries)),
                name, td, constructor, entries)
            self.assertEqual(info['original_rtti_class'], name)
            self.assertEqual(
                [r['object_offset_hex'] for r in info['vtable_subobjects']],
                ['0x0', '0x790', '0xC14'])
            self.assertEqual(len(info['vtable_subobjects']), 3)

    def test_corrupt_constructor_vptr_install_is_rejected(self):
        name, td, constructor, entries = audit.CLASSES[0]
        regions = witness(name, td, constructor, entries)
        mutated = [
            (va, value[:2] + b'\x00' + value[3:] if va == constructor else value)
            for va, value in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.verify_class(FakePE(mutated), name, td, constructor, entries)

    def test_corrupt_primary_vtable_col_is_rejected(self):
        name, td, constructor, entries = audit.CLASSES[1]
        regions = witness(name, td, constructor, entries)
        vptr = entries[0][2]
        mutated = [
            (va, struct.pack('<2I', 0x12345678, entries[0][3])
             if va == vptr - 4 else value)
            for va, value in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.verify_class(FakePE(mutated), name, td, constructor, entries)

    def test_corrupt_secondary_subobject_offset_is_rejected(self):
        name, td, constructor, entries = audit.CLASSES[0]
        regions = witness(name, td, constructor, entries)
        second_col = entries[1][1]
        bad = [
            (va, struct.pack('<5I', 0, 0x78C, 0, td, 0x224D2490)
             if va == second_col else value)
            for va, value in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.verify_class(FakePE(bad), name, td, constructor, entries)


    def test_original_shared_17_base_grunt_hierarchy(self):
        for name, hierarchy, base_array in audit.HIERARCHIES:
            parsed = audit.verify_hierarchy(
                FakePE(hierarchy_witness(name, hierarchy, base_array)),
                name, hierarchy, base_array)
            self.assertEqual(parsed['original_base_class_count_including_self'], 17)
            bases = {v['base_rtti_class']: v['member_displacement_hex']
                     for v in parsed['selected_inheritance_witnesses']}
            self.assertEqual(bases['CNPC_GruntBase'], '0x0')
            self.assertEqual(bases['CAI_BaseNPC'], '0x0')
            self.assertEqual(bases['IAI_MovementSink'], '0x790')
            self.assertEqual(bases['IBehaviorBackBridge'], '0xC14')

    def test_changed_movement_sink_inheritance_is_rejected(self):
        name, hierarchy, base_array = audit.HIERARCHIES[0]
        regions = hierarchy_witness(name, hierarchy, base_array)
        bcd = 0x13000000 + 0x40*15
        corrupted = [
            (va, struct.pack('<3I', 0x224DBE2C, 0, 0x794)
             if va == bcd else raw)
            for va, raw in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.verify_hierarchy(FakePE(corrupted), name, hierarchy, base_array)

if __name__ == '__main__':
    unittest.main()
