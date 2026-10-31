"""Synthetic PE fixture tests, no original game binaries bundled."""
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_viewmodel_relation import (
    ANCHORS, CALLS, INDEX_DESCRIPTORS, VIEWMODEL_INDEX_NAME,
    inspect,
)


class SparseMemory:
    def __init__(self): self.bytes = {}
    def write(self, address, value):
        for i, v in enumerate(value): self.bytes[address+i] = v
    def __getitem__(self, key):
        if isinstance(key, slice):
            return bytes(self.bytes.get(i, 0) for i in range(key.start, key.stop))
        return self.bytes.get(key, 0)


class SyntheticPE:
    def __init__(self): self.content = SparseMemory()
    def va_to_offset(self, address): return address


def fixture():
    f = SyntheticPE()
    f.content.write(VIEWMODEL_INDEX_NAME, b'm_nViewModelIndex\0')
    for va in INDEX_DESCRIPTORS:
        f.content.write(va, struct.pack('<III',5,VIEWMODEL_INDEX_NAME,0x498))
    for va, fragment, _ in ANCHORS:
        f.content.write(va, bytes.fromhex(fragment))
    for va, target, _ in CALLS:
        f.content.write(va, b'\xe8'+struct.pack('<i',target-va-5))
    return f


class Phase63Tests(unittest.TestCase):
    def assert_changed_rejected(self, address, content):
        f = fixture()
        f.content.write(address, content)
        with self.assertRaises(VerificationError):inspect(f)

    def test_full_metadata_and_lookup_fixture(self):
        r = inspect(fixture())
        self.assertEqual(r['owner_side_indexed_handle_array_offset'], '0xbdc')
        self.assertTrue(r['related_handle_checks_generation'])

    def test_field_name_corruption(self):
        self.assert_changed_rejected(VIEWMODEL_INDEX_NAME, b'X')

    def test_field_name_pointer_corruption(self):
        self.assert_changed_rejected(INDEX_DESCRIPTORS[0]+4, b'\0'*4)

    def test_second_descriptor_object_offset_corruption(self):
        self.assert_changed_rejected(INDEX_DESCRIPTORS[1]+8, b'\0'*4)

    def test_wrong_field_type(self):
        self.assert_changed_rejected(INDEX_DESCRIPTORS[0], b'\0'*4)

    def test_missing_owner_type_gate(self):
        self.assert_changed_rejected(0x222AE9A7, b'\x90')

    def test_wrong_weapon_index_read(self):
        self.assert_changed_rejected(0x222AE9B3, b'\x49')

    def test_wrong_owner_array_offset(self):
        self.assert_changed_rejected(0x2224AF07, b'\x04')

    def test_wrong_index_stride(self):
        self.assert_changed_rejected(0x2224AF20, b'\x02')

    def test_missing_generation_comparison(self):
        self.assert_changed_rejected(0x2224AF28, b'\x39\x01')

    def test_hidden_related_entity_guard_missing(self):
        self.assert_changed_rejected(0x222AEA58, b'\x00')

    def test_related_lookup_call_redirect(self):
        self.assert_changed_rejected(CALLS[0][0]+1, b'\0'*4)

    def test_visibility_reader_call_redirect(self):
        self.assert_changed_rejected(CALLS[1][0]+1, b'\0'*4)

    def test_second_visibility_reader_changed(self):
        self.assert_changed_rejected(0x222AD7E1, b'\x41')


if __name__ == '__main__': unittest.main()
