"""Regression tests for SiN whole-DLL system recovery queue, no original binaries."""
from __future__ import annotations
import pathlib
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_entire_dll_code_census import HASHES,CensusError
from build_sin_dll_recovery_queue import generate

def fake_rtti():
    return {
        "client":{
            "sha256":HASHES["client"],
            "vtables":[
                {"class":"C_DynamicDifficulty",
                 "functions":["0x241FAC90","0x241FAC90","0x241CF700"]},
                {"class":"CHudGrenades",
                 "function_va":["0x24235000","0x241CF700"]},
            ],
        },
        "server":{
            "sha256":HASHES["server"],
            "vtables":[
                {"class":"CGrenadeAR",
                 "functions":["0x22314570","0x223142F0"]},
                {"class":"CNPC_GruntBase",
                 "function_va":["0x22314570","0x2235DE00"]},
                {"class":"CDynamicDifficulty",
                 "functions":["0x222B9050"]},
            ],
        },
    }

class RecoveryQueueTests(unittest.TestCase):
    def test_systems_preserve_dll_boundaries_and_no_implementation_claim(self):
        rows=generate(fake_rtti())
        by={(x["dll"],x["system"]):x for x in rows}
        self.assertEqual(by["client.dll","dynamic_difficulty"]["distinct_virtual_target_addresses"],2)
        self.assertEqual(by["server.dll","dynamic_difficulty"]["distinct_virtual_target_addresses"],1)
        self.assertEqual(by["server.dll","grenade_projectile"]["distinct_virtual_target_addresses"],2)
        self.assertEqual(by["server.dll","sintek_ai"]["distinct_virtual_target_addresses"],2)
        self.assertIn("not_equivalent_gameplay",by["server.dll","sintek_ai"]["status"])
        self.assertTrue(by["client.dll","weapon_and_grenade_client"]["unknown_or_missing_requested_classes"])

    def test_one_vtable_entry_point_shared_by_multiple_classes_is_not_double_counted(self):
        rows=generate(fake_rtti())
        first=[x for x in rows if x["dll"]=="server.dll" and x["system"]=="grenade_projectile"][0]
        self.assertEqual(first["distinct_virtual_target_addresses"],2)

    def test_forged_original_rtti_sha_is_rejected(self):
        bad=fake_rtti()
        bad["client"]["sha256"]="0000"
        with self.assertRaisesRegex(CensusError,"fingerprint"):
            generate(bad)

    def test_both_rtti_vtable_function_key_formats_are_supported(self):
        entries=generate(fake_rtti())
        row=[x for x in entries if x["system"]=="poison_grenade_hud"][0]
        self.assertEqual(row["distinct_virtual_target_addresses"],2)
        self.assertEqual(row["rtti_classes_present"],["CHudGrenades"])

if __name__=="__main__":
    unittest.main()
