#!/usr/bin/env python3
"""Phase1659: audit original CPrediction slots8/13 host value/view reads.

Exact original PE32 owner-only original client.dll+engine.dll SHA pinned via
the previous Phase1658 prediction23 ABI audit, then authenticate two MORE
original complete functions by whole native SHA and structural operands.
Public --self-test uses invented x86 windows and mutations only. This
read-only research never loads original game code or engine state.

No original client.dll/server.dll/engine.dll bytes may be added to GitHub.
No production VClientPrediction001 CreateInterface registration is enabled.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE, CLIENT_VIRTUAL_TARGETS, verify_original,
    check, section_read,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

HOST_GLOBAL_VA = 0x24338CF0
# Global host used by these two methods is *not* cl_predict; the exact
# native class identity/engine lifetime is not reconstructed here.
HOST_MEMBER_FLAG_OFFSET = 0x1DC
HOST_MEMBER_VIEW_VECTOR_OFFSET = 0xF04
SLOT8 = 8
SLOT13 = 13
SLOT8_VA = 0x24195DD0
SLOT8_LENGTH = 17
SLOT8_SHA = "a96aab9ac13d897688080ecbf8ae113f179c813ea30a0723237ec3899b93ff26"
SLOT13_VA = 0x24195F10
SLOT13_LENGTH = 59
SLOT13_SHA = "702860c2c345ff06d4534b1a1f2d820f0d098d994f2662be466b027813c0f40f"
SLOT8_STACK_POP = 0
SLOT13_STACK_POP = 4
SLOT13_VECTOR_DWORDS = 3

def dword(blob: bytes, pos: int = 0) -> int:
    return struct.unpack_from("<I",blob,pos)[0]

def pack(value: int) -> bytes:
    return struct.pack("<I",value)

def verify_slot8(blob: bytes, host: int) -> None:
    check(len(blob)==SLOT8_LENGTH and
          blob[0]==0xA1 and dword(blob,1)==host,
          "slot8 does not read exact original host singleton")
    check(blob[5:10]==bytes.fromhex("85c07501c3") and
          blob[10:12]==bytes.fromhex("8b80") and
          dword(blob,12)==HOST_MEMBER_FLAG_OFFSET and
          blob[16]==0xC3,
          "slot8 not conditional host+0x1DC DWORD / null zero / RET0")

def verify_slot13(blob: bytes, host: int) -> None:
    check(len(blob)==SLOT13_LENGTH and
          blob[0]==0xA1 and dword(blob,1)==host,
          "slot13 does not read original host singleton")
    # Original null branch uses its caller's OUT pointer and zeroes
    # ALL three output DWORDs before returning RET 4.
    check(blob[5:11]==bytes.fromhex("33c93bc1750f") and
          blob[11:26]==bytes.fromhex(
              "8b4424048908894804894808c20400"),
          "slot13 original null-host branch not full 3-word zero")
    check(blob[26:28]==bytes.fromhex("8b90") and
          dword(blob,28)==HOST_MEMBER_VIEW_VECTOR_OFFSET and
          blob[32:38]==bytes.fromhex("8b4c24048911") and
          blob[38:40]==bytes.fromhex("8b90") and
          dword(blob,40)==HOST_MEMBER_VIEW_VECTOR_OFFSET+4 and
          blob[44:47]==bytes.fromhex("895104") and
          blob[47:49]==bytes.fromhex("8b80") and
          dword(blob,49)==HOST_MEMBER_VIEW_VECTOR_OFFSET+8 and
          blob[53:56]==bytes.fromhex("894108") and
          blob[56:59]==bytes.fromhex("c20400"),
          "slot13 does not copy precisely three host +0xF04 DWORDs "
          "to caller's OUT pointer and return RET4")

def audit(client_bytes: bytes,engine_bytes: bytes) -> dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "prior SHA-authenticated prediction vtable is not 23 slots")
    check(CLIENT_VIRTUAL_TARGETS[SLOT8]==SLOT8_VA and
          CLIENT_VIRTUAL_TARGETS[SLOT13]==SLOT13_VA,
          "original prediction vtable slot8/13 identities changed")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and pe.section_of(HOST_GLOBAL_VA)==".data",
          "original prediction host singleton not in client .data")
    reloc=pe_highlow_relocations(pe)
    for slot,address,size,digest,checker in (
        (8,SLOT8_VA,SLOT8_LENGTH,SLOT8_SHA,verify_slot8),
        (13,SLOT13_VA,SLOT13_LENGTH,SLOT13_SHA,verify_slot13),
    ):
        method=section_read(pe,address,size,".text")
        check(hashlib.sha256(method).hexdigest()==digest,
              "entire native prediction method SHA mismatch slot "+str(slot))
        checker(method,HOST_GLOBAL_VA)
        check(address+1 in reloc,
              "original host global instruction lacks HIGHLOW relocation "
              +str(slot))
        # Method ends immediately before compiler padding. Padding
        # validates extent but is not mistaken for executable method.
        check(section_read(pe,address+size,2,".text")==b"\xCC\xCC",
              "native prediction method unexpectedly overlaps next code")
    return {
        "phase": 1659,
        "provenance": "full-original-client-and-engine-SHA-gated-in-Phase1658",
        "original_prediction_slots_authenticated": [SLOT8,SLOT13],
        "original_complete_additional_native_method_bytes":
            SLOT8_LENGTH+SLOT13_LENGTH,
        "original_slot8_method_sha256": SLOT8_SHA,
        "original_slot13_method_sha256": SLOT13_SHA,
        "original_shared_host_global_va": hex(HOST_GLOBAL_VA),
        "slot8_host_int_member": hex(HOST_MEMBER_FLAG_OFFSET),
        "slot8_noarg_returns_zero_when_host_absent": True,
        "slot8_thiscall_stack_pop_bytes": SLOT8_STACK_POP,
        "slot13_out_pointer_dwords": SLOT13_VECTOR_DWORDS,
        "slot13_host_vector_start_member": hex(HOST_MEMBER_VIEW_VECTOR_OFFSET),
        "slot13_null_host_writes_zero_vector": True,
        "slot13_thiscall_stack_pop_bytes": SLOT13_STACK_POP,
        "independent_host_view_callback_is_synthetic": True,
        "retail_original_native_methods_executed": 0,
        "replacement_prediction_interface_registered": False,
        "windows_game_startup_verified": False
    }

def reject(fn,description: str) -> None:
    try: fn()
    except ValueError: return
    raise AssertionError("wrong synthetic original-view ABI was accepted: "+
                         description)

def self_test() -> None:
    # Invented absolute pointer, NOT retail code or machine-code copied
    # from original PE32 file; the instruction identities are structural.
    fake_host=0x55600000
    slot8=(b"\xA1"+pack(fake_host)+bytes.fromhex("85c07501c3")+
           bytes.fromhex("8b80")+pack(HOST_MEMBER_FLAG_OFFSET)+b"\xC3")
    check(len(slot8)==17,"invented slot8 extent differs")
    verify_slot8(slot8,fake_host)
    for i in range(len(slot8)):
        broken=bytearray(slot8);broken[i]^=1
        reject(lambda data=bytes(broken):verify_slot8(data,fake_host),
               "slot8 x86 byte "+str(i))
    slot13=(b"\xA1"+pack(fake_host)+bytes.fromhex(
        "33c93bc1750f8b4424048908894804894808c20400")+
        bytes.fromhex("8b90")+pack(HOST_MEMBER_VIEW_VECTOR_OFFSET)+
        bytes.fromhex("8b4c24048911")+bytes.fromhex("8b90")+
        pack(HOST_MEMBER_VIEW_VECTOR_OFFSET+4)+bytes.fromhex("895104")+
        bytes.fromhex("8b80")+pack(HOST_MEMBER_VIEW_VECTOR_OFFSET+8)+
        bytes.fromhex("894108c20400"))
    check(len(slot13)==59,"invented slot13 method extent differs")
    verify_slot13(slot13,fake_host)
    for i in range(len(slot13)):
        broken=bytearray(slot13);broken[i]^=1
        reject(lambda data=bytes(broken):verify_slot13(data,fake_host),
               "slot13 x86 byte "+str(i))
    for fn,data in ((verify_slot8,slot8),(verify_slot13,slot13)):
        reject(lambda f=fn,b=data:f(b,0x44444444),
               "forged host global accepted")
        reject(lambda f=fn,b=data:f(b[:-1],fake_host),
               "truncated original method accepted")
        reject(lambda f=fn,b=data:f(b+b"\xCC",fake_host),
               "original method plus padding accepted")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "retail original SHA bypass")
    print("PASS Phase1659 original-free synthetic: SHA-pinned parent "
          "prediction23, slot8 host+0x1DC optional DWORD RET0, "
          "slot13 host+0xF04 triple-DWORD OUT RET4, null-host "
          "zero vector, byte-level corruption/extent/false SHA refusal")

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be mixed with retail DLL paths")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),sort_keys=True,indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
