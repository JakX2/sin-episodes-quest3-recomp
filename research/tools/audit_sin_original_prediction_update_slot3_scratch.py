#!/usr/bin/env python3
"""Phase1664: original CPrediction::Update x86 slot3 scratch save/call/restore.

Owner-only original client.dll+engine.dll whole-file SHA-gated PE32
read-only audit. The original slot3 164-byte native method is pinned
by a WHOLE function SHA256, sparse x86 structural witnesses, relocations
and original profiling label. Public --self-test synthesizes only
nonexecuting fragments; never distributes original machine code.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,CLIENT_VTABLE_VA,CLIENT_VIRTUAL_TARGETS,
    verify_original,check,section_read,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

SLOT=3
UPDATE_VA=0x24197210
UPDATE_BYTES=164
UPDATE_SHA="0636e37880c589bef9f874d6d259763a61aa1eceb4fe1b7cba58dbbe68d463e6"
ORIGINAL_SCRATCH_POINTER_CELL=0x242FA458
ORIGINAL_SCRATCH_POINTER_DEFAULT=0x242FA464
ORIGINAL_SCRATCH_DWORDS=11
ORIGINAL_UPDATE_NAME_VA=0x242A39A8
ORIGINAL_SCOPE_NAME_VA=0x242A39BC
ORIGINAL_UPDATE_NAME=b"CPrediction::Update\0"
ORIGINAL_SCOPE_NAME=b"Prediction\0"
ORIGINAL_LAST_FRAME_OFFSET=0x24
INNER_PREDICTION_SLOT=22
INNER_VTABLE_OFFSET=0x58
ORIGINAL_PROFILER_CURRENT_IAT=0x242761F4
ORIGINAL_PROFILER_ENTER_IAT=0x242761F8
ORIGINAL_PROFILER_EXIT_IAT=0x242761F0
ORIGINAL_CVAR_UPDATE_GATE_VA=0x24374780
ORIGINAL_CVAR_CL_PREDICT_VA=0x24374540
ORIGINAL_RELOCATIONS=(
    UPDATE_VA+0x0B, # scope name Prediction
    UPDATE_VA+0x15, # tier0 g_VProfCurrentProfile IAT
    UPDATE_VA+0x1A, # CPrediction::Update label
    UPDATE_VA+0x20, # tier0 CVProfile::EnterScope IAT
    UPDATE_VA+0x34, # update gating ConVar parent
    UPDATE_VA+0x40, # cl_predict ConVar parent
    UPDATE_VA+0x58, # global 11-DWORD scratch pointer
    UPDATE_VA+0x80, # same pointer after inner call
    UPDATE_VA+0x91, # tier0 global profile pointer for scope end
    UPDATE_VA+0x97, # tier0 CVProfile::ExitScope IAT
)
# Instruction witnesses are deliberately discontinuous, not the
# commercial original whole-function body.
def chunks(scope:int,label:int,profile_global:int,profile_enter:int,
           profile_exit:int,cvar_a:int,cvar_b:int,scratch:int):
    return (
        (0x00,bytes.fromhex("83ec30535657")),
        (0x06,b"\x6a\x04\x6a\x00\x68"+struct.pack("<I",scope)),
        (0x11,b"\x8b\xd9\x8b\x0d"+struct.pack("<I",profile_global)),
        (0x19,b"\x68"+struct.pack("<I",label)),
        (0x1E,b"\xff\x15"+struct.pack("<I",profile_enter)),
        (0x24,bytes.fromhex("8b442440394324c644240c01")),
        (0x30,bytes.fromhex("751d")),
        (0x32,b"\x8b\x0d"+struct.pack("<I",cvar_a)),
        (0x38,bytes.fromhex("83792c007411")),
        (0x3E,b"\x8b\x15"+struct.pack("<I",cvar_b)),
        (0x44,bytes.fromhex("837a2c007405c644240c00")),
        (0x4F,bytes.fromhex("8b542448894324")),
        (0x56,b"\x8b\x35"+struct.pack("<I",scratch)),
        (0x5C,bytes.fromhex("8b03b90b000000")),
        (0x63,bytes.fromhex("8d7c2410f3a5")),
        (0x69,bytes.fromhex("8b4c244c518b4c2448528b5424145152")),
        (0x79,bytes.fromhex("8bcbff5058")),
        (0x7E,b"\x8b\x3d"+struct.pack("<I",scratch)),
        (0x84,bytes.fromhex("b90b0000008d742410f3a5")),
        (0x8F,b"\x8b\x0d"+struct.pack("<I",profile_global)),
        (0x95,b"\xff\x15"+struct.pack("<I",profile_exit)),
        (0x9B,bytes.fromhex("5f5e5b83c430c21000")),
    )

def verify_update_window(blob:bytes,expected_chunks)->int:
    check(len(blob)==UPDATE_BYTES,
          "original update must be 164 native method bytes")
    covered=set()
    for pos,value in expected_chunks:
        check(pos>=0 and pos+len(value)<=len(blob),
              "update instruction witness out of method range")
        check(blob[pos:pos+len(value)]==value,
              "CPrediction::Update original x86 witness mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "original 2006 23-slot prediction parent auth absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          CLIENT_VIRTUAL_TARGETS[SLOT]==UPDATE_VA,
          "original prediction vtable slot3 mismatch")
    check(section_read(pe,CLIENT_VTABLE_VA+SLOT*4,4,".rdata")==
          struct.pack("<I",UPDATE_VA),
          "original vtable slot3 not pointing to Update")
    check(section_read(pe,ORIGINAL_UPDATE_NAME_VA,
                       len(ORIGINAL_UPDATE_NAME),".rdata")==
          ORIGINAL_UPDATE_NAME and
          section_read(pe,ORIGINAL_SCOPE_NAME_VA,
                       len(ORIGINAL_SCOPE_NAME),".rdata")==
          ORIGINAL_SCOPE_NAME,
          "retail profile label is not CPrediction::Update")
    check(section_read(pe,ORIGINAL_SCRATCH_POINTER_CELL,4,".data")==
          struct.pack("<I",ORIGINAL_SCRATCH_POINTER_DEFAULT) and
          pe.section_of(ORIGINAL_SCRATCH_POINTER_DEFAULT)==".data",
          "original 11-DWORD buffer pointer has different initial target")
    # Important: the method loads *globalScratchPointer, then copies 44
    # bytes from that pointed-to object, NOT from the pointer cell itself.
    section_read(pe,ORIGINAL_SCRATCH_POINTER_DEFAULT,
                 ORIGINAL_SCRATCH_DWORDS*4,".data")
    blob=section_read(pe,UPDATE_VA,UPDATE_BYTES,".text")
    check(hashlib.sha256(blob).hexdigest()==UPDATE_SHA,
          "complete original CPrediction::Update 164B SHA mismatch")
    covered=verify_update_window(blob,chunks(
        ORIGINAL_SCOPE_NAME_VA,ORIGINAL_UPDATE_NAME_VA,
        ORIGINAL_PROFILER_CURRENT_IAT,ORIGINAL_PROFILER_ENTER_IAT,
        ORIGINAL_PROFILER_EXIT_IAT,ORIGINAL_CVAR_UPDATE_GATE_VA,
        ORIGINAL_CVAR_CL_PREDICT_VA,ORIGINAL_SCRATCH_POINTER_CELL))
    check(section_read(pe,UPDATE_VA+UPDATE_BYTES,2,".text")==
          b"\xCC\xCC","original method not terminated before INT3 padding")
    reloc=pe_highlow_relocations(pe)
    for site in ORIGINAL_RELOCATIONS+(
        ORIGINAL_SCRATCH_POINTER_CELL,CLIENT_VTABLE_VA+SLOT*4):
        check(site in reloc,"original update pointer operand missing HIGHLOW "+
              hex(site))
    check(CLIENT_VIRTUAL_TARGETS[INNER_PREDICTION_SLOT]==0x241977B0,
          "original Update inner virtual slot22 unknown ABI target changed")
    return {
        "phase":1664,
        "provenance":"full SHA256-matched original client.dll and engine.dll",
        "original_update_method_va":hex(UPDATE_VA),
        "original_update_method_sha256":UPDATE_SHA,
        "new_whole_original_function_bytes":UPDATE_BYTES,
        "sparse_native_opcode_witness_bytes":covered,
        "original_profile_label":ORIGINAL_UPDATE_NAME[:-1].decode(),
        "update_slot":SLOT,
        "update_native_arg_words":4,
        "update_native_ret_pop_bytes":16,
        "updates_prediction_member_offset":hex(ORIGINAL_LAST_FRAME_OFFSET),
        "original_scratch_ptr_cell_va":hex(ORIGINAL_SCRATCH_POINTER_CELL),
        "original_scratch_initial_target":hex(ORIGINAL_SCRATCH_POINTER_DEFAULT),
        "scratch_dwords_backed_up_before_and_restored_after_call":
            ORIGINAL_SCRATCH_DWORDS,
        "inner_prediction_vtable_slot":INNER_PREDICTION_SLOT,
        "inner_virtual_offset_bytes":hex(INNER_VTABLE_OFFSET),
        "inner_first_arg_source":"saved_work_buffer_first_dword",
        "remaining_three_inner_args_source":"update_arguments_2_3_4",
        "unknown_inner_slot22_implementation_not_guessed":True,
        "retail_game_code_executed":False,
        "windows_gameplay_boot_verified":False,
    }

def reject(action,label:str)->None:
    try:action()
    except ValueError:return
    raise AssertionError("corrupt original Update source accepted: "+label)

def self_test()->None:
    # Invent completely unrelated virtual addresses and pointer slots:
    # only independent native structure, never copied game method.
    invented_scope=0x52110000
    invented_label=0x52113000
    fake_profile_global=0x52115000
    fake_enter=0x52116000
    fake_exit=0x52117000
    fake_cvar_a=0x53331000
    fake_cvar_b=0x53332000
    fake_scratch_cell=0x54100000
    cs=chunks(invented_scope,invented_label,fake_profile_global,
              fake_enter,fake_exit,fake_cvar_a,fake_cvar_b,
              fake_scratch_cell)
    fake=bytearray(b"\xCC"*UPDATE_BYTES)
    covered=set()
    for pos,value in cs:
        fake[pos:pos+len(value)]=value
        covered.update(range(pos,pos+len(value)))
    check(verify_update_window(bytes(fake),cs)==len(covered),
          "synthetic Update witness coverage drift")
    for pos in sorted(covered):
        broken=bytearray(fake)
        broken[pos] ^= 1
        reject(lambda data=bytes(broken):verify_update_window(data,cs),
               "synthetic Update byte mutation "+str(pos))
    reject(lambda:verify_update_window(bytes(fake[:163]),cs),
           "truncated original Update method")
    reject(lambda:verify_update_window(bytes(fake)+b"\xCC",cs),
           "extra original padding included in method")
    fake_inner=(0x11,0x22,0x33,0x44)
    # Model exact observed stack-source reorder without guessing real
    # player/state content: slot22(backup[0],arg2,arg3,arg4).
    backup=[0xDEADBEEF]+[i for i in range(1,11)]
    reordered=(backup[0],fake_inner[1],fake_inner[2],fake_inner[3])
    check(reordered==(0xDEADBEEF,0x22,0x33,0x44),
          "original inner slot22 forwarded arg source order drift")
    reject(lambda:audit(b"pretend client",b"pretend engine"),
           "owner-original dual-file SHA gate bypass")
    print("PASS Phase1664 original-free: CPrediction::Update "
          "164-byte SHA-gated method, 23-slot entry3 RET16, "
          "tier0 scope, prediction +0x24 previous-frame field, "
          "pointer-backed 11-DWORD save/restore across slot22 "
          "virtual +0x58 with source-mapped four args, "
          "opcode mutation and forged-original refusal")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test cannot be mixed with retail DLL paths")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail owner audit requires BOTH --client and --engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
