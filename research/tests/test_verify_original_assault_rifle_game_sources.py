"""Synthetic regressions for the read-only original assault-rifle source audit.

Real proprietary SDK, scripts, and DLLs are not committed to the repository.
"""
import importlib.util
from pathlib import Path
import unittest

MODULE = Path(__file__).resolve().parents[1] / 'tools' / 'verify_original_assault_rifle_game_sources.py'
spec = importlib.util.spec_from_file_location('ar_sources', MODULE)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def synthetic_sources():
    fgd = (b'@NPCClass base(BaseGrunt) '
           b'studio("models/characters/MobGrunt/mobgrunt.mdl") = '
           b'npc_druglab_grunt_assault_rifle : "Druglab Grunt"\n'
           b'[\nadditionalequipment(choices) : "Weapons" : '
           b'"weapon_assault_rifle" =\n[\n'
           b'"weapon_assault_rifle" : "Assault Rifle"\n]\n]\n')
    values = {
        'viewmodel': 'models/weapons/v_assault_rifle/v_assault_rifle.mdl',
        'playermodel': 'models/weapons/w_assault_rifle/w_assault_rifle.mdl',
        'anim_prefix': 'smg2', 'ZoomFOV': '40',
        'clip_size': '45', 'clip2_size': '-1',
        'default_clip': '45', 'default_clip2': '0',
        'primary_ammo': 'AR', 'secondary_ammo': 'ARNADE',
        'weight': '7', 'MeleeWeapon': '0',
        'kickMin': '0.175 0.0 0.0',
        'kickRampMin': '0 -1 -0.275',
        'kickRampMax': '1.5 1 0.275',
        'kickRampSecs': '1.5',
        'kickRampStartScale': '0.2',
        'kickRampEndScale': '0.7',
    }
    for key in audit.SOUND_KEYS:
        values[key] = 'Weapon_Assault.' + key
    weapon = ('WeaponData\n{\n' +
              ''.join(f'"{k}" "{v}"\n' for k,v in values.items()) + '}\n').encode()
    sounds = ''.join(f'"Weapon_Assault.{k}"\n{{\n"wave" "sample.wav"\n}}\n'
                     for k in audit.SOUND_KEYS).encode()
    client = b'CWeaponAssaultRifle\0weapon_assault_rifle\0'
    server = client + b'CEntityFactory@VCWeaponAssaultRifle\0grenade_ar\0ARNADE\0'
    return fgd, weapon, sounds, server, client


class OriginalAssaultRifleSourcesTests(unittest.TestCase):
    def test_fixture_without_original_dll_hash_claim(self):
        values = audit.inspect(*synthetic_sources(), verify_hash=False)
        self.assertEqual(values['sdk_npc_default_equipment'], 'weapon_assault_rifle')
        self.assertEqual(values['weapon_script']['clip_size'], '45')
        self.assertEqual(values['weapon_script']['secondary_ammo'], 'ARNADE')
        self.assertEqual(values['original_secondary_projectile_classname'], 'grenade_ar')
        self.assertFalse(values['dlls']['server.dll']['pinned_hash_verified'])

    def test_hash_checks_are_mandatory_by_default(self):
        with self.assertRaises(audit.VerificationError):
            audit.inspect(*synthetic_sources())

    def test_fails_closed_on_wrong_npc_weapon(self):
        fgd, weapon, sounds, server, client = synthetic_sources()
        fgd = fgd.replace(b'"weapon_assault_rifle"', b'"weapon_ar2"')
        with self.assertRaises(audit.VerificationError):
            audit.inspect(fgd, weapon, sounds, server, client, verify_hash=False)

    def test_fails_closed_on_wrong_clip_size_or_missing_sound(self):
        fgd, weapon, sounds, server, client = synthetic_sources()
        with self.assertRaises(audit.VerificationError):
            audit.inspect(fgd, weapon.replace(b'"clip_size" "45"', b'"clip_size" "30"'),
                          sounds, server, client, verify_hash=False)
        with self.assertRaises(audit.VerificationError):
            audit.inspect(fgd, weapon, sounds.replace(b'"Weapon_Assault.single_shot"',
                                                      b'"Unknown.sound"'),
                          server, client, verify_hash=False)

    def test_fails_closed_on_missing_factory_marker(self):
        fgd, weapon, sounds, server, client = synthetic_sources()
        with self.assertRaises(audit.VerificationError):
            audit.inspect(fgd, weapon, sounds,
                          server.replace(b'CEntityFactory@VCWeaponAssaultRifle', b'other'),
                          client, verify_hash=False)


if __name__ == '__main__':
    unittest.main()
