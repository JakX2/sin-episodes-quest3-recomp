"""Synthetic-only unit tests. No actual Ritual VMF/weapon content in repository."""
import importlib.util
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
import zlib

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "audit_sin_sdk.py"
SPEC = importlib.util.spec_from_file_location("sin_sdk_audit", SCRIPT)
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)

SAMPLE = b'''versioninfo
{
 "editorversion" "400"
}
entity
{
\t"id" "1"
\t"classname" "logic_relay"
\t"targetname" "relay"
\tconnections
\t{
\t\t"OnTrigger" "door,Open,,0.25,-1"
\t\t"OnTrigger" "door,Open,,0.35,-1"
\t}
}
entity
{
\t"id" "2"
\t"classname" "trigger_dynamic_difficulty"
\t"EstimatedDifficulty" "3"
\tsolid
\t{
\t\t"id" "3"
\t}
}
'''


def make_fixture_zip():
    packed = bytearray()
    tree = bytearray(b'vmf\x00sesdk_content/mapsrc\x00')
    for name in ['se1_docks04', 'se1_highrise01', 'arena_office']:
        offset = len(packed)
        packed.extend(SAMPLE)
        tree.extend(name.encode() + b'\0')
        tree.extend(struct.pack('<IHHIIH', zlib.crc32(SAMPLE), 0, 0,
                                offset, len(SAMPLE), 0xffff))
    tree.extend(b'\0\0\0')
    directory = struct.pack('<7I', 0x55aa1234, 2, len(tree), 0, 0, 0, 0) + tree
    script = '''WeaponData
{
"viewmodel" "models/test.mdl"
"ZoomFOV" "50"
"clip_size" "20"
"clip2_size" "-1"
"default_clip" "20"
"default_clip2" "0"
"primary_ammo" "MAG"
"secondary_ammo" "DUAG"
}
'''
    out = io.BytesIO()
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(audit.VPK_DIR, directory)
        archive.writestr(audit.VPK_DATA, packed)
        for weapon in audit.WEAPONS:
            archive.writestr(audit.ZIP_ROOT + 'SE1/scripts/' + weapon + '.txt', script)
    out.seek(0)
    return out


class AuditSdkTests(unittest.TestCase):
    def test_sample_map(self):
        data = audit.audit_vmf(SAMPLE)
        self.assertEqual(data['entity_count'], 2)
        self.assertEqual(data['output_connection_count'], 2)
        self.assertEqual(data['selected_class_counts']['logic_relay'], 1)
        self.assertEqual(data['dynamic_difficulty_tier_counts'], {'3': 1})
        self.assertNotIn('door', json.dumps(data))

    def test_bad_output_shape(self):
        changed = SAMPLE.replace(b'Open,,0.25,-1', b'Open,bad')
        self.assertEqual(audit.audit_vmf(changed)['output_shapes_needing_review'], 1)

    def test_non_on_outputs_valid(self):
        self.assertEqual(
            audit.audit_vmf(SAMPLE.replace(b'"OnTrigger"', b'"OutValue"'))['output_shapes_needing_review'], 0)

    def test_malformed_vmfs_fail_closed(self):
        with self.assertRaises(audit.SDKFormatError):
            audit.audit_vmf(SAMPLE[:-4])

    def test_weapon_field_validation(self):
        with self.assertRaises(audit.SDKFormatError):
            audit.extract_weapon_keys('"ZoomFOV" "50"')

    def test_vpk_not_a_directory(self):
        with self.assertRaises(audit.SDKFormatError):
            audit.read_vpk_directory(b'junk')

    def test_full_synthetic_zip_audit(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'sdk.zip'
            path.write_bytes(make_fixture_zip().getvalue())
            result = audit.audit_sdk(path)
        self.assertEqual(len(result['maps']), 3)
        self.assertEqual(result['weapons']['weapon_magnum']['ZoomFOV'], 50)
        self.assertEqual(result['maps']['se1_docks04.vmf']['entity_count'], 2)
        self.assertNotIn('door', json.dumps(result))

    def test_vpk_crc_guard(self):
        src = make_fixture_zip()
        with zipfile.ZipFile(src) as z:
            index = audit.read_vpk_directory(z.read(audit.VPK_DIR))
            data = z.read(audit.VPK_DATA)
        with self.assertRaisesRegex(audit.SDKFormatError, 'CRC'):
            audit.vpk_file(index, data.replace(b'logic_relay', b'logic_trlay'),
                           audit.MAP_PATHS[0])

    def test_vpk_bounds_guard(self):
        src = make_fixture_zip()
        with zipfile.ZipFile(src) as z:
            index = audit.read_vpk_directory(z.read(audit.VPK_DIR))
            data = z.read(audit.VPK_DATA)
        a, b, c, d, e = index[audit.MAP_PATHS[0]]
        index[audit.MAP_PATHS[0]] = (a, b, len(data) + 1, d, e)
        with self.assertRaises(audit.SDKFormatError):
            audit.vpk_file(index, data, audit.MAP_PATHS[0])


if __name__ == '__main__':
    unittest.main()
