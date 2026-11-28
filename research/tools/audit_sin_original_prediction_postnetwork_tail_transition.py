#!/usr/bin/env python3
"""Phase1682: exact-source audit of the 58-byte cl_predict state
transition at original CPrediction::PostNetworkDataReceived slot6
0x2419646B..0x241964A4 inside the EXISTING SHA-authenticated
1039-byte function 0x241960E0 (no new native code bytes claimed).

The original, after its earlier player/prediction/entity branches,
checks BOOL(ConVar parent m_nValue) against prediction byte+0x10.
When it changes from zero, calls still-opaque 0x24195C20 before
inline reset of +0x28/+0x2C and -1 at +0x24; then writes new
+0x10. The next 0x240C4940 helper remains opaque and populates
additional member words; no complete post-loop behavior claimed.

Requires owner SHA256 exact original retail client+engine for real
audit. Public --self-test only tests invented non-executable sparse
native opcode fragments and rejects all witnessed bit mutations.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_prediction_postnetwork_slot6 import (
    POST_NETWORK_VA,POST_NETWORK_SIZE,POST_NETWORK_SHA,
    CL_PREDICT_PARENT_VA,
)
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,verify_original,check,section_read,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

TRANSITION_START_VA=0x2419646B
TRANSITION_END_EXCLUSIVE_VA=0x241964A5
TRANSITION_SOURCE_BYTES=TRANSITION_END_EXCLUSIVE_VA-TRANSITION_START_VA
OPAQUE_DISABLED_TO_ENABLED_VA=0x24195C20
OPAQUE_SNAPSHOT_REFRESH_VA=0x240C4940
HELPER_CALL_SITE=0x24196485
SNAPSHOT_CALL_SITE=0x241964A9
SOURCE_OWNER_ENABLED_OFFSET=0x10
SOURCE_OWNER_FRAME_OFFSET=0x2C
SOURCE_OWNER_RESET_INDEX_OFFSET=0x24
SOURCE_OWNER_RESET_A_OFFSET=0x28
SOURCE_OWNER_RESET_B_OFFSET=0x2C
ORIGINAL_CL_PREDICT_PARENT_VALUE_OFFSET=0x2C

def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witness_chunks(*,predict_va:int,helper_va:int,
                   snapshot_va:int):
    return (
        (0x38B,b"\x8B\x0D"+struct.pack("<I",predict_va)+
            bytes.fromhex("39792c8a43100f95c23ad0")),
        (0x39C,bytes.fromhex("5e741884c075078bcb")),
        (0x3A5,relcall(HELPER_CALL_SITE,helper_va)),
        (0x3AA,bytes.fromhex("897b28897b2cc74324ffffffff")),
        (0x3B7,b"\xA1"+struct.pack("<I",predict_va)+
            bytes.fromhex("39782c0f95c1884b10")),
        (0x3C5,bytes.fromhex("8b4c242c")+
            relcall(SNAPSHOT_CALL_SITE,snapshot_va)),
        (0x3E7,bytes.fromhex("807c240f00740c807c240e007505")),
        (0x3FA,bytes.fromhex("8b0d")+
            struct.pack("<I",0x242761F4)+
            bytes.fromhex("ff15")+
            struct.pack("<I",0x242761F0)),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" whole original 1039-byte method extent changed")
    covered=set()
    for off,fragment in w:
        check(0<=off and off+len(fragment)<=length,
              name+" native source witness exceeds method extent")
        check(blob[off:off+len(fragment)]==fragment,
              name+" source transition byte changed @"+hex(off))
        covered.update(range(off,off+len(fragment)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "whole original client+engine provenance missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and TRANSITION_SOURCE_BYTES==58,
          "retail prediction tail source boundary changed")
    whole=section_read(pe,POST_NETWORK_VA,POST_NETWORK_SIZE,".text")
    check(hashlib.sha256(whole).hexdigest()==POST_NETWORK_SHA,
          "complete original prediction slot6 SHA changed")
    covered=verify_sparse(whole,POST_NETWORK_SIZE,witness_chunks(
        predict_va=CL_PREDICT_PARENT_VA,
        helper_va=OPAQUE_DISABLED_TO_ENABLED_VA,
        snapshot_va=OPAQUE_SNAPSHOT_REFRESH_VA),
        "original_postnetwork_tail")
    relocs=pe_highlow_relocations(pe)
    check(TRANSITION_START_VA+2 in relocs and
          0x24196498 in relocs,
          "native cl_predict parent absolute relocation drift")
    return {
        "phase":1682,
        "provenance":"BOTH SHA-authenticated original retail PE32 client+engine",
        "original_whole_method_sha256":POST_NETWORK_SHA,
        "original_whole_slot6_native_method_bytes":POST_NETWORK_SIZE,
        "newly_modeled_source_tail_start":hex(TRANSITION_START_VA),
        "newly_modeled_source_tail_end_exclusive":hex(TRANSITION_END_EXCLUSIVE_VA),
        "newly_modeled_existing_method_bytes":TRANSITION_SOURCE_BYTES,
        "new_original_native_function_bytes_claimed":0,
        "original_predict_parent_global":hex(CL_PREDICT_PARENT_VA),
        "original_parent_value_offset":hex(ORIGINAL_CL_PREDICT_PARENT_VALUE_OFFSET),
        "prediction_cached_enable_byte_offset":hex(SOURCE_OWNER_ENABLED_OFFSET),
        "original_if_change_reenable_call":hex(OPAQUE_DISABLED_TO_ENABLED_VA),
        "original_reenable_call_site":hex(HELPER_CALL_SITE),
        "original_on_change_reset_words":[
            hex(SOURCE_OWNER_RESET_INDEX_OFFSET),
            hex(SOURCE_OWNER_RESET_A_OFFSET),
            hex(SOURCE_OWNER_RESET_B_OFFSET)],
        "original_final_cache_enabled_byte_offset":hex(SOURCE_OWNER_ENABLED_OFFSET),
        "original_following_opaque_snapshot_helper":hex(OPAQUE_SNAPSHOT_REFRESH_VA),
        "original_following_snapshot_call_site":hex(SNAPSHOT_CALL_SITE),
        "native_verified_opcode_witness_bytes":covered,
        "native_real_game_method_executions":0,
        "unknown_rest_of_postloop":True,
        "original_windows_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original prediction tail accepted: "+label)
def self_test()->None:
    # Invented fake VA islands; real source bytes never shipped.
    invented_base=0x45110000
    ws=witness_chunks(
        predict_va=0x46210000,
        helper_va=0x45120000,
        snapshot_va=0x45130000)
    fake=bytearray(b"\xCC"*POST_NETWORK_SIZE)
    covered=set()
    for off,fragment in ws:
        fake[off:off+len(fragment)]=fragment
        covered.update(range(off,off+len(fragment)))
    check(verify_sparse(bytes(fake),POST_NETWORK_SIZE,ws,
                        "invented")==len(covered),
          "invented source transition witness coverage drift")
    for pos in sorted(covered):
        changed=bytearray(fake);changed[pos]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,POST_NETWORK_SIZE,ws,"mutated"),
            str(pos))
    reject(lambda:verify_sparse(bytes(fake[:-1]),POST_NETWORK_SIZE,
                                ws,"short"),
           "truncated original method")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",
                                POST_NETWORK_SIZE,ws,"long"),
           "included INT3 as source method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged original image SHA")
    print("PASS Phase1682 original-free: whole original 1039B "
          "PostNetwork slot6 source SHA, 58B tail cl_predict "
          "toggle gate with +0x10 status, disabled->enabled opaque "
          "0x24195C20 call then +0x28/+0x2C clear and +0x24=-1, "
          "next +0x10 write, later opaque 0x240C4940 snapshot, "
          "sparse mutations/forged retail-source rejection")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test rejects original owner image paths")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail original audit requires BOTH client and engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
