#!/usr/bin/env python3
"""Phase1668: source-authenticate the bounded original PostNetworkDataReceived
entry and predictable-entity reconciliation loop, NOT a full implementation
of its 1039-byte original 2006 PE32 method.

The parent original audit requires BOTH complete retail DLL SHA256 values.
This phase checks the ENTIRE original slot6 method SHA plus pinned .text
boundaries, the exact virtual slot, profiler literal, original HIGHLOW
sites, entry stores, player / cl_predict gates, client entity-list
Count/Get, IsPredictable, conditional reconciliation and RET4.
Later diagnostics/correction/recovery within this 1039-byte method are
EXPLICITLY OPAQUE. They are NOT implemented in the native fixture.

Public --self-test constructs only non-executing invented sparse-byte
witnesses, never a retail method body or executable original DLL.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE, CLIENT_VTABLE_VA, CLIENT_VIRTUAL_TARGETS,
    verify_original, check, section_read,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

SLOT=6
POST_NETWORK_VA=0x241960E0
POST_NETWORK_SIZE=0x40F
POST_NETWORK_SHA="968f8965dba88a881e3c752020ea9e5ec33a7930056ade4cc972c9d2120bf8a3"
PROFILE_NAME_VA=0x242A380C
PROFILE_NAME=b"CPrediction::PostNetworkDataReceived\0"
PROFILE_GROUP_VA=0x242D393C
PROFILE_GROUP=b"Prediction\0"
PROFILER_CURRENT_IAT=0x242761F4
PROFILER_ENTER_IAT=0x242761F8
PROFILER_EXIT_IAT=0x242761F0
CLIENT_PLAYER_GLOBAL_VA=0x24338CF0
CL_PREDICT_PARENT_VA=0x24374540
OPTIMIZE_CVAR_PARENT_VA=0x243746A8
ENTITY_LIST_GLOBAL_VA=0x242F7EC8
IS_PREDICTABLE_VA=0x240C5500
RECONCILE_ENTITY_VA=0x240C6900
ARGUMENT_MEMBER_VA_OFFSET=0x2C
RECONCILED_ANY_MEMBER_OFFSET=0x30
COUNT_VPTR_OFFSET=4
GET_VPTR_OFFSET=0

# These are original client PE32 HIGHLOW relocation sites within this
# ONE complete 1039-byte method. None is guessed or recovered by
# platform-specific source-engine equivalence.
RELOC_SITES=(
0x241960EB,0x241960F3,0x241960F9,0x24196103,
0x24196125,0x2419613B,0x2419614B,0x24196154,
0x241961A2,0x241961F6,0x2419620D,0x2419621A,
0x2419626A,0x24196271,0x24196277,0x24196289,
0x24196294,0x241962C0,0x241962C7,0x241962CD,
0x241962DD,0x241962E8,0x24196357,0x2419636C,
0x2419639A,0x241963A1,0x241963B3,0x241963B8,
0x241963BD,0x241963E2,0x241963E9,0x24196412,
0x24196430,0x2419644D,0x2419646D,0x24196498,
0x241964DC,0x241964E2,
)

def pack(value:int)->bytes:
    return struct.pack("<I",value)

def rel32(src:int,target:int)->bytes:
    return struct.pack("<i",target-src-5)

def witness_chunks(*,base:int,group:int,name:int,
                   current:int,enter:int,exit_scope:int,
                   player:int,predict:int,optimize:int,entities:int,
                   predictable:int,reconcile:int):
    # Sparse islands of x86 evidence; original whole method is NOT
    # embedded in this source file.
    return (
        (0x00,bytes.fromhex("83ec7c53555733ff575768")+pack(group)),
        (0x11,b"\x8B\x0D"+pack(current)),
        (0x18,b"\x68"+pack(name)),
        (0x21,b"\xFF\x15"+pack(enter)),
        (0x2E,bytes.fromhex("3bef0f9f44240f")),
        (0x3E,bytes.fromhex("896b2c897b30")),
        (0x44,b"\xA1"+pack(player)),
        (0x54,b"\x0F\x84"+rel32(base+0x53,base+0x3FA)),
        (0x5A,b"\xA1"+pack(predict)+bytes.fromhex("39782c")),
        (0x63,b"\x0F\x84"+struct.pack("<i",0x2419640C-(POST_NETWORK_VA+0x63+6))),
        (0x69,b"\x8B\x0D"+pack(optimize)+
              b"\x8B\x51\x2C\x8B\x0D"+pack(entities)),
        (0xAB,bytes.fromhex("ff5004")),
        (0xC0,b"\x8B\x0D"+pack(entities)+
              bytes.fromhex("8b1156ff12")),
        (0xD5,b"\x8B\xCF\xE8"+
              rel32(base+0xD7,predictable)),
        (0xE0,b"\x55\x8B\xCF\xE8"+
              rel32(base+0xE3,reconcile)),
        (0xE9,bytes.fromhex("84c07407c7433001000000")),
        (0x250,bytes.fromhex("83c6013b74242c89742424")),
        (0x25B,b"\x0F\x8C"+struct.pack("<i",
              (base+0xC0)-(base+0x25B+6))),
        (0x38B,b"\x8B\x0D"+pack(predict)+
              bytes.fromhex("39792c8a4310")),
        (0x3B7,b"\xA1"+pack(predict)+
              bytes.fromhex("39782c0f95c1884b10")),
        (0x3FA,b"\x8B\x0D"+pack(current)+b"\xFF\x15"+
              pack(exit_scope)),
        (0x405,bytes.fromhex("5f5d5b83c47cc20400")),
    )

def verify_sparse(blob:bytes,length:int,w,context:str)->int:
    check(len(blob)==length,
          context+" whole PostNetworkDataReceived function span changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              context+" sparse native opcode witness out of range")
        check(blob[pos:pos+len(value)]==value,
              context+" native opcode/operand differs @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent original CPrediction vtable not authenticated")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          CLIENT_VIRTUAL_TARGETS[SLOT]==POST_NETWORK_VA,
          "original PostNetworkDataReceived vtable slot6 mismatch")
    check(section_read(pe,CLIENT_VTABLE_VA+SLOT*4,4,".rdata")==
          pack(POST_NETWORK_VA),
          "client prediction vtable cell6 does not point to exact original")
    for addr,literal in ((PROFILE_NAME_VA,PROFILE_NAME),
                         (PROFILE_GROUP_VA,PROFILE_GROUP)):
        check(section_read(pe,addr,len(literal),".rdata")==literal,
              "native profile method/group literal is not original")
    blob=section_read(pe,POST_NETWORK_VA,POST_NETWORK_SIZE,".text")
    check(hashlib.sha256(blob).hexdigest()==POST_NETWORK_SHA,
          "complete original 1039-byte PostNetworkDataReceived SHA mismatch")
    count=verify_sparse(blob,POST_NETWORK_SIZE,witness_chunks(
        base=POST_NETWORK_VA,group=PROFILE_GROUP_VA,name=PROFILE_NAME_VA,
        current=PROFILER_CURRENT_IAT,enter=PROFILER_ENTER_IAT,
        exit_scope=PROFILER_EXIT_IAT,player=CLIENT_PLAYER_GLOBAL_VA,
        predict=CL_PREDICT_PARENT_VA,optimize=OPTIMIZE_CVAR_PARENT_VA,
        entities=ENTITY_LIST_GLOBAL_VA,predictable=IS_PREDICTABLE_VA,
        reconcile=RECONCILE_ENTITY_VA),"original_post_network")
    check(section_read(pe,POST_NETWORK_VA+POST_NETWORK_SIZE,1,".text")==
          b"\xCC","native original slot6 not followed by INT3 padding")
    reloc=pe_highlow_relocations(pe)
    check(CLIENT_VTABLE_VA+SLOT*4 in reloc,
          "original slot6 vtable cell missing PE HIGHLOW")
    present={site for site in reloc
             if POST_NETWORK_VA<=site<POST_NETWORK_VA+POST_NETWORK_SIZE}
    check(present==set(RELOC_SITES),
          "original 1039-byte method PE HIGHLOW relocation set differs")
    return {
        "phase":1668,
        "provenance":"BOTH complete original client.dll+engine.dll SHA256",
        "original_method":"CPrediction::PostNetworkDataReceived",
        "original_virtual_slot":SLOT,
        "original_method_va":hex(POST_NETWORK_VA),
        "original_whole_method_sha256":POST_NETWORK_SHA,
        "original_whole_function_bytes":POST_NETWORK_SIZE,
        "native_pe_highlow_sites":len(RELOC_SITES),
        "native_sparse_witness_bytes":count,
        "original_one_arg_stack_pop_bytes":4,
        "original_packet_arg_copied_to_member":hex(ARGUMENT_MEMBER_VA_OFFSET),
        "original_any_reconciled_member":hex(RECONCILED_ANY_MEMBER_OFFSET),
        "original_current_player_global":hex(CLIENT_PLAYER_GLOBAL_VA),
        "original_cl_predict_convar_parent":hex(CL_PREDICT_PARENT_VA),
        "original_optimization_convar_parent":hex(OPTIMIZE_CVAR_PARENT_VA),
        "client_entitylist_global":hex(ENTITY_LIST_GLOBAL_VA),
        "entity_count_vcall_offset":COUNT_VPTR_OFFSET,
        "entity_get_vcall_offset":GET_VPTR_OFFSET,
        "is_predictable_native_va":hex(IS_PREDICTABLE_VA),
        "reconcile_per_predictable_native_va":hex(RECONCILE_ENTITY_VA),
        "per_entity_callback_arg":"PostNetworkDataReceived arg1",
        "reconcile_AL_nonzero_sets_member_plus_0x30_to_one":True,
        "later_debug_and_correction_branches_implemented":False,
        "unknown_reconcile_entity_method_implemented":False,
        "original_native_method_calls_executed":0,
        "retail_windows_gameplay_boot_verified":False,
    }

def reject(fn,reason:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original postnetwork witness accepted: "+
                         reason)

def self_test()->None:
    # Invented addresses, invented opcode islands, non-executable and
    # independent of the actual original image in the retail owner audit.
    fake_base=0x49100000
    w=witness_chunks(
        base=fake_base,group=0x4A000000,name=0x4A000100,
        current=0x4A000200,enter=0x4A000300,exit_scope=0x4A000400,
        player=0x4B000000,predict=0x4B000100,optimize=0x4B000200,
        entities=0x4B000300,predictable=0x49200000,
        reconcile=0x49201000)
    invented=bytearray(b"\xCC"*POST_NETWORK_SIZE)
    covered=set()
    for pos,value in w:
        invented[pos:pos+len(value)]=value
        covered.update(range(pos,pos+len(value)))
    check(verify_sparse(bytes(invented),POST_NETWORK_SIZE,
                        w,"fake") == len(covered),
          "synthetic native PostNetwork witness coverage mismatch")
    for pos in sorted(covered):
        broken=bytearray(invented)
        broken[pos]^=1
        reject(lambda data=bytes(broken):verify_sparse(
            data,POST_NETWORK_SIZE,w,"fake"),
            "synthetic opcode/operand byte "+str(pos))
    reject(lambda:verify_sparse(bytes(invented[:-1]),
                  POST_NETWORK_SIZE,w,"short"),"truncated function")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",
                  POST_NETWORK_SIZE,w,"long"),"INT3 padding in method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged dual full-DLL SHA")
    check(POST_NETWORK_SIZE==1039 and len(RELOC_SITES)==38 and
          all(POST_NETWORK_VA<=x<POST_NETWORK_VA+POST_NETWORK_SIZE
              for x in RELOC_SITES),
          "method span or exact relocation count changed")
    print("PASS Phase1668 original-free: whole-SHA-gated native "
          "1039-byte PostNetworkDataReceived slot6 RET4 with "
          "38 relocations, this+0x2C/+0x30 initialization, "
          "local player/cl_predict, entity-list Count/Get, "
          "predictable reconciliation boolean, explicit opaque "
          "later branches, all sparse opcode-byte mutants rejected")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot combine with owner binary paths")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,
                         sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
