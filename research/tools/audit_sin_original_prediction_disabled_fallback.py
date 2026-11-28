#!/usr/bin/env python3
"""Phase1666: original CPrediction disabled-predictable shutdown fallback.

The owner-only audit SHA-authenticates exact original 2006 PE32
client.dll and engine.dll before examining one 118-byte native method
at 0x24195720, plus its direct edge from PreEntityPacketReceived.
The public self-test constructs only non-executable sparse invented
x86 witnesses. Neither original code nor commercial DLLs are shipped.
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
from audit_sin_original_prediction_packet_entity_pair import (
    PRE_VA,PRE_BYTES,PRE_SHA,PRE_DISABLED_FALLBACK_VA,
    ENTITY_LIST_GLOBAL_VA,IS_PREDICTABLE_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

FALLBACK_VA=0x24195720
FALLBACK_LENGTH=118
FALLBACK_SHA="88a6354f6a65291b7036c5305c67e3a307ce828fb1196172af78b31fccbe9f8f"
PREDICTABLE_SHUTDOWN_VA=0x240CA9F0
CLIENT_CREATED_EMBEDDED_VPTR_OFFSET=8
CLIENT_CREATED_VIRTUAL_SHUTDOWN_OFFSET=4
ENTITY_LIST_COUNT_VPTR_OFFSET=4
ENTITY_LIST_GET_VPTR_OFFSET=0
SUMMARY_FORMAT_VA=0x242A3560
SUMMARY_FORMAT=b"Shutdown %i predictable entities and %i client-created entities\n\0"
SUMMARY_PRINT_IAT_VA=0x242761EC
RELOC_OFFSETS=(0x02,0x22,0x65,0x6B)
PRE_DIRECT_FALLBACK_CALL_OFFSET=0x34

def pack(value:int)->bytes:
    return struct.pack("<I",value)

def rel_call(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witnesses(base:int,entitylist:int,predictable:int,shutdown:int,
              summary:int,print_iat:int):
    # Sparse islands of x86 evidence; no original whole code body.
    return (
        (0x00,b"\x8B\x0D"+pack(entitylist)),
        (0x06,bytes.fromhex("8b0153555733ed33dbff5004")),
        (0x12,bytes.fromhex("8bf883ef017859")),
        (0x19,b"\x56"),
        (0x20,b"\x8B\x0D"+pack(entitylist)),
        (0x26,bytes.fromhex("8b1157ff12")),
        (0x2B,bytes.fromhex("8bf085f67423")),
        (0x31,bytes.fromhex("8bce")),
        (0x33,rel_call(base+0x33,predictable)),
        (0x38,bytes.fromhex("84c0740c8bce")),
        (0x3E,rel_call(base+0x3E,shutdown)),
        (0x43,bytes.fromhex("83c501eb0c")),
        (0x48,bytes.fromhex("8b46088d4e08ff5004")),
        (0x51,bytes.fromhex("83c301")),
        (0x54,bytes.fromhex("83ef0179c7")),
        (0x59,bytes.fromhex("85db5e7f04")),
        (0x5E,bytes.fromhex("85ed7e10")),
        (0x62,b"\x53\x55\x68"+pack(summary)),
        (0x69,b"\xFF\x15"+pack(print_iat)),
        (0x6F,bytes.fromhex("83c40c")),
        (0x72,bytes.fromhex("5f5d5bc3")),
    )

def verify_sparse(blob:bytes,length:int,w,context:str)->int:
    check(len(blob)==length,
          context+" original 118-byte native method extent differs")
    positions=set()
    for pos,value in w:
        check(pos>=0 and pos+len(value)<=len(blob),
              context+" sparse x86 witness exceeds method")
        check(blob[pos:pos+len(value)]==value,
              context+" sparse original x86 instructions differ @"+
              hex(pos))
        positions.update(range(pos,pos+len(value)))
    return len(positions)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "source-attested CPrediction parent ABI missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          PRE_DISABLED_FALLBACK_VA==FALLBACK_VA,
          "original disabled-prediction fallback target changed")
    native=section_read(pe,FALLBACK_VA,FALLBACK_LENGTH,".text")
    check(hashlib.sha256(native).hexdigest()==FALLBACK_SHA,
          "original complete 118-byte fallback method SHA changed")
    count=verify_sparse(native,FALLBACK_LENGTH,
        witnesses(FALLBACK_VA,ENTITY_LIST_GLOBAL_VA,
                  IS_PREDICTABLE_VA,PREDICTABLE_SHUTDOWN_VA,
                  SUMMARY_FORMAT_VA,SUMMARY_PRINT_IAT_VA),
        "disabled_predictable_shutdown")
    check(section_read(pe,FALLBACK_VA+FALLBACK_LENGTH,2,".text")==
          b"\xCC\xCC",
          "original fallback method boundary not INT3 padded")
    check(section_read(pe,SUMMARY_FORMAT_VA,
                       len(SUMMARY_FORMAT),".rdata")==SUMMARY_FORMAT,
          "original predictable/client-created shutdown summary differs")
    pre=section_read(pe,PRE_VA,PRE_BYTES,".text")
    check(hashlib.sha256(pre).hexdigest()==PRE_SHA and
          pre[PRE_DIRECT_FALLBACK_CALL_OFFSET:
              PRE_DIRECT_FALLBACK_CALL_OFFSET+5]==rel_call(
                  PRE_VA+PRE_DIRECT_FALLBACK_CALL_OFFSET,FALLBACK_VA),
          "native disabled cl_predict branch does not call exact helper")
    reloc=pe_highlow_relocations(pe)
    for off in RELOC_OFFSETS:
        check(FALLBACK_VA+off in reloc,
              "fallback original pointer operand lacks HIGHLOW relocation "+
              hex(FALLBACK_VA+off))
    return {
        "phase":1666,
        "provenance":"full original SHA256 matched client.dll and engine.dll",
        "original_fallback_entry_va":hex(FALLBACK_VA),
        "original_whole_method_sha256":FALLBACK_SHA,
        "new_original_fully_authenticated_method_bytes":FALLBACK_LENGTH,
        "native_sparse_opcode_witness_bytes":count,
        "pre_prediction_disabled_direct_calls_fallback":True,
        "fallback_n_stack_args":0,
        "fallback_ret_pop_bytes":0,
        "client_entity_list_global_va":hex(ENTITY_LIST_GLOBAL_VA),
        "client_entity_count_virtual_offset":ENTITY_LIST_COUNT_VPTR_OFFSET,
        "client_entity_get_virtual_offset":ENTITY_LIST_GET_VPTR_OFFSET,
        "iterates_backward_from_count_minus_one_to_zero":True,
        "skips_null_entities":True,
        "predictable_flag_getter_va":hex(IS_PREDICTABLE_VA),
        "predictable_shutdown_callback_va":hex(PREDICTABLE_SHUTDOWN_VA),
        "client_created_embedded_vptr_offset":
            CLIENT_CREATED_EMBEDDED_VPTR_OFFSET,
        "client_created_shutdown_vcall_offset":
            CLIENT_CREATED_VIRTUAL_SHUTDOWN_OFFSET,
        "summary_print_import_va":hex(SUMMARY_PRINT_IAT_VA),
        "summary_only_if_either_count_positive":True,
        "summary_predictable_count_first":True,
        "original_method_calls_executed":0,
        "retail_windows_gameplay_boot_verified":False,
    }

def reject(fn,reason:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("invalid original fallback was accepted: "+reason)

def self_test()->None:
    fake_base=0x45110000
    w=witnesses(fake_base,0x46200000,0x45121000,
                0x45122000,0x46210000,0x46211000)
    invented=bytearray(b"\xCC"*FALLBACK_LENGTH)
    positions=set()
    for off,fragment in w:
        invented[off:off+len(fragment)]=fragment
        positions.update(range(off,off+len(fragment)))
    check(verify_sparse(bytes(invented),FALLBACK_LENGTH,w,
                        "invented_shutdown")==len(positions),
          "synthetic method sparse witness coverage drift")
    for off in sorted(positions):
        bad=bytearray(invented)
        bad[off] ^= 0x01
        reject(lambda data=bytes(bad):verify_sparse(
            data,FALLBACK_LENGTH,w,"invented_shutdown"),
            "x86 opcode or operand mutation "+str(off))
    reject(lambda:verify_sparse(bytes(invented[:-1]),
                                FALLBACK_LENGTH,w,"short_method"),
           "truncated original method accepted")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",
                                FALLBACK_LENGTH,w,"overlong_method"),
           "original method plus INT3 padding accepted")
    fake_call=rel_call(0x45130000,fake_base)
    check(fake_call[0]==0xE8 and
          0x45130005+struct.unpack_from("<i",fake_call,1)[0]==fake_base,
          "invented Pre direct CALL target calculation differs")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "fake whole-image SHA was accepted")
    print("PASS Phase1666 original-free: whole-SHA-gated "
          "118-byte disabled-prediction shutdown helper, "
          "reverse client entity-list traversal, IsPredictable "
          "split, distinct client-created vptr+4 shutdown, "
          "two authentic count/print argument orders, skip-zero "
          "summary, direct Pre fallback CALL and synthetic corruption")

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
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
