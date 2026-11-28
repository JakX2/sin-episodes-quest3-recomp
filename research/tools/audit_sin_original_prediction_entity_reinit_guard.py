#!/usr/bin/env python3
"""Phase1684: owner-private original 2006 PE32 entity-prediction reinit
guard (0x240CC2D0, 126 bytes), two entire 7B/8B embedded-state
helpers, and exact original Phase1683 parent CALL with one RET4 arg.

This is the downstream helper of ReinitPredictables 0x24195C20,
not an unconditional entity initialization. Source cl_predict gate,
this+0xC0 state latch, this+0xA4 encoded-state DWORD, global
0x2433F684 virtual+0x30, entity virtual+0x1A0/+0x1BC,
this+0x454 gate and opaque final 0x240CAF40 are authenticated.
Unknown virtual-call and final-helper bodies are NOT guessed.
No original binaries/machine-code methods are bundled in public CI.

--self-test invents non-executable sparse code islands and refuses
mutants and fake original whole-client/engine hashes. Owner-private
--client/--engine requires BOTH unchanged retail images.
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
from audit_sin_original_prediction_reinit_predictables import (
    METHOD_VA as PARENT_VA,
    METHOD_BYTES as PARENT_BYTES,
    METHOD_SHA as PARENT_SHA,
    OPAQUE_REINIT_ENTITY_VA as PARENT_CALLEE_VA,
)

METHOD_VA=0x240CC2D0
METHOD_BYTES=126
METHOD_SHA="222233997800f9d90828afef3f4a73eb44f20fd12664945a7646f52a399794e4"
PARENT_CALL_SITE=0x24195C64
EMBEDDED_HAS_STATE_VA=0x241952A0
EMBEDDED_HAS_STATE_BYTES=7
EMBEDDED_HAS_STATE_SHA="de10d879f972c106f535bcedb18af6100950d793a358b52f7b2f146235ff216c"
EMBEDDED_INDEX_VA=0x241952D0
EMBEDDED_INDEX_BYTES=8
EMBEDDED_INDEX_SHA="a6e9e6573fcb22882747e8bc5c03cdd05af19899393d0fd0d5c5818e9163aef6"
PREDICT_CONVAR_PARENT_GLOBAL=0x24374540
MODE_INDEX_PROVIDER_GLOBAL=0x2433F684
HAS_STATE_CALL_SITE=METHOD_VA+0x20
INDEX_CALL_SITE=METHOD_VA+0x3C
FINAL_HELPER_CALL_SITE=METHOD_VA+0x74
FINAL_HELPER_VA=0x240CAF40
ORIGINAL_MEMBER_EMBEDDED_STATE_OFFSET=0xA4
ORIGINAL_MEMBER_LATCH_OFFSET=0xC0
ORIGINAL_MEMBER_DISABLE_GATE_OFFSET=0x454
ORIGINAL_VTBL_NEGATIVE_PREDICATE_OFFSET=0x1A0
ORIGINAL_VTBL_POSITIVE_PREDICATE_OFFSET=0x1BC
ORIGINAL_INDEX_PROVIDER_VTBL_OFFSET=0x30
RELOC_SITES=(METHOD_VA+0x01,METHOD_VA+0x2B)
STACK_ARGS=1
RET_POP_BYTES=4

def pack(value:int)->bytes:
    return struct.pack("<I",value)
def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)
def witnesses(*,base:int,predict:int,index_global:int,
              has_state:int,state_index:int,final_helper:int):
    return (
        (0x00,b"\xA1"+pack(predict)),
        (0x05,bytes.fromhex("83782c00")),
        (0x0E,bytes.fromhex("80bec000000000")),
        (0x20,relcall(base+0x20,has_state)),
        (0x29,b"\x8B\x0D"+pack(index_global)),
        (0x32,bytes.fromhex("ff5230")),
        (0x3C,relcall(base+0x3C,state_index)),
        (0x46,bytes.fromhex("c686c000000001")),
        (0x51,bytes.fromhex("ff90a0010000")),
        (0x5F,bytes.fromhex("ff92bc010000")),
        (0x69,bytes.fromhex("83be5404000000")),
        (0x74,relcall(base+0x74,final_helper)),
        (0x7B,bytes.fromhex("c20400")),
    )
def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" whole method size differs from original")
    covered=set()
    for pos,value in w:
        check(pos>=0 and pos+len(value)<=length,
              name+" native opcode evidence outside method")
        check(blob[pos:pos+len(value)]==value,
              name+" native opcode/operand drift at +"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)
def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "dual whole original owner PE32 provenance unverified")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and PARENT_CALLEE_VA==METHOD_VA,
          "original Phase1683 ReinitPredictables child target drift")
    parent_bytes=section_read(pe,PARENT_VA,PARENT_BYTES,".text")
    check(hashlib.sha256(parent_bytes).hexdigest()==PARENT_SHA,
          "original 110-byte parent whole-SHA changed")
    check(section_read(pe,PARENT_CALL_SITE,5,".text")==
          relcall(PARENT_CALL_SITE,METHOD_VA),
          "parent no longer calls original one-argument entity reinit")
    code=section_read(pe,METHOD_VA,METHOD_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==METHOD_SHA,
          "entire original 126-byte entity reinit SHA changed")
    covered=verify_sparse(code,METHOD_BYTES,witnesses(
        base=METHOD_VA,predict=PREDICT_CONVAR_PARENT_GLOBAL,
        index_global=MODE_INDEX_PROVIDER_GLOBAL,
        has_state=EMBEDDED_HAS_STATE_VA,
        state_index=EMBEDDED_INDEX_VA,
        final_helper=FINAL_HELPER_VA),
        "reinit_entity_guard")
    check(section_read(pe,METHOD_VA+METHOD_BYTES,2,".text")==b"\xCC\xCC",
          "original entity helper RET4 not followed by two INT3s")
    source_has=section_read(pe,EMBEDDED_HAS_STATE_VA,
                            EMBEDDED_HAS_STATE_BYTES,".text")
    source_idx=section_read(pe,EMBEDDED_INDEX_VA,
                            EMBEDDED_INDEX_BYTES,".text")
    check(hashlib.sha256(source_has).hexdigest()==
          EMBEDDED_HAS_STATE_SHA and
          hashlib.sha256(source_idx).hexdigest()==
          EMBEDDED_INDEX_SHA,
          "entire original embedded 7B/8B helper SHA changed")
    check(source_has==bytes.fromhex("8339000f95c0c3") and
          source_idx==bytes.fromhex("8b01d1e883e01fc3"),
          "exact original embedded encoded-state operations changed")
    check(section_read(pe,EMBEDDED_HAS_STATE_VA+
                       EMBEDDED_HAS_STATE_BYTES,9,".text")==
          b"\xCC"*9 and
          section_read(pe,EMBEDDED_INDEX_VA+
                       EMBEDDED_INDEX_BYTES,8,".text")==
          b"\xCC"*8,
          "original compact embedded helper INT3 terminators differ")
    reloc=pe_highlow_relocations(pe)
    method_reloc={r for r in reloc
                  if METHOD_VA<=r<METHOD_VA+METHOD_BYTES}
    has_reloc={r for r in reloc if EMBEDDED_HAS_STATE_VA<=r<
               EMBEDDED_HAS_STATE_VA+EMBEDDED_HAS_STATE_BYTES}
    index_reloc={r for r in reloc if EMBEDDED_INDEX_VA<=r<
                 EMBEDDED_INDEX_VA+EMBEDDED_INDEX_BYTES}
    check(method_reloc==set(RELOC_SITES) and
          not has_reloc and not index_reloc,
          "original entity reinit relocation positions drift")
    return {
        "phase":1684,
        "provenance":"BOTH SHA-authenticated full original 2006 PE32 client+engine",
        "original_entity_helper_va":hex(METHOD_VA),
        "original_whole_method_bytes":METHOD_BYTES,
        "original_whole_method_sha256":METHOD_SHA,
        "original_cdecl_or_thiscall":"ECX=this, __thiscall one stack pointer argument, RET4",
        "parent_reinit_predictables_va":hex(PARENT_VA),
        "parent_original_call_site":hex(PARENT_CALL_SITE),
        "source_cl_predict_parent_global":hex(PREDICT_CONVAR_PARENT_GLOBAL),
        "source_cl_predict_parent_numeric_offset":hex(0x2C),
        "source_embedded_state_dword_offset":hex(
            ORIGINAL_MEMBER_EMBEDDED_STATE_OFFSET),
        "source_latch_byte_offset":hex(
            ORIGINAL_MEMBER_LATCH_OFFSET),
        "source_disable_gate_dword_offset":hex(
            ORIGINAL_MEMBER_DISABLE_GATE_OFFSET),
        "source_mode_provider_global":hex(MODE_INDEX_PROVIDER_GLOBAL),
        "source_mode_provider_virtual_offset":hex(
            ORIGINAL_INDEX_PROVIDER_VTBL_OFFSET),
        "source_mode_required":"provider_result - 1 == ((state_dword >> 1) & 31)",
        "source_first_virtual_negative_gate_offset":hex(
            ORIGINAL_VTBL_NEGATIVE_PREDICATE_OFFSET),
        "source_second_virtual_positive_gate_offset":hex(
            ORIGINAL_VTBL_POSITIVE_PREDICATE_OFFSET),
        "source_final_opaque_helper_va":hex(FINAL_HELPER_VA),
        "whole_7B_has_embedded_state_sha256":EMBEDDED_HAS_STATE_SHA,
        "whole_8B_state_index_sha256":EMBEDDED_INDEX_SHA,
        "original_highlow_sites":[hex(r) for r in RELOC_SITES],
        "native_witnessed_bytes":covered,
        "newly_authenticated_distinct_native_bytes":
            METHOD_BYTES+EMBEDDED_HAS_STATE_BYTES+EMBEDDED_INDEX_BYTES,
        "still_opaque_virtual_calls_and_final_helper":True,
        "actual_game_code_executions":0,
        "retail_windows_gameplay_boot_verified":False,
    }
def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original prediction entity reinit accepted: "+label)
def self_test()->None:
    # Invented non-executable sparse witnesses, independent of
    # owner's original image address space.
    w=witnesses(base=0x45110000,
                predict=0x46210000,index_global=0x46220000,
                has_state=0x45120000,
                state_index=0x45130000,
                final_helper=0x45140000)
    fake=bytearray(b"\xCC"*METHOD_BYTES)
    covered=set()
    for pos,frag in w:
        fake[pos:pos+len(frag)]=frag
        covered.update(range(pos,pos+len(frag)))
    check(verify_sparse(bytes(fake),METHOD_BYTES,w,"invented")==
          len(covered),"invented original-free witness coverage")
    for pos in sorted(covered):
        changed=bytearray(fake);changed[pos]^1
        reject(lambda data=bytes(changed):verify_sparse(
            data,METHOD_BYTES,w,"mutated"),str(pos))
    reject(lambda:verify_sparse(bytes(fake[:-1]),METHOD_BYTES,w,
                                "truncated"),"truncated")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",METHOD_BYTES,w,
                                "overlong"),"extra native padding")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged original complete client+engine SHA")
    print("PASS Phase1684 original-free: whole 126B entity reinit "
          "RET4 SHA, parent 110B CALL, exact embedded 7B/8B "
          "state helper SHA, cl_predict flag and +0xA4/+0xC0/"
          "+0x454 fields, two source virtual predicates and "
          "mode-provider vcall, final 0x240CAF40 call, two exact "
          "HIGHLOW relocations, synthetic mutants and fake-original refusal")
def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be combined with original images")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner requires BOTH original client and engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
