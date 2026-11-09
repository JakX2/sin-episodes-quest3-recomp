"""Synthetic regression tests for a whole-original-DLL recovery atlas.

No owner-provided proprietary binaries, disassembly dumps or game data.
"""
import csv
import hashlib
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from build_sin_both_dll_recovery_atlas import (
    HASHES, assemble, class_catalog, check_code_region,
    check_side_metadata, load_candidates, network_catalog, validate_original_files,
    write_catalog, CensusError,
)

def inputs():
    rtti={}
    for side in ("client","server"):
        classname="C_WeaponMagnum" if side=="client" else "CWeaponMagnum"
        rtti[side]={
            "sha256":HASHES[side],
            "types":[{"class":classname},{"class":"CSharedBase"}],
            "hierarchies":{classname:[{"class":"CSharedBase"}]},
            "vtables":[{"class":classname,"slot_count":2,"functions":[
                "0x24000100" if side=="client" else "0x22200100",
                "0x24000200" if side=="client" else "0x22200100"]}],
            "counts":{"classes_with_hierarchy":1,"distinct_vftables":1},
        }
    census={
        "client":{"sha256":HASHES["client"],
                  "counts":{"union_entrypoint_candidates":1}},
        "server":{"sha256":HASHES["server"],
                  "counts":{"union_entrypoint_candidates":2}},
    }
    candidates={
        "client":[{"entry_va":"0x24000100","seed_evidence":"vtable",
                   "direct_callsite_count":"0","owner_examples":"C_WeaponMagnum"}],
        "server":[
            {"entry_va":"0x22200100","seed_evidence":"vtable+direct_call",
             "direct_callsite_count":"12","direct_jump_to_entry_count":"1",
             "owner_examples":"CWeaponMagnum | CBaseWeapon",
             "nearby_string_examples":"weapon_magnum | other"},
            {"entry_va":"0x22200200","seed_evidence":"direct_call",
             "direct_callsite_count":"1"}]
    }
    factories={
        "sha256":{side+".dll":HASHES[side] for side in ("client","server")},
        "registry":{"registrations":[
            {"classname":"weapon_magnum","registration_va":"0x22300010",
             "factory_object_va":"0x22500010"}]},
        "actual_createinterface_registrations":{
            "client":[{"name":"VClient011","registration_va":"0x24100010",
                       "factory_callback_va":"0x24100020"}],
            "server":[{"name":"ServerGameDLL003","registration_va":"0x22100010",
                       "factory_callback_va":"0x22100020"}],
        }
    }
    manifest={
        "schema_version":1,"dll_sha256":HASHES,
        "methods":[
            {"dll":"client","entry_va":"0x24000100","label":"original client verified",
             "owner":"VClient011","recovery_status":"machine_code_verified",
             "original_code_sha256":hashlib.sha256(b"client").hexdigest(),
             "original_code_length":6,"evidence_path":"research/client.md"},
            {"dll":"server","entry_va":"0x22200100","label":"original server gameplay",
             "owner":"CWeaponMagnum","recovery_status":"behavior_reconstructed",
             "original_code_sha256":hashlib.sha256(b"server").hexdigest(),
             "original_code_length":6,"evidence_path":"research/server.md",
             "implementation_path":"include/weapon.hpp","test_path":"tests/weapon.cpp"},
        ]
    }
    return rtti,census,candidates,factories,manifest

class AtlasTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo=pathlib.Path(self.temp.name)
        for path in ("research/client.md","research/server.md",
                     "include/weapon.hpp","tests/weapon.cpp"):
            full=self.repo/path
            full.parent.mkdir(parents=True,exist_ok=True)
            full.write_text("synthetic fixture\n")
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()

    def atlas(self):
        return assemble(self.rtti,self.census,self.candidates,self.factories,
                        self.manifest,self.repo)

    def test_complete_synthetic_original_free_integration(self):
        atlas=self.atlas()
        self.assertEqual(len(atlas["functions"]),3)
        self.assertEqual(len(atlas["classes"]),4)
        self.assertEqual(len(atlas["entities"]),1)
        self.assertEqual(len(atlas["interfaces"]),2)
        self.assertEqual(atlas["summary"]["counts"]["server"]["census_entrypoint_candidates"],2)
        self.assertEqual(atlas["summary"]["counts"]["server"]["behavior_reconstructed_entries_in_this_ledger"],1)
        self.assertEqual(atlas["summary"]["counts"]["client"]["behavior_reconstructed_entries_in_this_ledger"],0)
        self.assertIsNone(atlas["summary"]["full_decompilation_percentage"])
        self.assertTrue(atlas["summary"]["census_is_not_an_exhaustive_function_list"])
        m=next(x for x in atlas["functions"] if x["entry_va"]=="0x22200100")
        self.assertEqual(m["direct_callsite_count"],12)
        self.assertEqual(m["owner_verified"],"CWeaponMagnum")
        self.assertEqual(m["nearby_string_hints"],["weapon_magnum","other"])
        c=next(x for x in atlas["classes"] if x["rtti_class"]=="CWeaponMagnum")
        self.assertEqual(c["distinct_virtual_targets"],1)
        self.assertIn("combat_weapons",c["prioritized_system_groups"])

    def test_exports_real_csv_and_summary_with_field_order(self):
        atlas=self.atlas()
        dest=self.repo/"private_generated"
        write_catalog(dest,atlas)
        self.assertEqual(len(list(dest.glob("*"))),7)
        summary=json.loads((dest/"SiN_2006_Both_DLLs_Recovery_Atlas.json").read_text())
        self.assertEqual(summary["method_ledger"]["evidence_ledger_entries"],{"client":1,"server":1})
        with (dest/"SiN_2006_Function_Recovery_Atlas.csv").open(newline="") as f:
            rows=list(csv.DictReader(f))
        self.assertEqual(len(rows),3)
        self.assertIn("recovery_status",rows[0])
        self.assertEqual(sum(r["recovery_status"]=="behavior_reconstructed" for r in rows),1)

    def test_verified_entry_not_in_census_is_not_silently_counted_as_candidate(self):
        m=dict(self.manifest["methods"][1])
        m["entry_va"]="0x22200300"
        self.manifest["methods"].append(m)
        atlas=self.atlas()
        summary=atlas["summary"]
        self.assertEqual(summary["counts"]["server"]["census_entrypoint_candidates"],2)
        self.assertEqual(summary["counts"]["server"]["documented_additional_verified_entries"],1)
        x=next(r for r in atlas["functions"] if r["entry_va"]=="0x22200300")
        self.assertFalse(x["census_candidate"])
        self.assertEqual(x["seed_evidence"],"manual_original_code")

    def test_mutation_bad_hashes_fail_closed(self):
        self.rtti["server"]["sha256"]="0"*64
        with self.assertRaisesRegex(CensusError,"RTTI mismatch"):
            self.atlas()
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()
        self.factories["sha256"]["client.dll"]="0"*64
        with self.assertRaisesRegex(CensusError,"factory metadata mismatch"):
            self.atlas()

    def test_census_duplicates_counts_and_unknown_seeds_rejected(self):
        self.candidates["server"].append(dict(self.candidates["server"][0]))
        with self.assertRaisesRegex(CensusError,"duplicate"):
            self.atlas()
        self.candidates["server"].pop()
        self.candidates["server"].pop()
        with self.assertRaisesRegex(CensusError,"row count"):
            self.atlas()
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()
        self.candidates["client"][0]["seed_evidence"]="nearby_string_only"
        with self.assertRaisesRegex(CensusError,"unknown candidate evidence"):
            self.atlas()

    def test_verified_label_code_hash_length_and_evidence_required(self):
        method=self.manifest["methods"][1]
        method["original_code_sha256"]="bad"
        with self.assertRaisesRegex(CensusError,"SHA256"):
            self.atlas()
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()
        method=self.manifest["methods"][1]
        method["original_code_length"]=0
        with self.assertRaisesRegex(CensusError,"code length"):
            self.atlas()
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()
        method=self.manifest["methods"][1]
        method["implementation_path"]="nonexistent"
        with self.assertRaisesRegex(CensusError,"implementation does not exist"):
            self.atlas()

    def test_no_unsubstantiated_runtime_success_or_duplicate_provenance(self):
        m=self.manifest["methods"][1]
        m["recovery_status"]="engine_runtime_verified"
        with self.assertRaisesRegex(CensusError,"game-engine run log"):
            self.atlas()
        self.rtti,self.census,self.candidates,self.factories,self.manifest=inputs()
        self.manifest["methods"].append(dict(self.manifest["methods"][1]))
        with self.assertRaisesRegex(CensusError,"duplicate manually verified"):
            self.atlas()

    def test_exact_original_sha_pinned_function_region(self):
        text=b"0123client789server"
        start=0x24000100
        good=hashlib.sha256(b"client").hexdigest()
        check_code_region(start,text,"0x24000104",6,good)
        with self.assertRaisesRegex(CensusError,"SHA256 mismatch"):
            check_code_region(start,text,"0x24000104",6,"0"*64)
        with self.assertRaisesRegex(CensusError,"outside original .text"):
            check_code_region(start,text,"0x24000200",6,good)
        with self.assertRaisesRegex(CensusError,"code length"):
            check_code_region(start,text,"0x24000104",0,good)

    def test_network_atlas_preserves_constructor_and_string_only_evidence(self):
        recv={
            "sha256":{side+".dll":HASHES[side] for side in ("client","server")},
            "client":{
                "tables":[{"table":"DT_Weapon","registration_va":"0x24100200",
                           "property_count":2}],
                "named_props":[{
                    "table":"DT_Weapon","table_registration_va":"0x24100200",
                    "slot":0,"name":"m_iClip1","client_object_offset":0x7E0,
                    "property_constructor_va":"0x24101000"}],
                "unresolved_slots":[{
                    "table":"DT_Weapon","table_registration_va":"0x24100200",
                    "slot":1}],
            },
            "server":{"name_strings":["DT_Weapon","DT_UnregisteredName"]},
        }
        send={
            "sha256":HASHES,
            "server":{
                "tables":[{"table":"DT_Weapon","registration_va":"0x22100200",
                           "property_count":1}],
                "props":[{
                    "table":"DT_Weapon","table_registration_va":"0x22100200",
                    "slot":0,"name":"m_iClip1","server_object_offset":0x4EC,
                    "property_constructor_va":"0x22103000"}],
                "unresolved_prop_slots":[],
            },
        }
        tables,fields=network_catalog(recv,send)
        self.assertEqual(len(tables),3)
        self.assertEqual(len(fields),3)
        self.assertEqual(sum(t["source_kind"]=="DT_string_only_unverified_registration"
                             for t in tables),1)
        missing=next(f for f in fields if f["evidence_status"]=="unresolved_property_slot")
        self.assertEqual(missing["slot"],1)
        atlas=assemble(self.rtti,self.census,self.candidates,
                       self.factories,self.manifest,self.repo,recv,send)
        self.assertTrue(atlas["summary"]["network_audit_supplied"])
        self.assertEqual(atlas["summary"]["counts"]["client"]["recognized_named_network_properties"],1)
        self.assertEqual(atlas["summary"]["counts"]["server"]["DT_string_only_without_verified_constructor"],1)
        send["server"]["props"][0]["slot"]=3
        with self.assertRaisesRegex(CensusError,"do not cover declared"):
            network_catalog(recv,send)
        send["server"]["props"][0]["slot"]=0
        recv["sha256"]["client.dll"]="0"*64
        with self.assertRaisesRegex(CensusError,"RecvTable audit SHA256"):
            network_catalog(recv,send)

    def test_wrong_proprietary_originals_rejected_without_writing_outputs(self):
        fake=self.repo/"fake.dll"
        fake.write_bytes(b"totally unrelated sample, not a game DLL")
        with self.assertRaisesRegex(CensusError,"incorrect original SHA256"):
            validate_original_files(fake,fake)

if __name__=="__main__":
    unittest.main()
