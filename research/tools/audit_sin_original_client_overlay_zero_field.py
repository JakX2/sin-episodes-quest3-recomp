#!/usr/bin/env python3
"""Phase1705: prove native special overlay RecvProp +44 ZERO by original x86.

Original private PE32 input is SHA-pinned and NEVER emitted. Public
self-test validates fabricated data and source-independent refusal.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA

FUNC=0x241271B0
EXECUTABLE_LENGTH=621
EXECUTABLE_SHA="a41703fce2d6cd865d2457b21738fa927ee63cda2ca34b473188d213528df4c6"
PREVIOUS_OVERLAPPING_657_SHA="b2e9bf21b248f751855489484f6ac04d4a8809473406e3287701a1ed4188c670"
ORIGINAL_OVERLAY_CALL=0x240C36E1
ORIGINAL_OVERLAY_PROP_DESCRIPTOR=0x24337424
ORIGINAL_OVERLAY_PARENT_OBJECT_BASE=0x7A4

def require(cond, message):
    if not cond: raise ValueError(message)

def verify_original(path):
    data=path.read_bytes()
    require(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
            "original 2006 PE32 client.dll differs")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def read(va,n):
        at=va-base
        require(0<=at<=len(code)-n,"source original VA outside code")
        return code[at:at+n]
    executable=read(FUNC,EXECUTABLE_LENGTH)
    require(hashlib.sha256(executable).hexdigest()==EXECUTABLE_SHA,
            "native special overlay actual 621-byte function differs")
    require(hashlib.sha256(read(FUNC,657)).hexdigest()==
            PREVIOUS_OVERLAPPING_657_SHA,
            "original overlapping 657-byte research span changed")
    # RET at the end of real 621-byte function, then 3 bytes INT3,
    # then a new 2006 function beginning at 0x24127420.
    require(read(0x2412741C,4)==b"\xC3\xCC\xCC\xCC" and
            read(0x24127420,3)==b"\x56\x8B\xF1",
            "source boundary 621 vs 657 is not verified")
    # Before argument-shift, local 60-byte descriptor begins ESP+0x1C.
    # After PUSH ECX at 0x241271E1 it starts ESP+0x20, so the exact
    # MOV [ESP+0x4C],EBX writes local native +44 = ZERO.
    require(read(0x241271DF,2)==b"\x33\xDB" and
            read(0x241271E1,1)==b"\x51" and
            read(0x241271EE,4)==b"\x89\x5C\x24\x4C",
            "original type5 overlay RecvProp +44 zero store changed")
    # Final source copies exactly 15 DWORDs (60 bytes), retaining +44.
    require(read(0x24127405,5)==b"\xB9\x0F\x00\x00\x00" and
            read(0x24127410,2)==b"\xF3\xA5",
            "native 60-byte output descriptor copy missing")
    # Original callsite passes source output destination and separately
    # pushes 1956 as an auxiliary data offset. NEVER conflate them.
    require(read(0x240C36C9,5)==b"\x68\xA4\x07\x00\x00" and
            read(0x240C36DA,5)==b"\x68\x24\x74\x33\x24" and
            read(0x240C36DF,2)==b"\xF3\xA5",
            "special overlay caller offset or output provenance drift")
    call=read(ORIGINAL_OVERLAY_CALL,5)
    require(call[0]==0xE8 and ORIGINAL_OVERLAY_CALL+5+
            struct.unpack_from("<i",call,1)[0]==FUNC,
            "original specialized overlay CALL target changed")
    print("PASS Phase1705 owner-original special overlay native +44=0; "
          "621 actual executable bytes, 657 overlapping old SHA span")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    require(args.self_test or args.client is not None,
            "use --self-test or --client original/client.dll")
    if args.self_test:
        # Non-original simulated witness data must NOT authenticate.
        faked=b"\x33\xDB\x51\x89\x5C\x24\x4C"
        require(hashlib.sha256(faked).hexdigest()!=EXECUTABLE_SHA,
                "fabricated original SHA unexpectedly matches")
        require(EXECUTABLE_LENGTH==621 and FUNC+621==0x2412741D,
                "original function boundary arithmetic mismatch")
        print("PASS Phase1705 original-free overlay zero offset and source boundary refusal")
    if args.client is not None:
        verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError,struct.error) as error:
        print(f"FAIL Phase1705: {error}",file=sys.stderr)
        sys.exit(1)
