#!/usr/bin/env python3
"""Synthetic PE32 regression tests for the read-only batch analyzer.

No retail binaries, disassembly bytes, or game assets are used by CI.
"""
import csv
import json
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import inventory_original_pe as inventory


def fake_pe32(base):
    data = bytearray(0xc00)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\x00\x00'
    struct.pack_into('<HHIIIHH', data, 0x84,
                     0x14c, 4, 0, 0, 0, 0xe0, 0x210e)
    opt = 0x98
    struct.pack_into('<H', data, opt, 0x10b)
    struct.pack_into('<I', data, opt + 16, 0x1000)
    struct.pack_into('<I', data, opt + 28, base)
    struct.pack_into('<I', data, opt + 92, 16)
    for index, rva, size in (
        (0, 0x2000, 0x90), (1, 0x2100, 0x28),
        (5, 0x4000, 10)):
        struct.pack_into('<II', data, opt + 96 + index * 8,
                         rva, size)
    for i, (name, rva, offset, length, flags) in enumerate((
        ('.text', 0x1000, 0x200, 0x200, 0x60000020),
        ('.rdata', 0x2000, 0x400, 0x400, 0x40000040),
        ('.data', 0x3000, 0x800, 0x200, 0xc0000040),
        ('.reloc', 0x4000, 0xa00, 0x200, 0x42000040),
    )):
        sec = opt + 0xe0 + i * 40
        data[sec:sec + len(name)] = name.encode('ascii')
        struct.pack_into('<IIII', data, sec + 8,
                         length, rva, length, offset)
        struct.pack_into('<I', data, sec + 36, flags)
    # One real named export, CreateInterface, ordinal 1.
    struct.pack_into('<IIHHIIIIIII', data, 0x400,
                     0, 0, 0, 0, 0x2050, 1, 1, 1,
                     0x2060, 0x2064, 0x2068)
    data[0x450:0x45a] = b'test.dll\x00\x00'
    struct.pack_into('<I', data, 0x460, 0x1010)
    struct.pack_into('<I', data, 0x464, 0x2070)
    struct.pack_into('<H', data, 0x468, 0)
    data[0x470:0x480] = b'CreateInterface\x00'
    # One imported function from a synthetic vstdlib.dll descriptor.
    struct.pack_into('<IIIII', data, 0x500,
                     0x2150, 0, 0, 0x2128, 0x2160)
    data[0x528:0x534] = b'vstdlib.dll\x00'
    struct.pack_into('<I', data, 0x550, 0x2170)
    struct.pack_into('<I', data, 0x560, 0x2170)
    struct.pack_into('<H', data, 0x570, 0)
    data[0x572:0x57c] = b'RandomInt\x00'
    # One HIGHLOW relocation to a code function pointer in .data.
    struct.pack_into('<I', data, 0x808, base + 0x1010)
    struct.pack_into('<IIH', data, 0xa00, 0x3000, 10, 0x3008)
    return bytes(data)


class BatchDiscoveryTests(unittest.TestCase):
    def test_pe32_export_import_relocation_layout(self):
        pe = inventory.PE32(fake_pe32(0x24000000))
        self.assertEqual(pe.base, 0x24000000)
        self.assertEqual(pe.entry_rva, 0x1000)
        self.assertTrue(pe.executable(0x24001010))
        self.assertFalse(pe.executable(0x24002010))
        self.assertEqual(pe.at(0x24003008, 4),
                         (0x24001010).to_bytes(4, 'little'))
        self.assertEqual(pe.relocations(), [0x24003008])
        self.assertEqual(pe.exports(), [{
            'address': '0x24001010', 'name': 'CreateInterface',
            'ordinal': 1, 'forwarded': False,
        }])
        self.assertEqual(pe.imports(), [{
            'dll': 'vstdlib.dll', 'symbol': 'RandomInt',
            'iat_address': '0x24002160',
        }])

    def test_reject_wrong_headers_and_bounds(self):
        blob = fake_pe32(0x24000000)
        with self.assertRaises(inventory.PEError):
            inventory.PE32(b'bad')
        bad = bytearray(blob)
        bad[0x80] = ord('X')
        with self.assertRaises(inventory.PEError):
            inventory.PE32(bytes(bad))
        pe = inventory.PE32(blob)
        with self.assertRaises(inventory.PEError):
            pe.at(0x24009000, 1)
        with self.assertRaises(inventory.PEError):
            pe.at(0x24001100, 0x200)
        with self.assertRaises(inventory.PEError):
            pe.string(0x24002000, 1)

    @staticmethod
    def fake_disassembly(path, pe):
        base = pe.base
        return ([
            (base + 0x1001, base + 0x1010, 'call'),
            (base + 0x1002, base + 0x1020, 'call'),
            (base + 0x1003, base + 0x1020, 'jmp'),
        ], {base + 0x1030}, {
            base + 0x1000, base + 0x1010,
            base + 0x1020, base + 0x1030,
        })

    def test_discovery_preserves_reviewed_and_heuristic_tiers(self):
        blob = fake_pe32(0x24000000)
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'client.dll'
            source.write_bytes(blob)
            with mock.patch.object(inventory, 'disassemble',
                                   side_effect=self.fake_disassembly):
                summary, rows, edges, exports, imports = inventory.discover(
                    blob, source, {0x24001020: 'SHA-reviewed test entry'})
        by_address = {row['address']: row for row in rows}
        self.assertEqual(summary['reviewed_entries_from_catalog'], 1)
        self.assertEqual(summary['direct_call_instructions'], 2)
        self.assertEqual(summary['direct_jump_instructions'], 1)
        self.assertEqual(summary['exports'], 1)
        self.assertEqual(summary['imports'], 1)
        self.assertEqual(summary['relocations'], 1)
        self.assertEqual(by_address['0x24001020']['tier'], 'reviewed_catalog')
        self.assertEqual(by_address['0x24001020']['call_sites'], 1)
        self.assertEqual(by_address['0x24001020']['tail_jumps'], 1)
        self.assertEqual(by_address['0x24001010']['tier'], 'strong_anchor')
        self.assertIn('address_taken_data',
                      by_address['0x24001010']['evidence'])
        self.assertEqual(
            by_address['0x24001030']['tier'], 'heuristic_int3_boundary')
        self.assertTrue(all(row['decoded_instruction_start'] for row in rows))

    def test_archive_end_to_end_never_emits_pe_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            archive = tmp / 'bin.zip'
            catalog = tmp / 'verified.json'
            output = tmp / 'reports'
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('client.dll', fake_pe32(0x24000000))
                z.writestr('server.dll', fake_pe32(0x22000000))
            catalog.write_text(json.dumps({'methods': [
                {'dll': 'client', 'entry_va': '0x24001020',
                 'label': 'synthetic test only'},
            ]}), encoding='utf-8')
            with mock.patch.object(inventory, 'disassemble',
                                   side_effect=self.fake_disassembly):
                result = inventory.run(archive, output, catalog)
            self.assertEqual(result['modules']['client.dll'][
                'reviewed_entries_from_catalog'], 1)
            self.assertEqual(result['modules']['server.dll'][
                'reviewed_entries_from_catalog'], 0)
            self.assertTrue((output / 'summary.json').is_file())
            self.assertTrue((output / 'client_direct_edges.csv').is_file())
            self.assertTrue((output / 'server_function_candidates.csv').is_file())
            self.assertFalse((output / 'client.dll').exists())
            self.assertFalse((output / 'server.dll').exists())
            with (output / 'client_function_candidates.csv').open(
                    newline='', encoding='utf-8') as f:
                rows = list(csv.DictReader(f))
            self.assertTrue(any(r['tier'] == 'reviewed_catalog' for r in rows))
            self.assertIn('not verified functions',
                          (output / 'README.md').read_text())

if __name__ == '__main__':
    unittest.main()
