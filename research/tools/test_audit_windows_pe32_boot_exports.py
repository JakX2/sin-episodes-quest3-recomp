#!/usr/bin/env python3
"""Original-free synthetic PE32 export-target regression tests."""
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("audit_windows_pe32_boot_exports.py")
spec = importlib.util.spec_from_file_location("pe32_preflight", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def make_pe(*, ordinal=0, target=0x1200, name=b"CreateInterface\x00"):
    data = bytearray(0x800)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3c, 0x80)
    data[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HH", data, 0x84, 0x14c, 1)
    struct.pack_into("<H", data, 0x94, 224)
    opt = 0x98
    struct.pack_into("<H", data, opt, 0x10b)
    struct.pack_into("<I", data, opt+92, 16)
    struct.pack_into("<II", data, opt+96, 0x1000, 0x180)
    section = opt+224
    struct.pack_into("<IIII", data, section+8, 0x600, 0x1000, 0x600, 0x200)
    ed = 0x200
    struct.pack_into("<II", data, ed+20, 1, 1)
    struct.pack_into("<III", data, ed+28, 0x1040, 0x1050, 0x1060)
    struct.pack_into("<I", data, 0x240, target)
    struct.pack_into("<I", data, 0x250, 0x1080)
    struct.pack_into("<H", data, 0x260, ordinal)
    data[0x280:0x280+len(name)] = name
    data[0x400] = 0xc3
    return bytes(data)


class PE32PreflightTests(unittest.TestCase):
    def check(self, payload):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"synthetic.dll"
            path.write_bytes(payload)
            return audit.inspect(path)

    def test_real_local_export(self):
        self.assertTrue(self.check(make_pe())["create_interface_export"])

    def test_forwarder_not_accepted(self):
        self.assertFalse(self.check(make_pe(target=0x1100))["create_interface_export"])

    def test_bad_ordinal_rejected(self):
        with self.assertRaises(audit.PEError):
            self.check(make_pe(ordinal=1))

    def test_unmapped_target_rejected(self):
        with self.assertRaises(audit.PEError):
            self.check(make_pe(target=0x9000))

    def test_missing_name(self):
        self.assertFalse(self.check(make_pe(name=b"OtherFactory\x00"))["create_interface_export"])

    def test_truncated_section_table(self):
        with self.assertRaises(audit.PEError):
            self.check(make_pe()[:0x180])


if __name__ == "__main__":
    unittest.main()
