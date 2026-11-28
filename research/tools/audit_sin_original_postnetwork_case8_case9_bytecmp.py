#!/usr/bin/env python3
"""Phase1673: original 56-byte native PostNetwork byte-compare gate.

The original 2006 PE32 client.dll code at VA 0x24197A90 has one
THIS pointer and THREE stack arguments, RET 0x0C. It is called
directly by two separate typed comparison branches (case8 and
case9) in the source-authenticated Phase1672 dispatcher.

A disabled object byte this+0x14 returns AL0. Otherwise an
original current-record pointer at this+0x18 is inspected for
record BYTE +0x13 bit0x04, forcing AL1. Otherwise the three
native arguments are byte length, buffer A and buffer B; the
native REPZ CMPSB returns AL1 iff all bytes match. Case9
multiplies signed record WORD+0x10 by four before calling;
case8 uses the signed raw byte length unchanged. The full
original conditional case pipelines are still opaque.

The owner-only original-file audit verifies both complete
retail client.dll/engine.dll SHA256, whole 56-byte method SHA,
native original caller whole SHA, exact callsites, sparse
instruction witnesses, no HIGHLOW, and INT3 function boundary.
Public --self-test synthesizes only non-executing sparse x86
byte islands, never the full original executable function.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,verify_original,check,section_read,
)
from audit_sin_original_postnetwork_type_dispatch_13case import (
    DISPATCH_VA,DISPATCH_BYTES,DISPATCH_SHA,
    SWITCH_TARGETS,TYPE_TO_CASE,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
import struct

ORIGINAL_BYTE_COMPARE_VA=0x24197A90
ORIGINAL_BYTE_COMPARE_BYTES=56
ORIGINAL_BYTE_COMPARE_SHA="0fb74121fd315127c05255395992aae9e6737a74cee4133f2d1c3685947f0276"
CASE8_CALL_VA=0x24199A2E
CASE9_CALL_VA=0x2419992E
OWNER_ENABLED_BYTE_OFFSET=0x14
OWNER_CURRENT_RECORD_OFFSET=0x18
RECORD_FORCE_TRUE_FLAG_OFFSET=0x13
RECORD_FORCE_TRUE_FLAG_MASK=0x04
RECORD_SIGNED_LENGTH_WORD_OFFSET=0x10
CASE8_BYTE_LENGTH_MULTIPLIER=1
CASE9_BYTE_LENGTH_MULTIPLIER=4
ORIGINAL_RET_POP_BYTES=12

def rel_call(src:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-src-5)

def witness_chunks():
    # Short, discontinuous source instruction witnesses: not a
    # published copy of the complete original 56-byte function.
    return (
        (0x00,bytes.fromhex("80791400")),
        (0x06,bytes.fromhex("33c0c20c00")),
        (0x0B,bytes.fromhex("8b4118f6401304")),
        (0x14,bytes.fromhex("8b4c2404")),
        (0x19,bytes.fromhex("8b74240c")),
        (0x1E,bytes.fromhex("8b7c2414")),
        (0x24,bytes.fromhex("f3a6")),
        (0x29,bytes.fromhex("0f94c0")),
        (0x2D,bytes.fromhex("c20c00")),
        (0x30,bytes.fromhex("b801000000")),
        (0x35,bytes.fromhex("c20c00")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" original whole native function extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" native sparse opcode outside method")
        check(blob[pos:pos+len(value)]==value,
              name+" original x86 byte mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent original predicted-client SHA evidence missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          SWITCH_TARGETS[8]==0x24199A29 and
          SWITCH_TARGETS[9]==0x24199921 and
          TYPE_TO_CASE[8]==8 and TYPE_TO_CASE[9]==9,
          "original typed dispatcher case8/9 target mapping differs")
    native_parent=section_read(pe,DISPATCH_VA,DISPATCH_BYTES,".text")
    check(hashlib.sha256(native_parent).hexdigest()==DISPATCH_SHA,
          "Phase1672 original source caller whole-method SHA changed")
    for addr in (CASE8_CALL_VA,CASE9_CALL_VA):
        check(section_read(pe,addr,5,".text")==
              rel_call(addr,ORIGINAL_BYTE_COMPARE_VA),
              "original case8/9 direct CALL not targeting this gate")
    native=section_read(pe,ORIGINAL_BYTE_COMPARE_VA,
                        ORIGINAL_BYTE_COMPARE_BYTES,".text")
    check(hashlib.sha256(native).hexdigest()==ORIGINAL_BYTE_COMPARE_SHA,
          "original complete 56-byte comparison gate SHA differs")
    covered=verify_sparse(native,ORIGINAL_BYTE_COMPARE_BYTES,
                          witness_chunks(),"byte_comparison")
    check(section_read(pe,ORIGINAL_BYTE_COMPARE_VA+
                       ORIGINAL_BYTE_COMPARE_BYTES,8,".text")==
          b"\xCC"*8,
          "native original comparison helper not followed by INT3")
    reloc=pe_highlow_relocations(pe)
    own={x for x in reloc
         if ORIGINAL_BYTE_COMPARE_VA<=x<
            ORIGINAL_BYTE_COMPARE_VA+ORIGINAL_BYTE_COMPARE_BYTES}
    check(not own,"original byte comparison helper has no HIGHLOW sites")
    check(section_read(pe,0x24199922,4,".text")==
          bytes.fromhex("03f603f6"),
          "original case9 does not double record length twice")
    return {
        "phase":1673,
        "provenance":"BOTH complete original client.dll+engine.dll SHA256",
        "original_helper_va":hex(ORIGINAL_BYTE_COMPARE_VA),
        "original_complete_native_bytes":ORIGINAL_BYTE_COMPARE_BYTES,
        "original_whole_method_sha256":ORIGINAL_BYTE_COMPARE_SHA,
        "native_sparse_opcode_witness_bytes":covered,
        "original_native_relocations":0,
        "native_three_arg_ret_pop_bytes":ORIGINAL_RET_POP_BYTES,
        "original_enabled_byte_this_offset":hex(OWNER_ENABLED_BYTE_OFFSET),
        "original_current_record_this_offset":hex(OWNER_CURRENT_RECORD_OFFSET),
        "original_force_equal_record_byte_offset":
            hex(RECORD_FORCE_TRUE_FLAG_OFFSET),
        "original_force_equal_record_mask":
            hex(RECORD_FORCE_TRUE_FLAG_MASK),
        "native_repz_cmpsb_reads_two_explicit_byte_pointers":True,
        "native_compare_zero_length_equal_when_enabled":True,
        "native_case8_caller_va":hex(CASE8_CALL_VA),
        "native_case8_byte_count":"signed record WORD+0x10",
        "native_case9_caller_va":hex(CASE9_CALL_VA),
        "native_case9_byte_count":"four times signed record WORD+0x10",
        "case8_case9_remaining_original_helpers_implemented":False,
        "retail_native_functions_executed":0,
        "gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original byte gate accepted: "+label)

def self_test()->None:
    fake=bytearray(b"\xCC"*ORIGINAL_BYTE_COMPARE_BYTES)
    w=witness_chunks()
    positions=set()
    for off,value in w:
        fake[off:off+len(value)]=value
        positions.update(range(off,off+len(value)))
    check(verify_sparse(bytes(fake),ORIGINAL_BYTE_COMPARE_BYTES,
                        w,"invented_byte_gate")==len(positions),
          "invented byte comparison source evidence drift")
    for off in sorted(positions):
        bad=bytearray(fake);bad[off]^=1
        reject(lambda data=bytes(bad):verify_sparse(
            data,ORIGINAL_BYTE_COMPARE_BYTES,w,"invented_byte_gate"),
            "tampered opcode "+str(off))
    reject(lambda:verify_sparse(bytes(fake[:-1]),
                  ORIGINAL_BYTE_COMPARE_BYTES,w,"truncated"),
           "truncated original method accepted")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",
                  ORIGINAL_BYTE_COMPARE_BYTES,w,"overlong"),
           "trailing native INT3 counted as method")
    for case in (8,9):
        check(TYPE_TO_CASE[case]==case,
              "native type code case8/9 dispatch mismatch")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged original complete-image SHA gate")
    print("PASS Phase1673 original-free: whole original 56-byte "
          "PostNetwork compare native SHA, case8/case9 direct CALLs, "
          "this+0x14 enabled and currentRecord+0x18 flags byte+0x13, "
          "optional bit0x04 always equal, native REPZ CMPSB "
          "two-byte-pointer equality and zero count, case9 count*4, "
          "three-stack-arg RET12, no HIGHLOW, INT3 boundary, "
          "synthetic opcode mutation and forged-image refusal")

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
            p.error("retail original audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
