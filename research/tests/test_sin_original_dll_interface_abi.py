"""Phase177: independent synthetic PE32 export/registration mutation tests.

Never requires, copies, or fabricates matching original proprietary DLL bytes.
"""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError, parse_pe_exports,
    read_name, scan_static_registration_wrappers, verify_original_interface_abi,
)
BASE = 0x10000000


def synthetic():
    b = bytearray(0x5000)
    b[:2] = b'MZ'
    struct.pack_into('<I', b, 0x3c, 0x80)
    b[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', b, 0x84, 0x14c, 3)
    struct.pack_into('<H', b, 0x94, 0xe0)
    opt = 0x98
    struct.pack_into('<H', b, opt, 0x10b)
    struct.pack_into('<I', b, opt+28, BASE)
    struct.pack_into('<I', b, opt+92, 16)
    struct.pack_into('<II', b, opt+96, 0x2100, 0x100)
    sections = (
        (b'.text', 0x1000), (b'.rdata', 0x2000), (b'.data', 0x3000)
    )
    for i, (name, va) in enumerate(sections):
        at = opt+0xe0+i*40
        b[at:at+len(name)] = name
        struct.pack_into('<IIII', b, at+8, 0x1000, va, 0x1000, va)
    b[0x2040:0x204b] = b'server.dll\0'
    b[0x2060:0x2070] = b'CreateInterface\0'
    b[0x2080:0x208f] = b'TestInterface1\0'
    struct.pack_into('<IIHHIIIIIII', b, 0x2100,
                     0, 0, 0, 0, 0x2040, 1, 1, 1, 0x2200, 0x2210, 0x2220)
    struct.pack_into('<I', b, 0x2200, 0x1600)
    struct.pack_into('<I', b, 0x2210, 0x2060)
    struct.pack_into('<H', b, 0x2220, 0)
    register = 0x1200
    ctor = BASE+0x1500
    b[register:register+21] = (b'\x68' + struct.pack('<I', BASE+0x2080) +
        b'\x68' + struct.pack('<I', BASE+0x1700) +
        b'\xb9' + struct.pack('<I', BASE+0x3000) +
        b'\xe8' + struct.pack('<i', ctor - (BASE+register+20)) + b'\xc3')
    return b


class Tests(unittest.TestCase):
    def test_native_entrypoint_constants(self):
        self.assertEqual(ORIGINALS['server']['constructor'], 0x221fb5d0)
        self.assertEqual(ORIGINALS['client']['constructor'], 0x24164160)
        self.assertEqual(ORIGINALS['server']['export'], 0x221fb5f0)
        self.assertEqual(ORIGINALS['client']['export'], 0x24164180)
        self.assertEqual(ORIGINALS['server']['head'], 0x22573c5c)
        self.assertEqual(ORIGINALS['client']['head'], 0x24369378)
        self.assertEqual(ORIGINALS['server']['registrations'], 10)
        self.assertEqual(ORIGINALS['client']['registrations'], 55)
        self.assertEqual(len(ORIGINALS['server']['exports']), 5)
        self.assertEqual(len(ORIGINALS['client']['exports']), 1)
        for value in ORIGINALS.values():
            for key in ('constructor_digest', 'dispatch_digest', 'registration_manifest_digest'):
                self.assertEqual(len(bytes.fromhex(value[key])), 32)

    def test_synthetic_pe_export_table(self):
        pe = PEImage(bytes(synthetic()))
        self.assertEqual(pe.base, BASE)
        self.assertEqual(pe.section_of(BASE+0x1600), '.text')
        self.assertEqual(pe.section_of(BASE+0x2200), '.rdata')
        self.assertEqual(pe.section_of(BASE+0x3000), '.data')
        self.assertEqual(parse_pe_exports(pe), {'CreateInterface': BASE+0x1600})

    def test_synthetic_static_registration(self):
        pe = PEImage(bytes(synthetic()))
        rows = scan_static_registration_wrappers(pe, BASE+0x1500)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['interface'], 'TestInterface1')
        self.assertEqual(rows[0]['factory_va'], BASE+0x1700)
        self.assertEqual(rows[0]['registration_node_va'], BASE+0x3000)
        self.assertEqual(rows[0]['registration_va'], BASE+0x1200)
        self.assertEqual(read_name(pe, BASE+0x2080), 'TestInterface1')

    def test_frame_pointer_epilogue_variant(self):
        b = synthetic()
        b[0x1214:0x1216] = b'\x5d\xc3'
        rows = scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500)
        self.assertEqual(len(rows), 1)

    def test_corrupted_registration_call_destination(self):
        b = synthetic()
        b[0x1210] ^= 1
        self.assertEqual(scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500), [])

    def test_corrupted_registration_shape(self):
        for offset in (0x1200, 0x1205, 0x120a, 0x1214):
            with self.subTest(offset=hex(offset)):
                b = synthetic()
                b[offset] ^= 1
                with self.assertRaisesRegex(VerificationError, 'noncanonical'):
                    scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500)

    def test_bad_name_section_and_content(self):
        for value in (BASE+0x1100, BASE+0x3000):
            b = synthetic()
            struct.pack_into('<I', b, 0x1201, value)
            with self.assertRaisesRegex(VerificationError, 'not in .rdata'):
                scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500)
        b = synthetic()
        b[0x2081] = 0xff
        with self.assertRaisesRegex(VerificationError, 'invalid original interface name'):
            scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500)

    def test_bad_factory_or_node_section(self):
        for offset, value in ((0x1206, BASE+0x2100), (0x120b, BASE+0x1100)):
            b = synthetic()
            struct.pack_into('<I', b, offset, value)
            with self.assertRaisesRegex(VerificationError, 'unexpected original section'):
                scan_static_registration_wrappers(PEImage(bytes(b)), BASE+0x1500)

    def test_pe_export_ordinal_mutations(self):
        b = synthetic()
        struct.pack_into('<H', b, 0x2220, 2)
        with self.assertRaisesRegex(VerificationError, 'ordinal beyond'):
            parse_pe_exports(PEImage(bytes(b)))
        b = synthetic()
        struct.pack_into('<I', b, 0x2200, 0x2104)
        with self.assertRaisesRegex(VerificationError, 'forwarded'):
            parse_pe_exports(PEImage(bytes(b)))

    def test_full_original_hash_gate_rejects_synthetic(self):
        for module in ('server', 'client'):
            with self.subTest(module=module):
                with self.assertRaisesRegex(VerificationError, 'full SHA256'):
                    verify_original_interface_abi(bytes(synthetic()), module)

    def test_invalid_pe_geometry_rejected(self):
        b = synthetic()
        b[0:2] = b'QQ'
        with self.assertRaisesRegex(VerificationError, 'DOS/PE'):
            PEImage(bytes(b))
        b = synthetic()
        struct.pack_into('<H', b, 0x84, 0x8664)
        with self.assertRaisesRegex(VerificationError, '32-bit x86'):
            PEImage(bytes(b))


if __name__ == '__main__':
    unittest.main()