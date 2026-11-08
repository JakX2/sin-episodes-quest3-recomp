#!/usr/bin/env python3
"""Read-only SHA-pinned audit of SiN Episodes 2006 additive entity velocity."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_jessica_throw import PE32

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
START=0x2214BA20
END=0x2214BAD1
FUNCTION_SHA="5eb87e06301e647f9ab83b4463e940e6bc14b5e78eae08897a5723ad58e5a3f4"
CALLS={
    0x2214BA92:0x2213D750,  # dirty absolute velocity refresh
    0x2214BAC4:0x22141A00,  # set absolute velocity after component add
    0x22362529:0x2214BA20,  # Jessica's animation throw caller
}
def verify(data):
    if hashlib.sha256(data).hexdigest()!=SERVER_SHA:
        raise ValueError("original 2006 server.dll identity mismatch")
    pe=PE32(data)
    if pe.base!=0x22000000:
        raise ValueError("unexpected PE32 preferred image base")
    raw=pe.read(START,END-START)
    if hashlib.sha256(raw).hexdigest()!=FUNCTION_SHA:
        raise ValueError("full original velocity impulse function differs")
    # Recheck control-flow anchors, not just a lookalike named function.
    checks={
        0x2214BA2D: bytes.fromhex("d905b8855822"), # shared 3-float sentinel
        0x2214BA60: bytes.fromhex("83be0402000006"), # move type==6
        0x2214BA74: bytes.fromhex("ff90b0000000"), # physics vtable +0xB0
        0x2214BA82: bytes.fromhex("8b8eb8000000c1e90cf6c101"), # bit12
        0x2214BA9D: bytes.fromhex("d88650020000"), # +0x250
        0x2214BAB0: bytes.fromhex("d84704"), # second component
        0x2214BABD: bytes.fromhex("d84708"), # third component
        0x2214BACE: bytes.fromhex("c20400"), # ret +4
    }
    for addr,payload in checks.items():
        if pe.read(addr,len(payload))!=payload:
            raise ValueError("instruction witness mismatch at "+hex(addr))
    for at,target in CALLS.items():
        if pe.branch_target(at,0xE8)!=target:
            raise ValueError("call target mismatch at "+hex(at))
    if pe.read(0x2214BA69,6)!=bytes.fromhex("8b8e18020000"):
        raise ValueError("physics object +0x218 pointer mismatch")
    if pe.read(0x2214BA97,2)!=b"\xd9\x07":
        raise ValueError("non-physics accumulation missing")
    return {
        "original_server_sha256":SERVER_SHA,
        "original_velocity_impulse_va":hex(START),
        "function_sha256":FUNCTION_SHA,
        "instructions_checked":len(checks)+2,
        "calls_checked":len(CALLS),
        "mode6_physics_interface_vtable_byte_offset":0xB0,
        "other_mode_absolute_velocity_add":True,
        "original_binary_shipped":False
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    args=p.parse_args()
    try:
        print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (ValueError,OSError) as e:
        p.exit(1,"FAIL: "+str(e)+"\n")
if __name__=="__main__":
    main()
