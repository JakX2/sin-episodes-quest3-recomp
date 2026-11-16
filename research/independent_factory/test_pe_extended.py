"""Constructed PE32 fixtures only; no retail DLL data."""
import struct
import unittest
from audit_pe_extended import audit, forbidden, PEError

def fixture():
    b=bytearray(2048)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",b,0x84,0x14c,1,0,0,0,224,0x2002)
    o=0x98
    struct.pack_into("<H",b,o,0x10b)
    struct.pack_into("<I",b,o+28,0x10000000)
    struct.pack_into("<I",b,o+60,0x200)
    struct.pack_into("<I",b,o+92,16)
    s=o+224
    struct.pack_into("<IIII",b,s+8,0x600,0x1000,0x600,0x200)
    return b

def delayed(name=b"CLIENT.DLL", attrs=1):
    b=fixture()
    struct.pack_into("<II",b,0x98+96+13*8,0x1000,64)
    struct.pack_into("<II",b,0x200,attrs,0x1100 if attrs else 0x10001100)
    b[0x300:0x300+len(name)+1]=name+b"\0"
    return b

def forwarded(name=b"SERVER.CreateInterface"):
    b=fixture()
    struct.pack_into("<II",b,0x98+96,0x1000,0x180)
    struct.pack_into("<I",b,0x200+20,1)
    struct.pack_into("<I",b,0x200+28,0x1080)
    struct.pack_into("<I",b,0x280,0x1100)
    b[0x300:0x300+len(name)+1]=name+b"\0"
    return b

class Tests(unittest.TestCase):
    def test_empty(self): self.assertEqual(audit(fixture()),{"delay_imports":[],"export_forwarders":[]})
    def test_delay(self): self.assertEqual(audit(delayed())["delay_imports"],["CLIENT.DLL"])
    def test_delay_va(self): self.assertEqual(audit(delayed(attrs=0))["delay_imports"],["CLIENT.DLL"])
    def test_delay_denied(self): self.assertTrue(forbidden(audit(delayed()),["client.dll"])["delay_imports"])
    def test_forward(self): self.assertEqual(audit(forwarded())["export_forwarders"],["SERVER.CreateInterface"])
    def test_forward_denied(self): self.assertTrue(forbidden(audit(forwarded()),["server.dll"])["export_forwarders"])
    def test_forward_explicit_dll_module_denied(self):
        self.assertTrue(forbidden(audit(forwarded(b"CLIENT.dll.CreateInterface")), ["client.dll"])["export_forwarders"])
    def test_forward_ordinal_denied(self):
        self.assertTrue(forbidden(audit(forwarded(b"SERVER.#7")), ["server.dll"])["export_forwarders"])
    def test_forward_unrelated_module_allowed(self):
        self.assertFalse(forbidden(audit(forwarded(b"KERNEL32.Sleep")), ["server.dll"])["export_forwarders"])
    def test_forward_malformed_rejected(self):
        with self.assertRaises(PEError):
            forbidden(audit(forwarded(b"SERVER.")), ["server.dll"])
    def test_delay_without_extension_denied(self):
        self.assertTrue(forbidden(audit(delayed(b"CLIENT")), ["client.dll"])["delay_imports"])
    def test_invalid_delay_name(self):
        b=delayed()
        struct.pack_into("<I",b,0x204,0x90000000)
        with self.assertRaises(PEError): audit(b)
    def test_truncated(self):
        with self.assertRaises(PEError): audit(fixture()[:100])
    def test_wrong_arch(self):
        b=fixture()
        struct.pack_into("<H",b,0x84,0x8664)
        with self.assertRaises(PEError): audit(b)

if __name__=="__main__": unittest.main()
