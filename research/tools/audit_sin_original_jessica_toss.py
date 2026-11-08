#!/usr/bin/env python3
"""Read-only, SHA-pinned x86 audit of original 2006 Jessica ballistics.

Original SiN game bytes are never embedded, printed, or uploaded. Give this
script your own legally obtained unchanged Windows server.dll.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
WITNESSES={
  "ballistic_helper":(0x2204C850,0x2204C953,"78cef6fd526a309f1dc9b9bacc89cc71d3513f1b74d68f6ac0d376f9ff3da433"),
  "jessica_caller":(0x223624E7,0x22362537,"ab1b737ca83e91416e201ba24c5f778020c82313edc3108b13b8bcd043fdbe41"),
  "sv_gravity_registration":(0x223BF9E0,0x223BFA0A,"954ff5d7164d5f35d0e3bfa4e1a31c1ccc25f135d7515f673d7d22884090219d"),
  "entity_velocity_setter":(0x2214BA20,0x2214BAD1,"5eb87e06301e647f9ab83b4463e940e6bc14b5e78eae08897a5723ad58e5a3f4"),
}
CALLS={
  0x2236246A:0x220EF7E0,  # right-hand attachment
  0x2236248B:0x22144DA0,  # entity creation
  0x2236251A:0x2204C850,  # ballistic helper
  0x22362529:0x2214BA20,  # initial velocity
  0x223BF9F9:0x2225BDA0,  # ConVar registration
}
def verify(data):
    digest=hashlib.sha256(data).hexdigest()
    if digest!=SHA: raise ValueError("original server.dll SHA-256 mismatch")
    pe=PE32(data)
    if pe.base!=0x22000000: raise ValueError("wrong original preferred image base")
    for name,(first,end,expected) in WITNESSES.items():
        if hashlib.sha256(pe.read(first,end-first)).hexdigest()!=expected:
            raise ValueError("original function witness changed: "+name)
    for at,target in CALLS.items():
        if pe.branch_target(at,0xE8)!=target:
            raise ValueError("unexpected original call at "+hex(at))
    # 0x22572DC0 is original sv_gravity ConVar object, with its internal
    # pointer at +0x18. Jessica reads that pointer's float at +0x28.
    if pe.read(0x22445168,11)!=b"sv_gravity\0":
        raise ValueError("gravity cvar name mismatch")
    if pe.read(0x22445174,4)!=b"600\0":
        raise ValueError("gravity registered default mismatch")
    if pe.read(0x223BF9F4,5)!=b"\xb9\xc0\x2d\x57\x22":
        raise ValueError("gravity ConVar object mismatch")
    if pe.read(0x223624ED,9)!=b"\x8b\x15\xd8\x2d\x57\x22\x8b\x42\x28":
        raise ValueError("Jessica gravity field access mismatch")
    if pe.read(0x223624FD,5)!=b"\x68\x00\x00\x70\x41":
        raise ValueError("15.0 arc parameter mismatch")
    if pe.read(0x22362509,6)!=b"\xff\x92\x14\x02\x00\x00":
        raise ValueError("player target virtual call mismatch")
    helper=pe.read(0x2204C850,0x103)
    if helper.count(b"\xd9\xfa")!=2 or b"\xde\xfa" not in helper:
        raise ValueError("x87 sqrt/divide structure mismatch")
    if struct.unpack("<f",pe.read(0x223D2590,4))[0]!=0.0:
        raise ValueError("zero comparison witness mismatch")
    return {"server_sha256":digest,"verified_functions":len(WITNESSES),
            "verified_calls":len(CALLS),"gravity":"sv_gravity",
            "gravity_default":600,"arc_height":15.0,"helper":"0x2204C850"}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    args=p.parse_args()
    try: print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (ValueError,OSError) as e: p.exit(1,"FAIL: "+str(e)+"\n")
if __name__=="__main__": main()
