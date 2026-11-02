"""Synthetic original-PE server SendTable constructor and field-layout comparison."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_send_tables import (
    analyze, scan_server_tables, scan_server_properties,
    VerificationError, SEND_TABLE_CTOR,
)
from test_gameplay_entity_registry import fake_registered_server
from test_sin_recv_tables import synthetic_client

BASE=0x22000000


def synthetic_server() -> bytes:
    result=bytearray(fake_registered_server())
    table_va=BASE+0x1300
    name_global=BASE+0x4d6800
    name_va=BASE+0x3d2500
    props_va=BASE+0x4d7000
    tbl=BASE+0x4d7300
    nva=BASE+0x3d2550
    offset=0x4ec
    def put(va,raw):result[va-BASE:va-BASE+len(raw)]=raw
    put(name_va,b"DT_TestNetTable\x00")
    put(nva,b"m_iClip1\x00")
    put(name_global,struct.pack("<I",name_va))
    # mov eax,[global]; push eax; push count; push pProps;
    # mov ecx,sendtable; call SendTable::Construct.
    call=(b"\xa1"+struct.pack("<I",name_global)+b"\x50\x6a\x01"+
          b"\x68"+struct.pack("<I",props_va)+b"\xb9"+struct.pack("<I",tbl)+
          b"\xe8"+struct.pack("<i",SEND_TABLE_CTOR-(table_va+23)))
    assert len(call)==23
    put(table_va,call)
    # Server sendprop integer constructor, with extra flags/bits inputs
    prop_va=BASE+0x1200
    prop_helper=BASE+0x25ebf0
    inst=(b"\x6a\x04"+b"\x68"+struct.pack("<I",offset)+
          b"\x68"+struct.pack("<I",nva)+
          b"\x68"+struct.pack("<I",props_va)+
          b"\xe8"+struct.pack("<i",prop_helper-(prop_va+22)))
    assert len(inst)==22
    put(prop_va,inst)
    return bytes(result)


class ServerTablesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.server=synthetic_server()

    def test_verified_server_sendtable_constructor(self):
        tables=scan_server_tables(self.server,check_hash=False)
        self.assertEqual(len(tables),1)
        self.assertEqual(tables[0]["table"],"DT_TestNetTable")
        self.assertEqual(tables[0]["property_count"],1)

    def test_sendprop_offset_is_distinct_from_client(self):
        tables=scan_server_tables(self.server,check_hash=False)
        props,missing=scan_server_properties(self.server,tables)
        self.assertFalse(missing)
        self.assertEqual(props[0]["name"],"m_iClip1")
        self.assertEqual(props[0]["server_object_offset"],0x4ec)
        full=analyze(synthetic_client(),self.server,check_hash=False)
        pair=full["matching_client_server_slots"][0]
        self.assertEqual(pair["client_offset"],0x7e0)
        self.assertEqual(pair["server_offset"],0x4ec)
        self.assertFalse(pair["same_numeric_offset"])

    def test_change_ctor_target_rejects_table(self):
        bad=bytearray(self.server)
        bad[0x1300+19]^=1
        self.assertEqual(scan_server_tables(bytes(bad),check_hash=False),[])

    def test_remove_field_initializer_leaves_unknown(self):
        bad=bytearray(self.server)
        bad[0x1200+13]^=0x40
        props,missing=scan_server_properties(
            bytes(bad),scan_server_tables(bytes(bad),check_hash=False))
        self.assertFalse(props)
        self.assertEqual(missing[0]["slot"],0)

    def test_hash_rejects_synthetic(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            scan_server_tables(self.server)


if __name__=="__main__":
    unittest.main()
