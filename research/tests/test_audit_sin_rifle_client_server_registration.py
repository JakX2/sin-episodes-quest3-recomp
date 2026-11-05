"""No proprietary DLLs needed for these synthetic x86 registration tests."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    'sin_rifle_reg', TOOLS / 'audit_sin_rifle_client_server_registration.py')
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    def __init__(self, parts):
        self.parts = parts

    def at(self, va, length):
        for start, blob in self.parts:
            if start <= va and va + length <= start + len(blob):
                return blob[va-start:va-start+length]
        raise a.VerificationError(f'missing fake PE region 0x{va:08X}')


def x86_call(entry, target):
    return b'\xe8' + struct.pack('<i', target - entry - 5)


def server_thunk(entry, name_va, factory_va):
    return (x86_call(entry, 0x2228B110) + b'\x8b\x10\x68' +
            struct.pack('<I', name_va) + b'\x68' +
            struct.pack('<I', factory_va) + b'\x8b\xc8\xff\x12\xc3')


def recv_table_thunk():
    return (b'\x68' + struct.pack('<I', 0x242C298C) + b'\x6a\x01' +
            b'\x68' + struct.pack('<I', 0x2437F034) +
            b'\xb9' + struct.pack('<I', 0x2437F598) +
            x86_call(0x2422A6B9, 0x24126A30))


def client_thunk():
    return (x86_call(0x24270B80, 0x24103BF0) + b'\x8b\x10\x68' +
            struct.pack('<I', 0x2422B5B0) + b'\x68' +
            struct.pack('<I', 0x830) + b'\x68' +
            struct.pack('<I', 0x242C2954) + b'\x68' +
            struct.pack('<I', 0x2429B020) + b'\x8b\xc8\xff\x12\xc3')


def client_regions(code):
    return [
        (0x24270B80, code),
        (0x2429B020, b'weapon_assault_rifle\0'),
        (0x242C2954, b'C_WeaponAssaultRifle\0'),
        (0x242C298C, b'DT_WeaponAssaultRifle\0'),
        (0x24320A60, b'.?AVC_WeaponAssaultRifle@@\0'),
        (0x2422A6A8, recv_table_thunk()),
        (0x2427643C, b'baseclass\0'),
        (0x2422A696, b'\x68\x3c\x64\x27\x24'),
    ]


class OriginalRifleRegistrationTests(unittest.TestCase):
    def test_relative_call_decoder_positive_and_negative(self):
        self.assertEqual(a.call_target(0x1000, x86_call(0x1000, 0x2000)), 0x2000)
        self.assertEqual(a.call_target(0x2000, x86_call(0x2000, 0x1000)), 0x1000)
        with self.assertRaises(a.VerificationError):
            a.call_target(0x1000, b'\x90'*5)

    def test_original_server_weapon_and_grenade_factories(self):
        for name, entry, string_va, factory_va in a.SERVER_REGISTRATIONS:
            expected = server_thunk(entry, string_va, factory_va)
            self.assertEqual(len(expected), 22)
            pe = FakePE([(entry, expected), (string_va, name.encode() + b'\0')])
            result = a.original_server_registration(
                pe, name, entry, string_va, factory_va)
            self.assertEqual(result['classname'], name)

    def test_server_factory_rejects_wrong_factory_and_getter(self):
        name, entry, name_va, factory_va = a.SERVER_REGISTRATIONS[0]
        wrong_factory = server_thunk(entry, name_va, factory_va+4)
        with self.assertRaises(a.VerificationError):
            a.original_server_registration(
                FakePE([(entry, wrong_factory),
                        (name_va, name.encode()+b'\0')]),
                name, entry, name_va, factory_va)
        wrong_getter = (x86_call(entry, 0x2228B100) +
                        server_thunk(entry, name_va, factory_va)[5:])
        with self.assertRaises(a.VerificationError):
            a.original_server_registration(
                FakePE([(entry, wrong_getter),
                        (name_va, name.encode()+b'\0')]),
                name, entry, name_va, factory_va)

    def test_original_client_registration(self):
        code = client_thunk()
        self.assertEqual(len(code), 32)
        pe = FakePE(client_regions(code))
        record = a.original_client_registration(pe)
        self.assertEqual(record['size_argument'], 0x830)
        self.assertEqual(record['rtti_class'], 'C_WeaponAssaultRifle')
        self.assertEqual(record['client_class_name'], 'C_WeaponAssaultRifle')
        self.assertEqual(record['recv_table_name'], 'DT_WeaponAssaultRifle')
        self.assertEqual(record['recv_table_declared_direct_props'], 1)

    def test_client_rejects_size_mutation(self):
        bad = bytearray(client_thunk())
        bad[13] ^= 1
        pe = FakePE(client_regions(bytes(bad)))
        with self.assertRaises(a.VerificationError):
            a.original_client_registration(pe)


    def test_client_rejects_recv_table_name_corruption(self):
        regions = client_regions(client_thunk())
        mutated = [(va, b'DT_CorruptWeapon\0' if va == 0x242C298C else blob)
                   for va, blob in regions]
        with self.assertRaises(a.VerificationError):
            a.original_client_registration(FakePE(mutated))

if __name__ == '__main__':
    unittest.main()
