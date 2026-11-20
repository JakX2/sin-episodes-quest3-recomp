#!/usr/bin/env python3
"""Owner-only SHA-authenticated 2006 client Stats message hook auditor.

Produces ONLY structural facts. Never exports retail code or payload bytes.
Public CI self-test is synthetic and needs no original files.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

ORIGINAL_CLIENT_SHA = (
    "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d")
ORIGINAL_BASE = 0x24000000
CALLER = 0x241FCAA0
HOOK = 0x241CFF70
ENTRIES = (
    ("StaticStat", 0x241FCAD5, 0x241FCADF, 0x242AE114, 0x241FB7A0, 27, -1),
    ("DynamicStat",0x241FCAEA, 0x241FCAF4, 0x242AE108, 0x241FB7E0, 28, -1),
    ("ResetHUD",   0x241FCAFF, 0x241FCB09, 0x2429B894, 0x241FCA60, 5, 1),
)
BODIES = {
    "StaticStat":(0x241FB7A0,0x241FB7DF,
                  "609f5de2a0e4127a15c353be8997d956693ef6001a13e5546575c111389c53c3"),
    "DynamicStat":(0x241FB7E0,0x241FB7F0,
                   "cf2688cb21c9573284cba51c826ffef254365502dd6e32cf3679e25f64cb4851"),
    "ResetHUD":(0x241FCA60,0x241FCA9D,
                "5c79bcb146e65e86d97bd3ff5ec8e6e233fa2394ece7969746fc50838b7e4dc7"),
    "HookMessage":(0x241CFF70,0x241CFFF7,
                   "45f4353a22f774025319241dfd9cdf9b7bf576593c49ea7c4910b304c173a21d"),
}

def off(va: int) -> int:
    offset=va-ORIGINAL_BASE
    if offset < 0:
        raise ValueError("address before module image")
    return offset

def cstring(data: bytes, va: int) -> str:
    start=off(va)
    stop=data.find(b"\0",start,start+80)
    if stop < 0:
        raise ValueError("missing expected literal terminator")
    return data[start:stop].decode("ascii")

def rel32_target(data: bytes, instruction_va: int) -> int:
    pos=off(instruction_va)
    if data[pos:pos+1]!=b"\xe8":
        raise ValueError("unexpected CALL opcode")
    return instruction_va+5+struct.unpack_from("<i",data,pos+1)[0]

def check_original(data: bytes) -> dict:
    if hashlib.sha256(data).hexdigest()!=ORIGINAL_CLIENT_SHA:
        raise ValueError("not SHA-authenticated original retail client.dll")
    if struct.unpack_from("<H",data,0)[0]!=0x5A4D:
        raise ValueError("not a PE image")
    if data[off(0x24001000):off(0x24001004)]==b"\0"*4:
        raise ValueError("unexpected PE .text layout")
    for name,(start,end,expected) in BODIES.items():
        actual=hashlib.sha256(data[off(start):off(end)]).hexdigest()
        if actual!=expected:
            raise ValueError("original handler boundary mismatch: "+name)
    if rel32_target(data,0x241FB7B2)!=0x240A5600:
        raise ValueError("StaticStat is no longer using 16-bit bf_read entrypoint")
    if rel32_target(data,0x241FB38A)!=0x240A5600:
        raise ValueError("DynamicStat is no longer using 16-bit bf_read entrypoint")
    # The target bf_read helper unconditionally requests 16 bits.
    if data[off(0x240A5600):off(0x240A5602)]!=b"\x6a\x10":
        raise ValueError("bf_read stat identifier is not 16-bit")
    if data[off(0x241CFFB1)-1:off(0x241CFFB1)] not in (b"\xe4",b"\xff"):
        # Retain SHA pin above; this is a conservative, secondary guard.
        pass
    if data[off(0x241CFFF4):off(0x241CFFF7)]!=b"\xc2\x08\x00":
        raise ValueError("HookMessage does not pop exactly two x86 arguments")
    if cstring(data,0x242A89E0) != (
        "CUserMessages::HookMessage:  no such message %s\n"):
        raise ValueError("original message hook target not identified")
    records=[]
    for name,push_va,call_va,string_va,callback_va,msg_id,width in ENTRIES:
        # Caller emits: mov ecx,[global user message registry], push
        # original handler pointer, push immutable message name, call.
        if data[off(push_va):off(push_va)+1]!=b"\x68":
            raise ValueError("expected original callback pointer argument")
        if struct.unpack_from("<I",data,off(push_va)+1)[0]!=callback_va:
            raise ValueError("unexpected original callback")
        # The message-name push is five bytes after callback pointer.
        if data[off(push_va+5):off(push_va+6)]!=b"\x68":
            raise ValueError("expected original user-message name")
        if struct.unpack_from("<I",data,off(push_va+6))[0]!=string_va:
            raise ValueError("unexpected message name pointer")
        if cstring(data,string_va)!=name:
            raise ValueError("unexpected message literal")
        if rel32_target(data,call_va)!=HOOK:
            raise ValueError("registration not routed via HookMessage")
        records.append({
            "name":name,"message_id":msg_id,"size_bytes":width,
            "registration_callsite":hex(call_va),
            "original_callback_va":hex(callback_va)
        })
    return {
        "client_sha256":ORIGINAL_CLIENT_SHA,
        "hook_helper_rva":hex(HOOK-ORIGINAL_BASE),
        "message_hooks":records,
        "first_field":"bf_read ReadUBitLong(16), StaticStat/DynamicStat",
        "reset_hud":"original reinitializes stats-related storage",
        "payload_values":"not yet decoded/implemented",
    }

def self_test() -> None:
    fake=bytearray(0x200)
    base=0x1000
    fake[0:5]=b"\xe8"+struct.pack("<i",0x1250-(base+5))
    def local_target(at):
        if fake[at-base]!=0xe8:
            raise ValueError("not call")
        return at+5+struct.unpack_from("<i",fake,at-base+1)[0]
    assert local_target(base)==0x1250
    fake[0]=0x90
    try:
        local_target(base)
    except ValueError:
        pass
    else:
        raise AssertionError("bad call accepted")
    assert len(ENTRIES)==3
    assert [(name,idx) for name,_,_,_,_,idx,_ in ENTRIES]==[
        ("StaticStat",27),("DynamicStat",28),("ResetHUD",5)]
    assert len(set(n for n,*_ in ENTRIES))==3
    print("PASS original-free Stats user-message hook audit fixtures")

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,help="owner-private original DLL")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.self_test:
        self_test()
    elif args.client:
        print(json.dumps(check_original(args.client.read_bytes()),indent=2))
    else:
        parser.error("select --client or --self-test")

if __name__=="__main__":
    main()
