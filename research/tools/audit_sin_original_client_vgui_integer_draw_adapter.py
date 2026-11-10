#!/usr/bin/env python3
"""Phase159: owner-only original 2006 SiN client.dll integer HUD draw audit.

Verifies SHA-pinned original client/server DLLs via Phase158, then pins
the COMPLETE 327-byte original 0x241420D0 x87 FISTP draw adapter,
34 deliberately partial opcode anchors and ten original direct CALLs.
No original executables, full code regions, or assets are emitted.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_vgui_draw_bridge import audit as phase158_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    "native_x87_five_to_nine_word_integer_draw_adapter":(
        0x241420D0, 327,
        "e0717404d4d24787028e5db0f7cef56b8c9c0f33f1f31b468db975a80e573c75")
}
# Partial independent instruction anchors verified on the exact original
# owner-held 2006 PE. A synthetic unit-test fixture cannot reproduce
# a complete commercially authored method from these sparse bytes.
OPCODES = {
    0x241420D0:"83ec0c",        # native 12-byte local frame
    0x241420D6:"8b86ac000000",  # resource end A
    0x241420DC:"2b86a8000000",  # subtract start A in signed 32-bit
    0x241420E2:"8b8eb4000000",  # resource end B
    0x241420E8:"2b8eb0000000",  # subtract start B
    0x241420F2:"db442408",      # FILD extent A
    0x241420FA:"d95c2408",      # FSTP m32 rounds extent A
    0x241420FE:"db442404",      # FILD extent B
    0x24142102:"d95c2404",      # FSTP m32 rounds extent B
    0x24142110:"8b542424",      # save raw fifth argument
    0x24142118:"db44240c",      # FILD primary integer global
    0x2414211D:"d8742424",      # FDIV fourth FLOAT argument
    0x24142121:"d9442408",      # reload float extent A
    0x2414212E:"80cc0c",        # original x87 CW truncate toward zero
    0x24142135:"d96c2428",      # FLDCW temporary control word
    0x24142139:"db5c2428",      # FISTP extent B (ordered original stack)
    0x24142146:"d9442410",      # reload float extent A
    0x24142153:"80cc0c",        # second truncation-CW override
    0x2414215E:"db5c242c",      # FISTP extent A
    0x2414216F:"d9442418",      # load float extent B
    0x24142177:"d8c9",          # multiply by primary dimension ratio
    0x2414217E:"80cc0c",        # scaled extent B round-toward-zero
    0x24142189:"db5c2438",      # FISTP scaled B
    0x24142196:"d9442420",      # load float extent A
    0x2414219E:"d8c9",          # multiply by primary dimension ratio
    0x241421A5:"80cc0c",        # scaled extent A round-toward-zero
    0x241421B0:"db5c243c",      # FISTP scaled A
    0x241421BF:"d9442434",      # load original caller float arg2
    0x241421D7:"db5c2440",      # FISTP caller arg2
    0x241421E4:"d9442434",      # load original caller float arg1
    0x241421FC:"db5c2444",      # FISTP caller arg1
    0x24142209:"8bce",          # set original resource this pointer
    0x24142210:"5e",            # restore nonvolatile register
    0x24142214:"c21400",        # RET 0x14, five original caller args
}
GETTER_AND_WORKER_CALLS = {
    0x24142106:0x240FFAE0, # secondary dimension getter first, unused result
    0x2414210B:0x240FFAD0, # primary global getter result in EAX
    0x2414220B:0x24141DD0, # 9-word worker from Phase158
}
ORIGINAL_EXTERNAL_CALLERS = (
    0x2413FE27,0x2413FE6E,0x2422F355,0x2422FCC9,
    0x242358B6,0x242359D8,0x24236993,
)
CALLS = {
    **GETTER_AND_WORKER_CALLS,
    **{site:0x241420D0 for site in ORIGINAL_EXTERNAL_CALLERS}
}

def verify_original_integer_draw(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base != 0x24000000:
        raise VerificationError("original client.dll image base mismatch")
    for va,hexcode in OPCODES.items():
        wanted=bytes.fromhex(hexcode)
        if pe.at(va,len(wanted)) != wanted:
            raise VerificationError("original 2006 integer HUD draw opcode differs at %#x"%va)
    for site,target in CALLS.items():
        wanted=b"\xE8"+struct.pack("<i",target-site-5)
        if pe.at(site,5)!=wanted:
            raise VerificationError("original integer HUD draw call differs at %#x"%site)
    if check_hashes:
        for label,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError("original x87 integer HUD draw SHA256 mismatch: "+label)
    return {
        "native_original_entry":"0x241420D0",
        "original_complete_method_sha256":REGIONS[
            "native_x87_five_to_nine_word_integer_draw_adapter"][2],
        "complete_native_code_regions_verified":len(REGIONS) if check_hashes else 0,
        "partial_original_opcode_witnesses":len(OPCODES),
        "original_direct_call_edges":len(CALLS),
        "external_callers_of_original_function":len(ORIGINAL_EXTERNAL_CALLERS),
        "original_native_stack_words":{"incoming":5,"worker":9},
        "native_return_cleanup_bytes":20,
        "native_worker":"0x24141DD0",
        "original_getters_order":["0x240FFAE0","0x240FFAD0"],
        "rounding_contract":[
            "Original x87 FILD integer extent then FSTP float32 before FISTP conversion",
            "Original FPU CW OR AH,0x0C sets round toward zero for FISTP signed i32",
            "Masked invalid/overflow FISTP writes integer-indefinite 0x80000000",
            "x87 ratio = FILD(primary dimension) / original caller float argument 4",
            "Native original caller third argument is not read in the function body",
            "Output: int(arg1),int(arg2),int(width*ratio),int(height*ratio),0,0,int(width),int(height),raw_arg5",
        ],
        "scope":"Complete native argument computations modeled, but actual 2006 VGUI draw worker ABI/callbacks are not implemented."
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    prior=phase158_audit(client_bytes,server_bytes)
    prior["original_client_x87_integer_draw_adapter"]=verify_original_integer_draw(
        PEImage(client_bytes))
    return prior

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",type=Path,required=True)
    ap.add_argument("--server",type=Path,required=True)
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding="utf-8")
    else:
        print(result,end="")

if __name__=="__main__":
    main()
