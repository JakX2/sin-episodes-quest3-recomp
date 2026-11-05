"""Synthetic Source 1 studio model sequence-table tests."""
import importlib.util
from pathlib import Path
import struct
import unittest

path = Path(__file__).resolve().parents[1] / 'tools' / 'list_source_mdl_sequences.py'
spec = importlib.util.spec_from_file_location('sin_mdl_sequences', path)
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)


def fixture(names, version=44):
    size = 512 + len(names) * 212 + len(names) * 64
    data = bytearray(size)
    data[:4] = b'IDST'
    struct.pack_into('<I', data, 4, version)
    struct.pack_into('<I', data, 188, len(names))
    struct.pack_into('<I', data, 192, 512)
    for index, name in enumerate(names):
        base = 512 + index * 212
        name_offset = 512 + len(names) * 212 + index * 64
        struct.pack_into('<I', data, base + 4, name_offset - base)
        data[name_offset:name_offset + len(name)] = name.encode('ascii')
    return bytes(data)


class StudioSequencesTests(unittest.TestCase):
    def test_list_local_labels(self):
        version, names = parser.list_sequences(fixture(
            ['stand_idle', 'walk_all', 'run_all']
        ))
        self.assertEqual(version, 44)
        self.assertEqual(names, [(0, 'stand_idle'), (1, 'walk_all'), (2, 'run_all')])

    def test_empty_local_animations_possible_for_include(self):
        self.assertEqual(parser.list_sequences(fixture([])), (44, []))

    def test_reject_non_mdl(self):
        with self.assertRaises(ValueError):
            parser.list_sequences(b'xxxx' + fixture(['idle'])[4:])

    def test_reject_truncated_sequence_table(self):
        with self.assertRaises(ValueError):
            parser.list_sequences(fixture(['idle'])[:400])

    def test_reject_bogus_name_offset(self):
        data = bytearray(fixture(['idle']))
        struct.pack_into('<I', data, 516, 100000)
        with self.assertRaises(ValueError):
            parser.list_sequences(bytes(data))


if __name__ == '__main__':
    unittest.main()
