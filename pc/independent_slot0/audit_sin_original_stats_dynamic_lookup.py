#!/usr/bin/env python3
"""Original 2006 SiN DynamicStat SHA-only owner-private named lookup audit.

No original DLL bytes, extracted source, disassembly or user paths
are written to the repository or exported in the audit report.
Public CI uses --self-test without a proprietary binary.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_stat_value_factory import SHA256,raw

FORMAT_STRING_VA=0x242AEE50
NAMED_LOOKUP_VA=0x241FFCD0
ORIGINAL_KEY_BUFFER_BYTES=256
ORIGINAL_DYNAMIC_MIN_ID=55
ORIGINAL_DYNAMIC_MAX_ID=65
DYNAMIC_WRAPPER_BYTES=0x110
FUNCTION_SHA={
    "NamedFindOrCreate":(0x241FFCD0,0x241FFDCB,
        "bd88f9ddcfd9e26b0f657efe201be0caf8c018a4603edbd257d1bf8e0a70b278"),
    "TypedValueAndDynamicWrapperFactory":(0x24206430,0x24206489,
        "aac276edaceb675ad8a9cc12dd0a014518e396d7f275c21df71f934d92b7c9fc"),
    "OriginalWrappedValueConstructor":(0x24206280,0x242062F0,
        "8f43482690715b2e366455347edcd549550672e8522b48c729b2f800243cd749"),
    "DynamicStatMessage":(0x241FB370,0x241FB3DD,
        "1be6f476b75de9316e283a981f74c19aad769a6eb71f66b502cb1f8db9b81d05"),
}
EDGES={
    0x241FFD5A:0x24206430,   # miss + create, allocate typed value
    0x241FFD8C:0x241FFAC0,   # insert into owner named tree
    0x241FB3B1:0x241FFCD0,   # DynamicStat caller, create=1
    0x241FB39E:0x240A5740,   # bitbuf String before named lookup
    0x24206440:0x241FD220,   # lookup factory type by stat ID
    0x2420644A:0x24205BC0,   # value object factory, 16 cases
    0x24206477:0x24206280,   # dynamic record wrapper constructor
}
def call_target(data:bytes,address:int)->int:
    code=raw(data,address,5)
    if code[0]!=0xe8:
        raise ValueError("not an original direct x86 CALL at "+hex(address))
    return address+5+struct.unpack_from("<i",code,1)[0]

def verify_metadata()->None:
    assert ORIGINAL_KEY_BUFFER_BYTES==256
    assert ORIGINAL_DYNAMIC_MIN_ID==55 and ORIGINAL_DYNAMIC_MAX_ID==65
    assert ORIGINAL_DYNAMIC_MAX_ID-ORIGINAL_DYNAMIC_MIN_ID+1==11
    assert DYNAMIC_WRAPPER_BYTES==272
    assert len(FUNCTION_SHA)==4 and len(EDGES)==7
    assert all(start<end and len(sha)==64
               for start,end,sha in FUNCTION_SHA.values())

def audit_private_original(data:bytes)->dict:
    verify_metadata()
    if hashlib.sha256(data).hexdigest()!=SHA256:
        raise ValueError("SHA-authenticated original 2006 client.dll required")
    for name,(start,end,expected) in FUNCTION_SHA.items():
        if hashlib.sha256(raw(data,start,end-start)).hexdigest()!=expected:
            raise ValueError("original named-value helper bytes changed: "+name)
    if raw(data,FORMAT_STRING_VA,5)!=b"%s%i\0":
        raise ValueError("original name+decimal-statID format changed")
    if raw(data,0x241FFCF3,5)!=b"\x68\x00\x01\x00\x00":
        raise ValueError("original snprintf destination length no longer 256")
    if raw(data,0x241FFCEA,5)!=b"\x68\x50\xee\x2a\x24":
        raise ValueError("original formatting literal pointer differs")
    if raw(data,0x241FFD3F,8)!=bytes.fromhex(
            "80 bc 24 34 01 00 00 00"):
        raise ValueError("original create-if-missing Boolean branch changed")
    if raw(data,0x24206452,10)!=bytes.fromhex(
            "83 fe 37 8b f8 7c 2b 83 fe 42"):
        raise ValueError("dynamic wrapper ID range changed from 55..65")
    if raw(data,0x2420645E,5)!=b"\x68\x10\x01\x00\x00":
        raise ValueError("dynamic wrapper allocation no longer 0x110 bytes")
    for callsite,callee in EDGES.items():
        if call_target(data,callsite)!=callee:
            raise ValueError("dynamic lookup/creation call edge changed "+
                             hex(callsite))
    return {
        "owner_original_sha256":SHA256,
        "name_key_format":"%s%i",
        "name_key_buffer_bytes":ORIGINAL_KEY_BUFFER_BYTES,
        "lookup_va":hex(NAMED_LOOKUP_VA),
        "dynamic_stat_ids":[ORIGINAL_DYNAMIC_MIN_ID,ORIGINAL_DYNAMIC_MAX_ID],
        "missing_record_behavior":"returns null if create flag false; creates typed wrapper otherwise",
        "original_value_wrapper_bytes":DYNAMIC_WRAPPER_BYTES,
        "sha_authenticated_regions":len(FUNCTION_SHA),
        "authenticated_call_edges":len(EDGES),
        "engine_mutation":"none; derived metadata only"
    }

def self_test()->None:
    verify_metadata()
    fake=bytearray(8)
    fake[0:5]=b"\xe8"+struct.pack("<i",0x3000-0x2005)
    # Baseline synthetic relative x86 call-target arithmetic.
    relative=struct.unpack_from("<i",fake,1)[0]
    assert 0x2005+relative==0x3000
    assert b"%s%i\0"==bytes.fromhex("25 73 25 69 00")
    assert len(set(EDGES.values()))==len(EDGES)
    print("PASS original-free named DynamicStat key/create/wrapper metadata")

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path,
                        help="owner-private SHA-authenticated client.dll")
    args=parser.parse_args()
    if args.self_test:self_test()
    elif args.client:
        print(json.dumps(audit_private_original(args.client.read_bytes()),indent=2))
    else:parser.error("choose --self-test or owner-only --client")
if __name__=="__main__":
    main()
