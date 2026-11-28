#!/usr/bin/env python3
"""Phase1661: SHA-authenticated original CPrediction slots9-12 player calls.

Checks TWO entire original retail DLL file SHAs via Phase1658's verifier,
plus four previously unreviewed full original 2006 PE32 client.dll methods
by whole function SHA, pinned prediction virtual target, .text boundary,
HIGHLOW global reference and sparse x86 structural witnesses.

Public --self-test builds artificial nonexecuting sparse opcode windows
and mutates each COVERED witness byte. No proprietary full native method
bytes, actual engine images or original game process are shipped or run.
The synthetic C++ counterpart only owns invented player state/callbacks.
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
from audit_sin_original_client_prediction_view_readers import HOST_GLOBAL_VA
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

# (slot, VA, method extent, full original SHA256, player virtual offset)
GETTERS=(
    (9,0x24195DF0,58,
     "8ad6c6cc4e0e1f03fe8b7c444622224cbc799ee5f51b1d30d1c2e024f3fa65ff",
     0xE4),
    (11,0x24195E80,58,
     "a182f1e271cb3d50d82d45c749ddaa74a4ce9fa328fc9a4b9d11dfa60f7be977",
     0xEC),
)
# (slot, VA, extent, complete SHA, player vcall, mirror start,
#  embedded notifier object offset). Original notifier vcall is +0x0C.
SETTERS=(
    (10,0x24195E30,69,
     "0873e7c509ffea683f8677d629df469abd45624946e1b1f2ebb71277135c6ff4",
     0xE8,0x2C8,0x22C),
    (12,0x24195EC0,69,
     "d933b2d8fac56d107628bf53d2bca6148be9a4b5dbfbaa30db81edbf2c1d1242",
     0xF0,0x2D4,0x268),
)
ORIGINAL_OUT_POINTER_DWORDS=3
ORIGINAL_OUT_POINTER_STACK_POP=4

def pack(value: int) -> bytes:
    return struct.pack("<I",value)

def witness_getter(virtual_offset: int,global_va: int):
    # Sparse witnesses, deliberately NOT a complete executable function.
    return (
        (0,b"\x8B\x0D"+pack(global_va)),
        (6,bytes.fromhex("33d23bca750f")),
        (12,bytes.fromhex("8b4424048910895004895008c20400")),
        (27,b"\x8B\x01\xFF\x90"+pack(virtual_offset)),
        (35,bytes.fromhex("8b108b4c24048911")),
        (55,b"\xC2\x04\x00"),
    )

def witness_setter(virtual_offset: int,mirror: int,
                   notifier: int,global_va: int):
    # sparse original structural checks: global, non-null branch, player
    # virtual, mirror 3 DWORD fields, embedded notifier +0x0C, RET4.
    return (
        (0,b"\x56\x8B\x35"+pack(global_va)),
        (7,bytes.fromhex("85f67436")),
        (21,b"\xFF\x90"+pack(virtual_offset)),
        (29,b"\x89\x8E"+pack(mirror)),
        (38,b"\x89\x96"+pack(mirror+4)),
        (47,b"\x8D\x8E"+pack(notifier)),
        (53,b"\x89\x86"+pack(mirror+8)),
        (61,b"\xFF\x52\x0C"),
        (66,b"\xC2\x04\x00"),
    )

def verify_sparse(blob: bytes,length: int,checks,desc: str) -> int:
    check(len(blob)==length,
          desc+" complete original function extent changed")
    covered=set()
    for pos,value in checks:
        check(0<=pos and pos+len(value)<=len(blob),
              desc+" out-of-bounds witness")
        check(blob[pos:pos+len(value)]==value,
              desc+" original x86 structural witness differs @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes: bytes,engine_bytes: bytes) -> dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "prediction parent table is not original 23-slot version")
    client=PEImage(client_bytes)
    check(client.base==CLIENT_BASE and
          client.section_of(HOST_GLOBAL_VA)==".data",
          "original prediction player owner global outside client .data")
    reloc=pe_highlow_relocations(client)
    total_bytes=0
    covered=0
    for slot,va,size,digest,player_vcall in GETTERS:
        check(CLIENT_VIRTUAL_TARGETS[slot]==va,
              "getter slot target changed: "+str(slot))
        method=section_read(client,va,size,".text")
        check(hashlib.sha256(method).hexdigest()==digest,
              "getter entire original function SHA changed: "+str(slot))
        covered+=verify_sparse(
            method,size,witness_getter(player_vcall,HOST_GLOBAL_VA),
            "getter slot"+str(slot))
        check(va+2 in reloc,
              "getter local-player global lacks HIGHLOW relocation")
        check(section_read(client,va+size,2,".text")==b"\xCC\xCC",
              "getter original function end/padding boundary changed")
        total_bytes+=size
    for slot,va,size,digest,player_vcall,mirror,notifier in SETTERS:
        check(CLIENT_VIRTUAL_TARGETS[slot]==va,
              "setter slot target changed: "+str(slot))
        method=section_read(client,va,size,".text")
        check(hashlib.sha256(method).hexdigest()==digest,
              "setter entire original function SHA changed: "+str(slot))
        covered+=verify_sparse(
            method,size,
            witness_setter(player_vcall,mirror,notifier,HOST_GLOBAL_VA),
            "setter slot"+str(slot))
        check(va+3 in reloc,
              "setter local-player global lacks HIGHLOW relocation")
        check(section_read(client,va+size,2,".text")==b"\xCC\xCC",
              "setter original function end/padding boundary changed")
        total_bytes+=size
    return {
        "phase": 1661,
        "provenance": "both full original 2006 client and engine SHA matched",
        "original_prediction_virtual_getters": [x[0] for x in GETTERS],
        "original_prediction_virtual_setters": [x[0] for x in SETTERS],
        "original_new_whole_method_sha_count": 4,
        "original_new_whole_method_bytes": total_bytes,
        "original_sparse_opcode_witness_bytes": covered,
        "original_local_C_BasePlayer_global_va": hex(HOST_GLOBAL_VA),
        "getters_delegate_to_player_vtable_bytes": [
            hex(x[4]) for x in GETTERS],
        "setters_delegate_to_player_vtable_bytes": [
            hex(x[4]) for x in SETTERS],
        "setters_mirror_three_dwords_at_player_offsets": [
            hex(x[5]) for x in SETTERS],
        "setters_notify_embedded_subobject_offsets": [
            hex(x[6]) for x in SETTERS],
        "getter_null_player_zero_fills_three_out_dwords": True,
        "setter_null_player_noops": True,
        "getter_and_setter_stack_pop_bytes": ORIGINAL_OUT_POINTER_STACK_POP,
        "original_host_method_calls_executed": 0,
        "production_prediction_factory_registered": False,
        "original_game_startup_verified": False
    }

def reject(fn,what: str) -> None:
    try: fn()
    except ValueError: return
    raise AssertionError("corrupt original prediction view contract accepted: "+
                         what)

def exercise_synthetic_windows(size: int,checks,description: str) -> None:
    # Each fixture is a bytearray of INT3 padding plus disjoint synthetic
    # fragments. Only the tested fragments have prescribed instruction
    # shapes. It is not a functional, runnable native game method.
    invented=bytearray(b"\xCC"*size)
    touched=set()
    for pos,data in checks:
        invented[pos:pos+len(data)]=data
        touched.update(range(pos,pos+len(data)))
    check(verify_sparse(bytes(invented),size,checks,description)==
          len(touched),"synthetic witness coverage mismatch")
    for pos in sorted(touched):
        broken=bytearray(invented)
        broken[pos] ^= 0x01
        reject(lambda data=bytes(broken):verify_sparse(
            data,size,checks,description),
            description+" byte "+str(pos))
    reject(lambda:verify_sparse(bytes(invented[:-1]),size,checks,description),
           description+" truncated native extent")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",size,checks,description),
           description+" extra padding mistaken for method")

def self_test() -> None:
    invented_host=0x5E000000  # not retail original file VA
    for slot,_,size,_,vcall in GETTERS:
        exercise_synthetic_windows(
            size,witness_getter(vcall,invented_host),
            "synthetic player vector getter slot "+str(slot))
        reject(lambda v=vcall,l=size:verify_sparse(
            b"\xCC"*l,l,witness_getter(v,invented_host),
            "forged getter"),"getter all-int3 data")
    for slot,_,size,_,vcall,mirror,notifier in SETTERS:
        exercise_synthetic_windows(
            size,witness_setter(vcall,mirror,notifier,invented_host),
            "synthetic player vector setter slot "+str(slot))
        reject(lambda v=vcall,l=size,m=mirror,n=notifier:verify_sparse(
            b"\xCC"*l,l,witness_setter(v,m,n,invented_host),
            "forged setter"),"setter all-int3 data")
    check(set(x[0] for x in GETTERS+SETTERS)=={9,10,11,12},
          "unexpected original CPrediction contiguous slots")
    check(SETTERS[0][5]+12==SETTERS[1][5],
          "original mirrored player Vec3 fields should be adjacent")
    reject(lambda:audit(b"synthetic client",b"synthetic engine"),
           "fake original full-file SHA accepted")
    print("PASS Phase1661 original-free: CPrediction slots9/11 "
          "player virtual +0xE4/+0xEC Vec3 getters, slots10/12 "
          "player virtual +0xE8/+0xF0 setters, separate mirror "
          "+0x2C8/+0x2D4 and notifier +0x22C/+0x268, RET4, "
          "byte-mutation and exact original SHA refusal")

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be mixed with owner DLL paths")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),sort_keys=True,indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
