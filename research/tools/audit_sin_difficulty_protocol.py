#!/usr/bin/env python3
"""Verify original SiN 2006 dynamic-difficulty user-message registrations and payload.

This is read-only binary EVIDENCE, not a new packet implementation:
- paired original server/client name and declaration-site instructions;
- original client user-message hook callbacks;
- the original DDDecisionMade sender/receiver typed read/write sequence;
- ten two-float pairs in DDSuccessModifiers.

Only exact SHA-pinned owner-held Windows PE32 files are accepted by CLI.
Original bytes are not committed; JSON output contains x86 addresses only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, CLIENT_SHA256, SERVER_SHA256, VerificationError

# Original addresses are for the same exact 2006 game DLLs, never a different
# update, HL2's DLLs, Source SDK 2013, or the Android ARM64 modules.
MESSAGES = {
    # Name: (client string VA, client registration push VA,
    #        server string VA, server registration push VA, declared size).
    "GamePlayVarValue": (0x242ADFF4,0x2420F882,0x2245CAAC,0x222CFE32,6),
    "Progression": (0x242ADFE8,0x2420F894,0x2245C8F4,0x222CFE44,4),
    "DDInfo": (0x242ADFE0,0x2420F8A6,0x2245CB10,0x222CFE56,-1),
    "DDAdvisorScores": (0x242ADFBC,0x2420F8B8,0x2245CB18,0x222CFE68,-1),
    "DDAdvisorChannels": (0x242ADFCC,0x2420F8CA,0x2245CB28,0x222CFE7A,-1),
    "DDAdvisorsUpdate": (0x242B1B00,0x2420F8DC,0x2245EBAC,0x222CFE8C,-1),
    "DDDecisionMade": (0x242ADFAC,0x2420F8EE,0x2245CA7C,0x222CFE9E,-1),
    "DDDecisionsPerf": (0x242ADF9C,0x2420F900,0x2245CB3C,0x222CFEB0,-1),
    "DDSuccessModifiers": (0x242ADF88,0x2420F912,0x2245CB4C,0x222CFEC2,-1),
}
# Callback trampoline registrations in C_DynamicDifficulty's client init
# routine at 0x241FAC90. DDAdvisorsUpdate is registered as a user message in
# the engine but has no entry in this particular hook registration block.
HOOKS = {
    "GamePlayVarValue":(0x241FAC9B,0x241F9960),
    "Progression":(0x241FACB0,0x241F9970),
    "DDInfo":(0x241FACC5,0x241F9F80),
    "DDAdvisorChannels":(0x241FACDA,0x241FAC40),
    "DDAdvisorScores":(0x241FACEF,0x241FAC50),
    "DDDecisionMade":(0x241FAD04,0x241FAC60),
    "DDDecisionsPerf":(0x241FAD19,0x241F9F90),
    "DDSuccessModifiers":(0x241FAD2E,0x241F9990),
}
OTHER_CLIENT_HOOKS = {
    "ArenaMode":(0x242ADF7C,0x241FAD43,0x241F9FA0),
    "NewGameInfo":(0x242ADF70,0x241FAD58,0x241F9FB0),
    "Cheating":(0x242ADF64,0x241FAD6D,0x241F9FC0),
}
CLIENT_DELEGATIONS = {
    # Hook callback +10 is CALL rel32 to the actual reader.
    "DDInfo":(0x241F9F8A,0x241F99A0),
    "DDAdvisorChannels":(0x241FAC4A,0x241FA850),
    "DDAdvisorScores":(0x241FAC5A,0x241FAA60),
    "DDDecisionMade":(0x241FAC6A,0x241FAB10),
    "DDDecisionsPerf":(0x241F9F9A,0x241F9C20),
    "DDSuccessModifiers":(0x241F999A,0x241F9770),
}
# Verified DDDecisionMade packet:
# 1 initial float (special sentinel marker on some paths),
# then 3 ints and 4 floats for an actual decision record.
DECISION_SERVER_WRITES = [
    (0x222B9787,"f32",0x221E8830),
    (0x222B9790,"i32",0x221E87A0),
    (0x222B9799,"i32",0x221E87A0),
    (0x222B97A2,"i32",0x221E87A0),
    (0x222B97AB,"f32",0x221E8830),
    (0x222B97B4,"f32",0x221E8830),
    (0x222B97BD,"f32",0x221E8830),
    (0x222B97C6,"f32",0x221E8830),
]
DECISION_CLIENT_READS = [
    (0x241FAB1D,"f32",0x240A5630),
    (0x241FAB7C,"i32",0x240A5600),
    (0x241FAB85,"i32",0x240A5600),
    (0x241FAB8E,"i32",0x240A5600),
    (0x241FAB99,"f32",0x240A5630),
    (0x241FABA4,"f32",0x240A5630),
    (0x241FABAF,"f32",0x240A5630),
    (0x241FABBA,"f32",0x240A5630),
]
SUCCESS_CALLS = {
    "server":[(0x222BAA15,0x221E8830),(0x222BAA29,0x221E8830)],
    "client":[(0x241F9782,0x240A5630),(0x241F9798,0x240A5630)],
}
SUCCESS_LOOP_TEST = {"client":0x241F97AF,"server":0x222BAA34}
REGISTRY_FUNCTION = {"client":0x241D0280,"server":0x220229A0}

# Original CDynamicDifficultyAction::debug-print routine at server
# 0x2215BDD0 names the original class field offsets itself:
# "--Time started", "--Value", "--Decision", "--Advisor".
ACTION_FIELDS = {
    "time_started":(0x2215BE07,bytes.fromhex("d986bc020000"),0x2215BE10,
                    0x22411368,b"--Time started: %.4f\n\x00","0x2BC","f32"),
    "value":(0x2215BE17,bytes.fromhex("d986c8020000"),0x2215BE23,
             0x224115A0,b"--Value: %.4f\n\x00","0x2C8","f32"),
    "decision":(0x2215BE2A,bytes.fromhex("8b86c4020000"),0x2215BE31,
                0x2241158C,b"--Decision: %.i\n\x00","0x2C4","i32"),
    "advisor":(0x2215BE38,bytes.fromhex("8b8ec0020000"),0x2215BE3F,
               0x2241157C,b"--Advisor: %.i\n\x00","0x2C0","i32"),
}
HOOK_REGISTER = 0x241CFF70

def push(pe,site:int,imm:int)->None:
    if pe.at(site,5)!=b"\x68"+struct.pack("<I",imm):
        raise VerificationError(f"push-imm32 evidence mismatch at 0x{site:08X}")

def relcall(pe,site:int,target:int)->None:
    raw=pe.at(site,5)
    if raw[0]!=0xE8 or site+5+struct.unpack_from("<i",raw,1)[0]!=target:
        raise VerificationError(f"x86 CALL target mismatch at 0x{site:08X}")

def register(pe,site:int,name_va:int,name:str,declared_size:int,side:str)->None:
    signed8=declared_size & 0xff
    if pe.at(site-2,2)!=bytes([0x6A,signed8]):
        raise VerificationError(f"message declared length mismatch at 0x{site:08X}")
    push(pe,site,name_va)
    relcall(pe,site+5,REGISTRY_FUNCTION[side])
    if pe.at(name_va,len(name)+1)!=name.encode("ascii")+b"\0":
        raise VerificationError(f"registered message name mismatch at 0x{name_va:08X}")

def verify_evidence(client_pe,server_pe)->dict:
    rows=[]
    for name,(cv,cs,sv,ss,size) in MESSAGES.items():
        register(client_pe,cs,cv,name,size,"client")
        register(server_pe,ss,sv,name,size,"server")
        hook=HOOKS.get(name)
        if hook:
            site,callback=hook
            push(client_pe,site-5,callback)
            push(client_pe,site,cv)
            relcall(client_pe,site+5,HOOK_REGISTER)
        rows.append({
            "name":name,
            "original_client_registration_va":f"0x{cs:08X}",
            "original_server_registration_va":f"0x{ss:08X}",
            "declared_message_length":size,
            "client_callback_va":f"0x{hook[1]:08X}" if hook else None,
            "hook_scope":"C_DynamicDifficulty constructor registration"
                         if hook else "no hook in inspected C_DynamicDifficulty registration block",
        })
    for name,(name_va,site,callback) in OTHER_CLIENT_HOOKS.items():
        push(client_pe,site-5,callback)
        push(client_pe,site,name_va)
        relcall(client_pe,site+5,HOOK_REGISTER)
        if client_pe.at(name_va,len(name)+1)!=name.encode()+b"\0":
            raise VerificationError("other client hook name mismatch")
    for site,target in CLIENT_DELEGATIONS.values():
        relcall(client_pe,site,target)
    for site,_,target in DECISION_SERVER_WRITES:
        relcall(server_pe,site,target)
    for site,_,target in DECISION_CLIENT_READS:
        relcall(client_pe,site,target)
    for site,target in SUCCESS_CALLS["server"]:
        relcall(server_pe,site,target)
    for site,target in SUCCESS_CALLS["client"]:
        relcall(client_pe,site,target)
    # Both real loops execute 10 iterations; two f32 sends/reads each.
    for side,pe in (("client",client_pe),("server",server_pe)):
        if pe.at(SUCCESS_LOOP_TEST[side],3)!=b"\x83\xfe\x0a":
            raise VerificationError(f"ten-pair {side} loop limit mismatch")
    # Original DDDecisionMade client handler compares first float to 0 and -1.
    for site,va in ((0x241FAB26,0x242DE2F8),(0x241FAB37,0x24276F84)):
        if client_pe.at(site,6)!=b"\xd8\x1d"+struct.pack("<I",va):
            raise VerificationError("decision sentinel compare instruction mismatch")
    if client_pe.at(0x242DE2F8,4)!=struct.pack("<f",0.0):
        raise VerificationError("decision zero sentinel changed")
    if client_pe.at(0x24276F84,4)!=struct.pack("<f",-1.0):
        raise VerificationError("decision negative sentinel changed")
    # Two server-side actual message beginnings (send writer functions).
    for site,name in ((0x222B9775,"DDDecisionMade"),
                      (0x222BA9F6,"DDSuccessModifiers"),
                      (0x222BA5FD,"DDInfo"),
                      (0x222BA6EE,"DDAdvisorScores"),
                      (0x222BA7CD,"DDAdvisorChannels")):
        push(server_pe,site,MESSAGES[name][2])
    action_fields={}
    for name,(read_va,read_bytes,fmt_push_va,fmt_va,fmt_text,offset,dtype) in ACTION_FIELDS.items():
        if server_pe.at(read_va,len(read_bytes))!=read_bytes:
            raise VerificationError("original difficulty action member read mismatch at "+hex(read_va))
        push(server_pe,fmt_push_va,fmt_va)
        if server_pe.at(fmt_va,len(fmt_text))!=fmt_text:
            raise VerificationError("original difficulty action debug label mismatch at "+hex(fmt_va))
        action_fields[name]={
            "original_server_field_offset":offset,
            "type":dtype,
            "original_read_va":f"0x{read_va:08X}",
            "label":fmt_text.decode("ascii").rstrip("\x00\n"),
        }
    return {
        "paired_messages":rows,
        "server_dynamic_difficulty_action_fields":action_fields,
        "paired_message_count":len(rows),
        "client_hooks_in_dynamic_difficulty_block":len(HOOKS),
        "extra_client_hooks_nearby":list(OTHER_CLIENT_HOOKS),
        "decision_record":{
            "message":"DDDecisionMade",
            "server_writer_va":"0x222B9770",
            "client_reader_va":"0x241FAB10",
            "ordinary_record_sequence":[kind for _,kind,_ in DECISION_CLIENT_READS],
            "server_writer_sequence":[kind for _,kind,_ in DECISION_SERVER_WRITES],
            "special_first_float_sentinels":[0.0,-1.0],
            "warning":"Sentinel branches change payload handling; ordinary record is not universal."
        },
        "success_modifiers":{
            "message":"DDSuccessModifiers",
            "pairs_per_message":10,
            "scalar_type":"f32",
            "values_per_pair":2,
            "server_message_writer_va":"0x222BA9C0",
            "client_handler_va":"0x241F9770",
        },
        "known_limitations":[
            "Message length -1 marks variable-length registration; not inferred payload size.",
            "DDAdvisorsUpdate is registered by engine but not hooked in the inspected client DD constructor block.",
            "The meaning of advisor indices and difficulty-scoring formulas is not reconstructed.",
            "These are original 2006 Windows x86 message instructions, not a compatible ARM64 protocol implementation.",
            "Network message naming matches do not prove data semantics for all nine messages.",
        ],
    }

def audit(client:bytes,server:bytes,*,check_hash:bool=True)->dict:
    digests={
        "client.dll":hashlib.sha256(client).hexdigest(),
        "server.dll":hashlib.sha256(server).hexdigest(),
    }
    if check_hash and (digests["client.dll"]!=CLIENT_SHA256 or
                       digests["server.dll"]!=SERVER_SHA256):
        raise VerificationError("original SiN game DLL SHA-256 mismatch")
    result=verify_evidence(PEImage(client),PEImage(server))
    result["original_binary_sha256"]=digests
    return result

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    try:
        data=audit(args.client.read_bytes(),args.server.read_bytes())
        args.output.write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,struct.error) as e:
        p.exit(1,"SiN difficulty message audit FAILED: "+str(e)+"\n")
    print(json.dumps({
        "paired_messages":data["paired_message_count"],
        "client_hooks":data["client_hooks_in_dynamic_difficulty_block"],
        "ordinary_decision_fields":len(data["decision_record"]["ordinary_record_sequence"]),
        "success_modifier_float_pairs":data["success_modifiers"]["pairs_per_message"],
    },indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
