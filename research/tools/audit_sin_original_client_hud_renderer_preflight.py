#!/usr/bin/env python3
"""Phase157: owner-only original 2006 SiN client HUD render entry verifier.

Chains full original client/server fingerprints and Phase156 research.
Pins the entire original 0x24154D50 routine (NOT fully reconstructed),
both complete rectangle getter functions, seven direct client HUD
callers, exact mode jump table, original x87 entry-branch witnesses,
and native VGUI dispatch edges. Outputs metadata only, never PE bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_hud_coordinate_scale import audit as phase156_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    "resource_extent_a_getter":(0x241417A0,13,
        "e398c45791c0ff3cde3fe62f5b117d42ab0a6c4413411187922f938ef65e87f9"),
    "resource_extent_b_getter":(0x241417B0,13,
        "50ee9b1f22450afc4495431683d02390ac4d89474c01aa338d09eae515f13587"),
    # The complete 1271-byte native routine includes all drawing modes,
    # but this phase only reconstructs its common entry/preflight.
    "full_native_hud_render_body":(0x24154D50,1271,
        "73b217a2cde0cd55eaf467aeaa6352fd64e2fba8b844df4ec078d9b445ffaa54"),
}
OPCODES = {
    0x241417A0:"8b81ac000000",  # return end A of resource rectangle
    0x241417A6:"2b81a8000000",  # subtract start A
    0x241417B0:"8b81b4000000",  # return end B
    0x241417B6:"2b81b0000000",  # subtract start B
    0x24154D54:"8b74242c",      # primary arg6 resource pointer
    0x24154D58:"85f6",
    0x24154D5A:"0f84e0040000",  # missing resource early return
    0x24154D60:"d9442430",      # read fraction
    0x24154D64:"d81d08e32d24",  # compare original 1.0f
    0x24154D6C:"f6c441",
    0x24154D71:"c74424300000803f", # clamp ordered >1 to one
    0x24154D7F:"d81df8e22d24",  # compare original zero
    0x24154D87:"f6c405",
    0x24154D8A:"7a08",          # unordered NaN bypasses zero clamp
    0x24154D8C:"c744243000000000", # clamp ordered <0 to zero
    0x24154D94:"d90508e32d24",  # read original 1.0f again
    0x24154D9B:"d8642434",      # subtract normalized fraction
    0x24154DA2:"d95c2418",      # store 1.0 - fraction float
    0x24154DAF:"db442434",      # FILD resource dimension B
    0x24154DC2:"db442434",      # FILD resource dimension A
    0x24154DE1:"d874242c",      # divide original integer global by arg4
    0x24154DFB:"0f84a4000000",  # no secondary resource => skip prepass
    0x24154EA9:"83f803",        # mode >3 returns after optional prepass
    0x24154EAC:"0f878c030000",
    0x24154EB2:"ff248548521524", # jump table [mode*4 + 0x24155248]
    0x24155244:"c22800",      # native routine return, ten stack args
}
DIRECT_CALLS = {
    0x24154DA6:0x241417B0,
    0x24154DB9:0x241417A0,
    0x24154DCA:0x240FFAD0,
    0x24154EA0:0x24141DD0,
    0x24154F62:0x24141DD0,
    0x24155072:0x24141DD0,
    0x24155129:0x24141DD0,
    0x24155239:0x24141DD0,
    0x2422F2A3:0x24154D50,
    0x24230D6F:0x24154D50, # CHudBreathTimer
    0x24233774:0x24154D50,
    0x242337FD:0x24154D50,
    0x24234F50:0x24154D50,
    0x24235973:0x24154D50,
    0x24236208:0x24154D50, # CHudPoisonTimer
}
JUMP_TABLE_VA=0x24155248
MODE_JUMP_TARGETS=(0x24155080,0x24155137,0x24154EB9,0x24154F70)

def verify_original_hud_render_preflight(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("original HUD renderer PE32 image base mismatch")
    for va,hextext in OPCODES.items():
        wanted=bytes.fromhex(hextext)
        if pe.at(va,len(wanted))!=wanted:
            raise VerificationError("original HUD renderer entry opcode mismatch at %#x"%va)
    for va,target in DIRECT_CALLS.items():
        wanted=b"\xe8"+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError("original HUD renderer direct CALL mismatch at %#x"%va)
    for mode,target in enumerate(MODE_JUMP_TARGETS):
        if pe.at(JUMP_TABLE_VA+4*mode,4)!=struct.pack("<I",target):
            raise VerificationError("original HUD renderer drawing mode %d target changed"%mode)
    if check_hashes:
        for name,(va,length,expected) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=expected:
                raise VerificationError("original HUD renderer complete code SHA256 mismatch: "+name)
    return {
        "original_entry":"0x24154D50",
        "original_resource_extent_getters":["0x241417A0","0x241417B0"],
        "native_complete_body_sha256":REGIONS["full_native_hud_render_body"][2],
        "verified_complete_original_method_hashes":len(REGIONS) if check_hashes else 0,
        "original_binary_instruction_anchors":len(OPCODES),
        "verified_original_call_edges":len(DIRECT_CALLS),
        "hud_paint_callers":7,
        "original_mode_jump_table":[hex(x) for x in MODE_JUMP_TARGETS],
        "original_entry_contract":[
            "null primary resource (native sixth arg) -> return without drawing",
            "ordered scalar >1 -> 1, ordered scalar <0 -> 0, unordered NaN unchanged",
            "compute float(1 - scalar) before native draw dispatch",
            "two original resource geometry getter results computed as int32 subtractions",
            "apply original 32-bit global at 0x2433FC14 / caller fourth float argument to resource extents",
            "optional secondary resource can draw before mode switch",
            "mode 0..3 -> four original jump-table paths; mode >=4 -> return",
        ],
        "critical_limits":[
            "The 1271-byte original renderer has more complex geometry and four draw cases; only common preflight and mode classification are modeled.",
            "Original game VGUI callbacks, clipping, output pixel layout and packed-color byte ownership remain unresolved.",
            "This is not an ABI-compatible VClient011 replacement or a playable game module.",
        ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    metadata=phase156_audit(client_bytes,server_bytes)
    metadata["original_client_hud_renderer_preflight"]=verify_original_hud_render_preflight(
        PEImage(client_bytes))
    return metadata

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",required=True,type=Path)
    parser.add_argument("--server",required=True,type=Path)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    obj=audit(args.client.read_bytes(),args.server.read_bytes())
    string=json.dumps(obj,indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(string,encoding="utf-8")
    else:
        print(string,end="")

if __name__=="__main__":main()
