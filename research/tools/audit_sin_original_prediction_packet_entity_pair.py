#!/usr/bin/env python3
"""Phase1665: authentic original CPrediction packet entity lifecycle slots4/5.

Owner-only full SHA-authenticated 2006 PE32 client+engine read-only audit:
PreEntityPacketReceived / PostEntityPacketReceived, original entity
list enumeration via primary virtual0/1, original IsPredictable
native +0x2E8 flag, per-eligible entity callbacks, cl_predict and
current-player gates. Public --self-test uses invented sparse bytes;
never ships or executes original copyrighted native function bodies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,CLIENT_VTABLE_VA,CLIENT_VIRTUAL_TARGETS,
    verify_original,check,section_read,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

PRE_SLOT=4
POST_SLOT=5
PRE_VA=0x24195F90
PRE_BYTES=174
PRE_SHA="3ad38230016d32a96a657b96440112b51bc8535e420b7f0a8db7c17c73f7d0e0"
POST_VA=0x24196040
POST_BYTES=153
POST_SHA="12c676eee586917d9880b32cc134370e75e696052512b478bc4da9f217fd7090"
IS_PREDICTABLE_VA=0x240C5500
IS_PREDICTABLE_BYTES=7
IS_PREDICTABLE_SHA="eb493d88758526793c40c2cb3442a38fd0974612d4c95680b330121dce5a3228"
PRE_DISABLED_FALLBACK_VA=0x24195720
PRE_ENTITY_CALLBACK_VA=0x240C5510
POST_ENTITY_CALLBACK_VA=0x240C68C0
PREDICTABLE_FLAG_OFFSET=0x2E8
SCOPE_NAME_VA=0x242D393C
PRE_PROFILE_NAME_VA=0x242A3738
POST_PROFILE_NAME_VA=0x242A3760
PRE_PROFILE_NAME=b"CPrediction::PreEntityPacketReceived\0"
POST_PROFILE_NAME=b"CPrediction::PostEntityPacketReceived\0"
PROFILE_CURRENT_VA=0x242761F4
PROFILE_ENTER_IAT=0x242761F8
PROFILE_EXIT_IAT=0x242761F0
CL_PREDICT_PARENT_VA=0x24374540
LOCAL_PLAYER_GLOBAL_VA=0x24338CF0
ENTITY_LIST_GLOBAL_VA=0x242F7EC8
ORIGINAL_MEMBER_ARG2_OFFSET=0x0C
ENTITY_COUNT_VIRTUAL_OFFSET=4
ENTITY_BY_INDEX_VIRTUAL_OFFSET=0
MAX_SYNTHETIC_ENTITIES=4096
PRE_RELOCS=(0x06,0x10,0x15,0x1B,0x28,0x3B,0x41,
            0x4B,0x54,0x72,0x9E,0xA4)
POST_RELOCS=(0x02,0x0B,0x12,0x18,0x1D,0x29,0x2F,
             0x35,0x3E,0x44,0x4A,0x62,0x8D,0x95)

def pack(v:int)->bytes:
    return struct.pack("<I",v)
def rel_call(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def pre_witnesses(*,base:int,profile_current:int,profile_enter:int,
                  profile_exit:int,name:int,scope:int,cvar:int,
                  player:int,entities:int,disabled:int,
                  predictable:int,prepare:int):
    return (
        (0x00,b"\x56\x6A\x00\x6A\x00\x68"+pack(scope)+b"\x6A\x00\x8B\xF1"),
        (0x0E,b"\x8B\x0D"+pack(profile_current)),
        (0x14,b"\x68"+pack(name)+b"\xFF\x15"+pack(profile_enter)),
        (0x1F,bytes.fromhex("8b44240c89460c")),
        (0x26,b"\x8B\x0D"+pack(cvar)+bytes.fromhex("83792c00")),
        (0x30,bytes.fromhex("7517")),
        (0x32,bytes.fromhex("8bce")),
        (0x34,rel_call(base+0x34,disabled)),
        (0x3B,b"\x8B\x0D"+pack(profile_current)),
        (0x41,b"\xFF\x15"+pack(profile_exit)),
        (0x49,b"\x83\x3D"+pack(player)+bytes.fromhex("0074e7")),
        (0x52,b"\x8B\x0D"+pack(entities)),
        (0x5C,bytes.fromhex("ff5204")),
        (0x70,b"\x8B\x0D"+pack(entities)),
        (0x77,bytes.fromhex("ff10")),
        (0x83,rel_call(base+0x83,predictable)),
        (0x8F,rel_call(base+0x8F,prepare)),
        (0xA8,bytes.fromhex("5f5b5ec20800")),
    )

def post_witnesses(*,base:int,profile_current:int,profile_enter:int,
                   profile_exit:int,name:int,scope:int,cvar:int,
                   player:int,entities:int,predictable:int,post:int):
    return (
        (0x00,b"\x8B\x0D"+pack(profile_current)),
        (0x06,b"\x6A\x00\x6A\x00\x68"+pack(scope)+b"\x6A\x00"),
        (0x11,b"\x68"+pack(name)+b"\xFF\x15"+pack(profile_enter)),
        (0x1C,b"\xA1"+pack(cvar)+bytes.fromhex("83782c00750c")),
        (0x27,b"\x8B\x0D"+pack(profile_current)+
              b"\xFF\x25"+pack(profile_exit)),
        (0x33,b"\x83\x3D"+pack(player)+bytes.fromhex("00750c")),
        (0x3C,b"\x8B\x0D"+pack(profile_current)+
              b"\xFF\x25"+pack(profile_exit)),
        (0x48,b"\x8B\x0D"+pack(entities)),
        (0x52,bytes.fromhex("ff5204")),
        (0x60,b"\x8B\x0D"+pack(entities)),
        (0x6A,bytes.fromhex("ff10")),
        (0x73,rel_call(base+0x73,predictable)),
        (0x7E,rel_call(base+0x7E,post)),
        (0x93,b"\xFF\x25"+pack(profile_exit)),
    )

def verify_sparse(blob:bytes,length:int,witnesses,kind:str)->int:
    check(len(blob)==length,
          kind+" original complete native function extent changed")
    covered=set()
    for pos,value in witnesses:
        check(0<=pos and pos+len(value)<=len(blob),
              kind+" native x86 witness out of method range")
        check(blob[pos:pos+len(value)]==value,
              kind+" original x86 structural witness mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def pre_original()->tuple:
    return pre_witnesses(
        base=PRE_VA,profile_current=PROFILE_CURRENT_VA,
        profile_enter=PROFILE_ENTER_IAT,profile_exit=PROFILE_EXIT_IAT,
        name=PRE_PROFILE_NAME_VA,scope=SCOPE_NAME_VA,
        cvar=CL_PREDICT_PARENT_VA,player=LOCAL_PLAYER_GLOBAL_VA,
        entities=ENTITY_LIST_GLOBAL_VA,disabled=PRE_DISABLED_FALLBACK_VA,
        predictable=IS_PREDICTABLE_VA,prepare=PRE_ENTITY_CALLBACK_VA)

def post_original()->tuple:
    return post_witnesses(
        base=POST_VA,profile_current=PROFILE_CURRENT_VA,
        profile_enter=PROFILE_ENTER_IAT,profile_exit=PROFILE_EXIT_IAT,
        name=POST_PROFILE_NAME_VA,scope=SCOPE_NAME_VA,
        cvar=CL_PREDICT_PARENT_VA,player=LOCAL_PLAYER_GLOBAL_VA,
        entities=ENTITY_LIST_GLOBAL_VA,predictable=IS_PREDICTABLE_VA,
        post=POST_ENTITY_CALLBACK_VA)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "original prediction 23-slot vtable provenance absent")
    client=PEImage(client_bytes)
    check(client.base==CLIENT_BASE and
          CLIENT_VIRTUAL_TARGETS[PRE_SLOT]==PRE_VA and
          CLIENT_VIRTUAL_TARGETS[POST_SLOT]==POST_VA,
          "original prediction slots4 and 5 vtable targets changed")
    for slot,address in ((PRE_SLOT,PRE_VA),(POST_SLOT,POST_VA)):
        check(section_read(client,CLIENT_VTABLE_VA+slot*4,4,".rdata")==
              pack(address),
              "original packet virtual table entry differs "+str(slot))
    for va,label in ((PRE_PROFILE_NAME_VA,PRE_PROFILE_NAME),
                     (POST_PROFILE_NAME_VA,POST_PROFILE_NAME)):
        check(section_read(client,va,len(label),".rdata")==label,
              "original packet lifecycle profiler literal mismatch")
    pre=section_read(client,PRE_VA,PRE_BYTES,".text")
    post=section_read(client,POST_VA,POST_BYTES,".text")
    check(hashlib.sha256(pre).hexdigest()==PRE_SHA and
          hashlib.sha256(post).hexdigest()==POST_SHA,
          "complete native packet lifecycle method SHA mismatch")
    pre_count=verify_sparse(pre,PRE_BYTES,pre_original(),"pre_packet")
    post_count=verify_sparse(post,POST_BYTES,post_original(),"post_packet")
    check(section_read(client,PRE_VA+PRE_BYTES,2,".text")==b"\xCC\xCC"
          and section_read(client,POST_VA+POST_BYTES,2,".text")==
              b"\xCC\xCC",
          "original packet methods not bounded by native INT3 padding")
    predictable=section_read(client,IS_PREDICTABLE_VA,
                             IS_PREDICTABLE_BYTES,".text")
    check(hashlib.sha256(predictable).hexdigest()==IS_PREDICTABLE_SHA
          and predictable==b"\x8A\x81"+pack(PREDICTABLE_FLAG_OFFSET)+b"\xC3"
          and section_read(client,IS_PREDICTABLE_VA+7,2,".text")==
              b"\xCC\xCC",
          "original C_BaseEntity IsPredictable +0x2E8 BYTE RET0 differs")
    reloc=pe_highlow_relocations(client)
    for slot in (PRE_SLOT,POST_SLOT):
        check(CLIENT_VTABLE_VA+slot*4 in reloc,
              "original packet slot pointer not relocated")
    for method,offsets in ((PRE_VA,PRE_RELOCS),(POST_VA,POST_RELOCS)):
        for off in offsets:
            check(method+off in reloc,
                  "native packet pointer operand has no HIGHLOW: "+
                  hex(method+off))
    return {
        "phase":1665,
        "provenance":"full original SHA client.dll + engine.dll",
        "original_pre_packet_va":hex(PRE_VA),
        "original_pre_packet_method_bytes":PRE_BYTES,
        "original_pre_packet_whole_sha256":PRE_SHA,
        "original_post_packet_va":hex(POST_VA),
        "original_post_packet_method_bytes":POST_BYTES,
        "original_post_packet_whole_sha256":POST_SHA,
        "original_predictable_flag_getter_va":hex(IS_PREDICTABLE_VA),
        "original_predictable_flag_getter_whole_sha256":IS_PREDICTABLE_SHA,
        "new_original_fully_authenticated_method_bytes":
            PRE_BYTES+POST_BYTES+IS_PREDICTABLE_BYTES,
        "pre_native_opcode_witness_bytes":pre_count,
        "post_native_opcode_witness_bytes":post_count,
        "pre_native_arg_words":2,
        "pre_ret_pop_bytes":8,
        "pre_argument2_recorded_at_prediction_member":hex(ORIGINAL_MEMBER_ARG2_OFFSET),
        "pre_argument1_forwarded_to_predictable_entity_callback":True,
        "pre_disabled_cl_predict_calls_opaque_original_fallback_va":
            hex(PRE_DISABLED_FALLBACK_VA),
        "post_native_arg_words":0,
        "post_ret_pop_bytes":0,
        "post_exit_tail_jump_to_profiler_import":True,
        "global_client_entity_list_va":hex(ENTITY_LIST_GLOBAL_VA),
        "entity_count_virtual_vptr_byte_offset":4,
        "entity_by_index_virtual_vptr_byte_offset":0,
        "is_predictable_entity_flag_offset":hex(PREDICTABLE_FLAG_OFFSET),
        "pre_entity_callback_va":hex(PRE_ENTITY_CALLBACK_VA),
        "post_entity_callback_va":hex(POST_ENTITY_CALLBACK_VA),
        "retail_native_method_invocations":0,
        "replacement_prediction_interface_registered":False,
        "gameplay_boot_verified":False,
    }

def reject(action,label:str)->None:
    try:action()
    except ValueError:return
    raise AssertionError("corrupt original packet ABI incorrectly accepted: "+
                         label)

def synthetic_windows(witness_fn,size:int,slot:str):
    # Every byte is invented/non-running; these byte islands do NOT
    # reproduce a copyrighted original full function body.
    invented_base=0x42110000 if slot=="pre" else 0x42120000
    vars=dict(base=invented_base,profile_current=0x43110000,
              profile_enter=0x43111000,profile_exit=0x43112000,
              name=0x43113000,scope=0x43114000,cvar=0x44110000,
              player=0x44111000,entities=0x44112000,
              predictable=0x42130000)
    if slot=="pre":
        vars.update(disabled=0x42131000,prepare=0x42132000)
    else:
        vars.update(post=0x42133000)
    witnesses=witness_fn(**vars)
    fake=bytearray(b"\xCC"*size)
    covered=set()
    for pos,value in witnesses:
        fake[pos:pos+len(value)]=value
        covered.update(range(pos,pos+len(value)))
    check(verify_sparse(bytes(fake),size,witnesses,slot)==len(covered),
          "synthetic packet witness coverage mismatch")
    for pos in sorted(covered):
        broken=bytearray(fake)
        broken[pos] ^= 0x1
        reject(lambda data=bytes(broken):verify_sparse(
            data,size,witnesses,slot),
            slot+" byte mutation "+str(pos))
    reject(lambda:verify_sparse(bytes(fake[:-1]),size,witnesses,slot),
           slot+" truncated original")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",size,witnesses,slot),
           slot+" extra method byte")

def self_test()->None:
    synthetic_windows(pre_witnesses,PRE_BYTES,"pre")
    synthetic_windows(post_witnesses,POST_BYTES,"post")
    check(PRE_SLOT==4 and POST_SLOT==5 and
          PRE_BYTES+POST_BYTES==327 and
          IS_PREDICTABLE_BYTES==7,
          "packet original source method extents/index drift")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner original complete DLL SHA bypass")
    print("PASS Phase1665 original-free: two original CPrediction "
          "packet virtuals slots4 RET8 / 5 RET0, exact profiler labels, "
          "predictable C_BaseEntity +0x2E8 flag, client list count/get "
          "vtable indices, enabled/disabled/no-player gates, pre "
          "first-argument forward and second-argument this+0xC, "
          "post no-arg opaque callback, source SHA and byte-mutation refusal")

def main()->int:
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
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
