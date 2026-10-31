"""Synthetic positive/mutation checks; fixtures include no copyrighted DLL data."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_viewmodel_entity_lifecycle import (
    inspect, CLASSNAME, BASE_CLASS_RTTI, FACTORY_RTTI,
    FACTORY_OBJECT, ANCHORS, CALLS,
)
from test_weapon_viewmodel_relation import SparseMemory


class SyntheticPE:
    def __init__(self):
        self.content = SparseMemory()
    def va_to_offset(self, va):
        return va


def fixture():
    pe = SyntheticPE()
    for address, content in (
        (CLASSNAME, b'viewmodel\0'),
        (BASE_CLASS_RTTI, b'.?AVCBaseViewModel@@\0'),
        (FACTORY_RTTI, b'.?AV?$CEntityFactory@VCBaseViewModel@@@@\0'),
        (FACTORY_OBJECT, struct.pack('<I', 0x2244F290)),
        (0x2244F28C, struct.pack('<I', 0x224C7D94)),
        (0x224C7DA0, struct.pack('<I', FACTORY_RTTI - 8)),
    ):
        pe.content.write(address, content)
    for va, h, _ in ANCHORS:
        pe.content.write(va, bytes.fromhex(h))
    for va, target, _ in CALLS:
        pe.content.write(va, b'\xe8'+struct.pack('<i', target-va-5))
    return pe


class Phase65Tests(unittest.TestCase):
    def mutate(self, va, data):
        p=fixture()
        p.content.write(va, data)
        with self.assertRaises(VerificationError): inspect(p)

    def test_clean_fixture(self):
        x=inspect(fixture())
        self.assertEqual(x['classname'], 'viewmodel')
        self.assertEqual(x['observed_cleanup_order'], [1,0])
        self.assertEqual(x['direct_call_targets'], len(CALLS))

    def test_classname_mutation(self): self.mutate(CLASSNAME,b'X')
    def test_rtti_mutation(self): self.mutate(BASE_CLASS_RTTI,b'X')
    def test_factory_rtti_mutation(self): self.mutate(FACTORY_RTTI,b'X')
    def test_factory_pointer_mutation(self): self.mutate(FACTORY_OBJECT,b'\0')
    def test_factory_rtti_locator_mutation(self): self.mutate(0x2244F28C,b'\0')
    def test_factory_rtti_descriptor_mutation(self): self.mutate(0x224C7DA0,b'\0')
    def test_registration_name_mutation(self): self.mutate(0x223C1008,b'\0')
    def test_factory_object_register_mutation(self): self.mutate(0x223C100D,b'\0')
    def test_indexed_slot_read_mutation(self): self.mutate(0x2224BEAB,b'\0')
    def test_generation_guard_mutation(self): self.mutate(0x2224BECC,b'\x00')
    def test_spawn_call_target_mutation(self): self.mutate(0x2224BEDF,b'\0\0\0\0')
    def test_owner_slot_assignment_mutation(self): self.mutate(0x2224BF43,b'\0')
    def test_handle_commit_call_mutation(self): self.mutate(0x2224BF4B,b'\0\0\0\0')
    def test_cleanup_reverse_direction_mutation(self): self.mutate(0x2224BFBD,b'\0')
    def test_cleanup_removal_target_mutation(self): self.mutate(0x2224BF9D,b'\0\0\0\0')
    def test_cleanup_handle_clear_target_mutation(self): self.mutate(0x2224BFB9,b'\0\0\0\0')
    def test_initializer_second_slot_mutation(self): self.mutate(0x22253984,b'\0')


if __name__ == '__main__': unittest.main()
