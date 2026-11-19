#!/usr/bin/env python3
"""Audit original 2006 client CHudTextureHandle converter and registrar ABI.

Consumes owner-held original client.dll only, enforces entire-file SHA256,
emits metadata and hashes, never emits executable code or copies DLL bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE, CLIENT_SHA256

IMAGE_BASE=0x24000000
REGIONS={
    "property_converter_registration":(0x24009340,101,
        "a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e"),
    "hud_texture_getdata":(0x241EE190,63,
        "5333d07db76f00b568573f0d28dda9b3f5eefff04ba7d2f2423bc66616f142e2"),
    "hud_texture_setdata":(0x241EE1D0,72,
        "068038676bdacc50bddfcf3fc5f969aa80fabe3eb6a8474f9fb56a2e789e1ace"),
    "hud_texture_initfromdefault":(0x241EE220,58,
        "0b7072358a0c49198703a8013b99056fc40d8ccd3030ce13df04739e3a689602")
}
VTABLE_VA=0x242AC530
STATIC_CONVERTER_VA=0x2431DDCC
PROPERTY_TYPE_VA=0x242AC4F8
DUPLICATE_MESSAGE_VA=0x242D3708
ONE_TIME_FLAG_VA=0x24378B28
CALL_VA=0x241EE013
ANCHORS={
    0x241EDFF9:bytes.fromhex("803d288b372400"), # initialized?
    0x241EE00C:bytes.fromhex("c605288b372401"), # flag before registration
    0x241EE002:bytes.fromhex("68ccdd3124"), # converter pointer
    0x241EE007:bytes.fromhex("68f8c42a24"), # literal name
    0x241EE1B7:bytes.fromhex("c20c00"), # GetData 3 params
    0x241EE20C:bytes.fromhex("c20c00"), # SetData 3 params
    0x241EE24E:bytes.fromhex("c20800"), # InitFromDefault 2 params
    0x241EE19A:bytes.fromhex("ff5614"), # entry's offset 0x14 callback
    0x241EE1DB:bytes.fromhex("ff5614"),
    0x241EE22B:bytes.fromhex("ff5614")
}
EXPECTED_LITERALS={
    PROPERTY_TYPE_VA:b"CHudTextureHandle\x00",
    DUPLICATE_MESSAGE_VA:b"Already have converter for type %s, ignoring...\n\x00"
}
class AuditError(ValueError):pass

def verify_layout(pe,regions=REGIONS):
    if pe.base!=IMAGE_BASE:raise AuditError("original image base mismatch")
    for key,(va,size,digest) in regions.items():
        if hashlib.sha256(pe.read(va,size)).hexdigest()!=digest:
            raise AuditError(f"original function SHA mismatch: {key}")
    ptr=lambda va:struct.unpack("<I",pe.read(va,4))[0]
    if ptr(STATIC_CONVERTER_VA)!=VTABLE_VA:
        raise AuditError("static converter object vtable mismatch")
    methods=(REGIONS["hud_texture_getdata"][0],
             REGIONS["hud_texture_setdata"][0],
             REGIONS["hud_texture_initfromdefault"][0])
    for idx,expected in enumerate(methods):
        if ptr(VTABLE_VA+4*idx)!=expected:
            raise AuditError(f"original converter vtable slot {idx} mismatch")
    for va,expected in EXPECTED_LITERALS.items():
        if pe.read(va,len(expected))!=expected:
            raise AuditError(f"original converter literal mismatch: {va:#x}")
    for va,expected in ANCHORS.items():
        if pe.read(va,len(expected))!=expected:
            raise AuditError(f"original converter opcode anchor mismatch: {va:#x}")
    target=REGIONS["property_converter_registration"][0]
    if pe.read(CALL_VA,5)!=b"\xe8"+struct.pack("<i",target-CALL_VA-5):
        raise AuditError("original registrar call edge mismatch")
    return {
        "client":CLIENT_SHA256,
        "original_registered_type":"CHudTextureHandle",
        "registrar":{"va":hex(target),"region_sha256":REGIONS["property_converter_registration"][2],
                     "duplicate_behavior":"warn-and-ignore-existing",
                     "one_time_flag_va":hex(ONE_TIME_FLAG_VA),
                     "flag_set_before_registration":True},
        "converter":{"static_object_va":hex(STATIC_CONVERTER_VA),
            "vtable_va":hex(VTABLE_VA),
            "virtuals":[{"slot":i,"name":name,"va":hex(methods[i]),
                "sha256":REGIONS[key][2],"stdcall_stack_bytes":pop}
                for i,(name,key,pop) in enumerate((
                    ("GetData","hud_texture_getdata",12),
                    ("SetData","hud_texture_setdata",12),
                    ("InitFromDefault","hud_texture_initfromdefault",8)))],
            "entry_lookup_callback_offset":20},
        "bounded_sha_regions":len(regions),
        "opcode_anchors":len(ANCHORS),
        "original_virtual_calls_executed":0,
        "limitations":["no live registration performed","original symbol map and heap not reimplemented",
                       "no complete client Init return or game boot"]
    }

def audit_original(path):
    raw=pathlib.Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA256 mismatch")
    return verify_layout(OriginalPE(raw))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client",help="SHA-authenticated owner original 2006 client.dll")
    args=ap.parse_args()
    print(json.dumps(audit_original(args.client),sort_keys=True,indent=2))
if __name__=="__main__":main()
