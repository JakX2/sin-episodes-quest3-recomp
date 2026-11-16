"""Synthetic PE-only tests; never access original gameplay binaries."""
import importlib.util
import struct
import unittest
from pathlib import Path

spec=importlib.util.spec_from_file_location("pe_import_audit",Path(__file__).with_name("audit_pe_imports.py"))
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def fixture(name="kernel32.dll"):
    b=bytearray(1024)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",b,0x84,0x14c,1,0,0,0,224,0x2002)
    o=0x98
    struct.pack_into("<H",b,o,0x10b)
    struct.pack_into("<I",b,o+60,0x200)
    struct.pack_into("<I",b,o+92,16)
    struct.pack_into("<II",b,o+104,0x1000,40)
    s=o+224
    struct.pack_into("<IIII",b,s+8,0x200,0x1000,0x200,0x200)
    struct.pack_into("<I",b,0x200+12,0x1050)
    b[0x250:0x250+len(name)+1]=name.encode()+b"\0"
    return b

class Tests(unittest.TestCase):
    def test_valid(self):
        self.assertEqual(mod.audit(fixture())["imports"],["kernel32.dll"])
    def test_not_x86(self):
        b=fixture()
        struct.pack_into("<H",b,0x84,0x8664)
        with self.assertRaises(mod.PEError): mod.audit(b)
    def test_truncation(self):
        with self.assertRaises(mod.PEError): mod.audit(fixture()[:260])
    def test_bad_rva(self):
        b=fixture()
        struct.pack_into("<I",b,0x200+12,0x900000)
        with self.assertRaises(mod.PEError): mod.audit(b)
    def test_case_insensitive_names(self):
        self.assertIn("client.dll", [x.casefold() for x in mod.audit(fixture("CLIENT.DLL"))["imports"]])

if __name__=="__main__": unittest.main()
