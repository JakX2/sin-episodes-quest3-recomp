#!/usr/bin/env python3
"""Read-only, pinned 2006 SiN grunt input-registration and FGD audit.

Owner supplies original server.dll and SinEpisodesSDK.zip. No DLL/FGD bytes,
game assets, disassembly or original implementation code are emitted.
Finding strings near handler labels is REGISTRATION EVIDENCE, NOT proof of
an input handler's internal execution or full AI schedule behavior.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile

SERVER_SHA256 = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
FGD_SHA256 = "4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9"
FGD_MEMBER = "SinEpisodesSDK/bin/SinEpisodes.fgd"
INPUTS = ("LookOn", "LookOff", "StartPatrolling", "StopPatrolling", "ThrowGrenades")
FIELDS = (("GrenadeHeight", "m_flGrenadeHeight"), ("NumGrenades", "m_iNumGrenades"))
DEFAULT_WEAPONS = {
    "npc_sintek_grunt": "weapon_assault_rifle",
    "npc_druglab_grunt_pistol": "weapon_npc_pistol",
    "npc_druglab_grunt_scattergun": "weapon_scattergun",
    "npc_druglab_grunt_assault_rifle": "weapon_assault_rifle",
    "npc_druglab_grunt_elite": "weapon_chaingun",
}

class EvidenceError(ValueError):
    pass


def nearby_pair(blob, literal, handler, max_gap=128):
    """Consecutive NUL-terminated strings in the DLL .rdata registry region."""
    literal_bytes = literal.encode("ascii") + b"\0"
    handler_bytes = handler.encode("ascii") + b"\0"
    cursor = 0
    while True:
        found = blob.find(literal_bytes, cursor)
        if found < 0:
            return None
        # Do not match suffixes in other symbol strings.
        if found == 0 or blob[found-1] == 0:
            candidate = blob.find(handler_bytes, found+len(literal_bytes),
                                  found+len(literal_bytes)+max_gap)
            if candidate >= 0 and (candidate == 0 or blob[candidate-1] == 0):
                return (found,candidate)
        cursor = found+len(literal_bytes)


def summarize(server, fgd, server_sha=SERVER_SHA256, fgd_sha=FGD_SHA256):
    if hashlib.sha256(server).hexdigest() != server_sha:
        raise EvidenceError("owner server.dll SHA-256 mismatch")
    if hashlib.sha256(fgd).hexdigest() != fgd_sha:
        raise EvidenceError("owner SinEpisodes.fgd SHA-256 mismatch")
    text = fgd.decode("latin1")
    m = re.search(r'@BaseClass\s+base\(BaseVehiclePassengerNPC,\s*RappelNPC\)\s*=\s*BaseGrunt\s*\[([\s\S]*?)\n\]\s*\n', text)
    if not m:
        raise EvidenceError("BaseGrunt declaration not found")
    block = m.group(1)
    for name in INPUTS:
        if not re.search(r'\binput\s+'+name+r'\s*\(\s*(bool|void)\s*\)', block):
            raise EvidenceError("missing FGD input declaration: "+name)
        if nearby_pair(server, name, "Input"+name) is None:
            raise EvidenceError("missing original DLL input registry pair: "+name)
    for field,member in FIELDS:
        if field not in block or nearby_pair(server, field,member) is None:
            raise EvidenceError("missing original grunt datamap field: "+field)
    for flag in ("65536", "16777216"):
        if not re.search(r'(?m)^\s*'+flag+r'\s*:', block):
            raise EvidenceError("missing grunt FGD spawn flag: "+flag)
    if not re.search(r'NumGrenades\(choices\)[^\n]*:\s*"Number of Grenades"\s*:\s*"5"', block):
        raise EvidenceError("NumGrenades default changed")
    if '"999999" : "Unlimited"' not in block:
        raise EvidenceError("Grenade unlimited choice missing")
    expected = []
    for cls,weapon in DEFAULT_WEAPONS.items():
        exp = r'@NPCClass\s+base\(BaseGrunt\)[^\n]*=\s*'+cls+r'\s*:[\s\S]*?additionalequipment\(choices\)[^\n]*:\s*"Weapons"\s*:\s*"'+weapon+r'"'
        if not re.search(exp, text):
            raise EvidenceError("original FGD grunt equipment default mismatch: "+cls)
        if (cls+"\0").encode() not in server:
            raise EvidenceError("original DLL missing grunt class registration: "+cls)
        expected.append(cls)
    return {
        "original_server_dll_sha256_verified": True,
        "original_fgd_sha256_verified": True,
        "declared_inputs_correlated_with_dll_registry": list(INPUTS),
        "grenade_fields_correlated_with_dll_registry": [x[0] for x in FIELDS],
        "supported_grunt_classnames": expected,
        "default_grenade_count": 5,
        "unlimited_grenade_choice": 999999,
        "grunt_start_lookoff_flag": 65536,
        "grunt_no_secondary_flag": 16777216,
        "actual_ai_handlers_disassembled": False,
        "original_ai_scheduler_executed": False,
        "real_sin_npc_spawned": False,
        "quest_hmd_tested": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server_dll",type=Path)
    parser.add_argument("sdk_zip",type=Path)
    args = parser.parse_args()
    try:
        with zipfile.ZipFile(args.sdk_zip) as z:
            result=summarize(args.server_dll.read_bytes(),z.read(FGD_MEMBER))
    except (OSError,ValueError,zipfile.BadZipFile,KeyError) as error:
        parser.exit(1, "grunt provenance audit failed: "+str(error)+"\n")
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__ == "__main__":
    main()
