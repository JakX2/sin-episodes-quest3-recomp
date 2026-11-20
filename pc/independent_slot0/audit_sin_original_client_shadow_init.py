#!/usr/bin/env python3
"""Phase 490: SHA-authenticated original CClientShadowMgr::Init call evidence.

Owner private --client/--archive checks exact 146-byte method and
original vtable+literal strings. Public --self-test builds fabricated
code regions. Never exports original DLL bytes or invokes renderer.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

INIT_VA=0x24118880
INIT_SIZE=146
INIT_SHA="32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438"
VTABLE=0x24297C4C
MATERIAL_VA=0x24297D14
MATERIAL_NAME="decals/simpleshadow"
CATEGORY_VA=0x2427D384
CATEGORY_NAME="Decal textures"
MATERIAL_HELPER_VA=0x24167810
CONDITIONAL_HELPER_VA=0x24118610
CALLBACK_VA=0x24115E70
MARKERS=(
    (0x24118880,"83ec0c"),
    (0x24118886,"6884d32724"),
    (0x2411888B,"68147d2924"),
    (0x24118893,"c686cd00000000"),
    (0x2411889A,"e871ef0400"),
    (0x241188A8,"c7442408cdcccc3d"),
    (0x241188B0,"c744240ccdcccc3d"),
    (0x241188B8,"c7442410000080bf"),
    (0x241188C0,"ff5060"),        # this-vtable byte offset +0x60
    (0x241188C5,"6800004842"),    # exact float32 bits for 50.0
    (0x241188CC,"ff526c"),        # this-vtable +0x6C
    (0x241188D1,"680ad7a33b"),    # exact float32 bits for 0.005
    (0x241188D8,"ff5070"),        # this-vtable +0x70
    (0x241188DB,"c786d800000008000000"),
    (0x241188E5,"8b0dc0813524"), # source external object
    (0x241188EB,"83792c00"),     # conditional DWORD +0x2C
    (0x241188EF,"7407"),
    (0x241188F3,"e818fdffff"),   # optional helper
    (0x241188F8,"8b0d9cf63324"),# source callback global
    (0x24118900,"68705e1124"),   # callback pointer
    (0x24118905,"ff92d0010000"),# external virtual +0x1D0
    (0x2411890B,"b001"),         # original returns true
)
CALLS=(
    (0x2411889A,MATERIAL_HELPER_VA),
    (0x241188F3,CONDITIONAL_HELPER_VA),
)
def audit_method(method,verify_hash=True):
    if len(method)!=INIT_SIZE:
        raise ValueError("original shadow Init byte length differs")
    if verify_hash and hashlib.sha256(method).hexdigest()!=INIT_SHA:
        raise ValueError("original ShadowMgr Init whole-method SHA mismatch")
    for va,raw in MARKERS:
        op=bytes.fromhex(raw)
        at=va-INIT_VA
        if method[at:at+len(op)]!=op:
            raise ValueError("original ShadowMgr instruction witness mismatch at %#x"%va)
    for va,target in CALLS:
        offset=va-INIT_VA
        code=b"\xe8"+struct.pack("<i",target-va-5)
        if method[offset:offset+5]!=code:
            raise ValueError("original ShadowMgr helper CALL differs at %#x"%va)
    return {
        "original_shadow_init_method_sha_verified":bool(verify_hash),
        "original_method_bytes":INIT_SIZE,
        "original_virtual_this_slots":[24,27,28],
        "external_callback_virtual_slot":116,
        "original_shadow_material":MATERIAL_NAME,
        "original_material_category":CATEGORY_NAME,
        "original_shadow_setup_float_bits":
            ["0x3DCCCCCD","0x3DCCCCCD","0xBF800000",
             "0x42480000","0x3BA3D70A"],
        "original_flag_byte_cd_zero":True,
        "original_dword_d8_eight":True,
        "conditional_original_helper_va":hex(CONDITIONAL_HELPER_VA),
        "registered_callback_original_va":hex(CALLBACK_VA),
        "original_return_true":True,
        "real_renderer_or_callback_invoked":False
    }
def private_audit(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("original SiN 2006 whole-client SHA differs")
    pe=parse_image(client)
    if pe[0]!=0x24000000:
        raise ValueError("original client image base unexpected")
    if struct.unpack("<I",region_at_va(client,pe,VTABLE,4))[0]!=INIT_VA:
        raise ValueError("original ShadowMgr vtable slot0 does not match Init")
    for va,raw in ((MATERIAL_VA,MATERIAL_NAME),(CATEGORY_VA,CATEGORY_NAME)):
        data=region_at_va(client,pe,va,len(raw)+1)
        if data!=raw.encode("ascii")+b"\0":
            raise ValueError("original ShadowMgr string constant mismatch")
    result=audit_method(region_at_va(client,pe,INIT_VA,INIT_SIZE))
    result["original_client_sha_verified"]=True
    result["original_vtable_and_string_refs_verified"]=True
    return result
def self_test():
    synthetic=bytearray(b"\xcc"*INIT_SIZE)
    for va,raw in MARKERS:
        op=bytes.fromhex(raw)
        at=va-INIT_VA
        synthetic[at:at+len(op)]=op
    actual=audit_method(synthetic,False)
    assert actual["original_virtual_this_slots"]==[24,27,28]
    for va in (0x24118893,0x2411889A,0x241188C0,0x241188CC,
               0x241188D8,0x241188DB,0x241188EB,0x24118905,
               0x2411890B):
        changed=bytearray(synthetic)
        changed[va-INIT_VA]^=0x01
        try:audit_method(changed,False)
        except ValueError:pass
        else:raise ValueError("corrupt synthetic ShadowMgr original evidence accepted")
    return {
        "original_free_shadow_init_audit":"PASS",
        "selected_source_instruction_witnesses":len(MARKERS),
        "two_direct_original_rel32_calls_checked":True,
        "no_retail_dll_or_game_assets_used":True
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument("--self-test",action="store_true")
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    a=p.parse_args()
    if a.self_test:result=self_test()
    elif a.client:result=private_audit(a.client.read_bytes())
    else:
        with zipfile.ZipFile(a.archive) as z:
            result=private_audit(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original ShadowMgr Init audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
