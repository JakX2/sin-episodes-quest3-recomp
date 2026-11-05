"""Synthetic tests: no original SiN DLLs or game code embedded."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    'sin_mobgrunt_datamap', TOOLS / 'audit_sin_original_mobgrunt_datamap.py'
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class FakePE:
    def __init__(self, regions):
        self.regions = regions

    def at(self, va, size):
        for base, data in self.regions:
            if base <= va and va + size <= base + len(data):
                return data[va-base:va-base+size]
        raise audit.VerificationError(f'unmapped synthetic VA 0x{va:08X}')


def descriptor(dtype, name_va, offset, flags, external_va=0, handler=0):
    return (struct.pack('<8I', dtype, name_va, offset, 0,
                        (flags << 16) | 1, external_va, 0, handler)
            + b'\x00' * 20)


def test_regions():
    out = []
    names = []
    for name, desc, va, dtype, offset, flags, external in audit.MEMBERS:
        out.append((desc, descriptor(dtype, va, offset, flags, external)))
        names.append((va, name))
    for name, desc, va, dtype, external_name, external, handler in audit.INPUTS:
        out.append((desc, descriptor(dtype, va, 0, 8, external, handler)))
        names.append((va, name))
        names.append((external, external_name))
    names.extend([(0x22491024, 'NumGrenades'),
                  (0x22490F8C, 'GrenadeHeight')])
    out.extend((addr, label.encode('ascii') + b'\x00') for addr, label in names)
    return out


class OriginalMobGruntDataMapTests(unittest.TestCase):
    def test_actual_layout_hypotheses_have_matching_synthetic_witnesses(self):
        result = audit.audit_datamap(FakePE(test_regions()))
        members = {m['member']: m for m in result['member_descriptors']}
        self.assertEqual(members['m_iNumGrenades']['offset_hex'], '0xC38')
        self.assertEqual(members['m_flShotDelay']['offset_hex'], '0xC9C')
        self.assertEqual(members['m_flNextAltFireTime']['offset_hex'], '0xC90')
        self.assertEqual(members['m_flGrenadeHeight']['hammer_key'], 'GrenadeHeight')
        inputs = {e['external_input']: e['function_va']
                  for e in result['input_descriptors']}
        self.assertEqual(inputs['Assault'], '0x22357E50')
        self.assertEqual(inputs['ThrowGrenades'], '0x22357EA0')

    def test_refuse_mutated_datamap_field_offset(self):
        regions = test_regions()
        changed = [
            (va, descriptor(1, 0x22491040, 0xC94, 2)
             if va == 0x22535198 else data)
            for va, data in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.audit_datamap(FakePE(changed))

    def test_refuse_changed_handler_or_wrong_external_input(self):
        regions = test_regions()
        changed = [
            (va, descriptor(2, 0x22490F10, 0, 8, 0x223F5624, 0x22357EA0)
             if va == 0x225355A8 else data)
            for va, data in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.audit_datamap(FakePE(changed))

    def test_actual_handler_byte_witnesses(self):
        # The first handler state call is a short relative x86 E8 call.
        callsite, target = 0x22357E8B, 0x220CFC00
        call = b'\xe8' + struct.pack('<i', target-callsite-5)
        pe = FakePE([
            (0x22357E50, b'\x56\x8b\xf1'),
            (0x22357E5A, b'\x83\x79\x10\x02'),
            (0x22357E66, b'\xb8\xa8\x47\x3d\x22'),
            (0x22357E6D, b'\xe8' + struct.pack('<i', 0x221561D0-0x22357E72)),
            (0x22357E78, b'\xe8' + struct.pack('<i', 0x221F3DE0-0x22357E7D)),

            (0x22357E82, b'\x6a\x03'),
            (0x22357E85, b'\x8d\x8e\xb8\x0d\x00\x00'),
            (callsite, call),
            (0x22357EA4, b'\x83\x78\x18\x06'),
            (0x22357EAD, b'\xc7\x81\x70\x0c\x00\x00\x00\x00\x00\x00'),
            (0x22357EB7, b'\x88\x81\x6c\x0c\x00\x00'),
        ])
        value = audit.audit_handler_code(pe)
        self.assertEqual(value['assault_input']['submitted_state_argument'], 3)
        self.assertEqual(value['grenade_input']['bool_field_offset'], '0xC6C')

    def test_refuse_corrupt_original_input_handler_call(self):
        callsite = 0x22357E8B
        pe = FakePE([
            (0x22357E50, b'\x56\x8b\xf1'),
            (0x22357E82, b'\x6a\x03'),
            (0x22357E85, b'\x8d\x8e\xb8\x0d\x00\x00'),
            (callsite, b'\xe8\x00\x00\x00\x00'),
        ])
        with self.assertRaises(audit.VerificationError):
            audit.audit_handler_code(pe)


    def test_original_look_inputs_toggle_same_bit_in_opposite_directions(self):
        regions = [
            (0x2235DDC0, b'\x8b\x91\xb4\x00\x00\x00'),
            (0x2235DDC8, b'\x25\xff\xff\xfe\xff'),
            (0x2235DDD1, b'\x80\xb9\xf0\x01\x00\x00\x00'),
            (0x2235DDDA, b'\xc6\x81\x00\x02\x00\x00\x01'),
            (0x2235DDF4, b'\x83\x0a\x01'),
            (0x2235F260, b'\x8b\x91\xb4\x00\x00\x00'),
            (0x2235F268, b'\x0d\x00\x00\x01\x00'),
            (0x2235F271, b'\x80\xb9\xf0\x01\x00\x00\x00'),
            (0x2235F27A, b'\xc6\x81\x00\x02\x00\x00\x01'),
            (0x2235F294, b'\x83\x0a\x01'),
        ]
        state = audit.audit_look_target_flag(FakePE(regions))
        self.assertEqual(state['bit_mask'], '0x00010000')
        self.assertEqual(state['look_on_operation'], 'flags &= ~0x00010000')
        self.assertEqual(state['look_off_operation'], 'flags |= 0x00010000')
        # Alter only the LookOff OR bitmask and reject the result.
        modified = [
            (va, b'\x0d\x00\x00\x02\x00' if va == 0x2235F268 else data)
            for va, data in regions
        ]
        with self.assertRaises(audit.VerificationError):
            audit.audit_look_target_flag(FakePE(modified))

if __name__ == '__main__':
    unittest.main()
