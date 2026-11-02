#!/usr/bin/env python3
"""Audit original SiN Assault Rifle -> CGrenadeAR lifecycle and ammo paths.

Verifies byte-level machine-code evidence in the owner's ORIGINAL 2006
server.dll. C++ symbols/RTTI, class factory, vtable slot alignment and select
x86 instructions are all checked before reporting gameplay claims.
Requires prior audit_sin_rtti_vtables.py and audit_full_sin_game_dlls.py.
This does not recover or distribute the original game code.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_full_sin_game_dlls import scan_registrations
from audit_sin_rtti_vtables import scan
from verify_gameplay_entity_registry import PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256

AR = "CGrenadeAR"
BASE = "CBaseGrenade"
ROOTED_FACTORY = {
    "classname": "grenade_ar",
    "registration_va": "0x223C4FC0",
    "factory_object_va": "0x2252D500",
}

# Exact original 2006 compiled code witnesses. Instruction bytes rather than
# strings alone are essential for the ammo and owner-type reconstruction.
WITNESSES = {
    0x22313C77: bytes.fromhex("ff9020010000"),     # call [eax+0x120] (IsPlayer probe)
    0x22313C85: b"\x68" + struct.pack("<I", 0x2252D5B0),  # target CNPC_GruntBase
    0x22313C8A: b"\x68" + struct.pack("<I", 0x224DC1FC),  # from CBaseCombatCharacter
    0x22313C9E: bytes.fromhex("83c41485ff8bd8"),  # dynamic_cast result -> ebx
    0x22313CB1: bytes.fromhex("8b86e8040000"),    # weapon secondary ammo index
    0x22313CC6: bytes.fromhex("85c07e10"),        # <=0 follows unavailable-fire path
    0x22313DE7: bytes.fromhex("85db7408"),        # if grunt cast fails use 2000
    0x22313DDE: b"\xd9\x05" + struct.pack("<I", 0x224771F0),
    0x22313DED: b"\xd9\x05" + struct.pack("<I", 0x223E97F0),
    0x22313F2E: bytes.fromhex("8b8ee8040000516a01"),  # ammo index; push 1
    0x222AA693: bytes.fromhex("8b84be4c0600002bc3"),  # ammo array - requested delta
    0x222AA6BB: bytes.fromhex("899cbe4c060000"),      # commit ammo array update
    0x22314B9A: b"\x68" + struct.pack("<I", 0x22314360),  # periodic think
    0x22314BAB: b"\xc7\x86\x7c\x00\x00\x00" + struct.pack("<I", 0x223142F0),
    0x22314C2F: b"\x89\x8e\x98\x07\x00\x00",  # follow-up +0x798 write
    0x22314CED: b"\xc7\x86\xc4\x07\x00\x00" + struct.pack("<f", 100.0),
}
CALL_EDGES = {
    0x22313C99: 0x223A5BAB,  # MSVC __RTDynamicCast
    0x22313CC1: 0x222AA770,  # owner ammo-count getter
    0x22313E2A: 0x22146C40,  # live projectile creation
    0x22313F39: 0x222AA670,  # owner ammo-count decrement
    0x22314BB2: 0x2214D6F0,  # think registration
    0x22314BCA: 0x2214D7B0,  # next think scheduling
    0x22146C48: 0x22266B00,  # CreateEntityByName
}

MODEL_GLOBAL = 0x2252D1F0
MODEL_STRING = 0x22477208
MODEL_NAME = b"models/weapons/grenades/ar_grenade.mdl\x00"
GRENADE_TYPE_DESCRIPTOR = 0x2252D570
RTTI_DECORATED_NAME = b".?AVCGrenadeAR@@\x00"
CAST_TARGET = (0x2252D5B0, b".?AVCNPC_GruntBase@@\x00")
CAST_SOURCE = (0x224DC1FC, b".?AVCBaseCombatCharacter@@\x00")
NUMERIC_FIELDS = {
    0x224771F0: 2000.0,
    0x223E97F0: 1000.0,
    0x224771F8: 0.1,   # original single-precision value
}
OBJECT_FIELD_INIT = {
    # Actual compiled initialization instructions in CGrenadeAR's spawn setup.
    0x22314BFB: b"\xc7\x86" + struct.pack("<I",0x7AC) + struct.pack("<f",100.0),
    0x22314C05: b"\xb9" + struct.pack("<f",250.0),
    0x22314C0A: bytes.fromhex("398e98070000"),
    0x22314C2F: bytes.fromhex("898e98070000"),
    0x22314CED: b"\xc7\x86" + struct.pack("<I",0x7C4) + struct.pack("<f",100.0),
}
# Decorated exports are evidence for the INDEX of the Detonate virtual in
# other grenade subclasses. Then CGrenadeAR's same index gives its target.
KNOWN_GRENADE_DETONATE_EXPORTS = {
    "CGrenadeHomer": 0x2230F4E0,
    "CGrenadePathfollower": 0x22310EA0,
    "CGrenadeSpitMutant": 0x223120B0,
    "CGrenadeSpitPoison": 0x22312B70,
}


def decode_rel_call(pe: PEImage, site: int) -> int:
    instruction = pe.at(site, 5)
    if instruction[0] != 0xE8:
        raise VerificationError(f"not an x86 CALL rel32 at 0x{site:08X}")
    return site + 5 + struct.unpack_from("<i", instruction, 1)[0]


def check_bytes(pe: PEImage, addr: int, expected: bytes) -> None:
    if pe.at(addr, len(expected)) != expected:
        raise VerificationError(f"original x86 witness mismatch at 0x{addr:08X}")


def primary_vtable(rtti: dict, classname: str) -> dict:
    found = [v for v in rtti["vtables"]
             if v["class"] == classname and v["primary"]]
    if len(found) != 1:
        raise VerificationError(f"expected one primary vtable for {classname}")
    return found[0]


def read_named_exports(pe: PEImage) -> dict[str, int]:
    """PE32 Export Address Table and names, verified without third-party parser."""
    b = pe.data
    nt = struct.unpack_from("<I", b, 0x3C)[0]
    opt = nt + 24
    if struct.unpack_from("<H", b, opt)[0] != 0x10B:
        raise VerificationError("not PE32")
    export_rva, export_size = struct.unpack_from("<II", b, opt + 96)
    if not export_rva or not export_size:
        raise VerificationError("missing exports")
    base, nf, nn, functions_rva, names_rva, ordinals_rva = (
        struct.unpack("<IIIIII", pe.at(pe.base + export_rva + 16, 24))
    )
    if not (0 < nn <= nf <= 128):
        raise VerificationError("unexpected original export table shape")
    output = {}
    for i in range(nn):
        name_rva = struct.unpack("<I", pe.at(pe.base + names_rva + 4*i, 4))[0]
        ordinal = struct.unpack("<H", pe.at(pe.base + ordinals_rva + 2*i, 2))[0]
        if ordinal >= nf:
            raise VerificationError("bad PE export ordinal")
        function_rva = struct.unpack("<I",pe.at(pe.base + functions_rva+4*ordinal,4))[0]
        name = pe.at(pe.base+name_rva,100).split(b"\x00",1)[0].decode("ascii")
        output[name] = pe.base + function_rva
    return output


def assert_original_evidence(binary: bytes, *, check_hash: bool=True) -> dict:
    digest = hashlib.sha256(binary).hexdigest()
    if check_hash and digest != SERVER_SHA256:
        raise VerificationError("original SiN 2006 server.dll SHA-256 mismatch")
    pe = PEImage(binary)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original SiN image base")
    for va, expected in WITNESSES.items():
        check_bytes(pe, va, expected)
    for site, target in CALL_EDGES.items():
        if decode_rel_call(pe, site) != target:
            raise VerificationError(f"original CALL edge mismatch at 0x{site:08X}")
    for descriptor, expected in (CAST_SOURCE, CAST_TARGET,
                                 (GRENADE_TYPE_DESCRIPTOR, RTTI_DECORATED_NAME)):
        check_bytes(pe, descriptor+8, expected)
    check_bytes(pe, MODEL_GLOBAL, struct.pack("<I", MODEL_STRING))
    check_bytes(pe, MODEL_STRING, MODEL_NAME)
    for addr, numeric in NUMERIC_FIELDS.items():
        raw = pe.at(addr, 4)
        if struct.unpack("<f", raw)[0] != struct.unpack("<f", struct.pack("<f", numeric))[0]:
            raise VerificationError(f"unexpected original float at 0x{addr:08X}")
    for instruction_va, instruction in OBJECT_FIELD_INIT.items():
        check_bytes(pe, instruction_va, instruction)
    # IsPlayer-like virtual probe: base combat character returns false,
    # original CBasePlayer override returns true.
    check_bytes(pe, 0x220017E0, b"\x30\xc0\xc3")
    check_bytes(pe, 0x22253C30, b"\xb0\x01\xc3")
    return {"sha256":digest, "code_witnesses":len(WITNESSES),
            "verified_direct_calls":len(CALL_EDGES)}


def analyze(binary: bytes, client: bytes | None = None) -> dict:
    proof = assert_original_evidence(binary)
    if client is not None and hashlib.sha256(client).hexdigest() != CLIENT_SHA256:
        raise VerificationError("original 2006 SiN client.dll SHA-256 mismatch")
    pe = PEImage(binary)
    rtti = scan(binary, "server")
    registrations = scan_registrations(binary)
    factory = next((x for x in registrations if x["classname"] == "grenade_ar"), None)
    if not factory or any(factory[field].lower() != value.lower()
                          for field,value in ROOTED_FACTORY.items()):
        raise VerificationError("CGrenadeAR original factory registration mismatch")
    cls = primary_vtable(rtti, AR)
    parent = primary_vtable(rtti, BASE)
    if len(cls["functions"]) != 260 or len(parent["functions"]) != 260:
        raise VerificationError("unexpected CGrenadeAR base/child vtable length")
    hierarchies = rtti["hierarchies"]
    bases = [entry["class"] for entry in hierarchies.get(AR, [])]
    if not bases or bases[:2] != [AR, BASE]:
        raise VerificationError("original CGrenadeAR inheritance mismatch")
    differences = [{"slot":i,"grenade_ar_va":a,"base_grenade_va":b}
                   for i,(a,b) in enumerate(zip(cls["functions"],parent["functions"]))
                   if a != b]
    if [x["slot"] for x in differences] != [0,13,25,26,58,250]:
        raise VerificationError("unexpected original grenade AR override locations")

    # Identify Detonate's SAME INDEX from FOUR decorated Windows DLL exports;
    # only then map that virtual index into CGrenadeAR.
    exports = read_named_exports(pe)
    for classname, address in KNOWN_GRENADE_DETONATE_EXPORTS.items():
        name = f"?Detonate@{classname}@@UAEXXZ"
        if exports.get(name) != address:
            raise VerificationError("missing original decorated Detonate export "+name)
        other = primary_vtable(rtti, classname)
        if len(other["functions"]) <= 250 or int(other["functions"][250],16) != address:
            raise VerificationError("named Detonate export and vtable slot mismatch")
    if int(cls["functions"][250],16) != 0x22314570:
        raise VerificationError("CGrenadeAR Detonate slot mismatch")
    player = primary_vtable(rtti, "CBasePlayer")
    combat = primary_vtable(rtti, "CBaseCombatCharacter")
    if (int(player["functions"][72],16) != 0x22253C30 or
        int(combat["functions"][72],16) != 0x220017E0):
        raise VerificationError("player/non-player virtual discriminator mismatch")

    return {
        "source":"original 2006 Windows x86 SiN server.dll",
        **proof,
        "projectile":{
            "entity_classname":"grenade_ar",
            "C++_class":"CGrenadeAR",
            "direct_base":"CBaseGrenade",
            "original_entity_registration":factory,
            "original_model":MODEL_NAME.decode("ascii").rstrip("\0"),
            "model_global_va":f"0x{MODEL_GLOBAL:08X}",
            "vtable_slots":len(cls["functions"]),
            "changed_virtual_slots":differences,
            "detonate_virtual_slot":250,
            "detonate_original_x86_va":cls["functions"][250],
            "independent_named_detonate_exports":
                {name:f"0x{addr:08X}" for name,addr in KNOWN_GRENADE_DETONATE_EXPORTS.items()},
            "projectile_initialization_routine_va":"0x22314B10",
            "touch_handler_function_va":"0x223142F0",
            "think_handler_function_va":"0x22314360",
            "next_think_delta_single_precision":struct.unpack("<f",pe.at(0x224771F8,4))[0],
            "original_object_field_values":{
                "0x7AC":100.0, "0x798":250.0, "0x7C4":100.0,
            },
        },
        "firing":{
            "weapon_routine_va":"0x22313C60",
            "owner_type_probe":{
                "virtual_slot":72,
                "player_true_original_x86_va":"0x22253C30",
                "base_combat_character_false_original_x86_va":"0x220017E0",
            },
            "owner_rtti_cast":{
                "MSVC_helper":"0x223A5BAB",
                "source_type":"CBaseCombatCharacter",
                "target_type":"CNPC_GruntBase",
                "cast_result_held_in":"ebx",
            },
            "shot_vector_multiplier":{
                "not_grunt_base":2000.0,
                "is_grunt_base":1000.0,
                "selection_instruction_va":"0x22313DE7",
                "unknowns":"physical units and precise direction basis not verified",
            },
            "player_secondary_ammo":{
                "ammo_type_field_offset":"0x4E8",
                "ammo_count_getter_va":"0x222AA770",
                "ammo_count_adjuster_va":"0x222AA670",
                "decrement_quantity":1,
                "ammo_storage_base_offset":"0x64C",
                "conditional":"player path; valid finite ammo type; not on grunt-only path",
            },
            "grenade_entity_factory_call_va":"0x22313E2A",
            "timed_state_offset_candidates":["0x49C","0x4A0"],
        },
        "network_observation":{
            "client_contains_literal_DT_GrenadeAR":
                None if client is None else b"DT_GrenadeAR\0" in client,
            "server_contains_literal_DT_GrenadeAR":b"DT_GrenadeAR\0" in binary,
            "warning":"No table name found is NOT proof this entity is unnetworked. It may inherit/base-share a sendtable.",
        },
        "limitations":[
            "Subclasses can contain nonvirtual behavior not reflected in six changed slots.",
            "Damage/radius meanings for +0x798/+0x7AC/+0x7C4 not proven by these writes alone.",
            "Detonate label inferred with four independent named grenade exports at same virtual slot.",
            "Original x86 member offsets and vtable slots cannot be reused on ARM64 without compatible source-level ABI.",
            "One code path is not a complete game/weapon implementation.",
        ],
    }


def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path,
                   help="Optional original client.dll, for table-name comparison")
    p.add_argument("--output",type=Path,required=True)
    args=p.parse_args()
    try:
        result=analyze(args.server.read_bytes(),
                       args.client.read_bytes() if args.client else None)
        args.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,"SIN GRENADE LIFECYCLE AUDIT FAILED: "+str(exc)+"\n")
    print(json.dumps({
        "class":result["projectile"]["C++_class"],
        "changed_virtual_slots":len(result["projectile"]["changed_virtual_slots"]),
        "detonate_va":result["projectile"]["detonate_original_x86_va"],
        "player_ammo_delta":result["firing"]["player_secondary_ammo"]["decrement_quantity"],
    },indent=2))
    return 0


if __name__=="__main__":
    raise SystemExit(main())
