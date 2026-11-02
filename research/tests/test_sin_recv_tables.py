"""Synthetic PE32 RecvTable constructor and named slot recovery tests."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_recv_tables import (
    analyze, scan_client_tables, scan_client_properties,
    RECV_TABLE_CTOR, VerificationError,
)
from test_gameplay_entity_registry import fake_registered_server

CLIENT_BASE = 0x24000000
SERVER_BASE = 0x22000000
TABLE_NAME = b"DT_TestNetTable\x00"
FIELD1 = b"m_iClip1\x00"
FIELD2 = b"m_iClip2\x00"


def synthetic_client() -> bytes:
    original = bytearray(fake_registered_server())
    struct.pack_into("<I", original, 0x98 + 28, CLIENT_BASE)
    name_va = CLIENT_BASE + 0x3d2500
    field1_va = name_va + 0x50
    field2_va = name_va + 0x80
    props_va = CLIENT_BASE + 0x4d7000
    recvtable_va = props_va + 0x300
    ctor_va = CLIENT_BASE + 0x126a30
    registration_va = CLIENT_BASE + 0x1100
    def put(va: int, value: bytes) -> None:
        offset = va - CLIENT_BASE
        original[offset:offset+len(value)] = value
    put(name_va, TABLE_NAME)
    put(field1_va, FIELD1)
    put(field2_va, FIELD2)
    # Verified original constructor: push DT name; push nProps; push pProps;
    # mov ecx, recvtable; call RecvTable ctor.
    registration = (
        b"\x68" + struct.pack("<I", name_va) +
        b"\x6a\x02" +
        b"\x68" + struct.pack("<I", props_va) +
        b"\xb9" + struct.pack("<I", recvtable_va) +
        b"\xe8" + struct.pack("<i", ctor_va - (registration_va + 22))
    )
    assert len(registration) == 22
    put(registration_va, registration)
    for i, (name, offset) in enumerate(((field1_va, 0x7e0), (field2_va, 0x7e4))):
        call_va = CLIENT_BASE + 0x1200 + i*32
        call_helper = CLIENT_BASE + 0x126df0
        inst = (
            b"\x6a\x04" + b"\x68" + struct.pack("<I", offset) +
            b"\x68" + struct.pack("<I", name) +
            b"\x68" + struct.pack("<I", props_va+i*0x3c) +
            b"\xe8" + struct.pack("<i", call_helper - (call_va + 22))
        )
        assert len(inst) == 22
        put(call_va, inst)
    return bytes(original)


def synthetic_server() -> bytes:
    original = bytearray(fake_registered_server())
    original[0x3d2500:0x3d2500+len(TABLE_NAME)] = TABLE_NAME
    return bytes(original)


class OriginalNetworkTableTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.client = synthetic_client()
        cls.server = synthetic_server()

    def test_table_ctor_not_bare_string(self):
        tables = scan_client_tables(self.client, check_hash=False)
        self.assertEqual(len(tables), 1)
        t = tables[0]
        self.assertEqual(t["table"], "DT_TestNetTable")
        self.assertEqual(t["property_count"], 2)
        self.assertEqual(t["recv_table_va"], "0x244d7300")

    def test_two_verified_property_offsets(self):
        tables = scan_client_tables(self.client, check_hash=False)
        found, missing = scan_client_properties(self.client, tables)
        self.assertFalse(missing)
        self.assertEqual([(x["name"], x["client_object_offset"]) for x in found],
                         [("m_iClip1", 0x7e0), ("m_iClip2", 0x7e4)])

    def test_mutated_constructor_call_is_not_registration(self):
        b = bytearray(self.client)
        b[0x1100 + 18] ^= 0x08
        self.assertEqual(scan_client_tables(bytes(b), check_hash=False), [])

    def test_mutated_property_target_leaves_explicit_unresolved_slot(self):
        b = bytearray(self.client)
        b[0x1200 + 13] ^= 0x40
        tables = scan_client_tables(bytes(b), check_hash=False)
        found, missing = scan_client_properties(bytes(b), tables)
        self.assertEqual(len(found), 1)
        self.assertEqual([x["slot"] for x in missing], [0])

    def test_type_strings_do_not_count_as_complete_client_factories(self):
        b = bytearray(self.client)
        b[0x1100:0x1100+22] = bytes(22)
        self.assertFalse(scan_client_tables(bytes(b), check_hash=False))

    def test_report_crosschecks_server_strings_but_does_not_invent_sendprops(self):
        data = analyze(self.client, self.server, check_hash=False)
        self.assertEqual(data["client"]["constructor_call_count"], 1)
        self.assertEqual(data["client"]["recovered_named_recvprop_slots"], 2)
        self.assertEqual(data["intersection"]["client_recvtable_and_server_string_name_count"], 1)
        self.assertNotIn("sendprops", data["server"])
        self.assertIn("ONLY", data["server"]["warning"])

    def test_sha_pins_reject_synthetic_data(self):
        with self.assertRaisesRegex(VerificationError, "SHA-256"):
            scan_client_tables(self.client)


if __name__ == "__main__":
    unittest.main()
