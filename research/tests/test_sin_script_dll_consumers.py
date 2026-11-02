"""Synthetic, original-free tests for script and x86 consumer reconnaissance."""
import io
import pathlib
import struct
import sys
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_script_dll_consumers import (
    ScriptError, parse_kv, scalar, one_block, inspect_scripts,
    reference_pointers, analyze, KEYS,
)
from test_gameplay_entity_registry import fake_registered_server

WEAPON=b'''
// An original-free illustrative weapon: quotes and nested data.
WeaponData {
 "printname" "Synthetic // name"
 "clip_size" "20"
 "default_clip" "20"
 "ZoomFOV" "50"
 "primary_ammo" "MAG"
 "secondary_ammo" ""
 "SoundData" {
   "single_shot" "Weapon_Test.Single"
   "single_shot" "Weapon_Test.NPC"
 }
 "TextureData" {
   weapon { "font" "WeaponIcons" "character" "e" }
 }
}
'''
RULES=b'''
AttackSpreadRules {
 "class_names" { "player" "player_rule" }
 "player_rule" {
   "weapon_class_names" { "*" "generic_weapon" }
   "generic_weapon" {
     "target_class_names" { "*" "generic_target" }
     "generic_target" "0.0"
   }
 }
}
'''
PHONE=b'PhoneBook { "9999" "phonebook.9999" "0000" "npc_jessica.Objective" }'
SOUNDS=b'"phonebook.9999" { "wave" "test.wav" }'
CAR=b'''
"vehicle" {
 "engine" { "horsepower" "350" "gear" "1.86" "gear" "1.59" }
 "axle" { "wheel" { "radius" "20" } }
 "axle" { "wheel" { "radius" "20" } }
}
'''


def scripts_zip(weapon=WEAPON) -> bytes:
    out=io.BytesIO()
    with zipfile.ZipFile(out,"w") as z:
        for name,data in {
            "scripts/weapon_magnum.txt":weapon,
            "scripts/rules_combat.txt":RULES,
            "scripts/phonebook.txt":PHONE,
            "scripts/game_sounds_phonebook.txt":SOUNDS,
            "scripts/vehicles/sincar.txt":CAR,
            "scripts/vehicles/sincar_pit.txt":CAR,
        }.items():
            z.writestr(name,data)
    return out.getvalue()


class ScriptDllTests(unittest.TestCase):
    def test_nested_weapon_and_duplicate_sound_events(self):
        data=inspect_scripts(scripts_zip(),{"weapon_magnum"})
        self.assertEqual(data["weapon_count"],1)
        w=data["weapons"][0]
        self.assertEqual(w["specified_fields"]["printname"],"Synthetic // name")
        self.assertEqual(w["specified_fields"]["clip_size"],"20")
        self.assertEqual(w["specified_fields"]["secondary_ammo"],"")
        self.assertEqual(w["sound_event_keys"],["single_shot","single_shot"])
        self.assertEqual(w["client_texture_categories"],["weapon"])
        self.assertTrue(w["server_entity_factory_22byte_pattern"])
        self.assertEqual(data["combat"]["class_to_rule"],{"player":"player_rule"})
        self.assertEqual(data["combat"]["rules"]["player_rule"]["target_spreads"]
                         ["generic_weapon"]["literal_values"]["generic_target"],"0.0")
        self.assertEqual(data["phonebook"]["number_count"],2)
        self.assertEqual(data["phonebook"]["sound_references_in_dedicated_file"],
                         ["phonebook.9999"])
        self.assertEqual(data["vehicles"][0]["gear_entries"],2)
        self.assertEqual(data["vehicles"][0]["axle_entries"],2)

    def test_missing_factory_is_not_labeled_unavailable(self):
        data=inspect_scripts(scripts_zip(),set())
        self.assertFalse(data["weapons"][0]["server_entity_factory_22byte_pattern"])

    def test_reject_unsafe_kv_syntax(self):
        for invalid in (b'A { B }', b'A { B "foo"', b'A { "B" "x" } }',
                        b'A { { } }', b'A "unclosed'):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ScriptError):
                    parse_kv(invalid)

    def test_duplicate_scalar_rejected_not_silently_overridden(self):
        v=one_block(parse_kv(b'WeaponData {"clip_size" "3" "clip_size" "4"}'),
                    "WeaponData")
        with self.assertRaisesRegex(ScriptError,"duplicate"):
            scalar(v)

    def test_original_looking_push_pointer_needs_real_rdata_literal(self):
        b=bytearray(fake_registered_server())
        base=0x22000000
        text_va=0x22001310
        string_va=0x223d2500
        b[string_va-base:string_va-base+len(b"clip_size\x00")]=b"clip_size\x00"
        b[text_va-base:text_va-base+5]=b"\x68"+struct.pack("<I",string_va)
        found=reference_pointers(bytes(b),"clip_size")
        self.assertEqual(found,[{"literal_va":"0x223D2500",
                                 "push_imm32_va":"0x22001310"}])
        b[text_va-base]=0x90
        self.assertEqual(reference_pointers(bytes(b),"clip_size"),[])

    def test_actual_audit_refuses_synthetic_binary_hashes(self):
        with self.assertRaisesRegex(Exception,"SHA-256"):
            analyze(fake_registered_server(),fake_registered_server(),
                    scripts_zip())
        self.assertIn("PhoneBook",KEYS)


if __name__=="__main__":
    unittest.main()
