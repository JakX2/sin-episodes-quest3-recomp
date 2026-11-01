"""Original-free Android binary XML parser and launcher contract tests."""
import struct
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
import audit_hl2vr_activity_entry as audit


def chunk(typ, hsize, payload):
    return struct.pack('<HHI', typ, hsize, 8 + len(payload)) + payload


def build_axml(*, exported='true', process=':game', action='org.libsdl.app.SDLACTIVITY', utf8=False):
    strings = ['uses-sdk', 'targetSdkVersion', '24', 'manifest', 'package',
               'com.valvesoftware.source.vrtest', 'application', 'activity',
               'name', 'org.libsdl.app.SDLActivity', 'exported', 'process',
               ':game', 'true', 'false', 'intent-filter', 'action',
               'org.libsdl.app.SDLACTIVITY', 'wrong.action']
    pos = {s: i for i, s in enumerate(strings)}
    data = bytearray()
    offsets = []
    for s in strings:
        offsets.append(len(data))
        if utf8:
            enc = s.encode('utf8')
            data += bytes([len(s), len(enc)]) + enc + b'\0'
        else:
            enc = s.encode('utf-16-le')
            data += struct.pack('<H', len(s)) + enc + b'\0\0'
    header = struct.pack('<IIIII', len(strings), 0, 0x100 if utf8 else 0,
                         28 + 4*len(strings), 0)
    pool = chunk(1, 28, header +
                 b''.join(struct.pack('<I', x) for x in offsets) + data)

    def attr(key, value):
        return struct.pack('<IIIHBBI', 0xffffffff, pos[key], pos[value],
                           8, 0, 3, pos[value])

    def start(tag, attributes=()):
        values = b''.join(attr(k, v) for k, v in attributes)
        body = struct.pack('<II', 1, 0xffffffff)
        body += struct.pack('<IIHHHHHH', 0xffffffff, pos[tag], 20, 20,
                            len(attributes), 0, 0, 0) + values
        return chunk(0x102, 16, body)

    def end(tag):
        return chunk(0x103, 16, struct.pack('<IIII', 1, 0xffffffff,
                                             0xffffffff, pos[tag]))

    body = (start('manifest', [('package', 'com.valvesoftware.source.vrtest')]) +
            start('uses-sdk', [('targetSdkVersion', '24')]) + end('uses-sdk') +
            start('application') +
            start('activity', [('name', 'org.libsdl.app.SDLActivity'),
                               ('exported', exported), ('process', process)]) +
            start('intent-filter') +
            start('action', [('name', action)]) + end('action') +
            end('intent-filter') + end('activity') +
            end('application') + end('manifest'))
    return struct.pack('<HHI', 3, 8, 8+len(pool)+len(body)) + pool + body


class AndroidManifestTests(unittest.TestCase):
    def test_utf16_manifest(self):
        root = audit.parse_axml(build_axml())
        activity = audit.find_activity(root, 'org.libsdl.app.SDLActivity')
        self.assertEqual(activity['attrs']['process'], ':game')
        self.assertEqual(activity['attrs']['exported'], 'true')

    def test_utf8_manifest(self):
        root = audit.parse_axml(build_axml(utf8=True))
        self.assertEqual(root['attrs']['package'], 'com.valvesoftware.source.vrtest')

    def test_invalid_root_magic(self):
        with self.assertRaises(audit.ManifestError):
            audit.parse_axml(b'not an Android XML')

    def test_short_string_pool(self):
        with self.assertRaises(audit.ManifestError):
            audit.parse_axml(build_axml()[:31])

    def test_corrupted_chunk_size(self):
        blob = bytearray(build_axml())
        struct.pack_into('<I', blob, 12, 0xfffffff0)
        with self.assertRaises(audit.ManifestError):
            audit.parse_axml(bytes(blob))

    def test_missing_activity(self):
        with self.assertRaises(audit.ManifestError):
            audit.find_activity(audit.parse_axml(build_axml()), 'org.example.NoActivity')

    def test_manifest_fingerprint_rejects_synthetic(self):
        with self.assertRaisesRegex(audit.ManifestError, 'manifest SHA mismatch'):
            audit.verify_manifest_and_dex(build_axml(), b'invalid dex')

    def test_read_ushort_bound(self):
        with self.assertRaises(audit.ManifestError):
            audit._u16(b'abc', 2)

    def test_read_uint_bound(self):
        with self.assertRaises(audit.ManifestError):
            audit._u32(b'abc', 0)

    def test_utf8_length_bound(self):
        with self.assertRaises(audit.ManifestError):
            audit._length8(b'\x80', 0)

    def test_utf16_length_bound(self):
        with self.assertRaises(audit.ManifestError):
            audit._length16(b'\x00\x80', 0)

    def test_reject_unexported_activity(self):
        with self.assertRaisesRegex(audit.ManifestError, 'exported'):
            audit.validate_manifest_entry(audit.parse_axml(build_axml(exported='false')))

    def test_reject_wrong_action(self):
        with self.assertRaisesRegex(audit.ManifestError, 'action missing'):
            audit.validate_manifest_entry(audit.parse_axml(build_axml(action='wrong.action')))

    def test_accept_synthetic_exported_entry(self):
        result = audit.validate_manifest_entry(audit.parse_axml(build_axml()))
        self.assertEqual(result['attrs']['process'], ':game')


if __name__ == '__main__':
    unittest.main()
