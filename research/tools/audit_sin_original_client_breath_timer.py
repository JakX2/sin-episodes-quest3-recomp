#!/usr/bin/env python3
"""Phase155: exact original SiN 2006 CHudBreathTimer DLL evidence.

Chained to the prior original client/server SHA-pinned poison-HUD auditor.
Verifies actual breath HUD RTTI, both original vtables, four full client
method bodies, 2006 RecvProp/SendProp m_breathTimer sequences, sound tokens
and machine-code call/condition witnesses. Metadata only; no PE bytes output.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_poison_timer_fades import audit as prior_original_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

# (owner, original VA, exact bytes, original SHA256)
REGIONS = {
    "client_reset": ("client",0x24230A50,11,
        "8cb95062e758c4f0a32de92459dee9830edf87cd39c1c601676cdd6a51a75eb5"),
    "client_two_resource_bind": ("client",0x24230A60,51,
        "e80f0d9011daa95ee7bcd3a71cc86856219588ec61cff3d6ea773bf4c2ee8ef7"),
    "client_update_and_cues": ("client",0x24230BB0,224,
        "1a04ae5f33db4ca385e92b0328ad3b280e6dd0994f4c5ec467717b2ce4eb6114"),
    "client_paint": ("client",0x24230C90,233,
        "0b18199261f6fb6a0477a08022d60bb0791cd5379184ec0f9fcc14022b91a43b"),
    "client_recv_m_breathTimer": ("client",0x2421AD35,26,
        "85fa43b87b76f50680eff732bf0b851a9424f707fddcd2ebe22925905c34c1e1"),
    "server_send_m_breathTimer": ("server",0x223A2928,35,
        "8790ee840f413d633501963f9b0a68147048a02cfe21a58dc9c366ac02dd2456"),
}
# Original 2006 RTTI TypeDescriptor and COLs for CHudBreathTimer.
RTTI = {
    0x242C7698:0x242EF768,  # panel subobject vtable COL
    0x242C78C8:0x242EF7BC,  # primary vtable COL
    0x242EF774:0x24320D34, # panel COL+12 type descriptor
    0x242EF7C8:0x24320D34, # primary COL+12 type descriptor
}
VTABLE = {
    0x242C78D4:0x24230A50,  # primary slot2 reset
    0x242C78D8:0x24230A60,  # primary slot3 resources
    0x242C7810:0x24230BB0,  # panel slot93 update
    0x242C7850:0x24230C90,  # panel slot109 paint
}
TYPE_NAME_VA = 0x24320D3C
TYPE_NAME = b".?AVCHudBreathTimer@@\0"
SOUND_NAMES = {
    0x242C75BC:"BreathTimeFadeIn",
    0x242C75AC:"BreathTimeLow",
    0x242C759C:"BreathTimeEmpty",
    0x242C7588:"BreathTimeFadeOut",
}
NETWORK_NAMES = {
    "client":{0x242B5E3C:"m_breathTimer",0x242B5E24:"DT_HL2Local"},
    "server":{0x224AFA40:"m_breathTimer",0x224AFA28:"DT_HL2Local"},
}
OPCODES = {
    "client":{
        0x24230A50:"c781000100000000803f",
        0x24230A5A:"c3",
        0x24230A63:"8d86bc000000",
        0x24230A80:"898604010000",
        0x24230A8B:"898608010000",
        0x24230BB3:"8b3df08c3324",  # original local player ptr
        0x24230BC3:"8b87f0110000",  # player+0x11F0
        0x24230BC9:"d986ec000000",  # previous HUD+0xEC
        0x24230BD7:"dae9",        # FUCOMPP
        0x24230BDB:"f6c444",
        0x24230BDE:"0f8ba8000000",# ordered equality early exit
        0x24230BE8:"d81d08e32d24",# 1.0
        0x24230BF0:"f6c405",
        0x24230BF3:"7a1b",
        0x24230BF5:"80bea600000000",# status +0xA6 ==0
        0x24230C06:"68bc752c24",  # FadeIn token
        0x24230C14:"d81dec9e2724",# 0.25 threshold
        0x24230C1C:"f6c405",
        0x24230C1F:"7a07",
        0x24230C21:"68ac752c24",  # Low token
        0x24230C32:"dae9",
        0x24230C36:"f6c444",
        0x24230C39:"7a12",
        0x24230C43:"689c752c24",  # Empty token
        0x24230C57:"dae9",
        0x24230C5B:"f6c444",
        0x24230C5E:"7a20",
        0x24230C60:"80bea600000000",
        0x24230C67:"7617",       # unsigned status <=0
        0x24230C69:"6888752c24", # FadeOut token
        0x24230C76:"ff5024",     # original sound resolver slot+0x24
        0x24230C80:"8b8ff0110000",# store raw player float
        0x24230C86:"898eec000000",
        0x24230C96:"e80547ddff", # inherited paint
        0x24230C9B:"80bea600000000",# status nonzero
        0x24230CA8:"83bef000000000",# paint resource +0xF0
        0x24230CB5:"83bef400000000",# paint resource +0xF4
        0x2421AD3E:"6a28",       # recv nested offset +0x28
        0x2421AD40:"683c5e2b24", # recv property name pointer
    },
    "server":{
        0x223A293A:"6a28",
        0x223A293C:"6840fa4a22",
    },
}
CALLS = {
    "client":{
        0x24230A6F:0x241442C0,
        0x24230A86:0x241442C0,
        0x24230C7B:0x240104B0,
        0x24230C96:0x240053A0,
        0x24230CEE:0x24154AB0,
        0x24230D6F:0x24154D50,
        0x2421AD4A:0x24126D80,
    },
    "server":{0x223A2946:0x2225E8C0},
}
CONSTANTS = {
    0x242DE308:struct.pack("<f",1.0),
    0x242DE2F8:struct.pack("<f",0.0),
    0x24279EEC:struct.pack("<f",0.25),
}

def verify_original_breath(images:dict[str,PEImage],*,check_hashes=True)->dict:
    for side in ("client","server"):
        expected=0x24000000 if side=="client" else 0x22000000
        if images[side].base!=expected:
            raise VerificationError("original "+side+" load base mismatch")
    client=images["client"]
    for address,target in {**RTTI,**VTABLE}.items():
        if client.at(address,4)!=struct.pack("<I",target):
            raise VerificationError("original breath RTTI/vtable mismatch at %#x"%address)
    if client.at(TYPE_NAME_VA,len(TYPE_NAME))!=TYPE_NAME:
        raise VerificationError("original CHudBreathTimer RTTI name mismatch")
    for addr,name in {**SOUND_NAMES,**NETWORK_NAMES["client"]}.items():
        if client.at(addr,len(name)+1)!=name.encode()+b"\0":
            raise VerificationError("original client sound/network name mismatch at %#x"%addr)
    for addr,name in NETWORK_NAMES["server"].items():
        if images["server"].at(addr,len(name)+1)!=name.encode()+b"\0":
            raise VerificationError("original server breath network name mismatch")
    for address,data in CONSTANTS.items():
        if client.at(address,len(data))!=data:
            raise VerificationError("original breath floating threshold mismatch")
    for side,ops in OPCODES.items():
        for address,hextext in ops.items():
            wanted=bytes.fromhex(hextext)
            if images[side].at(address,len(wanted))!=wanted:
                raise VerificationError("original breath opcode mismatch at %#x"%address)
    for side,calls in CALLS.items():
        for va,target in calls.items():
            wanted=b"\xe8"+struct.pack("<i",target-va-5)
            if images[side].at(va,5)!=wanted:
                raise VerificationError("original breath call mismatch at %#x"%va)
    if check_hashes:
        for name,(side,va,length,want) in REGIONS.items():
            if hashlib.sha256(images[side].at(va,length)).hexdigest()!=want:
                raise VerificationError("original breath code-region SHA256 mismatch: "+name)
    return {
        "original_class":"CHudBreathTimer",
        "methods":{"reset":"0x24230A50","bind_resources":"0x24230A60",
                   "player_update":"0x24230BB0","panel_paint":"0x24230C90"},
        "original_sound_tokens":list(SOUND_NAMES.values()),
        "original_property":"DT_HL2Local.m_breathTimer",
        "original_player_offsets":{"client":"0x11F0","server":"0x13A4"},
        "verified_code_regions":len(REGIONS) if check_hashes else 0,
        "original_client_methods":4,
        "verified_opcode_witnesses":sum(map(len,OPCODES.values())),
        "verified_call_edges":sum(map(len,CALLS.values())),
        "verified_vtable_and_RTTI_pointers":len(RTTI)+len(VTABLE),
        "scope":"Original HUD predicate, cue ordering and resource binding modeled; actual VGUI paint/sound side effects remain native-engine work.",
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    metadata=prior_original_audit(client_bytes,server_bytes)
    metadata["original_breath_hud"]=verify_original_breath(
        {"client":PEImage(client_bytes),"server":PEImage(server_bytes)})
    return metadata

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    output=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(output,encoding="utf-8")
    else:
        print(output,end="")
if __name__=="__main__":
    main()
