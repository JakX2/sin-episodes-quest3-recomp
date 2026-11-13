#!/usr/bin/env python3
"""Synthetic-only Phase204 original PE import mutation tests. No game bytes."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_pe_imports import (
    PEImage,VerificationError,parse_imports,inspect_one,audit,
)

BASE=0x22000000
def fixture(*,ordinal=False,oft=True):
    b=bytearray(0x2400)
    b[:2]=b"MZ"
    struct.pack_into("<I",b,0x3c,0x80)
    b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HH",b,0x84,0x14c,1)
    struct.pack_into("<H",b,0x94,0xe0)
    opt=0x98
    struct.pack_into("<H",b,opt,0x10b)
    struct.pack_into("<I",b,opt+28,BASE)
    struct.pack_into("<I",b,opt+92,16)
    struct.pack_into("<II",b,opt+104,0x1000,40)
    sec=opt+0xe0
    b[sec:sec+8]=b".rdata\0\0"
    struct.pack_into("<IIII",b,sec+8,0x2000,0x1000,0x2000,0x400)
    off=lambda rva:0x400+(rva-0x1000)
    struct.pack_into("<IIIII",b,off(0x1000),0x1200 if oft else 0,0,0,0x1100,0x1200)
    b[off(0x1100):off(0x1100)+len(b"tier0.dll\0")]=b"tier0.dll\0"
    if ordinal:
        struct.pack_into("<I",b,off(0x1200),0x8000002a)
    else:
        struct.pack_into("<I",b,off(0x1200),0x1300)
        struct.pack_into("<H",b,off(0x1300),7)
        b[off(0x1302):off(0x1302)+12]=b"g_pMemAlloc\0"
    return b,off

class Tests(unittest.TestCase):
    def test_named_import(self):
        b,_=fixture()
        self.assertEqual(parse_imports(PEImage(bytes(b))),
            [{"dll":"tier0.dll","symbols":[{"name":"g_pMemAlloc","hint":7}]}])
    def test_ordinal(self):
        b,_=fixture(ordinal=True)
        self.assertEqual(parse_imports(PEImage(bytes(b)))[0]["symbols"],
                         [{"ordinal":42}])
    def test_original_first_thunk_fallback(self):
        b,_=fixture(oft=False)
        self.assertEqual(parse_imports(PEImage(bytes(b)))[0]["symbols"][0]["name"],
                         "g_pMemAlloc")
    def test_import_directory_needs_terminator(self):
        b,_=fixture();struct.pack_into("<II",b,0x98+104,0x1000,20)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_missing_import_directory(self):
        b,_=fixture();struct.pack_into("<II",b,0x98+104,0,0)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_bad_symbol_pointer(self):
        b,off=fixture();struct.pack_into("<I",b,off(0x1200),0xdeadbeef)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_reserved_ordinal_bits(self):
        b,off=fixture(ordinal=True);struct.pack_into("<I",b,off(0x1200),0x8001002a)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_unterminated_library_name(self):
        b,off=fixture();b[off(0x1100):off(0x1100)+128]=b"A"*128
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_sha_gate_on_all_originals(self):
        b,_=fixture()
        with self.assertRaises(VerificationError):inspect_one(bytes(b),"server")
        with self.assertRaises(VerificationError):audit(bytes(b),bytes(b))
        result=inspect_one(bytes(b),"server",verify_original=False)
        self.assertFalse(result["original_verified"])
        self.assertEqual(result["import_symbol_count"],1)
    def test_wrong_pe32_class(self):
        b,_=fixture();struct.pack_into("<H",b,0x98,0x20b)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_empty_symbol_table(self):
        b,off=fixture();struct.pack_into("<I",b,off(0x1200),0)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))
    def test_duplicate_import_library(self):
        b,off=fixture()
        b[off(0x1014):off(0x1028)]=b[off(0x1000):off(0x1014)]
        struct.pack_into("<II",b,0x98+104,0x1000,60)
        with self.assertRaises(VerificationError):parse_imports(PEImage(bytes(b)))

if __name__=="__main__":unittest.main()
