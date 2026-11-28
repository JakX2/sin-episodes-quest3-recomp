#!/usr/bin/env python3
"""Phase1680: original complete 121-byte CDECL PostNetwork preflight
0x24197F50, direct from unchanged owner 2006 PE32 client+engine.

One stack pointer argument, RET0/caller cleanup4. For a node array
with signed count at +4, marks node+0x10 byte=1 even when count<=0.
Records stride 0x34; type0 skipped; type10 recursively walks the
nested +0x20 node (before any flag check). Non-type10 records with
byte flag [record+0x12]&0x80 and node+0x0C nonnull call Phase1679
name finder 0x24197ED0 with name at record+4 and next node as
second arg; on nonzero match store its pointer at record+0x28.
Traverses node+0x0C only while the NEXT node+0x10 byte is zero.
The native code has no validity checks for arbitrary pointers.

Source requires SHA256-pinned whole original retail client+engine.
Public --self-test uses invented non-executable sparse code islands;
original x86 bytes/images are never shipped or executed.
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
    WRAPPER_VA,WRAPPER_BYTES,WRAPPER_SHA,PREFLIGHT_VA,
)
from audit_sin_original_postnetwork_record_name_lookup import (
    LOOKUP_VA,LOOKUP_BYTES,LOOKUP_SHA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

PRE_VA=0x24197F50
PRE_BYTES=121
PRE_SHA="27a9fc1f34bf44aa1bda7844dc4276085117fd2b5c2c6239a93f6ef59d9b84e7"
PRE_CALLSITE=WRAPPER_VA+0x16
SELF_RECURSE_SITE=PRE_VA+0x34
FIND_NAME_SITE=PRE_VA+0x50
PRE_RET_POP=0
PRE_STACK_ARGS=1
PRE_PADDING=7
NATIVE_REC_STRIDE=0x34
NODE_STATUS_OFFSET=0x10
NODE_NEXT_OFFSET=0x0C
NODE_COUNT_OFFSET=0x04
RECORD_TYPE_OFFSET=0
RECORD_NAME_OFFSET=4
RECORD_FLAGS_OFFSET=0x12
RECORD_NESTED_OFFSET=0x20
RECORD_ALIAS_OFFSET=0x28
FLAG_ALIAS_LOOKUP=0x80
TYPE_RECURSE=10

def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witness_chunks(*,base:int,lookup_va:int):
    return (
        (0x00,bytes.fromhex("535556578b7c2414eb06")),
        (0x10,bytes.fromhex("8b470485c0c64710017e4c")),
        (0x20,bytes.fromhex("8b378b042e03f585c0743483f80a750e")),
        (0x30,bytes.fromhex("8b462050")),
        (0x34,relcall(base+0x34,base)),
        (0x39,bytes.fromhex("83c404eb21")),
        (0x3E,bytes.fromhex("f6461280741b")),
        (0x44,bytes.fromhex("8b470c85c07414")),
        (0x4B,bytes.fromhex("8b4e045051")),
        (0x50,relcall(base+0x50,lookup_va)),
        (0x58,bytes.fromhex("85c07403894628")),
        (0x5F,bytes.fromhex("83c53483eb0175b9")),
        (0x67,bytes.fromhex("8b7f0c85ff7406807f1000749c")),
        (0x74,bytes.fromhex("5f5e5d5bc3")),
    )
def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,name+" whole method extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" native witness outside exact method")
        check(blob[pos:pos+len(value)]==value,
              name+" native opcode drift +"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)
def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "dual original image provenance not authenticated")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and PRE_VA==PREFLIGHT_VA,
          "original compare wrapper preflight target changed")
    wrapper=section_read(pe,WRAPPER_VA,WRAPPER_BYTES,".text")
    findcode=section_read(pe,LOOKUP_VA,LOOKUP_BYTES,".text")
    check(hashlib.sha256(wrapper).hexdigest()==WRAPPER_SHA and
          hashlib.sha256(findcode).hexdigest()==LOOKUP_SHA,
          "native authenticated wrapper/finder parent method drift")
    check(section_read(pe,PRE_CALLSITE,5,".text")==
          relcall(PRE_CALLSITE,PRE_VA),
          "wrapper no longer invokes original preflight")
    code=section_read(pe,PRE_VA,PRE_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==PRE_SHA,
          "whole original 121B preflight native SHA256 differs")
    covered=verify_sparse(code,PRE_BYTES,
        witness_chunks(base=PRE_VA,lookup_va=LOOKUP_VA),
        "original_preflight")
    check(code[-1:]==b"\xC3" and
          section_read(pe,PRE_VA+PRE_BYTES,PRE_PADDING,".text")==
          b"\xCC"*PRE_PADDING,
          "native RET0 / seven post-method INT3s changed")
    own={x for x in pe_highlow_relocations(pe)
         if PRE_VA<=x<PRE_VA+PRE_BYTES}
    check(not own,"original preflight unexpectedly contains HIGHLOW pointer")
    return {
        "phase":1680,
        "provenance":"BOTH SHA-authenticated whole original 2006 PE32 client.dll+engine.dll",
        "original_va":hex(PRE_VA),
        "original_native_method_bytes":PRE_BYTES,
        "original_whole_method_sha256":PRE_SHA,
        "original_cdecl_one_stack_arg_ret_pop":PRE_RET_POP,
        "original_wrapper_preflight_callsite":hex(PRE_CALLSITE),
        "original_self_recursive_callsite":hex(SELF_RECURSE_SITE),
        "original_name_finder_callsite":hex(FIND_NAME_SITE),
        "original_node_visited_status_byte_offset":hex(NODE_STATUS_OFFSET),
        "original_node_next_link_offset":hex(NODE_NEXT_OFFSET),
        "original_node_signed_count_offset":hex(NODE_COUNT_OFFSET),
        "original_record_stride":hex(NATIVE_REC_STRIDE),
        "original_record_flag_alias_lookup_bit":hex(FLAG_ALIAS_LOOKUP),
        "original_type10_recursive_first":True,
        "original_record_name_offset":hex(RECORD_NAME_OFFSET),
        "original_record_nested_node_offset":hex(RECORD_NESTED_OFFSET),
        "original_record_alias_output_offset":hex(RECORD_ALIAS_OFFSET),
        "original_next_node_already_processed_skip":True,
        "original_highlow_relocs":0,
        "postmethod_int3_padding":PRE_PADDING,
        "sparse_native_witness_bytes":covered,
        "original_retail_methods_executed":0,
        "retail_windows_gameplay_boot_verified":False,
    }
def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original preflight accepted: "+label)
def self_test()->None:
    ws=witness_chunks(base=0x45110000,lookup_va=0x45200000)
    fake=bytearray(b"\xCC"*PRE_BYTES)
    covered=set()
    for pos,value in ws:
        fake[pos:pos+len(value)]=value
        covered.update(range(pos,pos+len(value)))
    check(verify_sparse(bytes(fake),PRE_BYTES,ws,"invented")==
          len(covered),"invented original-free preflight coverage drift")
    for pos in sorted(covered):
        changed=bytearray(fake);changed[pos]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,PRE_BYTES,ws,"mutated"),str(pos))
    reject(lambda:verify_sparse(bytes(fake[:-1]),PRE_BYTES,ws,"short"),
           "truncated original method")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",PRE_BYTES,ws,"long"),
           "included INT3 as executable")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "whole original SHA bypass")
    print("PASS Phase1680 original-free: whole 121B SHA cdecl RET0 "
          "preflight, node+0x10 visit flag, 0x34 record stride, "
          "recursive type10, 0x80 alias lookup into next-node "
          "via whole-SHA original 121B finder, conditional record+0x28 "
          "write, visited next-node test, wrapper call and "
          "zero relocations, sparse mutants/forged retail refusal")
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
