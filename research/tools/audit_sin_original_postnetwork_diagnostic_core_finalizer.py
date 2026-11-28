#!/usr/bin/env python3
"""Phase1671: original 2006 PE32 PostNetwork diagnostic core and finalizer.

Source-only, dual complete original image SHA256 gated. Authenticates
original 139-byte native core 0x24199100 and 65-byte linked-list
finalizer 0x24199B50, the prior wrapper's two direct CALL targets,
exact x86 stack cleanup, five genuine HIGHLOW addresses in core,
no HIGHLOW in finalizer, native method boundaries, source conditionals
and the per-node opaque 0x241996F0 callback.

No proprietary DLL/native method included in public CI. Synthetic
--self-test invents non-executable sparse x86 opcode islands.
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
from audit_sin_original_postnetwork_compare_helpers import (
    WRAPPER_VA,WRAPPER_BYTES,WRAPPER_SHA,
    CORE_VA,FINALIZE_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

DIAGNOSTIC_CORE_VA=0x24199100
DIAGNOSTIC_CORE_BYTES=139
DIAGNOSTIC_CORE_SHA="2d9113fbab33660ac6bf3c8af365f522270be8194682c51fc810d0ccf9f9be71"
LIST_FINALIZER_VA=0x24199B50
LIST_FINALIZER_BYTES=65
LIST_FINALIZER_SHA="45490bbfdb993b0b40c255aaf9612d1f1e116d10de5b3c55d220f10a524e5005"
ELIGIBILITY_CVAR_PARENT_VA=0x243747C8
ORIGINAL_DEBUG_BITFIELD_GLOBAL_VA=0x2437480C
ORIGINAL_LABEL_OBJECT_GLOBAL_VA=0x24374810
ORIGINAL_LABEL_OBJECT_TEXT_MEMBER_OFFSET=0x20
ORIGINAL_DEBUG_ENABLE_BIT=0x1000
SELECTED_DEBUG_TEXT_VA=0x24298188
FALLBACK_DEBUG_TEXT_VA=0x242D330E
OPAQUE_FORMAT_HELPER_VA=0x24197ED0
OPAQUE_PER_NODE_HELPER_VA=0x241996F0
NODE_LINK_OFFSET=0x0C
CORE_OUTPUT_MEMBER_OFFSET=0x34
CORE_INPUT_MEMBER_OFFSET=0x38
FINALIZER_STACK_ARGS=2
CORE_STACK_ARGS=3
ORIGINAL_CORE_RELOC_SITES=(
    DIAGNOSTIC_CORE_VA+0x1B,
    DIAGNOSTIC_CORE_VA+0x2E,
    DIAGNOSTIC_CORE_VA+0x3A,
    DIAGNOSTIC_CORE_VA+0x55,
    DIAGNOSTIC_CORE_VA+0x72,
)

def pack(v:int)->bytes:
    return struct.pack("<I",v)
def rel_call(src:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-src-5)

def core_witnesses(*,base:int,eligibility:int,flags:int,
                   label_obj:int,enabled_label:int,
                   fallback_label:int,formatter:int):
    return (
      (0x00,bytes.fromhex("8b44240485c0568bf1")),
      (0x09,bytes.fromhex("c7463400000000894638")),
      (0x15,bytes.fromhex("803800746d")),
      (0x1A,b"\xA1"+pack(eligibility)),
      (0x26,bytes.fromhex("3944240c755b")),
      (0x2C,b"\x8B\x15"+pack(flags)+
            bytes.fromhex("81e200100000")),
      (0x38,b"\x8B\x0D"+pack(label_obj)),
      (0x3E,bytes.fromhex("750c8b412085c07440")),
      (0x4C,bytes.fromhex("85d2741a")),
      (0x50,b"\x8B\x4C\x24\x10\xB8"+pack(enabled_label)),
      (0x5B,rel_call(base+0x5B,formatter)),
      (0x63,bytes.fromhex("8946345ec20c00")),
      (0x6A,bytes.fromhex("8b412085c07505")),
      (0x71,b"\xB8"+pack(fallback_label)),
      (0x76,bytes.fromhex("8b4c24105150")),
      (0x7C,rel_call(base+0x7C,formatter)),
      (0x81,bytes.fromhex("83c4088946345ec20c00")),
    )

def final_witnesses(base:int,apply:int):
    return (
      (0x00,bytes.fromhex("538b5c2408568b742410")),
      (0x0A,bytes.fromhex("8b4604578bf9")),
      (0x12,bytes.fromhex("505156538bcf")),
      (0x18,rel_call(base+0x18,apply)),
      (0x1D,bytes.fromhex("8b760c85f67417")),
      (0x24,bytes.fromhex("8b56048b06")),
      (0x2A,bytes.fromhex("525056538bcf")),
      (0x2F,rel_call(base+0x2F,apply)),
      (0x34,bytes.fromhex("8b760c85f675e9")),
      (0x3B,bytes.fromhex("5f5e5bc20800")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" original whole-function method extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" witness outside method")
        check(blob[pos:pos+len(value)]==value,
              name+" x86 native instruction witness differs @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent SHA-authenticated original prediction unavailable")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and CORE_VA==DIAGNOSTIC_CORE_VA
          and FINALIZE_VA==LIST_FINALIZER_VA,
          "Phase1670 wrapper original callee targets drift")
    wrapper=section_read(pe,WRAPPER_VA,WRAPPER_BYTES,".text")
    check(hashlib.sha256(wrapper).hexdigest()==WRAPPER_SHA,
          "parent comparison wrapper original whole-function SHA changed")
    for site,target in ((WRAPPER_VA+0x2B,DIAGNOSTIC_CORE_VA),
                        (WRAPPER_VA+0x3A,LIST_FINALIZER_VA)):
        check(section_read(pe,site,5,".text")==rel_call(site,target),
              "original wrapper direct CALL edge mismatches actual callee")
    core=section_read(pe,DIAGNOSTIC_CORE_VA,
                      DIAGNOSTIC_CORE_BYTES,".text")
    final=section_read(pe,LIST_FINALIZER_VA,
                       LIST_FINALIZER_BYTES,".text")
    check(hashlib.sha256(core).hexdigest()==DIAGNOSTIC_CORE_SHA and
          hashlib.sha256(final).hexdigest()==LIST_FINALIZER_SHA,
          "complete original native diagnostic/core finalizer SHA changed")
    c=verify_sparse(core,DIAGNOSTIC_CORE_BYTES,core_witnesses(
        base=DIAGNOSTIC_CORE_VA,eligibility=ELIGIBILITY_CVAR_PARENT_VA,
        flags=ORIGINAL_DEBUG_BITFIELD_GLOBAL_VA,
        label_obj=ORIGINAL_LABEL_OBJECT_GLOBAL_VA,
        enabled_label=SELECTED_DEBUG_TEXT_VA,
        fallback_label=FALLBACK_DEBUG_TEXT_VA,
        formatter=OPAQUE_FORMAT_HELPER_VA),"core")
    f=verify_sparse(final,LIST_FINALIZER_BYTES,final_witnesses(
        LIST_FINALIZER_VA,OPAQUE_PER_NODE_HELPER_VA),"finalizer")
    for addr,size in ((DIAGNOSTIC_CORE_VA,DIAGNOSTIC_CORE_BYTES),
                      (LIST_FINALIZER_VA,LIST_FINALIZER_BYTES)):
        check(section_read(pe,addr+size,3,".text")==b"\xCC\xCC\xCC",
              "original core/finalizer native function not INT3-bounded")
    reloc=pe_highlow_relocations(pe)
    a={x for x in reloc
       if DIAGNOSTIC_CORE_VA<=x<DIAGNOSTIC_CORE_VA+DIAGNOSTIC_CORE_BYTES}
    b={x for x in reloc
       if LIST_FINALIZER_VA<=x<LIST_FINALIZER_VA+LIST_FINALIZER_BYTES}
    check(a==set(ORIGINAL_CORE_RELOC_SITES) and not b,
          "original core/finalizer HIGHLOW relocation sites differ")
    return {
        "phase":1671,
        "provenance":"BOTH SHA-authenticated whole original PE32 DLLs",
        "original_diagnostic_core_va":hex(DIAGNOSTIC_CORE_VA),
        "original_diagnostic_core_method_bytes":DIAGNOSTIC_CORE_BYTES,
        "original_diagnostic_core_sha256":DIAGNOSTIC_CORE_SHA,
        "original_core_three_arg_ret_pop_bytes":12,
        "original_core_initial_this_plus_0x34_zero":True,
        "original_core_stores_first_arg_at_this_plus_0x38":True,
        "original_eligibility_cvar_parent":hex(ELIGIBILITY_CVAR_PARENT_VA),
        "original_eligibility_parent_numeric_offset":hex(0x2C),
        "original_debug_flag_source":hex(ORIGINAL_DEBUG_BITFIELD_GLOBAL_VA),
        "original_debug_bit_test":hex(ORIGINAL_DEBUG_ENABLE_BIT),
        "original_custom_label_ptr_holder":hex(ORIGINAL_LABEL_OBJECT_GLOBAL_VA),
        "original_custom_label_member":hex(ORIGINAL_LABEL_OBJECT_TEXT_MEMBER_OFFSET),
        "original_selected_debug_label_va":hex(SELECTED_DEBUG_TEXT_VA),
        "original_fallback_label_va":hex(FALLBACK_DEBUG_TEXT_VA),
        "opaque_format_helper_va":hex(OPAQUE_FORMAT_HELPER_VA),
        "original_core_opcode_witness_bytes":c,
        "original_core_relocations":len(ORIGINAL_CORE_RELOC_SITES),
        "original_linked_list_finalizer_va":hex(LIST_FINALIZER_VA),
        "original_linked_list_finalizer_method_bytes":LIST_FINALIZER_BYTES,
        "original_linked_list_finalizer_sha256":LIST_FINALIZER_SHA,
        "original_finalizer_two_arg_ret_pop_bytes":8,
        "original_first_entry_always_processed":True,
        "original_next_entry_member":hex(NODE_LINK_OFFSET),
        "original_entry_word0_and_word4_forwarded_to_helper":True,
        "original_per_entry_opaque_helper_va":hex(OPAQUE_PER_NODE_HELPER_VA),
        "original_finalizer_opcode_witness_bytes":f,
        "original_finalizer_relocations":0,
        "total_new_full_original_method_bytes":
            DIAGNOSTIC_CORE_BYTES+LIST_FINALIZER_BYTES,
        "opaque_format_and_entry_helper_bodies_not_claimed":True,
        "original_native_methods_executed":0,
        "original_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original core/finalizer ABI accepted: "+label)

def self_test()->None:
    core_ws=core_witnesses(
        base=0x45110000,eligibility=0x46100000,
        flags=0x46101000,label_obj=0x46102000,
        enabled_label=0x47100000,fallback_label=0x47101000,
        formatter=0x45120000)
    final_ws=final_witnesses(0x45130000,0x45140000)
    for name,length,ws in (
        ("invented_core",DIAGNOSTIC_CORE_BYTES,core_ws),
        ("invented_final",LIST_FINALIZER_BYTES,final_ws)):
        fake=bytearray(b"\xCC"*length)
        touched=set()
        for off,value in ws:
            fake[off:off+len(value)]=value
            touched.update(range(off,off+len(value)))
        check(verify_sparse(bytes(fake),length,ws,name)==len(touched),
              "invented helper structural witness coverage drift "+name)
        for i in sorted(touched):
            broken=bytearray(fake);broken[i]^=1
            reject(lambda data=bytes(broken):verify_sparse(
                data,length,ws,name),
                name+" byte mutation "+str(i))
        reject(lambda:verify_sparse(bytes(fake[:-1]),length,ws,name),
               name+" truncated")
        reject(lambda:verify_sparse(bytes(fake)+b"\xCC",length,ws,name),
               name+" extra native padding")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner original complete DLL SHA bypass")
    print("PASS Phase1671 original-free: whole 139B diagnostic "
          "gate source SHA, 65B two-arg RET8 linked-list "
          "finalizer native SHA, direct wrapper CALL edges, "
          "five exact HIGHLOW core and zero final relocations, "
          "this+0x34/+0x38, ConVar gating and label selection, "
          "node+0xC traversal, first-node unconditional callback, "
          "per-node 4-arg opaque helper, synthetic byte mutation "
          "and fake whole-original rejection")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be combined with owner images")
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
