"""Original-free miniature DEX fixture tests for the Phase 82 launcher verifier."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import verify_hl2vr_renderer_contract as audit


def uleb(value):
    out = bytearray()
    while True:
        part = value & 127
        value >>= 7
        if value:
            out.append(part | 128)
        else:
            out.append(part)
            return bytes(out)


def synthetic_dex():
    # Self-authored DEX: one class, one method, one code item, three names.
    blob = bytearray(1024)
    blob[:8] = b'dex\n035\0'
    struct.pack_into('<I', blob, 32, len(blob))
    struct.pack_into('<II', blob, 56, 3, 112)
    struct.pack_into('<II', blob, 64, 1, 128)
    struct.pack_into('<II', blob, 88, 1, 144)
    struct.pack_into('<II', blob, 96, 1, 176)
    for i, off in enumerate((320, 350, 380)):
        struct.pack_into('<I', blob, 112 + i*4, off)
    struct.pack_into('<I', blob, 128, 0)
    struct.pack_into('<HHI', blob, 144, 0, 0, 1)
    struct.pack_into('<8I', blob, 176, 0, 1, 0xffffffff, 0, 0, 0, 224, 0)
    classdata = b'\x00\x00\x01\x00' + uleb(0) + uleb(1) + uleb(256)
    blob[224:224 + len(classdata)] = classdata
    struct.pack_into('<HHHHII', blob, 256, 1, 0, 0, 0, 0, 6)
    blob[272:284] = bytes.fromhex('1a 00 02 00 71 00 00 00 00 00 0e 00')
    for offset, word in zip((320, 350, 380), ('Ltest/Fixture;', 'run', 'fixture_key')):
        b = word.encode()
        blob[offset:offset + len(b) + 2] = uleb(len(b)) + b + b'\0'
    return bytes(blob)


class DexEvidenceTests(unittest.TestCase):
    def test_uleb(self):
        for value in (0, 5, 127, 128, 256, 16384, 2**28):
            result, at = audit.uleb(uleb(value), 0)
            self.assertEqual((result, at), (value, len(uleb(value))))

    def test_uleb_truncated(self):
        with self.assertRaisesRegex(audit.DexError, 'truncated'):
            audit.uleb(b'\x80', 0)

    def test_uleb_overlong(self):
        with self.assertRaisesRegex(audit.DexError, 'too long'):
            audit.uleb(b'\x80'*5, 0)

    def test_synthetic_method_resolution(self):
        dex = audit.Dex(synthetic_dex())
        bounds = dex.method_bounds('Ltest/Fixture;', 'run')
        self.assertEqual(bounds, (272, 284))
        dex.require(bounds, 272, 0x1a, 'fixture_key')
        dex.require(bounds, 276, 0x71, ('Ltest/Fixture;', 'run'))

    def test_missing_method(self):
        dex = audit.Dex(synthetic_dex())
        with self.assertRaisesRegex(audit.DexError, 'missing or ambiguous'):
            dex.method_bounds('Ltest/Fixture;', 'missing')

    def test_wrong_const_string(self):
        dex = audit.Dex(synthetic_dex())
        with self.assertRaisesRegex(audit.DexError, 'reference mismatch'):
            dex.require(dex.method_bounds('Ltest/Fixture;', 'run'), 272, 0x1a, 'wrong')

    def test_wrong_invoke_method(self):
        dex = audit.Dex(synthetic_dex())
        with self.assertRaisesRegex(audit.DexError, 'reference mismatch'):
            dex.require(dex.method_bounds('Ltest/Fixture;', 'run'), 276, 0x71,
                        ('Ltest/Fixture;', 'absent'))

    def test_instruction_outside_method(self):
        dex = audit.Dex(synthetic_dex())
        with self.assertRaisesRegex(audit.DexError, 'outside'):
            dex.require(dex.method_bounds('Ltest/Fixture;', 'run'), 284, 0x1a)

    def test_bad_header(self):
        with self.assertRaisesRegex(audit.DexError, 'header'):
            audit.Dex(b'notadex')

    def test_truncated_id_table(self):
        blob = bytearray(synthetic_dex())
        struct.pack_into('<II', blob, 56, 3, 1023)
        with self.assertRaisesRegex(audit.DexError, 'out of bounds'):
            audit.Dex(bytes(blob))

    def test_corrupt_code_length(self):
        blob = bytearray(synthetic_dex())
        struct.pack_into('<I', blob, 268, 10000000)
        with self.assertRaisesRegex(audit.DexError, 'instruction range'):
            audit.Dex(bytes(blob))

    def test_unpinned_pair_fail_closed(self):
        blob = synthetic_dex()
        with self.assertRaisesRegex(audit.DexError, 'SHA-256 mismatch'):
            audit.verify(blob, blob)


if __name__ == '__main__':
    unittest.main()
