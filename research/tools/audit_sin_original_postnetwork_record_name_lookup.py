#!/usr/bin/env python3
"""Phase1679: authenticate the COMPLETE original 121-byte PostNetwork
record-name finder at 0x24197ED0 from both SHA-matched retail images.

Previously marked opaque in Phase1671's 139-byte diagnostic gate,
this is NOT a text formatter: it is a two-DWORD cdecl, returning the
matching 0x34-byte record pointer or zero. It scans a node's [node+0]
array of [node+4] signed record count, ignores type0 records,
recurses into type10 record+0x20 before testing that record's own
name, compares [record+4] against input with a separate still-opaque
native case-insensitive comparison routine 0x2425F81B, and follows
node+0x0C linked nodes after local records. The native implementation
does not protect null pointers, invalid counts, cycles or recursion;
safe test-only implementations are explicitly bounded.

No executable original method bytes are distributed. Public self-test
builds invented non-executing sparse x86 opcode islands and mutations.
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
from audit_sin_original_postnetwork_diagnostic_core_finalizer import (
    DIAGNOSTIC_CORE_VA,DIAGNOSTIC_CORE_BYTES,
    DIAGNOSTIC_CORE_SHA,OPAQUE_FORMAT_HELPER_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

LOOKUP_VA=0x24197ED0
LOOKUP_BYTES=121
LOOKUP_SHA="ffd2e7d6539a429f896954abf6865665c971f7496d588a0d967209a3f956812b"
NAME_COMPARE_VA=0x2425F81B
CORE_CALLS=(DIAGNOSTIC_CORE_VA+0x5B,DIAGNOSTIC_CORE_VA+0x7C)
SELF_RECURSION_CALL=LOOKUP_VA+0x31
NAME_COMPARE_CALL=LOOKUP_VA+0x46
FOLLOWING_PADDING=7
NATIVE_STACK_ARGS=2
NATIVE_RET_POP=0
RECORD_STRIDE=0x34
RECORD_TYPE_OFF=0x00
RECORD_NAME_OFF=0x04
RECORD_NESTED_NODE_OFF=0x20
NODE_RECORD_ARRAY_OFF=0x00
NODE_SIGNED_COUNT_OFF=0x04
NODE_NEXT_OFF=0x0C

def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witness_chunks(*,base:int,name_compare_va:int):
    return (
        (0x00,bytes.fromhex(
            "53558b6c241056578b4d0433db85c9894c24187e4b")),
        (0x17,bytes.fromhex(
            "8b75008b043e03f785c0743383f80a7515")),
        (0x28,bytes.fromhex("8b46208b4c24145051")),
        (0x31,relcall(base+0x31,base)),
        (0x36,bytes.fromhex("83c40885c07537")),
        (0x3D,bytes.fromhex("8b5424148b46045250")),
        (0x46,relcall(base+0x46,name_compare_va)),
        (0x4B,bytes.fromhex("83c40885c07419")),
        (0x52,bytes.fromhex("8b4c241883c30183c7343bd97cb7")),
        (0x60,bytes.fromhex("8b450c85c0740b8be8eb9d")),
        (0x6B,bytes.fromhex("5f8bc65e5d5bc3")),
        (0x72,bytes.fromhex("33c05f5e5d5bc3")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" whole original native method extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" native instruction evidence outside body")
        check(blob[pos:pos+len(value)]==value,
              name+" native structure mismatch at +"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent original prediction 23-slot provenance absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and LOOKUP_VA==OPAQUE_FORMAT_HELPER_VA,
          "original previously opaque PostNetwork name-finder VA drift")
    core=section_read(pe,DIAGNOSTIC_CORE_VA,
                      DIAGNOSTIC_CORE_BYTES,".text")
    check(hashlib.sha256(core).hexdigest()==DIAGNOSTIC_CORE_SHA,
          "original Phase1671 diagnostic caller whole SHA drift")
    for site in CORE_CALLS:
        check(section_read(pe,site,5,".text")==relcall(site,LOOKUP_VA),
              "original debug diagnostic caller no longer enters finder")
    code=section_read(pe,LOOKUP_VA,LOOKUP_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==LOOKUP_SHA,
          "original entire 121B record-name finder SHA changed")
    covered=verify_sparse(code,LOOKUP_BYTES,witness_chunks(
        base=LOOKUP_VA,name_compare_va=NAME_COMPARE_VA),
        "original_record_name_finder")
    check(section_read(pe,LOOKUP_VA+LOOKUP_BYTES,
                       FOLLOWING_PADDING,".text")==
          b"\xCC"*FOLLOWING_PADDING,
          "whole original finder no longer ends before seven INT3s")
    check(code[-1:]==b"\xC3" and
          section_read(pe,NAME_COMPARE_VA,7,".text")==
          bytes.fromhex("833dd095382400"),
          "original RET0 boundary or native name-comparison entry drift")
    relocs=pe_highlow_relocations(pe)
    own={x for x in relocs
         if LOOKUP_VA<=x<LOOKUP_VA+LOOKUP_BYTES}
    check(not own,"original two-argument recursive finder has HIGHLOW relocations")
    return {
        "phase":1679,
        "provenance":"BOTH SHA-authenticated whole original 2006 PE32 client.dll+engine.dll",
        "original_va":hex(LOOKUP_VA),
        "original_method_bytes":LOOKUP_BYTES,
        "whole_original_method_sha256":LOOKUP_SHA,
        "native_return":"CDECL RET0 (caller removes two stack DWORDs)",
        "original_stack_arguments":NATIVE_STACK_ARGS,
        "original_caller_stack_cleanup":NATIVE_STACK_ARGS*4,
        "original_synthetic_safe_stride":RECORD_STRIDE,
        "original_record_type_offset":hex(RECORD_TYPE_OFF),
        "original_record_name_offset":hex(RECORD_NAME_OFF),
        "original_record_type10_nested_node_offset":hex(RECORD_NESTED_NODE_OFF),
        "original_node_record_array_offset":hex(NODE_RECORD_ARRAY_OFF),
        "original_node_signed_count_offset":hex(NODE_SIGNED_COUNT_OFF),
        "original_node_next_offset":hex(NODE_NEXT_OFF),
        "native_recursive_callsite":hex(SELF_RECURSION_CALL),
        "native_name_comparison_callsite":hex(NAME_COMPARE_CALL),
        "opaque_name_comparator_va":hex(NAME_COMPARE_VA),
        "phase1671_parent_callers":[hex(x) for x in CORE_CALLS],
        "native_highlow_relocation_sites":0,
        "method_following_INT3_padding":FOLLOWING_PADDING,
        "sparse_native_witness_bytes":covered,
        "original_native_method_executions":0,
        "real_game_windows_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original record-name finder passed: "+label)

def self_test()->None:
    w=witness_chunks(base=0x45110000,
                     name_compare_va=0x45200000)
    fake=bytearray(b"\xCC"*LOOKUP_BYTES)
    covered=set()
    for pos,frag in w:
        fake[pos:pos+len(frag)]=frag
        covered.update(range(pos,pos+len(frag)))
    check(verify_sparse(bytes(fake),LOOKUP_BYTES,w,"invented")==
          len(covered),"invented original-free witness coverage")
    for i in sorted(covered):
        changed=bytearray(fake);changed[i]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,LOOKUP_BYTES,w,"mutated"),str(i))
    reject(lambda:verify_sparse(bytes(fake[:-1]),LOOKUP_BYTES,w,
                                "truncated"),"truncated body")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",LOOKUP_BYTES,w,
                                "oversized"),"extra INT3 in body")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged dual retail whole-file SHA")
    print("PASS Phase1679 original-free: whole 121B SHA-authenticated "
          "native CDECL RET0 record-name finder, recursive type10 "
          "nested+0x20, 0x34 stride, node+0/4/0xC, cdecl "
          "record+4 name compare, two source parent callers, "
          "seven INT3 terminators, zero HIGHLOW relocations, "
          "synthetic native opcode mutations and fake-original refusal")

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
            p.error("retail owner requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
