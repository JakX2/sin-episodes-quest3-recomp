"""Mutation/positive tests for Phase 64. Fixtures contain no original game bytes."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_weapon_field_metadata import VerificationError
from verify_weapon_owner_viewmodel_field import (
    inspect, SERVER_NAME_VA, CLIENT_NAME_VA, SERVER_DYNAMIC_DESCRIPTOR_VA,
    CLIENT_STATIC_DESCRIPTOR_VA, SERVER_INITIALIZERS, CODE_ANCHORS, CALL_ANCHORS,
)
from test_weapon_viewmodel_relation import SparseMemory

class SyntheticPE:
    def __init__(self, base):
        self.base = base
        self.content = SparseMemory()
    def va_to_offset(self, va): return va


def fixture():
    s, c = SyntheticPE(0x22000000), SyntheticPE(0x24000000)
    s.content.write(SERVER_NAME_VA, b'm_hViewModel\0')
    c.content.write(CLIENT_NAME_VA, b'm_hViewModel\0')
    # Uninitialized server descriptor deliberately remains zero.
    c.content.write(CLIENT_STATIC_DESCRIPTOR_VA,
                    struct.pack('<III', 13, CLIENT_NAME_VA, 0x1058))
    c.content.write(CLIENT_STATIC_DESCRIPTOR_VA+16, struct.pack('<H', 2))
    c.content.write(CLIENT_STATIC_DESCRIPTOR_VA+36, struct.pack('<I', 8))
    for va, dest, value, _ in SERVER_INITIALIZERS:
        s.content.write(va, b'\xc7\x05'+struct.pack('<II', dest, value))
    for va, hex_bytes, _ in CODE_ANCHORS:
        (c if va>=0x24000000 else s).content.write(va, bytes.fromhex(hex_bytes))
    for va, target, _ in CALL_ANCHORS:
        (c if va>=0x24000000 else s).content.write(va,
                                    b'\xe8'+struct.pack('<i', target-va-5))
    return s, c


class Phase64Tests(unittest.TestCase):
    def mutate(self, role, va, replacement):
        s,c=fixture()
        (c if role == 'client' else s).content.write(va, replacement)
        with self.assertRaises(VerificationError): inspect(s,c)
    def test_positive(self):
        s,c=fixture();x=inspect(s,c)
        self.assertEqual(x['array_elements'],2)
        self.assertEqual(x['original_2006_server_offset'],'0xbdc')
        self.assertEqual(x['original_2006_client_offset'],'0x1058')
    def test_server_field_name_mutation(self):
        self.mutate('server',SERVER_NAME_VA,b'X')
    def test_client_field_name_mutation(self):
        self.mutate('client',CLIENT_NAME_VA,b'X')
    def test_server_descriptor_nonzero_before_registration(self):
        self.mutate('server',SERVER_DYNAMIC_DESCRIPTOR_VA,b'X')
    def test_server_initializer_name_pointer(self):
        self.mutate('server',SERVER_INITIALIZERS[1][0]+6,b'\0')
    def test_server_initializer_field_offset(self):
        self.mutate('server',SERVER_INITIALIZERS[2][0]+6,b'\0')
    def test_server_initializer_array_bytes(self):
        self.mutate('server',SERVER_INITIALIZERS[3][0]+6,b'\0')
    def test_client_field_type_mutation(self):
        self.mutate('client',CLIENT_STATIC_DESCRIPTOR_VA,b'\0')
    def test_client_field_offset_mutation(self):
        self.mutate('client',CLIENT_STATIC_DESCRIPTOR_VA+8,b'\0')
    def test_client_descriptor_array_count_mutation(self):
        self.mutate('client',CLIENT_STATIC_DESCRIPTOR_VA+16,b'\x01')
    def test_client_descriptor_byte_width_mutation(self):
        self.mutate('client',CLIENT_STATIC_DESCRIPTOR_VA+36,b'\x04')
    def test_server_array_registration_count_mutation(self):
        self.mutate('server',0x22249C32,b'\x03')
    def test_client_array_registration_count_mutation(self):
        self.mutate('client',0x240D3B05,b'\x03')
    def test_viewmodel_lookup_uses_other_offset(self):
        self.mutate('server',0x2224AF07,b'\x12')
    def test_server_registration_call_redirect(self):
        self.mutate('server',CALL_ANCHORS[1][0]+1,b'\0\0\0\0')
    def test_client_registration_call_redirect(self):
        self.mutate('client',CALL_ANCHORS[2][0]+1,b'\0\0\0\0')

if __name__ == '__main__': unittest.main()
