#!/usr/bin/env python3
"""Phase1667: exact original C_BaseEntity predictable shutdown 61B x86 method.

Original owner-only SHA gates: 2006 PE32 client.dll AND engine.dll,
whole function SHA of 0x240CA9F0, native call and clear order,
handle at entity+0xB4, flag at entity+0x2E8 and vptr+0x44 call.
Public --self-test synthesizes sparse non-executing instruction windows,
with byte mutation and forged SHA refusal. No retail code executes.
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
from audit_sin_original_prediction_disabled_fallback import (
    FALLBACK_VA,FALLBACK_LENGTH,FALLBACK_SHA,PREDICTABLE_SHUTDOWN_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

TARGET_VA=0x240CA9F0
TARGET_BYTES=61
TARGET_SHA="f8bc7dad78fd2752442ef4e7544464b618a377bfd3ba267c36c1ac4a742ec4d0"
ENTITY_HANDLE_FIELD_OFFSET=0xB4
ENTITY_PREDICTABLE_BYTE_OFFSET=0x2E8
GLOBAL_REGISTRATION_HELPER_OBJECT=0x242F7F7C
UNREGISTER_CALL=0x240C98B0
PRIMARY_TEARDOWN_CALL=0x240C5710
ENTITY_CALLBACK_VTABLE_OFFSET=0x44
FINAL_TEARDOWN_CALL=0x240C8F00

def pack(value:int)->bytes:
    return struct.pack("<I",value)

def rel_call(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witnesses(base:int,registry:int,
              unregister:int,primary:int,final:int):
    return (
        (0x00,bytes.fromhex("51568bf1")),
        (0x04,b"\x8B\x86"+pack(ENTITY_HANDLE_FIELD_OFFSET)),
        (0x0F,b"\xB9"+pack(registry)),
        (0x18,rel_call(base+0x18,unregister)),
        (0x1D,bytes.fromhex("8bce")),
        (0x1F,rel_call(base+0x1F,primary)),
        (0x24,bytes.fromhex("8b168bce")),
        (0x28,b"\xC6\x86"+pack(ENTITY_PREDICTABLE_BYTE_OFFSET)+b"\x00"),
        (0x2F,bytes.fromhex("ff5244")),
        (0x32,bytes.fromhex("508bce")),
        (0x35,rel_call(base+0x35,final)),
        (0x3A,bytes.fromhex("5e59c3")),
    )

def verify_sparse(blob:bytes,length:int,w)->int:
    check(len(blob)==length,
          "predictable entity teardown native function must be 61B")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              "predictable teardown out-of-range witness")
        check(blob[pos:pos+len(value)]==value,
              "predictable teardown x86 original witness mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "original prediction ABI whole-file parent missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          TARGET_VA==PREDICTABLE_SHUTDOWN_VA,
          "original fallback direct callback target differs")
    native=section_read(pe,TARGET_VA,TARGET_BYTES,".text")
    check(hashlib.sha256(native).hexdigest()==TARGET_SHA,
          "original complete C_BaseEntity 61-byte teardown SHA changed")
    covered=verify_sparse(native,TARGET_BYTES,witnesses(
        TARGET_VA,GLOBAL_REGISTRATION_HELPER_OBJECT,
        UNREGISTER_CALL,PRIMARY_TEARDOWN_CALL,FINAL_TEARDOWN_CALL))
    check(section_read(pe,TARGET_VA+TARGET_BYTES,2,".text")==b"\xCC\xCC",
          "original predictable cleanup not followed by INT3 padding")
    fallback=section_read(pe,FALLBACK_VA,FALLBACK_LENGTH,".text")
    check(hashlib.sha256(fallback).hexdigest()==FALLBACK_SHA and
          fallback[0x3E:0x43]==rel_call(
              FALLBACK_VA+0x3E,TARGET_VA),
          "Phase1666 disabled prediction fallback is not calling this method")
    reloc=pe_highlow_relocations(pe)
    check(TARGET_VA+0x10 in reloc,
          "native global registry immediate does not have HIGHLOW relocation")
    return {
        "phase":1667,
        "provenance":"full original SHA256-matched client.dll + engine.dll",
        "original_predictable_cleanup_va":hex(TARGET_VA),
        "original_complete_native_bytes":TARGET_BYTES,
        "original_whole_method_sha256":TARGET_SHA,
        "native_sparse_opcode_witness_bytes":covered,
        "original_predictable_entity_handle_member":hex(ENTITY_HANDLE_FIELD_OFFSET),
        "original_global_registration_service_va":
            hex(GLOBAL_REGISTRATION_HELPER_OBJECT),
        "original_unregister_helper_va":hex(UNREGISTER_CALL),
        "original_primary_teardown_helper_va":hex(PRIMARY_TEARDOWN_CALL),
        "original_eligibility_byte_cleared_at":
            hex(ENTITY_PREDICTABLE_BYTE_OFFSET),
        "original_entity_primary_vtable_callback_offset":
            hex(ENTITY_CALLBACK_VTABLE_OFFSET),
        "original_final_helper_va":hex(FINAL_TEARDOWN_CALL),
        "original_vcall_return_value_passed_to_final_helper":True,
        "original_native_cleanup_ret_bytes":0,
        "original_method_calls_executed":0,
        "retail_windows_gameplay_boot_verified":False,
    }

def reject(fn,reason:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original predictable shutdown accepted: "+
                         reason)

def self_test()->None:
    fake=0x45110000
    w=witnesses(fake,0x46110000,0x45120000,
                0x45130000,0x45140000)
    invented=bytearray(b"\xCC"*TARGET_BYTES)
    covered=set()
    for off,value in w:
        invented[off:off+len(value)]=value
        covered.update(range(off,off+len(value)))
    check(verify_sparse(bytes(invented),TARGET_BYTES,w)==len(covered),
          "invented native predictable cleanup structural witness drift")
    for off in sorted(covered):
        broken=bytearray(invented)
        broken[off]^=1
        reject(lambda data=bytes(broken):verify_sparse(data,TARGET_BYTES,w),
               "native x86 byte "+str(off))
    reject(lambda:verify_sparse(bytes(invented[:-1]),TARGET_BYTES,w),
           "original method truncation")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",TARGET_BYTES,w),
           "padding interpreted as method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner original complete file SHA forged")
    print("PASS Phase1667 original-free: C_BaseEntity 61-byte "
          "predictable teardown complete native SHA, +0xB4 handle, "
          "registry unregister -> primary cleanup -> flag+0x2E8 "
          "clear -> entity vcall+0x44 -> final-helper call with "
          "virtual return, original disabled fallback direct xref, "
          "HIGHLOW registry pointer and byte-level fake corruption")

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
