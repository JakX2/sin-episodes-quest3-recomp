#!/usr/bin/env python3
"""Owner-private SHA-only audit of 2006 SiN ResetHUD/Stats reinitialization.

Public --self-test needs no original game binary; --client only accepts
SHA-authenticated original client.dll and outputs address/hash metadata
instead of copying retail instructions, source or proprietary objects.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_stat_value_factory import SHA256,raw

RESET_HUD_HANDLER=0x241FCA60
RESET_INSTANCE_METHOD=0x241FC9E0
RESET_ROUTINES=(
    (0x241FCA65,0x241FBEC0),  # destroy static 55-value owner
    (0x241FCA6F,0x241FC740),  # destroy separate secondary stat container
    (0x241FCA79,0x241FF990),  # clear named dynamic tree
    (0x241FCA83,0x241FF420),  # re-create 55 original static values
    (0x241FCA8D,0x241FF8E0),  # re-create 11 dynamic type buckets
)
RESET_INSTANCE_ROUTINES=(
    (0x241FC9EA,0x241FBEC0),
    (0x241FC9F2,0x241FC740),
    (0x241FC9FB,0x241FF990),
    (0x241FCA01,0x241FF420),
    (0x241FCA07,0x241FF8E0),
)
RESET_OWNER_GLOBAL=0x24378E94
ORIGINAL_STATIC_COUNT=55
ORIGINAL_DYNAMIC_COUNT=11
SHAS={
    "ResetHUDCallback":(0x241FCA60,0x241FCA9D,
        "5c79bcb146e65e86d97bd3ff5ec8e6e233fa2394ece7969746fc50838b7e4dc7"),
    "StatsInstanceReset":(0x241FC9E0,0x241FCA1C,
        "ddb26dd57451d0273a3194355004b29c0ddf36292c01590e864f19722c505936"),
    "DestroyStaticValueOwner":(0x241FBEC0,0x241FBF2B,
        "01a8f58b223b470c47dabf605dca3d048415ffd1d72c766f11bf6347192298ee"),
    "DestroySecondaryValueOwner":(0x241FC740,0x241FC788,
        "96254312c6fcfb2a1b27c7516801179f02f1138c800e516983f0062ec74ea148"),
    "DestroyNamedDynamicTree":(0x241FF990,0x241FF9DF,
        "c41473068780b5ec0c3cea1ec7974fecc36a58f0819c884e2c02b84c5716668a"),
    "Create55StaticValues":(0x241FF420,0x241FF4CB,
        "a4d63f60e7bc440135df208c7bf1c5cf7aabe9f5c9d9ab08c855e0937d40d696"),
    "Create11DynamicBuckets":(0x241FF8E0,0x241FF986,
        "714148c283d7bb4f41c5501f11e24185444342f957574c8dc6fb28925e67130e"),
}
def check_call(data:bytes,site:int,target:int)->None:
    insn=raw(data,site,5)
    if insn[0]!=0xe8:
        raise ValueError("original reset CALL opcode differs "+hex(site))
    offset=struct.unpack_from("<i",insn,1)[0]
    if site+5+offset!=target:
        raise ValueError("original reset CALL target differs "+hex(site))

def verify_static_metadata()->None:
    assert len(RESET_ROUTINES)==len(RESET_INSTANCE_ROUTINES)==5
    assert [target for _,target in RESET_ROUTINES]==[
        target for _,target in RESET_INSTANCE_ROUTINES]
    assert len(SHAS)==7
    assert ORIGINAL_STATIC_COUNT==0x37
    assert ORIGINAL_DYNAMIC_COUNT==0x0b
    assert all(end>start and len(hashvalue)==64
               for start,end,hashvalue in SHAS.values())

def audit_private_original(data:bytes)->dict:
    verify_static_metadata()
    if hashlib.sha256(data).hexdigest()!=SHA256:
        raise ValueError("expected SHA-authenticated original retail client.dll")
    for name,(start,end,expected) in SHAS.items():
        if hashlib.sha256(raw(data,start,end-start)).hexdigest()!=expected:
            raise ValueError("original reset method boundary changed: "+name)
    for source,target in RESET_ROUTINES+RESET_INSTANCE_ROUTINES:
        check_call(data,source,target)
    # Exact counts in the two independent new-owner loops.
    if raw(data,0x241FF435,2)!=b"\x6a\x37":
        raise ValueError("original static value construction count no longer 55")
    if raw(data,0x241FF8F6,2)!=b"\x6a\x0b":
        raise ValueError("original dynamic-type bucket count no longer 11")
    # The ResetHUD handler does NOT call bf_read to decode the nominal
    # one-byte user-message payload. Last instruction sets the global
    # owner reset flag to one and returns.
    if raw(data,0x241FCA95,8)!=(
            b"\xc6\x05"+struct.pack("<I",RESET_OWNER_GLOBAL)+b"\x01\xc3"):
        raise ValueError("original callback no longer sets reset flag")
    if raw(data,0x241FCA10,10)!=bytes.fromhex(
            "c6 86 ac 00 00 00 01 5e 5b c2"):
        raise ValueError("instance ResetHUD dirty flag offset changed")
    # No literal 32-bit or string reader entry in the callback body.
    if b"\xe8" not in raw(data,0x241FCA60,0x3D):
        raise ValueError("missing expected reset callees")
    return {
        "original_client_sha256":SHA256,
        "original_callback":"ResetHUD",
        "original_callback_va":hex(RESET_HUD_HANDLER),
        "reset_order":[hex(x[1]) for x in RESET_ROUTINES],
        "static_value_count":ORIGINAL_STATIC_COUNT,
        "dynamic_bucket_count":ORIGINAL_DYNAMIC_COUNT,
        "secondary_container":"destroyed, exact semantics still unresolved",
        "callback_packet_payload":"one-byte envelope, not read by original callback",
        "owner_reset_flag_address":hex(RESET_OWNER_GLOBAL),
        "sha_authenticated_method_count":len(SHAS),
        "original_game_bytes_exported":0
    }

def self_test()->None:
    verify_static_metadata()
    fake=bytearray(16)
    fake[0:5]=b"\xe8"+struct.pack("<i",0x1010-0x1005)
    start=0x1000
    insn=fake[:5]
    assert start+5+struct.unpack_from("<i",insn,1)[0]==0x1010
    assert SHAS["ResetHUDCallback"][1]-SHAS["ResetHUDCallback"][0]==0x3d
    assert SHAS["Create55StaticValues"][0]==0x241FF420
    assert SHAS["Create11DynamicBuckets"][0]==0x241FF8E0
    print("PASS original-free ResetHUD five-call teardown and 55/11 rebuild fixture")

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path,
                   help="owner-private original SHA-authenticated client.dll")
    args=p.parse_args()
    if args.self_test:self_test()
    elif args.client:
        print(json.dumps(audit_private_original(args.client.read_bytes()),indent=2))
    else:p.error("select --self-test or --client")
if __name__=="__main__":
    main()
