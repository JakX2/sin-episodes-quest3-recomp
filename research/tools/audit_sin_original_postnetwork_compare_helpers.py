#!/usr/bin/env python3
"""Phase1670: source authenticate two original PostNetwork comparison helpers.

Original 2006 PE32 client.dll+engine.dll BOTH require the complete
SHA256 parent audit. Entire methods pinned, plus sparse x86 structural
witnesses, no original copyrighted native body shipped.

Method 0x241978A0: exactly 104 bytes, a ten-stack-DWORD __thiscall
initializer RET 0x28 writing a 52-byte target structure. Normalizes
two low BYTE flags, clears select fields, copies four scalar WORDs
and four input low BYTEs, with no imported/relocated addresses.

Method 0x24199BA0: exactly 71 bytes, a three-stack-DWORD
__thiscall RET 0x0C wrapper; increments global sequence DWORD,
conditionally invokes opaque preflight, invokes opaque core with
three caller args, invokes opaque finalize with sequence number
and third arg, then returns this+0x28. Its descendants are not
reconstructed as part of this phase.

Public --self-test fabricates disjoint, non-executable opcode
islands with invented addresses and rejects covered mutations.
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
from audit_sin_original_postnetwork_entity_compare import (
    CALLBACK_VA,CALLBACK_BYTES,CALLBACK_SHA,
    MATRIX_PREPARE_VA,DIFFERENCE_COUNT_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

PREPARE_VA=0x241978A0
PREPARE_BYTES=104
PREPARE_SHA="13bac9785ff2c2f21cd983d5d911f542538a449f4990bd21ab825a96d24f2c82"
WRAPPER_VA=0x24199BA0
WRAPPER_BYTES=71
WRAPPER_SHA="9dfd73ea32a62b437e7492038a3b7f01706d6ba62966d162e8b776c897cfeab3"
GLOBAL_COUNT_DWORD_VA=0x242FDB50
PREFLIGHT_VA=0x24197F50
CORE_VA=0x24199100
FINALIZE_VA=0x24199B50
PREPARE_CALLSITE_VA=CALLBACK_VA+0x87
WRAPPER_CALLSITE_VA=CALLBACK_VA+0x9F
PREPARE_DESTINATION_BYTES=0x34
PREPARE_ARG_DWORDS=10
WRAPPER_ARG_DWORDS=3
WRAPPER_RESULT_DWORD_OFFSET=0x28

def pack(v:int)->bytes:
    return struct.pack("<I",v)
def rel_call(src:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-src-5)

def prepare_chunks():
    return (
       (0x00,bytes.fromhex("8b5424088bc1")),
       (0x0A,bytes.fromhex("89500433d2")),
       (0x11,bytes.fromhex("8b4c2410894808")),
       (0x1A,bytes.fromhex("384c240c")),
       (0x21,bytes.fromhex("0f95c2")),
       (0x24,bytes.fromhex("89482089481c")),
       (0x2A,bytes.fromhex("884824884825")),
       (0x30,bytes.fromhex("89482889500c")),
       (0x38,bytes.fromhex("384c2414")),
       (0x40,bytes.fromhex("0f95c2894830")),
       (0x49,bytes.fromhex("8a542418885014")),
       (0x50,bytes.fromhex("8a54241c885015")),
       (0x57,bytes.fromhex("8a54242088502c")),
       (0x5E,bytes.fromhex("8a542424885016c22800")),
    )

def wrapper_chunks(base:int,counter:int,preflight:int,
                   core:int,finalize:int):
    return (
       (0x00,b"\x83\x05"+pack(counter)+b"\x01"),
       (0x07,bytes.fromhex("56578b7c2414807f10008bf17509")),
       (0x15,b"\x57"),
       (0x16,rel_call(base+0x16,preflight)),
       (0x1B,bytes.fromhex("83c404")),
       (0x1E,bytes.fromhex("8b4424108b4c240c5750518bce")),
       (0x2B,rel_call(base+0x2B,core)),
       (0x30,b"\x8B\x15"+pack(counter)),
       (0x36,bytes.fromhex("57528bce")),
       (0x3A,rel_call(base+0x3A,finalize)),
       (0x3F,bytes.fromhex("8b46285f5ec20c00")),
    )

def verify_sparse(blob:bytes,length:int,w,context:str)->int:
    check(len(blob)==length,
          context+" whole original native method extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              context+" sparse native opcode witness out of range")
        check(blob[pos:pos+len(value)]==value,
              context+" original x86 structural witness mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent original prediction 23-slot provenance absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and MATRIX_PREPARE_VA==PREPARE_VA
          and DIFFERENCE_COUNT_VA==WRAPPER_VA,
          "original PostNetwork entity comparison helper targets drift")
    entity=section_read(pe,CALLBACK_VA,CALLBACK_BYTES,".text")
    check(hashlib.sha256(entity).hexdigest()==CALLBACK_SHA,
          "source-attested Phase1669 caller complete SHA mismatch")
    for site,target in ((PREPARE_CALLSITE_VA,PREPARE_VA),
                        (WRAPPER_CALLSITE_VA,WRAPPER_VA)):
        check(section_read(pe,site,5,".text")==rel_call(site,target),
              "original PostNetwork entity callsite no longer reaches helper")
    prep=section_read(pe,PREPARE_VA,PREPARE_BYTES,".text")
    wrapper=section_read(pe,WRAPPER_VA,WRAPPER_BYTES,".text")
    check(hashlib.sha256(prep).hexdigest()==PREPARE_SHA and
          hashlib.sha256(wrapper).hexdigest()==WRAPPER_SHA,
          "whole original prepare/wrapper SHA256 differs")
    prep_covered=verify_sparse(prep,PREPARE_BYTES,prepare_chunks(),
                               "prepare_10arg")
    wrap_covered=verify_sparse(wrapper,WRAPPER_BYTES,wrapper_chunks(
        WRAPPER_VA,GLOBAL_COUNT_DWORD_VA,
        PREFLIGHT_VA,CORE_VA,FINALIZE_VA),"diff_wrapper")
    check(section_read(pe,PREPARE_VA+PREPARE_BYTES,3,".text")==
          b"\xCC\xCC\xCC" and
          section_read(pe,WRAPPER_VA+WRAPPER_BYTES,3,".text")==
          b"\xCC\xCC\xCC",
          "original methods not followed by authentic native INT3 padding")
    reloc=pe_highlow_relocations(pe)
    prep_sites={site for site in reloc
                if PREPARE_VA<=site<PREPARE_VA+PREPARE_BYTES}
    wrapper_sites={site for site in reloc
                   if WRAPPER_VA<=site<WRAPPER_VA+WRAPPER_BYTES}
    check(not prep_sites and wrapper_sites==
          {WRAPPER_VA+2,WRAPPER_VA+0x32},
          "original initializer/wrapper HIGHLOW relocation sites differ")
    return {
        "phase":1670,
        "provenance":"BOTH complete original client.dll+engine.dll SHA256",
        "original_prepare_va":hex(PREPARE_VA),
        "original_prepare_whole_sha256":PREPARE_SHA,
        "original_prepare_native_bytes":PREPARE_BYTES,
        "original_prepare_thiscall_arg_dwords":PREPARE_ARG_DWORDS,
        "original_prepare_ret_pop_bytes":40,
        "original_prepare_destination_bytes":PREPARE_DESTINATION_BYTES,
        "original_prepare_witness_bytes":prep_covered,
        "original_prepare_highlow_sites":0,
        "two_flags_normalized_from_low_argument_byte":True,
        "four_single_byte_input_fields_copied":True,
        "original_wrapper_va":hex(WRAPPER_VA),
        "original_wrapper_whole_sha256":WRAPPER_SHA,
        "original_wrapper_native_bytes":WRAPPER_BYTES,
        "original_wrapper_arg_dwords":WRAPPER_ARG_DWORDS,
        "original_wrapper_ret_pop_bytes":12,
        "original_wrapper_witness_bytes":wrap_covered,
        "original_wrapper_highlow_sites":2,
        "counter_global_va":hex(GLOBAL_COUNT_DWORD_VA),
        "global_count_incremented_before_callbacks":True,
        "opaque_preflight_conditional_on_input_byte_plus_0x10_zero":True,
        "opaque_preflight_va":hex(PREFLIGHT_VA),
        "opaque_core_va":hex(CORE_VA),
        "opaque_final_va":hex(FINALIZE_VA),
        "wrapper_reads_result_from_this_plus":hex(WRAPPER_RESULT_DWORD_OFFSET),
        "opaque_original_helper_bodies_not_claimed":True,
        "total_new_whole_sha_authentic_original_method_bytes":
            PREPARE_BYTES+WRAPPER_BYTES,
        "retail_native_functions_executed":0,
        "original_windows_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original helper ABI accepted: "+label)

def self_test()->None:
    fake=bytearray(b"\xCC"*PREPARE_BYTES)
    pw=prepare_chunks()
    touched=set()
    for off,value in pw:
        fake[off:off+len(value)]=value
        touched.update(range(off,off+len(value)))
    check(verify_sparse(bytes(fake),PREPARE_BYTES,pw,"fake_prep")==
          len(touched),"synthetic prep opcode coverage drift")
    for i in sorted(touched):
        broken=bytearray(fake);broken[i]^=1
        reject(lambda data=bytes(broken):verify_sparse(
            data,PREPARE_BYTES,pw,"fake_prep"),
               "synthetic prepare opcode mutation "+str(i))
    wa=wrapper_chunks(0x45110000,0x45200000,
                      0x45121000,0x45122000,0x45123000)
    fw=bytearray(b"\xCC"*WRAPPER_BYTES)
    touched=set()
    for off,value in wa:
        fw[off:off+len(value)]=value
        touched.update(range(off,off+len(value)))
    check(verify_sparse(bytes(fw),WRAPPER_BYTES,wa,"fake_wrap")==
          len(touched),"synthetic wrapper opcode coverage drift")
    for i in sorted(touched):
        broken=bytearray(fw);broken[i]^=1
        reject(lambda data=bytes(broken):verify_sparse(
            data,WRAPPER_BYTES,wa,"fake_wrap"),
               "synthetic wrapper opcode mutation "+str(i))
    reject(lambda:verify_sparse(bytes(fake[:-1]),PREPARE_BYTES,pw,"short"),
           "truncated original prepare")
    reject(lambda:verify_sparse(bytes(fw)+b"\xCC",WRAPPER_BYTES,wa,"long"),
           "wrapper with trailing padding")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "fake original SHA should be refused")
    print("PASS Phase1670 original-free: two complete original "
          "2006 x86 helper SHAs (104B + 71B), ten-argument "
          "RET40 0x34-byte initializer with normalized input low "
          "bytes, three-argument RET12 comparison wrapper with "
          "global count, conditional preflight, opaque core/final, "
          "this+0x28 result, caller rel32 and HIGHLOW evidence; "
          "synthetic byte mutation and forged SHA refusal")

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
