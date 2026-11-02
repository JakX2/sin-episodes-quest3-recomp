#!/usr/bin/env python3
"""Build an original-binary-grounded SiN client/server class recovery queue.

Uses pre-generated SHA-pinned original RTTI metadata, NOT copied DLL code.
Lists all relevant class virtual targets for manually verified CFG and source
reconstruction. Counts are per-group and inherited targets overlap.
"""
from __future__ import annotations
import argparse
import csv
import json
from pathlib import Path
from audit_sin_entire_dll_code_census import HASHES, CensusError

GROUPS={
 "client":{
  "player_and_prediction":["C_HL2_Player","C_HL2MP_Player","C_BasePlayer","CPrediction","CGameMovement"],
  "weapon_and_grenade_client":["C_WeaponMagnum","C_WeaponAssaultRifle","C_WeaponScatterGun","C_BaseGrenade"],
  "enemy_presentation":["C_NPC_GruntBase","C_AI_BaseNPC","C_BaseAnimating"],
  "dynamic_difficulty":["C_DynamicDifficulty"],
  "poison_grenade_hud":["CHudPoisonTimer","CHudGrenades","CHudHealth","CHudAmmo"],
  "vehicle_and_phone":["C_PropSinVehicle","C_PropPhone"],
 },
 "server":{
  "grenade_projectile":["CGrenadeAR","CBaseGrenade","CGrenadeSpitPoison","CGrenadeHomer"],
  "sintek_ai":["CNPC_GruntBase","CNPC_SintekGrunt","CNPC_SintekJetpack","CNPC_Quadralex"],
  "combat_weapons":["CWeaponAssaultRifle","CWeaponMagnum","CWeaponScatterGun","CBaseHLCombatWeapon"],
  "dynamic_difficulty":["CDynamicDifficulty","CDynamicDifficultyAction","CTriggerDynamicDifficulty"],
  "poison_and_antigen":["CPoisonGas","CAntigenDispenser","CAntigenCanister"],
  "phone":["CPropPhone"],
  "vehicle":["CPropSinVehicle","CFourWheelVehiclePhysics"],
 },
}

def generate(rtti:dict)->list[dict]:
    results=[]
    for side,groups in GROUPS.items():
        original=rtti[side]
        if original.get("sha256")!=HASHES[side]:
            raise CensusError("RTTI fingerprint mismatch for "+side)
        for system,names in groups.items():
            chosen=[x for x in original["vtables"] if x["class"] in names]
            present=sorted({x["class"] for x in chosen})
            targets=sorted({
                value for x in chosen
                for value in x.get("functions",x.get("function_va",[]))
            })
            results.append({
                "dll":side+".dll","system":system,"requested_classes":names,
                "rtti_classes_present":present,
                "unknown_or_missing_requested_classes":sorted(set(names)-set(present)),
                "matching_vtables":len(chosen),
                "distinct_virtual_target_addresses":len(targets),
                "virtual_target_examples":targets[:25],
                "status":"original_virtual_targets_found_not_equivalent_gameplay",
            })
    return results

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--rtti",type=Path,required=True)
    p.add_argument("--output-json",type=Path,required=True)
    p.add_argument("--output-csv",type=Path,required=True)
    a=p.parse_args()
    try:
        if a.output_json.resolve()==a.output_csv.resolve():
            raise CensusError("JSON and CSV output paths must differ")
        queue=generate(json.loads(a.rtti.read_text(encoding="utf-8")))
        a.output_json.write_text(json.dumps(queue,indent=2)+"\n",encoding="utf-8")
        with a.output_csv.open("w",newline="",encoding="utf-8") as handle:
            writer=csv.DictWriter(handle,fieldnames=[
                "dll","system","rtti_classes_present",
                "unknown_or_missing_requested_classes",
                "matching_vtables","distinct_virtual_target_addresses","status"])
            writer.writeheader()
            for row in queue:
                writer.writerow({key:" | ".join(row[key]) if isinstance(row.get(key),list)
                                 else row.get(key) for key in writer.fieldnames})
        print(json.dumps({x["dll"]+":"+x["system"]:x["distinct_virtual_target_addresses"]
                          for x in queue},indent=2))
    except (OSError,ValueError,KeyError,TypeError) as error:
        p.exit(1,"SiN original class recovery queue failed: "+str(error)+"\n")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
