#!/usr/bin/env python3
"""Phase1685: authenticate complete original 90-byte prediction-init
method 0x240CAF40, entered by the 126B Phase1684 entity guard.

Original __thiscall no-stack-arg RET0: mark entity+0x2E8 byte=1,
invoke entity virtual+0x44 and pass result to 0x240C8F00,
invoke 0x240C8720, pass entity+0xB4 DWORD to global object
0x242F7F78 via 0x240CAC00, invoke 0x240C6900 with zero,
then repeat exactly 90 calls to 0x240C5960 with
("InitPredictable", index=0..89, 0). The effects of all
downstream engine methods remain explicitly opaque.

The whole source and parent method SHA256 are required, plus
both complete owner-retail PE32 client+engine SHA provenance.
No private executable bytes or code are published: public
--self-test invents non-executable sparse witnesses/mutants.
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
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
from audit_sin_original_prediction_entity_reinit_guard import (
    METHOD_VA as PARENT_VA,
    METHOD_BYTES as PARENT_BYTES,
    METHOD_SHA as PARENT_SHA,
    FINAL_HELPER_CALL_SITE as PARENT_CALL_SITE,
    FINAL_HELPER_VA as PARENT_CALLEE_VA,
)

METHOD_VA=0x240CAF40
METHOD_BYTES=90
METHOD_SHA="78c3167df98218dd8298d9e7f7c194fac9bc224a0d3c773d65e023e22ff2ed2e"
ORIGINAL_FLAG_2E8=0x2E8
ORIGINAL_FIELD_B4=0xB4
ORIGINAL_ENTITY_VTBL_CALL44=0x44
OPAQUE_8F00_VA=0x240C8F00
OPAQUE_8720_VA=0x240C8720
GLOBAL_HANDLER_VA=0x242F7F78
OPAQUE_GLOBAL_HANDLE_VA=0x240CAC00
OPAQUE_RECONCILE_VA=0x240C6900
OPAQUE_PER_FIELD_VA=0x240C5960
INIT_NAME_VA=0x242893DC
INIT_NAME=b"InitPredictable\0"
ORIGINAL_FIELD_ITERATIONS=0x5A
ORIGINAL_LOOP_START=METHOD_VA+0x40
ORIGINAL_LOOP_CALL=METHOD_VA+0x4A
ORIGINAL_LOOP_BOUND_BRANCH=METHOD_VA+0x55
ORIGINAL_POST_INT3_BYTES=6
ORIGINAL_HIGHLOW_SITES=(METHOD_VA+0x2B,METHOD_VA+0x44)
ORIGINAL_RET_POP=0

def pack(value:int)->bytes:
    return struct.pack("<I",value)
def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)
def witnesses(*,base:int,child8f00:int,child8720:int,
              global_obj:int,child_global:int,
              child_reconcile:int,init_name:int,
              per_field:int):
    return (
        (0x06,bytes.fromhex("c686e802000001")),
        (0x0D,bytes.fromhex("ff5044")),
        (0x13,relcall(base+0x13,child8f00)),
        (0x1A,relcall(base+0x1A,child8720)),
        (0x1F,bytes.fromhex("518b8eb4000000")),
        (0x2A,b"\xB9"+pack(global_obj)),
        (0x2F,relcall(base+0x2F,child_global)),
        (0x38,relcall(base+0x38,child_reconcile)),
        (0x40,bytes.fromhex("6a005768")+pack(init_name)),
        (0x4A,relcall(base+0x4A,per_field)),
        (0x52,bytes.fromhex("83ff5a7ce9")),
        (0x59,b"\xC3"),
    )
def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" complete original native method size drift")
    covered=set()
    for pos,frag in w:
        check(0<=pos and pos+len(frag)<=length,
              name+" evidence outside source method")
        check(blob[pos:pos+len(frag)]==frag,
              name+" original source changed at +"+hex(pos))
        covered.update(range(pos,pos+len(frag)))
    return len(covered)
def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent dual-original SHA provenance absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and METHOD_VA==PARENT_CALLEE_VA,
          "source final parent CALL target differs")
    native_parent=section_read(pe,PARENT_VA,PARENT_BYTES,".text")
    check(hashlib.sha256(native_parent).hexdigest()==PARENT_SHA,
          "source original complete 126B parent changed")
    check(section_read(pe,PARENT_CALL_SITE,5,".text")==
          relcall(PARENT_CALL_SITE,METHOD_VA),
          "source guard no longer invokes InitPredictable")
    blob=section_read(pe,METHOD_VA,METHOD_BYTES,".text")
    check(hashlib.sha256(blob).hexdigest()==METHOD_SHA,
          "source original whole 90B prediction init SHA changed")
    covered=verify_sparse(blob,METHOD_BYTES,witnesses(
        base=METHOD_VA,child8f00=OPAQUE_8F00_VA,
        child8720=OPAQUE_8720_VA,
        global_obj=GLOBAL_HANDLER_VA,
        child_global=OPAQUE_GLOBAL_HANDLE_VA,
        child_reconcile=OPAQUE_RECONCILE_VA,
        init_name=INIT_NAME_VA,
        per_field=OPAQUE_PER_FIELD_VA),
        "original_initpredictable")
    check(blob[-1:]==b"\xC3" and
          section_read(pe,METHOD_VA+METHOD_BYTES,
                       ORIGINAL_POST_INT3_BYTES,".text")==
          b"\xCC"*ORIGINAL_POST_INT3_BYTES,
          "source 90B original RET0/six INT3 boundaries differ")
    check(section_read(pe,INIT_NAME_VA,len(INIT_NAME),
                       ".rdata")==INIT_NAME,
          "source original InitPredictable literal differs")
    section_read(pe,GLOBAL_HANDLER_VA,4,".data")
    reloc=pe_highlow_relocations(pe)
    own={r for r in reloc if METHOD_VA<=r<METHOD_VA+METHOD_BYTES}
    check(own==set(ORIGINAL_HIGHLOW_SITES),
          "native original init HIGHLOW positions drift")
    return {
        "phase":1685,
        "provenance":"BOTH SHA-authenticated full original 2006 PE32 client+engine",
        "original_method_va":hex(METHOD_VA),
        "original_complete_native_bytes":METHOD_BYTES,
        "original_whole_method_sha256":METHOD_SHA,
        "original_abi":"__thiscall ECX=this, zero stack args, RET0",
        "original_guard_caller_va":hex(PARENT_VA),
        "original_guard_callsite":hex(PARENT_CALL_SITE),
        "source_first_action":"entity BYTE+0x2E8 = 1",
        "source_entity_virtual_method_offset":hex(
            ORIGINAL_ENTITY_VTBL_CALL44),
        "original_first_opaque_child":hex(OPAQUE_8F00_VA),
        "original_second_opaque_child":hex(OPAQUE_8720_VA),
        "original_entity_dword_b4":hex(ORIGINAL_FIELD_B4),
        "original_global_handler_object":hex(GLOBAL_HANDLER_VA),
        "original_global_handle_child":hex(OPAQUE_GLOBAL_HANDLE_VA),
        "original_preloop_reconcile_child":hex(OPAQUE_RECONCILE_VA),
        "source_loop_name":INIT_NAME[:-1].decode(),
        "original_per_field_loop_indices":"0..89 inclusive",
        "original_per_field_iterations":ORIGINAL_FIELD_ITERATIONS,
        "original_loop_callsite":hex(ORIGINAL_LOOP_CALL),
        "original_per_field_child":hex(OPAQUE_PER_FIELD_VA),
        "original_zero_third_loop_argument":True,
        "original_highlow_sites":[hex(r) for r in ORIGINAL_HIGHLOW_SITES],
        "original_int3_padding":ORIGINAL_POST_INT3_BYTES,
        "native_witnessed_bytes":covered,
        "unknown_child_method_bodies":True,
        "original_native_method_executions_in_public_CI":0,
        "gameplay_windows_boot_verified":False,
    }
def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original prediction init accepted: "+label)
def self_test()->None:
    w=witnesses(base=0x45110000,
       child8f00=0x45120000,child8720=0x45130000,
       global_obj=0x46210000,child_global=0x45140000,
       child_reconcile=0x45150000,init_name=0x46310000,
       per_field=0x45160000)
    invented=bytearray(b"\xCC"*METHOD_BYTES)
    covered=set()
    for pos,fragment in w:
        invented[pos:pos+len(fragment)]=fragment
        covered.update(range(pos,pos+len(fragment)))
    check(verify_sparse(bytes(invented),METHOD_BYTES,w,"invented")==
          len(covered),"invented original-free witness coverage")
    for pos in sorted(covered):
        changed=bytearray(invented);changed[pos]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,METHOD_BYTES,w,"mutated"),str(pos))
    reject(lambda:verify_sparse(bytes(invented[:-1]),
                                METHOD_BYTES,w,"short"),
           "truncated original method")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",
                                METHOD_BYTES,w,"long"),
           "method includes INT3 padding")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged full original client+engine SHA")
    print("PASS Phase1685 original-free: whole 90B InitPredictable "
          "RET0 native SHA, original 126B parent CALL, BYTE+0x2E8 "
          "and DWORD+0xB4, five opaque native call edges and "
          "entity vcall+0x44, 90 exact 0..89 InitPredictable "
          "per-field invocations, two original HIGHLOW and six "
          "INT3 pads, synthetic mutation/fake-owner rejection")
def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be used with owner retail DLLs")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner requires BOTH original client and engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
