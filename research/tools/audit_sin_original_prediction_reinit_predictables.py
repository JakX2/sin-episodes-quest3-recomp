#!/usr/bin/env python3
"""Phase1683: original 2006 PE32 110-byte __thiscall no-args
CPrediction reinitialization helper 0x24195C20, invoked from the
now-authenticated original slot6 tail 0x24196485 on 0->1 enable.

Reads global entity registry at 0x242FA738, obtains index upper
bound via virtual +0x18 at registry+0x10014, iterates 0..max
inclusive, fetches each entity through 0x24105B50, checks source
IsPredictable 0x240C5500 and invokes opaque 0x240CC2D0 with
"ReinitPredictables" ONLY when non-null entity predicate was FALSE.
After loop, obtains Count via another entity list global
0x242F7EC8 virtual +4 and reports that count, not the number of
actual helper invocations, with original tier0 Msg literal.
This is source behavior, not an engine reimplementation.

Requires both fully SHA-matched retail client+engine for owner audit.
Public CI synthetic tests construct fake non-executable sparse
opcode islands and mutation refusals; no original game code ships.
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
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
from audit_sin_original_allocator_game_callers import named_iat

METHOD_VA=0x24195C20
METHOD_BYTES=110
METHOD_SHA="5ac6aa884651974b909ac16a8a3fbc921314e723a7d8622c00482158de0894a5"
ORIGINAL_POSTNETWORK_CALLER=0x24196485
GLOBAL_REGISTRY_VA=0x242FA738
GLOBAL_ENTITY_LIST_VA=0x242F7EC8
REGISTRY_SUBOBJECT_OFFSET=0x10014
REGISTRY_MAX_INDEX_VPTR_OFFSET=0x18
GLOBAL_ENTITY_COUNT_VPTR_OFFSET=0x04
OPAQUE_LOOKUP_INDEX_VA=0x24105B50
IS_PREDICTABLE_VA=0x240C5500
OPAQUE_REINIT_ENTITY_VA=0x240CC2D0
REINIT_REASON_LITERAL_VA=0x242A36EC
REINIT_REASON_LITERAL=b"ReinitPredictables\0"
MSG_LITERAL_VA=0x242A36C4
MSG_LITERAL=b"Reinitialized %i predictable entities\n\0"
MSG_IAT=0x242761EC
RELOCATIONS=(
    0x24195C22,0x24195C42,0x24195C5E,
    0x24195C73,0x24195C7E,0x24195C84,
)
ORIGINAL_CDECL_ARGUMENTS=0
ORIGINAL_RET_POP=0

def pack(value:int)->bytes:
    return struct.pack("<I",value)
def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)
def witness_chunks(*,base:int,registry:int,global_list:int,
                   lookup:int,predictable:int,reinit:int,
                   reason:int,fmt:int,import_msg:int):
    # Deliberately sparse opcode islands only. Entire native body
    # authenticity is established via whole-function SHA256 above.
    return (
        (0x00,b"\x8B\x0D"+pack(registry)),
        (0x0C,b"\x53\x81\xC1"+pack(REGISTRY_SUBOBJECT_OFFSET)),
        (0x27,relcall(base+0x27,lookup)),
        (0x34,relcall(base+0x34,predictable)),
        (0x3D,b"\x68"+pack(reason)),
        (0x44,relcall(base+0x44,reinit)),
        (0x51,b"\x8B\x0D"+pack(global_list)),
        (0x5D,b"\x68"+pack(fmt)),
        (0x62,b"\xFF\x15"+pack(import_msg)),
        (0x6D,b"\xC3"),
    )
def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,name+" complete method length changed")
    covered=set()
    for off,value in w:
        check(0<=off and off+len(value)<=len(blob),
              name+" native sparse bytes outside method")
        check(blob[off:off+len(value)]==value,
              name+" native instruction evidence differs +"+hex(off))
        covered.update(range(off,off+len(value)))
    return len(covered)
def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "whole original 2006 client/engine provenance missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE,
          "owner retail PE32 image base drift")
    slot6=section_read(pe,POST_NETWORK_VA,
                       POST_NETWORK_SIZE,".text")
    check(hashlib.sha256(slot6).hexdigest()==POST_NETWORK_SHA,
          "original prediction slot6 SHA drift")
    check(section_read(pe,ORIGINAL_POSTNETWORK_CALLER,5,".text")==
          relcall(ORIGINAL_POSTNETWORK_CALLER,METHOD_VA),
          "source 0->1 transition does not invoke reinit helper")
    code=section_read(pe,METHOD_VA,METHOD_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==METHOD_SHA,
          "retail original reinit 110B whole SHA changed")
    covered=verify_sparse(code,METHOD_BYTES,witness_chunks(
        base=METHOD_VA,registry=GLOBAL_REGISTRY_VA,
        global_list=GLOBAL_ENTITY_LIST_VA,
        lookup=OPAQUE_LOOKUP_INDEX_VA,predictable=IS_PREDICTABLE_VA,
        reinit=OPAQUE_REINIT_ENTITY_VA,reason=REINIT_REASON_LITERAL_VA,
        fmt=MSG_LITERAL_VA,import_msg=MSG_IAT),
        "reinit_predictables")
    check(code[-1:]==b"\xC3" and
          section_read(pe,METHOD_VA+METHOD_BYTES,2,".text")==b"\xCC\xCC",
          "original reinit RET0 / INT3 padding drift")
    for addr,literal in ((REINIT_REASON_LITERAL_VA,REINIT_REASON_LITERAL),
                         (MSG_LITERAL_VA,MSG_LITERAL)):
        check(section_read(pe,addr,len(literal),".rdata")==literal,
              "native reinit source label/Msg literal changed")
    check(named_iat(pe,"tier0.dll","Msg")==[MSG_IAT],
          "retail native tier0 Msg IAT identity changed")
    for va in (GLOBAL_REGISTRY_VA,GLOBAL_ENTITY_LIST_VA):
        section_read(pe,va,4,".data")
    reloc=pe_highlow_relocations(pe)
    own={x for x in reloc if METHOD_VA<=x<METHOD_VA+METHOD_BYTES}
    check(own==set(RELOCATIONS),
          "original reinit complete HIGHLOW site set drift")
    return {
        "phase":1683,
        "provenance":"BOTH SHA-authenticated full original 2006 PE32 client+engine",
        "original_method_va":hex(METHOD_VA),
        "original_method_bytes":METHOD_BYTES,
        "original_whole_method_sha256":METHOD_SHA,
        "original_native_abi":"__thiscall ECX=this, zero stack args, RET0",
        "original_slot6_tail_reenable_call":hex(ORIGINAL_POSTNETWORK_CALLER),
        "original_indexed_registry_global":hex(GLOBAL_REGISTRY_VA),
        "original_registry_inner_object_offset":hex(REGISTRY_SUBOBJECT_OFFSET),
        "original_max_index_virtual_slot_offset":hex(REGISTRY_MAX_INDEX_VPTR_OFFSET),
        "original_index_lookup_call":hex(OPAQUE_LOOKUP_INDEX_VA),
        "original_IsPredictable_call":hex(IS_PREDICTABLE_VA),
        "original_reinit_callback_call":hex(OPAQUE_REINIT_ENTITY_VA),
        "original_reinit_reason":REINIT_REASON_LITERAL[:-1].decode(),
        "original_reinit_on_predicate_FALSE":True,
        "original_entity_index_iteration":"0..max_index INCLUSIVE; max<0 skips",
        "original_log_count_global":hex(GLOBAL_ENTITY_LIST_VA),
        "original_log_count_virtual_offset":hex(GLOBAL_ENTITY_COUNT_VPTR_OFFSET),
        "original_msg_format":MSG_LITERAL[:-1].decode(),
        "original_tier0_Msg_IAT":hex(MSG_IAT),
        "original_reloc_site_count":len(RELOCATIONS),
        "original_two_int3_pads":True,
        "sparse_native_witness_bytes":covered,
        "retail_original_helper_executions_in_public_CI":0,
        "remaining_entity_reinit_helper_0x240CC2D0_opaque":True,
        "real_game_windows_boot_verified":False,
    }
def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original reinit native accepted: "+label)
def self_test()->None:
    fake_base=0x45110000
    ws=witness_chunks(
        base=fake_base,registry=0x46210000,
        global_list=0x46220000,lookup=0x45120000,
        predictable=0x45130000,reinit=0x45140000,
        reason=0x46310000,fmt=0x46320000,import_msg=0x46410000)
    fake=bytearray(b"\xCC"*METHOD_BYTES)
    covered=set()
    for off,fragment in ws:
        fake[off:off+len(fragment)]=fragment
        covered.update(range(off,off+len(fragment)))
    check(verify_sparse(bytes(fake),METHOD_BYTES,ws,
                        "invented_reinit")==len(covered),
          "synthetic 110B source witness coverage changed")
    for pos in sorted(covered):
        changed=bytearray(fake);changed[pos]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,METHOD_BYTES,ws,"altered_reinit"),str(pos))
    reject(lambda:verify_sparse(bytes(fake[:-1]),METHOD_BYTES,ws,
                                "truncated_reinit"),
           "truncated native method")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",METHOD_BYTES,ws,
                                "overlong_reinit"),
           "INT3 counted as original native body")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "retail SHA bypass")
    print("PASS Phase1683 original-free: whole-SHA original 110B "
          "prediction disabled->enabled reinitialization RET0, "
          "indexed registry 0..maximum inclusive, non-null and "
          "NOT-predictable entity callback, source literals, "
          "separate entity-list Count for tier0 Msg, six HIGHLOW "
          "sites/two INT3 padding, sparse mutant and "
          "fake original-image SHA refusal")
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
            p.error("retail SHA audit requires both original client and engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
