"""Synthetic instruction fixtures only. No original DLL bytes required."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_dynamic_descriptors import (
    BASE, STRIDE, INIT_START, NAME_HELPER, CALLBACKS, INPUT, OUTPUTS,
    absolute_store, helper_calls, inspect_initializer,
)


def sample() -> bytes:
    code = bytearray()
    code.extend(b'\xBE\x01\x00\x00\x00')  # mov esi, 1
    code.extend(b'\xBF\x20\x00\x00\x00')  # mov edi, 0x20
    code.extend(b'\xB9\x16\x00\x00\x00')  # mov ecx, 0x16
    code.extend(b'\xBA\x0B\x00\x00\x00')  # mov edx, 11
    for i, _name, name_va, handler_va in CALLBACKS:
        base = BASE + STRIDE * i
        code.extend(b'\x68' + struct.pack('<I', name_va))
        call_pos = len(code)
        code.extend(b'\xE8' + struct.pack('<i', NAME_HELPER - (INIT_START + call_pos + 5)))
        code.extend(b'\xA3' + struct.pack('<I', base + 4))
        code.extend(absolute_store(base + 28, handler_va))
        code.extend(b'\x66\x89\x35' + struct.pack('<I', base + 16))
        code.extend(b'\x66\x89\x3D' + struct.pack('<I', base + 18))
    i, _name, name_va, _public, external_va, handler_va = INPUT
    base = BASE + i * STRIDE
    code.extend(absolute_store(base + 4, name_va))
    code.extend(absolute_store(base + 20, external_va))
    code.extend(absolute_store(base + 28, handler_va))
    code.extend(b'\x66\xC7\x05' + struct.pack('<I', base + 18) + struct.pack('<H', 8))
    code.extend(b'\x66\x89\x35' + struct.pack('<I', base + 16))
    for i, _name, name_va, _public, external_va, offset in OUTPUTS:
        base = BASE + i * STRIDE
        code.extend(absolute_store(base + 4, name_va))
        code.extend(absolute_store(base + 20, external_va))
        code.extend(absolute_store(base + 8, offset))
        code.extend(b'\x66\x89\x0D' + struct.pack('<I', base + 18))
        code.extend(b'\x89\x15' + struct.pack('<I', base))
        code.extend(b'\x66\x89\x35' + struct.pack('<I', base + 16))
    return bytes(code)


class TestDynamicDescriptors(unittest.TestCase):
    def test_ten_slots_and_categories(self):
        entries = inspect_initializer(sample())
        self.assertEqual([e['index'] for e in entries], list(range(32, 42)))
        self.assertEqual([e['kind'] for e in entries].count('function'), 6)
        self.assertEqual([e['kind'] for e in entries].count('input'), 1)
        self.assertEqual([e['kind'] for e in entries].count('output'), 3)

    def test_callback_targets(self):
        self.assertEqual(inspect_initializer(sample())[0]['callback_va'], hex(CALLBACKS[0][3]))

    def test_helper_call_count_rejects_missing(self):
        code = bytearray(sample())
        pos = code.index(b'\xE8')
        code[pos] = 0x90
        with self.assertRaises(VerificationError):
            inspect_initializer(bytes(code))

    def test_malformed_callback_store_rejected(self):
        code = sample().replace(b'\xA3' + struct.pack('<I', BASE + 32 * STRIDE + 4), b'\x90' * 5)
        with self.assertRaises(VerificationError):
            inspect_initializer(code)

    def test_wrong_function_target_rejected(self):
        base = BASE + 33 * STRIDE
        code = sample().replace(absolute_store(base + 28, CALLBACKS[1][3]),
                                absolute_store(base + 28, CALLBACKS[1][3] + 0x10))
        with self.assertRaises(VerificationError):
            inspect_initializer(code)

    def test_bad_public_output_name_store_rejected(self):
        base = BASE + 40 * STRIDE
        code = sample().replace(absolute_store(base + 20, OUTPUTS[1][4]),
                                absolute_store(base + 20, OUTPUTS[1][4] + 4))
        with self.assertRaises(VerificationError):
            inspect_initializer(code)

    def test_malformed_input_flags_rejected(self):
        code = sample().replace(struct.pack('<I', BASE + 38 * STRIDE + 18) + struct.pack('<H', 8),
                                struct.pack('<I', BASE + 38 * STRIDE + 18) + struct.pack('<H', 0x10))
        with self.assertRaises(VerificationError):
            inspect_initializer(code)

    def test_wrong_output_offset_rejected(self):
        base = BASE + 41 * STRIDE
        code = sample().replace(absolute_store(base + 8, OUTPUTS[2][5]),
                                absolute_store(base + 8, OUTPUTS[2][5] + 4))
        with self.assertRaises(VerificationError):
            inspect_initializer(code)

    def test_helper_uses_relative_calls(self):
        self.assertEqual(len(helper_calls(sample(), INIT_START)), 6)


if __name__ == '__main__':
    unittest.main()
