"""Original-free PE parser tests for SHA-pinned legacy engine VGUI audit."""
import hashlib
import importlib.util
import pathlib
import struct
import tempfile
import unittest

path=pathlib.Path(__file__).resolve().parents[1]/"tools"/"audit_sin_client_engine_vgui_slot1.py"
spec=importlib.util.spec_from_file_location("audit_client_vgui",path)
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def synthetic_pe():
    data=bytearray(0x600)
    data[:2]=b"MZ"
    struct.pack_into("<I",data,0x3c,0x80)
    data[0x80:0x84]=b"PE"+bytes(2)
    struct.pack_into("<H",data,0x80+6,1)
    struct.pack_into("<H",data,0x80+20,0xe0)
    struct.pack_into("<H",data,0x80+24,0x10b)
    struct.pack_into("<I",data,0x80+24+28,audit.IMAGE_BASE)
    section=0x80+24+0xe0
    data[section:section+5]=b".text"
    struct.pack_into("<IIII",data,section+8,0x300,0x1000,0x300,0x200)
    data[0x200:0x500]=bytes((i%251 for i in range(0x300)))
    return data

class Verify(unittest.TestCase):
    def test_valid_pe_and_backed_va(self):
        p=audit.OriginalPE(synthetic_pe())
        self.assertEqual(p.base,audit.IMAGE_BASE)
        self.assertEqual(p.read(audit.IMAGE_BASE+0x1000,10),bytes(range(10)))
        with self.assertRaises(ValueError):p.read(audit.IMAGE_BASE+0x12ff,2)
        with self.assertRaises(ValueError):p.read(audit.IMAGE_BASE+0x5000,1)
        with self.assertRaises(ValueError):p.read(audit.IMAGE_BASE+0x1000,0)
    def test_reject_bad_headers_and_ranges(self):
        data=synthetic_pe()
        for index in (0,0x80,0x98):
            variant=bytearray(data)
            variant[index]=0
            with self.assertRaises(ValueError):audit.OriginalPE(variant)
        variant=bytearray(data)
        section=0x80+24+0xe0
        struct.pack_into("<I",variant,section+8+12,0xffffffff)
        with self.assertRaises(ValueError):audit.OriginalPE(variant)
    def test_sha_refusal_before_inspection(self):
        with tempfile.TemporaryDirectory() as root:
            p=pathlib.Path(root)/"client.dll"
            p.write_bytes(synthetic_pe())
            with self.assertRaisesRegex(ValueError,"SHA256 mismatch"):
                audit.verify_original(p)
    def test_original_regions_unique(self):
        self.assertEqual(len(audit.REGIONS),7)
        self.assertEqual(len(set(x[0] for x in audit.REGIONS.values())),5)
        for va,count,digest in audit.REGIONS.values():
            self.assertGreater(count,0)
            self.assertEqual(len(digest),64)
            int(digest,16)
        self.assertIn("VEngineVGui001",audit.STRINGS.values())
        self.assertIn("MatSystemSurface003",audit.STRINGS.values())

if __name__=="__main__":unittest.main()
