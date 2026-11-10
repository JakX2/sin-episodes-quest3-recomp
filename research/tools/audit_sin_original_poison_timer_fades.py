#!/usr/bin/env python3
"""Phase 154: original SiN 2006 CHudPoisonTimer fade CFG evidence.

The original 203-byte client.dll method was SHA-pinned in Phase153.
This audit adds branch-by-branch witnesses, the actual 0.25f threshold,
the equality early-exit, the HUD status byte at +0xA6, and exact sound
token dispatch sites. Verifies the owner's complete original DLL pair.
Does not distribute original PE code or game assets.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_poison_timer_hud import (
    audit as phase153_audit, REGIONS,
)
from verify_gameplay_entity_registry import PEImage, VerificationError

# Independently located by real original DLL disassembly at 0x24236320.
OPCODES = {
    0x24236323:"8b3df08c3324",0x24236329:"85ff",
    0x2423632D:"0f84b4000000",
    0x24236333:"8b87ec110000",0x24236339:"d9860c010000",
    0x24236343:"d9442408",0x24236347:"dae9",
    0x24236349:"dfe0",0x2423634B:"f6c444",
    0x2423634E:"0f8b93000000",
    0x24236354:"d9442408",0x24236358:"d81d08e32d24",
    0x2423635E:"dfe0",0x24236360:"f6c405",
    0x24236363:"7a1b",0x24236365:"80bea600000000",
    0x2423636C:"7512",0x2423636E:"8b0da0f83724",
    0x24236376:"68508e2c24",0x2423637B:"ff5224",
    0x2423637E:"eb4e",0x24236380:"d9442408",
    0x24236384:"d81dec9e2724",0x2423638A:"dfe0",
    0x2423638C:"f6c405",0x2423638F:"7b44",
    0x24236391:"d905f8e22d24",0x24236397:"d9442408",
    0x2423639B:"dae9",0x2423639D:"dfe0",
    0x2423639F:"f6c444",0x242363A2:"7b31",
    0x242363A4:"d9442408",0x242363A8:"d81d08e32d24",
    0x242363AE:"dfe0",0x242363B0:"f6c401",
    0x242363B3:"7520",0x242363B5:"80bea6000000ff",
    0x242363BC:"7517",0x242363BE:"8b0da0f83724",
    0x242363C6:"683c8e2c24",0x242363CB:"ff5024",
    0x242363CE:"8bc8",0x242363D0:"e8dba0ddff",
    0x242363D5:"d987ec110000",
    0x242363DB:"dc2d20e32d24",
    0x242363E1:"d99e0c010000",
}
SOUND_NAMES = {
    0x242C8E50:"PoisonTimeFadeIn",
    0x242C8E3C:"PoisonTimeFadeOut",
}
CONST_QUARTER_VA = 0x24279EEC
ORIGINAL_STATUS_BYTE_OFFSET = 0xA6
FULL_CODE_VA,FULL_CODE_LENGTH,FULL_CODE_SHA256 = REGIONS["poison_hud_player_update"]

def verify_fade_branches(pe: PEImage, *, check_hashes=True) -> dict:
    if pe.base != 0x24000000:
        raise VerificationError("unexpected original client load base")
    for site, hexcode in OPCODES.items():
        want=bytes.fromhex(hexcode)
        if pe.at(site,len(want))!=want:
            raise VerificationError("original poison fade opcode mismatch at %#x"%site)
    for site,name in SOUND_NAMES.items():
        wanted=name.encode("ascii")+b"\x00"
        if pe.at(site,len(wanted))!=wanted:
            raise VerificationError("poison fade literal sound token mismatch")
    if pe.at(CONST_QUARTER_VA,4)!=struct.pack("<f",0.25):
        raise VerificationError("original poison fade quarter threshold mismatch")
    if check_hashes:
        digest=hashlib.sha256(pe.at(FULL_CODE_VA,FULL_CODE_LENGTH)).hexdigest()
        if digest!=FULL_CODE_SHA256:
            raise VerificationError("CHudPoisonTimer original 203-byte update SHA256 mismatch")
    callback=pe.at(0x242363D0,5)
    target=0x242363D0+5+struct.unpack("<i",callback[1:])[0]
    if callback[0]!=0xE8 or target!=0x240104B0:
        raise VerificationError("original sound token follow-up CALL target mismatch")
    return {
        "original_class":"CHudPoisonTimer",
        "update_function_va":"0x24236320",
        "verified_complete_code_region_bytes":FULL_CODE_LENGTH if check_hashes else 0,
        "verified_original_branch_opcode_sites":len(OPCODES),
        "verified_original_sound_tokens":list(SOUND_NAMES.values()),
        "comparison_thresholds":[0.0,0.25,1.0],
        "local_player_field":"m_HL2Local.m_poisonTimer",
        "original_client_player_offset":"0x11EC",
        "original_hud_display_fraction_offset":"0x10C",
        "original_hud_sound_status_byte_offset":"0xA6",
        "fadestate_status_values":{"fade_in":0,"fade_out":255},
        "sound_interface_virtual_slot_byte_offset":"0x24",
        "post_sound_call_va":"0x240104B0",
        "control_flow":[
            "no player -> return without mutating HUD",
            "incoming player float == old HUD display float (ordered equality) -> return",
            "incoming player float < 1.0 AND HUD byte +0xA6 == 0 -> FadeIn",
            "otherwise: incoming >= 0.25, != 0, >= 1.0 and +0xA6 == 0xFF -> FadeOut",
            "NaNs do not cause either sound; unordered equality passes update gate",
            "optional sound token resolution+follow-up before storing display=1.0-input",
        ],
        "limitations":[
            "Original sound resolver virtual +0x24 and helper 0x240104B0 ABIs not rebuilt.",
            "This is a deeper control-flow reconstruction of an already-counted Phase153 method, not a new independent original function.",
            "Actual owner of +0xA6 status byte and timing/event lifecycle unresolved.",
            "Original x87 results may differ in last-bit rounding from portable float arithmetic.",
        ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    metadata=phase153_audit(client_bytes,server_bytes)
    metadata["original_poison_timer_fade_branches"]=verify_fade_branches(
        PEImage(client_bytes))
    return metadata

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    content=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),
                       indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(content,encoding="utf-8")
    else:
        print(content,end="")

if __name__=="__main__":
    main()
