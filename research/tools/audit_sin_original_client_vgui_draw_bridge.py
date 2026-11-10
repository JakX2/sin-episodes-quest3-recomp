#!/usr/bin/env python3
"""Phase158 original SiN 2006 client VGUI draw wrapper / backend witness.

This owner-only verifier chains previous full original client/server DLL SHA
verification. It pins the complete 69-byte 7-to-9 adapter 0x241417C0 and
the complete 757-byte surface draw function 0x24141DD0, while explicitly
not claiming that the latter's engine VGUI calls are all reimplemented.
Synthetic tests use only partial instruction witnesses, never PE code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_hud_renderer_preflight import (
    audit as phase157_audit,
)
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    "complete_seven_to_nine_argument_draw_wrapper":(
        0x241417C0,69,
        "1907a1db9b89a39b6e4d9583ecfddd9c3c6da4cd47a3096571723747870dc637"),
    "native_draw_worker_including_both_texture_routes":(
        0x24141DD0,757,
        "ee860c6fc19ae1f9954dd8a40920be425f6a5191dbb332198087868508de1766"),
}
# Sparse, disjoint, manually validated anchors. Do not add full
# executable byte sequences to this public source.
OPCODES = {
    0x241417C0:"8b44241c",     # wrapper input word 7
    0x241417C4:"8b542418",     # wrapper input word 6
    0x241417D8:"8b91b4000000", # resource dimension B end
    0x241417E5:"8b81ac000000", # resource dimension A end
    0x241417F2:"8b542420",     # original input word 2
    0x241417F7:"8b442420",     # original input word 1
    0x24141802:"c21c00",       # wrapper cleans up 7 words
    0x24141DD8:"80be8400000000", # branch on instance +0x84
    0x24141DE0:"0f843c010000",   # route to texture-id path
    0x24141DEB:"8b8e88000000", # original surface handle
    0x24141DF6:"ff92fc000000", # surface virtual +0xFC
    0x24141DFC:"8bac2494000000", # caller original arg8
    0x24141E07:"2bd5",          # signed wrapping int subtraction
    0x24141E10:"db842494000000", # x87 FILD numerator
    0x24141E1E:"dab42488000000", # x87 FIDIV denominator
    0x24141E8B:"ff928c010000", # native surface clip +0x18C
    0x24141E93:"0f8422020000", # rejected clip exits
    0x24141E99:"85db",         # branch on native arg6
    0x24141E9D:"d9442450",     # FLD second vertex
    0x24141ECD:"3bef",         # skip reverse if arg8==virtual limit
    0x24141ED1:"d90508e32d24", # original 1.0f complement
    0x24141F12:"ff9290010000", # surface draw virtual +0x190
    0x24141F22:"83be94000000ff",# if texture id=-1 return
    0x24141F2F:"8b96ac000000", # resource dimension A
    0x24141F3B:"8b86b4000000", # resource dimension B
    0x24141F5E:"d986a0000000", # UV right
    0x24141F88:"d986a4000000", # UV bottom
    0x24141F9E:"d8f2",        # 1 / resource width
    0x24141FCA:"d8f1",        # 1 / resource height
    0x24142030:"ff5270",      # bind original texture id
    0x24142087:"ff9278010000", # draw textured subrectangle
    0x2414208D:"8b0e",        # optional chained resource pointer
    0x241420C2:"c22400",      # renderer cleans 9 words
}
EXTERNAL_WORKER_CALLS = (
    0x241417FD,0x2414220B,0x24154EA0,0x24154F62,
    0x24155072,0x24155129,0x24155239,
)
VGUI_SINGLETON_CALLS = (
    0x24141DE6,0x24141E2C,0x24141E3F,0x24141E55,
    0x24141E74,0x24141F04,0x24142020,0x24142033,
    0x24142049,
)
CALLS = {
    **{va:0x24141DD0 for va in EXTERNAL_WORKER_CALLS},
    **{va:0x24001040 for va in VGUI_SINGLETON_CALLS},
    0x241420B6:0x241417C0, # original native overlay chain
}
def verify_original_draw_bridge(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("expected original 2006 client.dll image base")
    for va,code in OPCODES.items():
        wanted=bytes.fromhex(code)
        if pe.at(va,len(wanted))!=wanted:
            raise VerificationError("original VGUI draw bridge x86 opcode mismatch at %#x"%va)
    for va,target in CALLS.items():
        wanted=b"\xe8"+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError("original VGUI draw bridge CALL mismatch at %#x"%va)
    if check_hashes:
        for name,(va,length,sha) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=sha:
                raise VerificationError("original VGUI draw bridge full-code SHA mismatch: "+name)
    return {
        "original_client_functions":{
            "complete_seven_word_bridge":"0x241417C0",
            "partially_recovered_surface_draw_worker":"0x24141DD0",
        },
        "verified_complete_code_region_hashes":len(REGIONS) if check_hashes else 0,
        "verified_sparse_x86_instruction_anchors":len(OPCODES),
        "verified_direct_call_edges":len(CALLS),
        "original_callers_of_worker":len(EXTERNAL_WORKER_CALLS),
        "native_return_stack_words":{"bridge":7,"worker":9},
        "native_resource_fields":{
            "clipped_surface_flag":"+0x84",
            "surface_handle":"+0x88",
            "texture_id":"+0x94",
            "uv_low_a":"+0x98","uv_low_b":"+0x9C",
            "uv_high_a":"+0xA0","uv_high_b":"+0xA4",
            "rectangle_a":"+0xA8..+0xAC",
            "rectangle_b":"+0xB0..+0xB4",
            "chained_resource_pointer":"+0x00",
        },
        "native_branches":[
            "flag +0x84 nonzero: surface clip via virtual +0x18C then draw +0x190 if accepted",
            "flag +0x84 zero & texture id -1: return without drawing",
            "flag +0x84 zero & texture id != -1: compute four float UVs and draw virtual +0x178",
            "untextured-name is misleading: alternate native path binds texture ID +0x70",
            "when chain pointer +0x00 is nonnull, call the complete seven-word bridge 0x241417C0",
        ],
        "limits":[
            "Original VGUI surface interface and native drawing callbacks not implemented.",
            "Original x87 fractional last-bit parity remains platform dependent.",
            "Full 757-byte native worker is hashed/partially decoded, not fully recompiled.",
            "Full 2006 client.dll rebuild, Windows gameplay and ARM64 Quest execution unproven.",
        ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    out=phase157_audit(client_bytes,server_bytes)
    out["original_client_vgui_draw_bridge"]=verify_original_draw_bridge(
        PEImage(client_bytes))
    return out

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(data,encoding="utf-8")
    else:
        print(data,end="")
if __name__=="__main__":
    main()
