"""Synthetic verification of SiN DLL/SDK/VDC entity-evidence separation."""
from __future__ import annotations
import csv
import io
import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_entity_compatibility import (
    build_matrix, parse_fgd, read_factory_csv, read_sdk_fgds, read_wiki_plaintext,
)

BASE = b'''
@BaseClass = Targetname
[
 targetname(target_source) : "Name"
 input Kill(void) : "Delete"
 output OnUser1(void) : "User"
]
@PointClass base(Targetname) = logic_relay : "Shared relay"
[
 input Trigger(void) : "Fire"
]
@PointClass = info_target : "Only editor"
[
]
'''
SIN = b'''
@include "base.fgd"
@BaseClass base(Targetname) = SinThing
[
 sin_level(integer) : "Sin difficulty" : 0
 input SinEnable(void) : "Enable"
]
@NPCClass base(SinThing) = npc_sintek_grunt : "Grunt"
[
 output OnSquadDead(void) : "Squad dead"
]
@PointClass base(Targetname) = prop_phone : "Phone"
[
 input Dial(string) : "Phone call"
]
@PointClass = npc_sdk_only : "Editor-only NPC"
[
]
'''
FACTORIES = [
    {"classname": name, "register_va": "0x223a0000", "factory_va":"0x22500000"}
    for name in ("logic_relay", "npc_sintek_grunt", "binary_only")
]


class EntityMatrixTests(unittest.TestCase):
    def test_parse_fgd_direct_properties_and_nested_choices(self):
        raw = b'''
@BaseClass = BaseX
[
 health(integer) : "hp" : 50
 mode(choices) : "choose" : 0 =
 [
  0 : "off"
  1 : "on"
 ]
 input Start(void) : "Start"
]
@PointClass base(BaseX) = npc_test : "Doc // not comment"
[
 input Spawn(void) : "Spawn" // comment
 output OnDone(void) : "Done"
]
'''
        found = parse_fgd(raw, "SinEpisodes.fgd")
        self.assertEqual(len(found), 2)
        self.assertEqual(found[1]["classname"],"npc_test")
        self.assertEqual(found[0]["direct_keyvalues"],["health","mode"])

    def test_source_axes_never_assume_wiki_absence_or_quest_support(self):
        defs = parse_fgd(BASE,"base.fgd")+parse_fgd(SIN,"SinEpisodes.fgd")
        m = build_matrix(FACTORIES,defs)
        idx = {x["classname"]:x for x in m["entities"]}
        self.assertEqual(idx["npc_sintek_grunt"]["evidence_class"],"binary_and_sin_fgd")
        self.assertEqual(idx["logic_relay"]["evidence_class"],"binary_and_base_fgd")
        self.assertEqual(idx["binary_only"]["evidence_class"],"binary_only_no_sdk_declaration")
        self.assertEqual(idx["npc_sdk_only"]["evidence_class"],
                         "sin_fgd_no_recognized_binary_registration")
        self.assertIsNone(idx["prop_phone"]["on_wiki_list"])
        self.assertEqual(idx["npc_sintek_grunt"]["fgd_base_names"],["SinThing"])
        self.assertIn("SinEnable",idx["npc_sintek_grunt"]["fgd_inputs_including_inherited"])
        self.assertIn("Kill",idx["npc_sintek_grunt"]["fgd_inputs_including_inherited"])
        self.assertIn("OnSquadDead",idx["npc_sintek_grunt"]["fgd_outputs_including_inherited"])
        self.assertIn("sin_level",idx["npc_sintek_grunt"]["fgd_keyvalues_including_inherited"])
        self.assertEqual(idx["npc_sintek_grunt"]["implementation_status"],
                         "not_implemented_in_quest_port")

    def test_optional_wiki_names_are_only_user_supplied(self):
        defs=parse_fgd(BASE,"base.fgd")+parse_fgd(SIN,"SinEpisodes.fgd")
        matrix=build_matrix(FACTORIES,defs,{"npc_sintek_grunt","wiki_special"})
        i={row["classname"]:row for row in matrix["entities"]}
        self.assertTrue(i["npc_sintek_grunt"]["on_wiki_list"])
        self.assertFalse(i["prop_phone"]["on_wiki_list"])
        self.assertEqual(i["wiki_special"]["evidence_class"],
                         "wiki_only_unconfirmed_by_sdk_binary")

    def test_cycle_in_inherited_fgd_is_rejected(self):
        defs=parse_fgd(b'@BaseClass base(B)= A\n[]\n@BaseClass base(A)= B\n[]\n'
                       b'@PointClass base(A) = test\n[]\n',"base.fgd")
        with self.assertRaisesRegex(ValueError, "cyclic"):
            build_matrix([],defs)

    def test_sdk_zip_requires_exact_two_game_fgds(self):
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)/"sdk.zip"
            with zipfile.ZipFile(p,"w") as z:
                z.writestr("SinEpisodesSDK/bin/base.fgd",BASE)
            with self.assertRaisesRegex(ValueError,"missing"):
                read_sdk_fgds(p)
            with zipfile.ZipFile(p,"a") as z:
                z.writestr("SinEpisodesSDK/bin/SinEpisodes.fgd",SIN)
            self.assertEqual(len(read_sdk_fgds(p)),7)

    def test_wiki_plaintext_rejects_markup(self):
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)/"wiki.txt"
            p.write_text("npc_sintek_grunt\n# provenance\nprop_phone\n")
            self.assertEqual(read_wiki_plaintext(p),{"npc_sintek_grunt","prop_phone"})
            p.write_text("<html>not a verified list</html>")
            with self.assertRaisesRegex(ValueError,"wiki list line"):
                read_wiki_plaintext(p)

    def test_factory_csv_refuses_duplicates_or_malformed_addresses(self):
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)/"factory.csv"
            p.write_text("classname,register_va,factory_va\nnpc_test,0x22300000,0x22500000\n")
            self.assertEqual(len(read_factory_csv(p)),1)
            p.write_text(p.read_text()+"npc_test,0x22300001,0x22500001\n")
            with self.assertRaisesRegex(ValueError,"duplicate"):
                read_factory_csv(p)


if __name__=="__main__":
    unittest.main()
