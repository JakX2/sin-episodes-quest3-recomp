"""Synthetic tests only: original private server.dll is never committed."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    'mobgrunt_assault', TOOLS / 'audit_sin_mobgrunt_assault_rally_timing.py')
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    def __init__(self, mapping):
        self.mapping = mapping

    def at(self, va, size):
        for start, blob in self.mapping.items():
            if start <= va and va + size <= start + len(blob):
                return blob[va - start:va - start + size]
        raise a.VerificationError(f'no synthetic original range {va:08X}+{size}')


def call(src, dst):
    return b'\xe8' + struct.pack('<i', dst - src - 5)


def fixture():
    pairs = {
        0x22357E50: b'\x56\x8b\xf1\x8b\x4c\x24\x08',
        0x22357E5A: b'\x83\x79\x10\x02',
        0x22357E6D: call(0x22357E6D, 0x221561D0),
        0x22357E78: call(0x22357E78, 0x221F3DE0),
        0x22357E82: b'\x6a\x03\x51\x8d\x8e\xb8\x0d\x00\x00',
        0x22357E8B: call(0x22357E8B, 0x220CFC00),
        0x220CFC1A: b'\x68\x7c\x3c\x3f\x22',
        0x223F3C7C: b'CAI_AssaultBehavior::SetParameters\0',
        0x223F3CA0: b"**ERROR: Can't find a rally point named '%s'\n\0",
        0x22357EA4: b'\x83\x78\x18\x06',
        0x22357EAD: b'\xc7\x81\x70\x0c\x00\x00\x00\x00\x00\x00',
        0x22357EB7: b'\x88\x81\x6c\x0c\x00\x00',
        0x22357EC0: b'\x33\xc0\x89\x81\x70\x0c\x00\x00\x88\x81\x6c\x0c\x00\x00',
        0x22357FA0: b'\x80\xb9\x6c\x0c\x00\x00\x00\x74\x07',
        0x22357FA9: b'\xd9\x05\xd0\x99\x3d\x22\xc3',
        0x22357FB0: b'\xd9\x05\xf8\x22\x3d\x22\xc3',
        0x22357FC0: b'\x80\xb9\x6c\x0c\x00\x00\x00\x74\x07',
        0x22357FC9: b'\xd9\x05\xac\x7c\x40\x22\xc3',
        0x22357FD0: b'\xd9\x05\x80\xe1\x3d\x22\xc3',
        0x223D99D0: struct.pack('<f', 0.25),
        0x223D22F8: struct.pack('<f', 1.0),
        0x22407CAC: struct.pack('<f', 2.5),
        0x223DE180: struct.pack('<f', 6.0),
        0x2235F3DA: b'\x38\x9d\x6c\x0c\x00\x00',
        0x2235F3F2: b'\xd9\x05\xd0\x99\x3d\x22',
        0x2235F3FA: b'\xd9\x05\xf8\x22\x3d\x22',
        0x2235F406: b'\xd8\x42\x0c',
        0x2235F410: b'\xd9\x9d\x80\x0c\x00\x00',
        0x2235F461: b'\x88\x9d\x6c\x0c\x00\x00',
    }
    for classname, entry, string_va, factory_va in a.ASSAULT_REGISTRATIONS:
        parts = (call(entry, 0x2228B110) + b'\x8b\x10\x68' +
                 struct.pack('<I', string_va) + b'\x68' +
                 struct.pack('<I', factory_va) + b'\x8b\xc8\xff\x12\xc3')
        assert len(parts) == 22
        pairs[entry] = parts
        pairs[string_va] = classname.encode('ascii') + b'\0'
    pairs.update({
        0x220CFC70: b'\x8b\x86\xbc\x03\x00\x00\x83\xf8\xff\x74\x26',
        0x220CFC98: b'\x83\x39\x00\x0f\x85\x43\x01\x00\x00',
        0x220CFC62: b'\x83\xcd\xff',
        0x220CFCA1: b'\x8b\x86\xa0\x03\x00\x00',
        0x220CFCA7: b'\x3b\xc5\x7e\x07',
        0x220CFCAB: b'\x8b\xe8',
        0x220CFCB2: b'\x0f\x85\x2c\x01\x00\x00',
        0x220CFCCF: b'\x81\xc7\xbc\x02\x00\x00',
        0x220CFCFE: b'\xd9\x86\xbc\x02\x00\x00',
        0x220CFD4C: b'\xd9\x44\x24\x20\xd8\x4c\x24\x20',
        0x220CFD75: b'\xd9\x83\xbc\x02\x00\x00',
        0x220CFDBB: b'\xd9\x44\x24\x38\xd8\x4c\x24\x38',
        0x220CFDD7: b'\xd8\x5c\x24\x14\xdf\xe0\xf6\xc4\x41\x75\x02\x8b\xde',
        0x220CFE22: b'\x85\xdb',
        0x220CFE5A: b'\x89\x47\x18',
        0x220CFE30: b'\x68\xd0\x3c\x3f\x22',
        0x223F3CD0: b"%s Didn't find a best rally point!\n\0",
    })
    for name, desc, name_va, dtype, offset, flags, external_va, external_name in a.RALLY_TYPEDESC:
        pairs[desc] = (struct.pack('<8I', dtype, name_va, offset, 0,
                                   (flags << 16) | 1, external_va, 0, 0) + b'\x00' * 20)
        pairs[name_va] = name.encode('ascii') + b'\x00'
        if external_name:
            pairs[external_va] = external_name.encode('ascii') + b'\x00'
    for name, desc, name_va, dtype, offset, flags, external_va, external_name, handler in a.ASSAULT_POINT_TYPEDESC:
        pairs[desc] = (struct.pack('<8I', dtype, name_va, offset, 0,
                                   (flags << 16) | 1, external_va, 0, handler) + b'\0' * 20)
        pairs[name_va] = name.encode('ascii') + b'\0'
        pairs[external_va] = external_name.encode('ascii') + b'\0'
    pairs.update({
        0x220CDE74: b'\x83\x78\x18\x06',
        0x220CDE7D: b'\x88\x81\xb8\x03\x00\x00',
        0x220CDE88: b'\x88\x81\xb8\x03\x00\x00',
        0x220CDEA4: b'\x83\x78\x18\x06',
        0x220CDEAD: b'\x88\x81\xb9\x03\x00\x00',
        0x220CDEB8: b'\x88\x81\xb9\x03\x00\x00',
    })
    return pairs


class MobGruntOriginalAssaultTests(unittest.TestCase):
    def test_original_rally_point_and_timing_labels(self):
        result = a.audit_code(FakePE(fixture()))
        self.assertEqual(result['assault_input']['behavior_object_offset'], '0xDB8')
        self.assertEqual(result['rally_point_ranking']['original_priority_field_offset'], '0x3A0')
        fields = {f['member']: f for f in result['original_rally_point_datamap']['field_descriptors']}
        self.assertEqual(fields['m_iPriority']['original_offset'], '0x3A0')
        self.assertEqual(fields['m_hLockedBy']['original_offset'], '0x3BC')
        self.assertEqual(fields['m_AssaultPointName']['external_key'], 'assaultpoint')
        assault = {e['member']: e for e in result['original_assault_point_datamap']['fields']}
        self.assertEqual(assault['m_NextAssaultPointName']['external_name'], 'nextassaultpoint')
        self.assertEqual(assault['m_flAssaultTimeout']['offset'], '0x3B4')
        self.assertEqual(assault['m_bClearOnContact']['offset'], '0x3B8')
        self.assertEqual(assault['m_bAllowDiversion']['offset'], '0x3B9')
        self.assertEqual(assault['InputSetClearOnContact']['original_input_handler'], '0x220CDE70')
        self.assertEqual(result['rally_point_ranking']['original_entity_origin_field_offset'], '0x2BC')
        self.assertEqual([r['classname'] for r in result['registered_assault_entities']],
                         ['assault_rallypoint', 'assault_assaultpoint', 'ai_goal_assault'])
        self.assertEqual(result['assault_input']['passed_integer'], 3)
        self.assertEqual(result['throw_grenades_input']['toggle_byte_offset'], '0xC6C')
        self.assertEqual(result['conditional_values']['enabled'], [0.25, 2.5])
        self.assertEqual(result['conditional_values']['disabled'], [1.0, 6.0])
        self.assertEqual(result['conditional_values']['initialization_timestamp_offset'], '0xC80')

    def test_original_sdk_contract_explains_rally_point_parameter_and_cue_three(self):
        base = b'input Assault(string) : "Start an assault. Parameter passed in should be the name of the rally point."'
        game = (
            b'@PointClass = assault_rallypoint : "rally"\n'
            b'@PointClass = assault_assaultpoint : "assault"\n'
            b'@PointClass = ai_goal_assault : "goal"\n'
            b'AssaultCue(choices) : "Assault Cue" : 1 = [\n'
            b'1 : "Entity System Input"\n2 : "Gunfire"\n'
            b'3 : "Don\'t wait for a cue."\n]\n'
        )
        found = a.check_sdk_fgd(base, game)
        self.assertIn('rally point', found['assault_input'])
        self.assertEqual(found['assault_cue_value_3'], "Don't wait for a cue.")
        self.assertIn('assault_rallypoint', found['assault_entity_classnames'])

        with self.assertRaises(a.VerificationError):
            a.check_sdk_fgd(base.replace(b'rally point', b'assault point'), game)
        with self.assertRaises(a.VerificationError):
            a.check_sdk_fgd(base, game.replace(b'3 :', b'4 :'))

    def test_rejects_corrupted_assault_rallypoint_factory(self):
        fixture_bytes = fixture()
        name, entry, name_va, factory_va = a.ASSAULT_REGISTRATIONS[0]
        fixture_bytes[entry] = (
            call(entry, 0x2228B110) + b'\x8b\x10\x68' +
            struct.pack('<I', name_va) + b'\x68' +
            struct.pack('<I', factory_va + 4) + b'\x8b\xc8\xff\x12\xc3'
        )
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(fixture_bytes))

    def test_corrupted_assault_point_bool_setter_fails(self):
        mapping = fixture()
        mapping[0x220CDEAD] = b'\x88\x81\xb8\x03\x00\x00'
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(mapping))

    def test_corrupt_rallypoint_lock_datamap_fails(self):
        data = fixture()
        name, desc, name_va, dtype, offset, flags, external_va, external_name = a.RALLY_TYPEDESC[4]
        data[desc] = (struct.pack('<8I', dtype, name_va, offset + 4, 0,
                                 (flags << 16) | 1, external_va, 0, 0) + b'\0' * 20)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_corrupt_rally_priority_offset_fails(self):
        bytes_map = fixture()
        bytes_map[0x220CFCA1] = b'\x8b\x86\xa4\x03\x00\x00'
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(bytes_map))

    def test_refuses_wrong_assault_behavior_target(self):
        data = fixture()
        data[0x22357E8B] = call(0x22357E8B, 0x220CFC20)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_changed_rally_point_text(self):
        data = fixture()
        data[0x223F3C7C] = b'CAI_AssaultBehavior::SetParameterX\0'
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_modified_conditional_constant(self):
        data = fixture()
        data[0x22407CAC] = struct.pack('<f', 3.0)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_wrong_mobgrunt_input_field_offset(self):
        data = fixture()
        data[0x22357EB7] = b'\x88\x81\x70\x0c\x00\x00'
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))


if __name__ == '__main__':
    unittest.main()
