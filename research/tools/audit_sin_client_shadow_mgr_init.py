#!/usr/bin/env python3
"""Audit private original 2006 SiN client CClientShadowMgr::Init.

Never publishes executable original bytes; validates the full SHA-256,
named literal pointers, native virtual-slot offsets, and conditional
callback. Original input is kept owner-private.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE,CLIENT_SHA256

VA=0x24118880
SIZE=146
SHA256="32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438"
ORIGINAL_BASE=0x24000000
CALLS={
    0x2411889A:0x24167810,
    0x241188F3:0x24118610
}
# These short native witnesses are checked separately from the full
# original method digest, with controlled mutation fixture coverage.
ANCHORS={
    0x24118886:bytes.fromhex("6884d32724"),
    0x2411888B:bytes.fromhex("68147d2924"),
    0x24118893:bytes.fromhex("c686cd00000000"),
    0x241188C0:bytes.fromhex("ff5060"),  # original shadow-object vslot24
    0x241188CC:bytes.fromhex("ff526c"),  # original shadow-object vslot27
    0x241188D8:bytes.fromhex("ff5070"),  # original shadow-object vslot28
    0x241188DB:bytes.fromhex("c786d800000008000000"), # this+0xd8=8
    0x241188E5:bytes.fromhex("8b0dc0813524"),   # conditional provider
    0x241188EB:bytes.fromhex("83792c00"),     # provider+0x2c
    0x241188F8:bytes.fromhex("8b0d9cf63324"), # engine hook interface
    0x24118900:bytes.fromhex("68705e1124"),   # callback original addr
    0x24118905:bytes.fromhex("ff92d0010000"), # virtual slot116
    0x2411890B:bytes.fromhex("b0015e83c40cc3"), # true return
}
FLOATS={
    0x241188A8:0x3dcccccd, # 0.1f
    0x241188B0:0x3dcccccd,
    0x241188B8:0xbf800000, # -1.0f
    0x241188C5:0x42480000, # 50.0f
    0x241188D1:0x3ba3d70a, # 0.005f
}
LITERALS={
    0x2427D384:"Decal textures",
    0x24297D14:"decals/simpleshadow",
}
class AuditError(ValueError):pass
def verify(pe,method_sha=SHA256):
    if pe.base!=ORIGINAL_BASE:
        raise AuditError("Original client image base mismatch")
    if hashlib.sha256(pe.read(VA,SIZE)).hexdigest()!=method_sha:
        raise AuditError("Original shadow Init complete body SHA mismatch")
    for addr,expected in ANCHORS.items():
        if pe.read(addr,len(expected))!=expected:
            raise AuditError("Native shadow Init opcode witness mismatch: %#x"%addr)
    for addr,value in FLOATS.items():
        got=pe.read(addr,7 if addr in (0x241188A8,0x241188B0,0x241188B8) else 5)
        if addr in (0x241188C5,0x241188D1):
            if got!=b"\x68"+struct.pack("<I",value):
                raise AuditError("Native shadow scalar constant mismatch")
        elif got[:3]!=b"\xc7\x44\x24":
            raise AuditError("Native shadow vector store opcode mismatch")
        # Original initial stack store has its immediate at addr+4.
        elif pe.read(addr+4,4)!=struct.pack("<I",value):
            raise AuditError("Native shadow vector constant mismatch")
    for addr,target in CALLS.items():
        expected=b"\xe8"+struct.pack("<i",target-addr-5)
        if pe.read(addr,5)!=expected:
            raise AuditError("Native shadow Init direct call mismatch: %#x"%addr)
    for addr,literal in LITERALS.items():
        if pe.read(addr,len(literal)+1)!=literal.encode("ascii")+b"\0":
            raise AuditError("Native shadow resource literal mismatch: %#x"%addr)
    return {
        "owner_original_client_sha256":CLIENT_SHA256,
        "method":"CClientShadowMgr::Init",
        "va":hex(VA),
        "complete_method_bytes":SIZE,
        "complete_sha256":SHA256,
        "verified_native_witnesses":len(ANCHORS),
        "verified_direct_call_edges":len(CALLS),
        "original_texture":"decals/simpleshadow",
        "texture_category":"Decal textures",
        "own_virtual_slots":[24,27,28],
        "sdk2004_identified_methods":["SetShadowDirection","SetShadowDistance","SetShadowBlobbyCutoffArea"],
        "own_scalars":[50.0,0.005],
        "own_vector":[0.1,0.1,-1.0],
        "conditional_global":"0x243581C0+0x2C",
        "engine_virtual_slot":116,
        "sdk2004_engine_callback":"IMaterialSystem::AddRestoreFunc(ShadowRestoreFunc)",
        "engine_callback_original_va":"0x24115E70",
        "original_true_return":True,
        "limitations":["Original resource helper not independently implemented",
                       "Own virtual slot24/27/28 bodies not reconstructed",
                       "Original shadow callback and engine-side receiver not reconstructed",
                       "No normal VClient011 Init return or gameplay"]
    }
def audit_original(path):
    blob=pathlib.Path(path).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=CLIENT_SHA256:
        raise AuditError("Original client.dll file SHA mismatch")
    return verify(OriginalPE(blob))
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client",help="Owner-original SHA256-pinned client.dll")
    args=parser.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
