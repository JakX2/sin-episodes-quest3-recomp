#!/usr/bin/env python3
"""Phase211 owner-original SiN 2006 client snapshot argpack/dispatcher audit.

The original commercial DLLs remain owner-private. This script stores ONLY
entire-method SHA256s, short x86 witnesses, exact VAs and CALL destinations.
No original complete machine-code bodies or assets are published.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage,VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA,BASE

# name: (original entry VA, exact native code bytes, whole body SHA256,
#        following original INT3 padding, disjoint short native witnesses).
METHODS={
 "argpack":(
  0x241978a0,104,
  "13bac9785ff2c2f21cd983d5d911f542538a449f4990bd21ab825a96d24f2c82",
  8,{
   0x00:"8b542408",0x04:"8bc1",0x06:"8b4c2404",0x0a:"895004",
   0x0f:"8908",0x15:"894808",0x1a:"384c240c",0x21:"0f95c2",
   0x2a:"884824",0x2d:"884825",0x33:"89500c",0x38:"384c2414",
   0x3c:"8b4c2428",0x43:"894830",0x46:"895010",0x49:"8a542418",
   0x4d:"885014",0x50:"8a54241c",0x54:"885015",
   0x57:"8a542420",0x5b:"88502c",0x5e:"8a542424",
   0x62:"885016",0x65:"c22800"
  }),
 "dispatch":(
  0x24199ba0,71,
  "9dfd73ea32a62b437e7492038a3b7f01706d6ba62966d162e8b776c897cfeab3",
  9,{
   0x00:"830550db2f2401",0x09:"8b7c2414",0x0d:"807f1000",
   0x11:"8bf1",0x13:"7509",0x1b:"83c404",
   0x1e:"8b442410",0x22:"8b4c240c",0x26:"575051",
   0x29:"8bce",0x30:"8b1550db2f24",0x36:"5752",
   0x38:"8bce",0x3f:"8b4628",0x42:"5f5e",0x44:"c20c00"
  }),
 "record_walk":(
  0x24199b50,65,
  "45490bbfdb993b0b40c255aaf9612d1f1e116d10de5b3c55d220f10a524e5005",
  15,{
   0x00:"53",0x01:"8b5c2408",0x05:"56",0x06:"8b742410",
   0x0a:"8b4604",0x0d:"57",0x0e:"8bf9",0x10:"8b0e",
   0x12:"50515653",0x16:"8bcf",0x1d:"8b760c",0x20:"85f6",
   0x22:"7417",0x24:"8b5604",0x27:"8b06",
   0x29:"52505653",0x2d:"8bcf",0x34:"8b760c",
   0x37:"85f6",0x39:"75e9",0x3b:"5f5e5b",0x3e:"c20800"
  })
}
GLOBAL_COUNTER_VA=0x242fdb50
# Disassembled instruction-boundary-confirmed native CALL rel32s, including
# FOUR separate pairs of original snapshot constructor+dispatch callsites.
CALL_EDGES={
 "savedata_construct":(0x240c5a13,0x241978a0),
 "savedata_dispatch":(0x240c5a34,0x24199ba0),
 "alternate_construct_a":(0x240c6987,0x241978a0),
 "alternate_dispatch_a":(0x240c699f,0x24199ba0),
 "alternate_construct_b":(0x240cc4e3,0x241978a0),
 "alternate_dispatch_b":(0x240cc504,0x24199ba0),
 "alternate_construct_c":(0x24154a42,0x241978a0),
 "alternate_dispatch_c":(0x24154a5a,0x24199ba0),
 "dispatch_preprocess":(0x24199bb6,0x24197f50),
 "dispatch_prepare":(0x24199bcb,0x24199100),
 "dispatch_record_walk":(0x24199bda,0x24199b50),
 "walk_first_record":(0x24199b68,0x241996f0),
 "walk_next_records":(0x24199b7f,0x241996f0)
}
def relative_call(site:int,destination:int)->bytes:
    return b"\xe8"+struct.pack("<i",destination-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase211 original client.dll PE32 image base mismatch")
    opcode_checks=0
    for name,(va,size,digest,padding,witnesses) in METHODS.items():
        if client.section_of(va)!=".text":
            raise VerificationError("Phase211 original native function outside .text: "+name)
        body=client.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase211 complete original native body SHA mismatch: "+name)
        for offset,hx in witnesses.items():
            expected=bytes.fromhex(hx)
            if body[offset:offset+len(expected)]!=expected:
                raise VerificationError("Phase211 original native opcode witness mismatch: "+name+" +%#x"%offset)
            opcode_checks+=1
        if client.at(va+size,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase211 original native RET/INT3 boundary mismatch: "+name)
    if client.section_of(GLOBAL_COUNTER_VA)!=".data":
        raise VerificationError("Phase211 original process-global dispatcher counter not .data")
    if struct.unpack("<I",client.at(0x24199ba2,4))[0]!=GLOBAL_COUNTER_VA:
        raise VerificationError("Phase211 native early ADD global address mismatch")
    if struct.unpack("<I",client.at(0x24199bd2,4))[0]!=GLOBAL_COUNTER_VA:
        raise VerificationError("Phase211 native late MOV global address mismatch")
    for edge,(site,target) in CALL_EDGES.items():
        if client.section_of(site)!=".text" or client.section_of(target)!=".text":
            raise VerificationError("Phase211 call/target outside original .text: "+edge)
        if client.at(site,5)!=relative_call(site,target):
            raise VerificationError("Phase211 original direct CALL rel32 changed: "+edge)
    return {
        "new_sha_verified_client_functions":len(METHODS) if check_hash else 0,
        "new_sha_verified_original_x86_bytes":sum(row[1] for row in METHODS.values()) if check_hash else 0,
        "whole_original_native_function_sha256":{key:value[2] for key,value in METHODS.items()} if check_hash else None,
        "short_original_opcode_witnesses":opcode_checks,
        "exact_original_direct_call_edges":len(CALL_EDGES),
        "original_dispatcher_global_addresses_checked":2,
        "original_stack_argument_constructor_size":0x34,
        "native_dispatcher_global_counter":hex(GLOBAL_COUNTER_VA),
        "native_record_next_byte_offset":"0x0C",
        "native_record_flag_byte_offset":"0x10",
        "external_record_serializer_entry":"0x241996F0",
        "limits":[
            "Native 241996F0 record serialization implementation is not recovered",
            "Host-only linked-list cycle guards and token lookups are not native x86 semantics",
            "Four callsite pairs do not identify original source-level C++ classes",
            "No real retail Windows engine process executing replacement gameplay modules"
        ]
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    if hashlib.sha256(server_bytes).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client_bytes).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase211 requires BOTH complete SHA256-matched owner-original game DLLs")
    return verify_native(PEImage(client_bytes))

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    options=parser.parse_args()
    report=json.dumps(audit(options.server.read_bytes(),
                            options.client.read_bytes()),indent=2)+"\n"
    if options.output:
        options.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")

if __name__=="__main__":
    main()
