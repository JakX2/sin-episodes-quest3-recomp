#!/usr/bin/env python3
"""Phase156 owner-only original 2006 SiN client HUD coordinate scale audit.

Both original client/server SHA-256 fingerprints are checked by chaining the
Phase155 original-binary verifier. Verifies 3 complete original x86 method
regions, 13 direct CALL edges, original canvas float constants, and exact
instruction witnesses. Emits metadata, NEVER game executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_breath_timer import audit as phase155_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    "hud_float_pair_scale": (0x24154AB0,38,
       "50d91b1f479c892647073f2d261f57d59da75ac4bce34301551dac6607ff3e13"),
    "primary_dimension_getter": (0x240FFAD0,6,
       "a12626a34807f5853399a7d19183b8f64421a56c9825868a16a9f9c3cbe351f3"),
    "secondary_dimension_getter": (0x240FFAE0,6,
       "80f5c525298aecd4e3077fe952d3bc0977e1f06d65bbd7cef8fe3c80df1f618b"),
}
OPCODES = {
    0x24154AB0:"51",
    0x24154AB6:"890424",        # store returned screen integer on stack
    0x24154AB9:"db0424",        # FILD signed 32-bit dimension
    0x24154ABC:"8b442408",      # first pointer
    0x24154AC0:"d8742414",      # FDIV caller float denominator
    0x24154AC4:"d9c0",          # duplicate original x87 ratio
    0x24154AC6:"d808",          # multiply first pointed float
    0x24154AC8:"d918",          # store first float
    0x24154ACA:"8b44240c",      # second pointer
    0x24154ACE:"d808",          # multiply second float (aliasing matters)
    0x24154AD0:"d918",          # store second float
    0x24154AD2:"59",
    0x24154AD3:"c21000",        # four stdcall/thiscall stack args
    0x240FFAD0:"a114fc3324",  # first getter reads *0x2433FC14
    0x240FFAD5:"c3",
    0x240FFAE0:"a110fc3324",  # other getter reads *0x2433FC10
    0x240FFAE5:"c3",
}
# Callsites from independently inspected original client.dll disassembly;
# both CHudPoisonTimer and CHudBreathTimer paint methods are included.
HUD_SCALE_CALLS = (
    0x2413FE03, 0x2413FE48, 0x24147A6D, 0x241521D2,
    0x241525C4, 0x2422F228, 0x2422F332, 0x2422FCA6,
    0x24230CEE, 0x24234ED7, 0x242358F8, 0x2423618E,
)
CALLS = {
    0x24154AB1:0x240FFAD0,
    **{site:0x24154AB0 for site in HUD_SCALE_CALLS},
}
REFERENCE_CANVAS = {
    0x2429B69C:1600.0, # original x86 data constant, caller arg3 (unused)
    0x2429B6A0:1200.0, # original x86 data constant, caller arg4 (denominator)
}
PANEL_CALLS = {
    "CHudBreathTimer":0x24230CEE,
    "CHudPoisonTimer":0x2423618E,
}
def verify_original_hud_scale(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("original client.dll image base mismatch")
    for va,hexbytes in OPCODES.items():
        literal=bytes.fromhex(hexbytes)
        if pe.at(va,len(literal))!=literal:
            raise VerificationError("original client HUD coordinate opcode changed at %#x"%va)
    for va,target in CALLS.items():
        literal=b"\xe8"+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=literal:
            raise VerificationError("original client HUD coordinate call changed at %#x"%va)
    for va,float_value in REFERENCE_CANVAS.items():
        if pe.at(va,4)!=struct.pack("<f",float_value):
            raise VerificationError("original reference canvas float changed at %#x"%va)
    if check_hashes:
        for name,(va,n,sha) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=sha:
                raise VerificationError("original HUD scaler full SHA256 mismatch: "+name)
    return {
        "original_client_methods":{
            "x87_hud_pair_scale":"0x24154AB0",
            "dimension_getter_A":"0x240FFAD0",
            "dimension_getter_B":"0x240FFAE0",
        },
        "native_x87_scale_formula":"(int32_at_0x2433FC14 / caller_float_arg4) * float_pointer_values",
        "original_getter_global_values":{
            "first":"0x2433FC14","second":"0x2433FC10"},
        "reference_canvas_float_data":{
            "width_va":"0x2429B69C","width":1600.0,
            "height_va":"0x2429B6A0","height":1200.0},
        "native_hud_paint_callsites":{
            name:"0x%08X"%va for name,va in PANEL_CALLS.items()},
        "verified_full_function_regions":len(REGIONS) if check_hashes else 0,
        "verified_instruction_anchors":len(OPCODES),
        "verified_direct_calls":len(CALLS),
        "all_original_hud_scaler_direct_calls":len(HUD_SCALE_CALLS),
        "limitations":[
            "Original global integer getter names are not available in retail DLL.",
            "Native x87 precision/exception handling differs from some portable C++ targets.",
            "Original VGUI rendering 0x24154D50 remains engine-dependent.",
            "The semantics/writers of shared HUD byte +0xA6 remain unknown.",
            "Neither this helper nor the client.dll can be loaded as a rebuilt SiN gameplay DLL yet.",
        ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase155_audit(client_bytes,server_bytes)
    report["original_hud_coordinate_scale"]=verify_original_hud_scale(
        PEImage(client_bytes))
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")

if __name__=="__main__":
    main()
