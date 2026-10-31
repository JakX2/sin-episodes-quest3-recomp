"""Synthetic Phase 60 fixtures; intentionally do not contain copyrighted DLL bytes."""
from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

from verify_weapon_field_metadata import VerificationError
from verify_weapon_reload_constructor_refs import (
    CTOR_ZERO_STORES, SHARED_RELOAD_READS, EXTENDED_MAP_STORES,
    ADJACENT_REGISTRATION, ADJACENT_DATA, ADJACENT_NAME,
    check_constructor_sites, check_static_map_links, check_adjacent_registration, inspect_static,
)


class SparseBytes:
    def __init__(self):
        self.memory = {}

    def write(self, at: int, data: bytes):
        for i, b in enumerate(data):
            self.memory[at + i] = b

    def __getitem__(self, region):
        if isinstance(region, slice):
            return bytes(self.memory.get(i, 0) for i in range(region.start, region.stop))
        return self.memory.get(region, 0)


class SyntheticPE:
    def __init__(self):
        self.content = SparseBytes()

    def va_to_offset(self, va):
        return va


def make_fixture():
    pe = SyntheticPE()
    for target, encoded, source, producer in CTOR_ZERO_STORES:
        pe.content.write(target, encoded)
        pe.content.write(source, producer)
    for target, encoded in SHARED_RELOAD_READS:
        pe.content.write(target, encoded)
    for target, destination, value in EXTENDED_MAP_STORES:
        pe.content.write(target, b'\xc7\x05' + struct.pack('<II', destination, value))
    pe.content.write(ADJACENT_REGISTRATION,
        bytes.fromhex('e8 fb 85 ec ff 8b 10 68') + struct.pack('<I', ADJACENT_NAME)
        + b'\x68' + struct.pack('<I', ADJACENT_DATA) + bytes.fromhex('8b c8 ff 12 c3'))
    pe.content.write(ADJACENT_NAME, b'game_weapon_manager\0')
    pe.content.write(ADJACENT_DATA, struct.pack('<I', 0x2245ABC0))
    return pe


class Phase60RegressionTests(unittest.TestCase):
    def test_five_zero_stores(self):
        self.assertEqual(len(check_constructor_sites(make_fixture())), 5)

    def test_corrupted_constructor_byte_store_rejected(self):
        pe = make_fixture()
        pe.content.write(CTOR_ZERO_STORES[1][0], b'\x90')
        with self.assertRaises(VerificationError):
            check_constructor_sites(pe)

    def test_nonzero_immediate_store_rejected(self):
        pe = make_fixture()
        pe.content.write(CTOR_ZERO_STORES[2][0]+6, b'\x01')
        with self.assertRaises(VerificationError):
            check_constructor_sites(pe)

    def test_missing_zero_source_rejected(self):
        pe = make_fixture()
        pe.content.write(CTOR_ZERO_STORES[3][2], b'\x90')
        with self.assertRaises(VerificationError):
            check_constructor_sites(pe)

    def test_mutated_reload_read_rejected(self):
        pe = make_fixture()
        pe.content.write(SHARED_RELOAD_READS[0][0], b'\x90')
        with self.assertRaises(VerificationError):
            check_constructor_sites(pe)

    def test_both_map_pointer_links(self):
        self.assertEqual(len(check_static_map_links(make_fixture())), 2)

    def test_map_pointer_redirection_rejected(self):
        pe = make_fixture()
        pe.content.write(EXTENDED_MAP_STORES[0][0]+6, b'\x00')
        with self.assertRaises(VerificationError):
            check_static_map_links(pe)

    def test_adjacent_registration_has_different_class_name(self):
        info = check_adjacent_registration(make_fixture())
        self.assertEqual(info['name'], 'game_weapon_manager')

    def test_adjacent_registration_string_mismatch(self):
        pe = make_fixture()
        pe.content.write(ADJACENT_NAME, b'X')
        with self.assertRaises(VerificationError):
            check_adjacent_registration(pe)

    def test_adjacent_registration_parameter_rejected(self):
        pe = make_fixture()
        pe.content.write(ADJACENT_REGISTRATION + 13, b'\x90')
        with self.assertRaises(VerificationError):
            check_adjacent_registration(pe)

    def test_adjacent_data_pointer_mismatch(self):
        pe = make_fixture()
        pe.content.write(ADJACENT_DATA, b'\0\0\0\0')
        with self.assertRaises(VerificationError):
            check_adjacent_registration(pe)

    def test_combined_inspection(self):
        x = inspect_static(make_fixture())
        self.assertEqual(len(x['zero_stores']), 5)
        self.assertEqual(len(x['shared_reload_read_sites']), 2)
        self.assertEqual(len(x['extended_map_static_links']), 2)


if __name__ == '__main__':
    unittest.main()
