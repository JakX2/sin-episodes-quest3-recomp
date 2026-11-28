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

# Authenticating the global object identity: local C_BasePlayer.
# The global is assigned during the player's local-index update, and
# cleared in its destructor if it still names the same object. The
# installed primary MSVC vptr identifies C_BasePlayer via RTTI.
PLAYER_ASSIGN_VA = 0x240D5643
PLAYER_ASSIGN_LENGTH = 16
PLAYER_ASSIGN_SHA = "fe3bf68710202428d5d1e532b8587b6ad369904304687eee3ffe8688f130eb30"
PLAYER_CLEAR_VA = 0x240D5D15
PLAYER_CLEAR_LENGTH = 18
PLAYER_CLEAR_SHA = "dfed227d1d7cfafdca77fe728effea9ff1a7b7b74fa7a80aff07d5b314beec10"
PLAYER_DTOR_VPTR_STORE_VA = 0x240D5CC0
PLAYER_DTOR_VPTR_STORE_LENGTH = 20
PLAYER_DTOR_VPTR_STORE_SHA = "81107a35df10f0877456e77ae794c464731489a81dad7265c628c257af8a53b7"
PLAYER_PRIMARY_VTABLE_VA = 0x2428AAE4
PLAYER_PRIMARY_RTTI_PTR_VA = PLAYER_PRIMARY_VTABLE_VA - 4
PLAYER_RTTI_COL_VA = 0x242E2C7C
PLAYER_RTTI_TYPE_VA = 0x242F93C8
PLAYER_RTTI_CHD_VA = 0x242E2C6C
PLAYER_RTTI_TYPE_NAME = b".?AVC_BasePlayer@@\0"


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

def verify_player_assignment_window(blob: bytes, host: int) -> None:
    check(len(blob)==PLAYER_ASSIGN_LENGTH and
          blob[:10]==bytes.fromhex("3947488d77f8752bb001") and
          blob[10:12]==bytes.fromhex("8935") and dword(blob,12)==host,
          "local C_BasePlayer assignment not guarded EDI-8 -> global pointer")


def verify_player_clear_window(blob: bytes, host: int) -> None:
    check(len(blob)==PLAYER_CLEAR_LENGTH and
          blob[:2]==bytes.fromhex("3b35") and
          dword(blob,2)==host and
          blob[6:10]==bytes.fromhex("750ac705") and
          dword(blob,10)==host and blob[14:18]==b"\0"*4,
          "local player destructor does not compare then clear host singleton")


def verify_player_dtor_vptr(blob: bytes, vtable: int) -> None:
    check(len(blob)==PLAYER_DTOR_VPTR_STORE_LENGTH and
          blob[:5]==bytes.fromhex("568bf1c706") and
          dword(blob,5)==vtable,
          "C_BasePlayer destructor did not reinstall correct primary vptr")


def verify_player_rtti_col(blob: bytes, type_descriptor: int,
                           hierarchy: int) -> None:
    check(len(blob)==20 and struct.unpack("<IIIII",blob) ==
          (0,0,0,type_descriptor,hierarchy),
          "owner object not C_BasePlayer primary RTTI zero-offset hierarchy")


def verify_original_player_global_owner(pe: PEImage,
                                       relocated: set[int]) -> None:
    """Identify the host global as original C_BasePlayer, not a guess.

    The writer stores ESI=EDI-8 only after checking a local-client-index
    condition, and the destructor resets the global only if it still
    points to that same C_BasePlayer. The installed vptr carries an
    original MSVC COL named C_BasePlayer, primary offset zero.
    """
    assign=section_read(pe,PLAYER_ASSIGN_VA,PLAYER_ASSIGN_LENGTH,".text")
    check(hashlib.sha256(assign).hexdigest()==PLAYER_ASSIGN_SHA,
          "local player conditional writer whole x86 window SHA differs")
    verify_player_assignment_window(assign,HOST_GLOBAL_VA)
    clear=section_read(pe,PLAYER_CLEAR_VA,PLAYER_CLEAR_LENGTH,".text")
    check(hashlib.sha256(clear).hexdigest()==PLAYER_CLEAR_SHA,
          "local player destructor clear window whole SHA differs")
    verify_player_clear_window(clear,HOST_GLOBAL_VA)
    dtor=section_read(pe,PLAYER_DTOR_VPTR_STORE_VA,
                      PLAYER_DTOR_VPTR_STORE_LENGTH,".text")
    check(hashlib.sha256(dtor).hexdigest()==PLAYER_DTOR_VPTR_STORE_SHA,
          "original C_BasePlayer deleting/destructor vptr window SHA differs")
    verify_player_dtor_vptr(dtor,PLAYER_PRIMARY_VTABLE_VA)
    check(pe.section_of(PLAYER_PRIMARY_VTABLE_VA)==".rdata"
          and pe.section_of(PLAYER_RTTI_TYPE_VA)==".data",
          "original C_BasePlayer vtable/RTTI wrong data sections")
    check(section_read(pe,PLAYER_PRIMARY_RTTI_PTR_VA,4,".rdata")==
          pack(PLAYER_RTTI_COL_VA),
          "primary C_BasePlayer vftable[-1] RTTI COL pointer differs")
    col=section_read(pe,PLAYER_RTTI_COL_VA,20,".rdata")
    verify_player_rtti_col(col,PLAYER_RTTI_TYPE_VA,PLAYER_RTTI_CHD_VA)
    check(section_read(pe,PLAYER_RTTI_TYPE_VA+8,
                       len(PLAYER_RTTI_TYPE_NAME),".data")==
          PLAYER_RTTI_TYPE_NAME,
          "original owner RTTI is not C_BasePlayer")
    for site in (
        PLAYER_ASSIGN_VA+12,PLAYER_CLEAR_VA+2,PLAYER_CLEAR_VA+10,
        PLAYER_DTOR_VPTR_STORE_VA+5,
        PLAYER_PRIMARY_RTTI_PTR_VA,
        PLAYER_RTTI_COL_VA+12,PLAYER_RTTI_COL_VA+16,
    ):
        check(site in relocated,
              "original C_BasePlayer ownership pointer lacks HIGHLOW relocation")


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
    verify_original_player_global_owner(pe,reloc)
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
        "global_host_class_proven_by_original_RTTI": "C_BasePlayer",
        "original_player_primary_vtable_va": hex(PLAYER_PRIMARY_VTABLE_VA),
        "original_player_global_assignment_window_va": hex(PLAYER_ASSIGN_VA),
        "original_player_global_conditional_clear_window_va": hex(PLAYER_CLEAR_VA),
        "original_player_type_descriptor_va": hex(PLAYER_RTTI_TYPE_VA),
        "original_player_RTTI_primary_offset": 0,
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
    # Owner-global assignment and destruction evidence also gets
    # actual byte-mutation tests using INVENTED C_BasePlayer pointer
    # values. These are NOT retail code bytes from the original DLL.
    fake_player_global=0x4561C000
    fake_player_vtable=0x4524D000
    fake_player_type=0x4542C000
    fake_player_chd=0x4542D000
    fake_assign=(bytes.fromhex("3947488d77f8752bb0018935")+
                 pack(fake_player_global))
    verify_player_assignment_window(fake_assign,fake_player_global)
    for i in range(len(fake_assign)):
        broken=bytearray(fake_assign);broken[i]^=1
        reject(lambda data=bytes(broken):verify_player_assignment_window(
            data,fake_player_global),
            "synthetic player local-global assignment byte "+str(i))
    fake_clear=(bytes.fromhex("3b35")+pack(fake_player_global)+
                bytes.fromhex("750ac705")+pack(fake_player_global)+pack(0))
    verify_player_clear_window(fake_clear,fake_player_global)
    for i in range(len(fake_clear)):
        broken=bytearray(fake_clear);broken[i]^=1
        reject(lambda data=bytes(broken):verify_player_clear_window(
            data,fake_player_global),
            "synthetic player conditional clear byte "+str(i))
    fake_dtor=(bytes.fromhex("568bf1c706")+pack(fake_player_vtable)+
               b"\x90"*11)
    verify_player_dtor_vptr(fake_dtor,fake_player_vtable)
    for i in range(9):
        broken=bytearray(fake_dtor);broken[i]^=1
        reject(lambda data=bytes(broken):verify_player_dtor_vptr(
            data,fake_player_vtable),
            "synthetic player destructor vfptr store byte "+str(i))
    fake_col=pack(0)*3+pack(fake_player_type)+pack(fake_player_chd)
    verify_player_rtti_col(fake_col,fake_player_type,fake_player_chd)
    for i in range(len(fake_col)):
        broken=bytearray(fake_col);broken[i]^=1
        reject(lambda data=bytes(broken):verify_player_rtti_col(
            data,fake_player_type,fake_player_chd),
            "synthetic C_BasePlayer primary RTTI COL byte "+str(i))

    reject(lambda:audit(b"fake client",b"fake engine"),
           "retail original SHA bypass")
    print("PASS Phase1659 original-free synthetic: SHA-pinned parent "
          "prediction23, slot8 host+0x1DC optional DWORD RET0, "
          "slot13 host+0xF04 triple-DWORD OUT RET4, null-host "
          "zero vector, C_BasePlayer writer/destructor/RTTI identity, "
          "byte-level corruption/extent/false SHA refusal")

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
