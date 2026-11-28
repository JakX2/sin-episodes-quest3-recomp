#!/usr/bin/env python3
"""Phase1669: authenticate the original C_BaseEntity PostNetwork
reconciliation callback 0x240C6900, 192 native bytes, one-arg RET4,
and its source-attested call from CPrediction PostNetwork slot6.

This checks whole owner 2006 PE32 client+engine SHA, entire native
method SHA, three original HIGHLOW relocation sites, exact method
boundary (adjacent to a separate function WITHOUT INT3 padding),
source calls, conditional frame>0 comparison, 90-slot history ring,
primary entity virtual offset +0x40, and positive difference result.

The comparison helpers 0x241978A0 / 0x24199BA0 and extra embedded
entity methods are opaque; their native contents are NOT guessed.
Public --self-test uses invented nonexecutable sparse x86 islands.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,verify_original,check,section_read,
)
from audit_sin_original_prediction_postnetwork_slot6 import (
    POST_NETWORK_VA,POST_NETWORK_SIZE,POST_NETWORK_SHA,
    RECONCILE_ENTITY_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

CALLBACK_VA=0x240C6900
CALLBACK_BYTES=192
CALLBACK_SHA="c16da945561c4d57e3bfcfc148bdfe913879116d2e144ee315ff3cbb1f8b4fc1"
PROFILE_NAME_VA=0x24288FA8
PROFILE_NAME=b"PostNetworkDataReceived\0"
ORIGINAL_CONVAR_PARENT_VA=0x24374618
PROFILE_NATIVE_HELPER_VA=0x240C5960
MATRIX_PREPARE_VA=0x241978A0
DIFFERENCE_COUNT_VA=0x24199BA0
ORIGINAL_HISTORY_ACTIVE_OFFSET=0x454
ORIGINAL_HISTORY_DWORDS_OFFSET=0x2EC
ORIGINAL_HISTORY_CAPACITY=90
ORIGINAL_PRIMARY_ENTITY_VCALL_OFFSET=0x40
ORIGINAL_PARENT_DEBUG_GETTER_OFFSET=0x24
POST_NETWORK_CALLSITE_VA=0x241961C3
NATIVE_RELOCATIONS=(0x240C6912,0x240C6921,0x240C6997)

def pack(value:int)->bytes:
    return struct.pack("<I",value)

def rel_call(src:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-src-5)

def witness_chunks(*,base:int,name:int,convar:int,
                   init_helper:int,prepare:int,diff_count:int):
    return (
        (0x00,bytes.fromhex("83ec3c53558b6c24485657")),
        (0x0F,b"\x85\xED\x68"+pack(name)),
        (0x1B,rel_call(base+0x1B,init_helper)),
        (0x20,b"\xA1"+pack(convar)+bytes.fromhex("8b782c83ff02")),
        (0x36,bytes.fromhex("8b56088d4e08ff5224f7df3bc7")),
        (0x4A,bytes.fromhex("84db74668b8e5404000085c9")),
        (0x5C,bytes.fromhex("8d45ff99bf5a000000f7ff"+
                             "8b9496ec020000")),
        (0x80,bytes.fromhex("526a028d4c2438")),
        (0x87,rel_call(base+0x87,prepare)),
        (0x8C,bytes.fromhex("8b168bceff5240")),
        (0x93,bytes.fromhex("506aff68")+pack(0x242D330E)),
        (0x9F,rel_call(base+0x9F,diff_count)),
        (0xA4,bytes.fromhex("85c07e0c5f5e5db0015b83c43cc20400")),
        (0xB4,bytes.fromhex("5f5e5d32c05b83c43cc20400")),
    )

def verify_sparse(blob:bytes,length:int,w,what:str)->int:
    check(len(blob)==length,
          what+" original entire 192-byte method extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              what+" sparse native opcode witness outside original method")
        check(blob[pos:pos+len(value)]==value,
              what+" original source opcode witness differs at "+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent CPrediction original vtable provenance absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and CALLBACK_VA==RECONCILE_ENTITY_VA,
          "native PostNetwork source callee target changed")
    parent_method=section_read(pe,POST_NETWORK_VA,
                               POST_NETWORK_SIZE,".text")
    check(hashlib.sha256(parent_method).hexdigest()==POST_NETWORK_SHA,
          "retail PostNetwork parent method whole SHA changed")
    check(section_read(pe,POST_NETWORK_CALLSITE_VA,5,".text")==
          rel_call(POST_NETWORK_CALLSITE_VA,CALLBACK_VA),
          "PostNetwork per-predictable CALL no longer reaches exact callee")
    check(section_read(pe,PROFILE_NAME_VA,len(PROFILE_NAME),".rdata")==
          PROFILE_NAME,
          "native C_BaseEntity original PostNetwork profile literal changed")
    native=section_read(pe,CALLBACK_VA,CALLBACK_BYTES,".text")
    check(hashlib.sha256(native).hexdigest()==CALLBACK_SHA,
          "original full 192-byte entity reconciliation SHA mismatch")
    covered=verify_sparse(native,CALLBACK_BYTES,witness_chunks(
        base=CALLBACK_VA,name=PROFILE_NAME_VA,
        convar=ORIGINAL_CONVAR_PARENT_VA,
        init_helper=PROFILE_NATIVE_HELPER_VA,
        prepare=MATRIX_PREPARE_VA,diff_count=DIFFERENCE_COUNT_VA),
        "original_postnetwork_entity")
    check(section_read(pe,CALLBACK_VA+CALLBACK_BYTES,7,".text")==
          bytes.fromhex("81c16c010000e9"),
          "original method boundary is adjacent native thunk, not INT3")
    reloc=pe_highlow_relocations(pe)
    own={x for x in reloc
         if CALLBACK_VA<=x<CALLBACK_VA+CALLBACK_BYTES}
    check(own==set(NATIVE_RELOCATIONS),
          "original entity callback HIGHLOW relocations differ")
    return {
        "phase":1669,
        "provenance":"BOTH complete original client.dll+engine.dll SHA256",
        "original_postnetwork_callee_va":hex(CALLBACK_VA),
        "original_whole_method_sha256":CALLBACK_SHA,
        "whole_original_method_bytes":CALLBACK_BYTES,
        "original_relocation_sites":len(NATIVE_RELOCATIONS),
        "method_boundary_next_function_not_int3":True,
        "sparse_native_x86_witness_bytes":covered,
        "original_one_stack_arg_ret_pop_bytes":4,
        "argument_interpreted_as_signed_frame":True,
        "positive_frame_required_for_difference_path":True,
        "native_history_control_member":hex(ORIGINAL_HISTORY_ACTIVE_OFFSET),
        "native_history_ring_member_start":hex(ORIGINAL_HISTORY_DWORDS_OFFSET),
        "original_history_ring_capacity":ORIGINAL_HISTORY_CAPACITY,
        "original_history_index":"(frame-1) % 90 only for frame>0",
        "native_embedded_debug_vtable_offset":
            hex(ORIGINAL_PARENT_DEBUG_GETTER_OFFSET),
        "native_primary_entity_vtable_offset":
            hex(ORIGINAL_PRIMARY_ENTITY_VCALL_OFFSET),
        "opaque_comparison_prepare_va":hex(MATRIX_PREPARE_VA),
        "opaque_difference_count_va":hex(DIFFERENCE_COUNT_VA),
        "return_AL_1_iff_difference_count_positive":True,
        "original_comparison_helpers_implemented":False,
        "native_original_functions_executed":0,
        "retail_gameplay_boot_verified":False,
    }

def reject(fn,what:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original entity result contract accepted: "+
                         what)

def self_test()->None:
    base=0x45110000
    w=witness_chunks(base=base,name=0x46111000,
                     convar=0x47111000,
                     init_helper=0x45200000,
                     prepare=0x45300000,
                     diff_count=0x45310000)
    invented=bytearray(b"\xCC"*CALLBACK_BYTES)
    covered=set()
    for pos,value in w:
        invented[pos:pos+len(value)]=value
        covered.update(range(pos,pos+len(value)))
    check(verify_sparse(bytes(invented),CALLBACK_BYTES,w,
                        "invented_native")==len(covered),
          "invented sparse entity callback witness coverage drift")
    for pos in sorted(covered):
        broken=bytearray(invented);broken[pos]^=1
        reject(lambda data=bytes(broken):verify_sparse(
            data,CALLBACK_BYTES,w,"invented_native"),
            "native opcode/operand byte "+str(pos))
    reject(lambda:verify_sparse(bytes(invented[:-1]),
                  CALLBACK_BYTES,w,"truncated"),"wrong method length")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",
                  CALLBACK_BYTES,w,"overlong"),"extra padding")
    reject(lambda:audit(b"pretend original client",b"fake engine"),
           "owner original complete-SHA forgery")
    check(CALLBACK_BYTES==192 and len(NATIVE_RELOCATIONS)==3 and
          ORIGINAL_PRIMARY_ENTITY_VCALL_OFFSET==16*4,
          "source-attested native extent / vtable offset changed")
    print("PASS Phase1669 original-free: whole-SHA-gated "
          "192-byte PostNetwork per-entity callback, adjacent "
          "thunk method boundary, three HIGHLOW sites, signed "
          "frame>0 gating, ring (frame-1)%90, primary vptr+0x40, "
          "two opaque native prepare/count helpers, positive count "
          "AL1 versus zero/negative count AL0, x86 RET4")

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
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
