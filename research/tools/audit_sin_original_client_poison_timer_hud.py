#!/usr/bin/env python3
"""Owner-only original SiN Episodes (2006) CHudPoisonTimer PE32 auditor.

Pins BOTH original DLL SHA256 fingerprints, two verified client RTTI
vftables, four whole client x86 functions, direct call targets, original
field access instructions, comparison constants, and named sound tokens.
Writes metadata only. Does not upload or embed commercial executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, PEImage, VerificationError,
)

REGIONS = {
    "poison_hud_reset": (0x242360B0, 11,
        "4dfd88bca790581954a5a77ef16d80c8cd3e50c95525a01731d0998c0cce4151"),
    "poison_hud_bind_resources": (0x242360C0, 74,
        "b3e2c15990cc1a70df2faeeaca0bc09be2e224a43fa71484115c1fc1a030f06d"),
    "poison_hud_paint": (0x24236120, 243,
        "ca1b8cf8c14c650150eb8b2c8258deaaba3c712888bf6b6f495f0a458972d0e4"),
    "poison_hud_player_update": (0x24236320, 203,
        "921a342df76c159e6121789f111bcfaf66c4154cb47e4afd8b76fa2c06297150"),
}
TYPE_NAME = b".?AVCHudPoisonTimer@@\0"
RTTI = {
    0x242C91C0: 0x242EFAD4,  # primary vtable locator
    0x242C8F90: 0x242EFA80,  # secondary panel vtable locator
    0x242EFAE0: 0x24320EA8,  # primary COL + 12 TypeDescriptor
    0x242EFA8C: 0x24320EA8,  # secondary COL +12 TypeDescriptor
}
VTABLE = {
    0x242C91C4 + 2*4: 0x242360B0,  # primary virtual reset slot 2
    0x242C91C4 + 3*4: 0x242360C0,  # primary virtual resource binding slot 3
    0x242C8F94 + 93*4: 0x24236320, # secondary virtual update slot 93
    0x242C8F94 + 109*4: 0x24236120, # secondary virtual paint slot 109
}
OPCODES = {
    0x242360B0:"c7812001000000000000",
    0x242360BA:"c3",
    0x242360C3:"8d86bc000000",
    0x242360D4:"8d8edd000000",
    0x242360E0:"898624010000",
    0x242360EB:"8d96fe000000",
    0x242360F7:"898628010000",
    0x24236102:"89862c010000",
    0x2423612C:"d9860c010000",
    0x24236132:"d81df8e22d24",
    0x2423613A:"f6c441",
    0x2423613D:"7508",
    0x2423613F:"8bbe14010000",
    0x24236147:"8bbe18010000",
    0x2423614D:"83be1001000000",
    0x2423615A:"85ff",
    0x24236323:"8b3df08c3324",
    0x24236333:"8b87ec110000",
    0x24236339:"d9860c010000",
    0x24236343:"d9442408",
    0x24236347:"dae9",
    0x242363D5:"d987ec110000",
    0x242363DB:"dc2d20e32d24",
    0x242363E1:"d99e0c010000",
}
CALLS = {
    0x242360CF:0x241442C0,
    0x242360E6:0x241442C0,
    0x242360FD:0x241442C0,
    0x24236127:0x240053A0,
    0x2423618E:0x24154AB0,
    0x24236208:0x24154D50,
}
STRINGS = {
    0x242C8E3C:"PoisonTimeFadeOut",
    0x242C8E50:"PoisonTimeFadeIn",
}
CONSTANTS = {
    0x242DE2F8:struct.pack("<f",0.0),
    0x242DE308:struct.pack("<f",1.0),
    0x242DE320:struct.pack("<d",1.0),
}

def verify_client(pe: PEImage, *, check_hashes: bool = True) -> dict:
    if pe.base != 0x24000000:
        raise VerificationError("unexpected original SiN client.dll load base")
    for pointer,value in {**RTTI,**VTABLE}.items():
        if pe.at(pointer, 4) != struct.pack("<I",value):
            raise VerificationError("CHudPoisonTimer RTTI or vtable mismatch at %#x" % pointer)
    if pe.at(0x24320EB0,len(TYPE_NAME)) != TYPE_NAME:
        raise VerificationError("CHudPoisonTimer original MSVC class-name mismatch")
    for address,text in OPCODES.items():
        expected=bytes.fromhex(text)
        if pe.at(address,len(expected)) != expected:
            raise VerificationError("CHudPoisonTimer opcode mismatch at %#x" % address)
    for site,target in CALLS.items():
        expected=b"\xE8"+struct.pack("<i",target-site-5)
        if pe.at(site,5) != expected:
            raise VerificationError("CHudPoisonTimer CALL mismatch at %#x" % site)
    for address,name in STRINGS.items():
        literal=name.encode("ascii")+b"\0"
        if pe.at(address,len(literal))!=literal:
            raise VerificationError("CHudPoisonTimer original sound-token mismatch")
    for address,literal in CONSTANTS.items():
        if pe.at(address,len(literal)) != literal:
            raise VerificationError("CHudPoisonTimer float constant mismatch at %#x" % address)
    if check_hashes:
        for key,(address,length,wanted) in REGIONS.items():
            actual=hashlib.sha256(pe.at(address,length)).hexdigest()
            if actual!=wanted:
                raise VerificationError("CHudPoisonTimer full x86 code SHA256 mismatch: "+key)
    return {
        "original_class":"CHudPoisonTimer",
        "original_client_vtables":{
            "primary":"0x242C91C4", "secondary_panel":"0x242C8F94"},
        "virtuals":{
            "primary_slot2_reset":"0x242360B0",
            "primary_slot3_resource_binding":"0x242360C0",
            "panel_slot93_update":"0x24236320",
            "panel_slot109_paint":"0x24236120",
        },
        "verified_full_function_hashes":len(REGIONS) if check_hashes else 0,
        "verified_original_code_anchors":len(OPCODES),
        "verified_calls":len(CALLS),
        "verified_RTTI_and_virtual_pointers":len(RTTI)+len(VTABLE),
        "bounds":[
            "Reset and three sequential resource bindings fully modeled as host-neutral behavior.",
            "Render resource selection and player display-fraction projection modeled; actual engine paint/audio remain unimplemented.",
            "Named sound token references do not establish engine API signatures.",
            "No claim of x86 object/vtable ABI compatibility or a playable replacement DLL.",
        ],
    }

def audit(client_bytes: bytes, server_bytes: bytes) -> dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA256:
        raise VerificationError("wrong original client.dll fingerprint")
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA256:
        raise VerificationError("wrong original server.dll fingerprint")
    report=verify_client(PEImage(client_bytes))
    report["original_dll_sha256"]={"client":CLIENT_SHA256, "server":SERVER_SHA256}
    return report

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(data,encoding="utf-8")
    else:
        print(data,end="")

if __name__=="__main__":
    main()
