#!/usr/bin/env python3
"""Phase1672: original 2006 PE32 typed PostNetwork comparison dispatch shell.

Authenticates BOTH complete original retail client.dll and engine.dll
through the Phase1658 parent audit BEFORE inspecting the 1036-byte
C++ x86 method at 0x241996F0, two direct Phase1671 finalizer callsites,
the complete 13-target MSVC absolute switch jump table at 0x24199AFC,
and 27-entry type-code->case map at 0x24199B30.

IMPORTANT: the native method ends in RET 0x10 IMMEDIATELY BEFORE
the nonexecuting in-.text jump table. Never append table bytes to
the executable whole-method SHA or disassemble them as code.

Only the original source-authenticated four-argument RET16 outer
entry, record stride, sequence/flag gates, case-map selection, and
lifetime cleanup are implemented in a synthetic MSVC x86 facade.
Every specialist case and nested recursive comparison remains OPAQUE.
Public --self-test constructs only non-executing invented sparse
opcode islands and mutated fake pointer tables, not retail code.
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
    LIST_FINALIZER_VA,LIST_FINALIZER_BYTES,LIST_FINALIZER_SHA,
    OPAQUE_PER_NODE_HELPER_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

DISPATCH_VA=0x241996F0
DISPATCH_BYTES=1036
DISPATCH_SHA="71406e900eef6ba6d8abd37ab73f8730c71c35d95a8700a8cb4ab7c9a2b6a7a6"
SWITCH_TABLE_VA=0x24199AFC
SWITCH_ENTRY_COUNT=13
SWITCH_TABLE_SHA="ff8cdf1f255d6ad46c274954e0fd52391b16f301127095b83e0c3372c0593bc1"
TYPE_MAP_VA=0x24199B30
TYPE_COUNT=27
TYPE_MAP_SHA="481a950d38ce95aa847fe32951b237c29b5471f4f910ec35b36167cdb650ffb3"
ERROR_TEXT_VA=0x242A41E0
ERROR_PRINT_IAT_VA=0x242761E0
FINALIZER_CALLS=(LIST_FINALIZER_VA+0x18,LIST_FINALIZER_VA+0x2F)
ORIGINAL_METHOD_RELOCS=(
    DISPATCH_VA+0xD3,  # byte type-case lookup indexed by type
    DISPATCH_VA+0xDA,  # 13-target MSVC jump table
    DISPATCH_VA+0x3D8, # out-of-range and case12 diagnostic literal
    DISPATCH_VA+0x3DE, # original imported warning print
)
SWITCH_TARGETS=(
    0x24199AD5, # case 0, skip/next record
    0x24199847,
    0x2419987F,
    0x241998AD,
    0x241998E7,
    0x241999B7,
    0x2419997D,
    0x241999EF,
    0x24199A29,
    0x24199921,
    0x241997CE, # case10 recursively calls this dispatcher (opaque)
    0x24199A8E,
    0x24199AC7, # case12/error diagnostic
)
TYPE_TO_CASE=(
    0,1,2,3,4,5,6,7,8,9,10,0,0,11,
    0,0,0,0,0,0,12,0,12,12,12,12,0,
)
RECORD_STRIDE=0x34
TYPE_MAX=0x1A
OWNER_MODE_OFFSET=0x00
OWNER_LEFT_BIAS_OFFSET=0x04
OWNER_RIGHT_BIAS_OFFSET=0x08
OWNER_LEFT_INDEX_OFFSET=0x0C
OWNER_RIGHT_INDEX_OFFSET=0x10
OWNER_OPTION_OFFSET=0x15
OWNER_CURRENT_RECORD_OFFSET=0x18
OWNER_CACHED_NODE_VALUE_OFFSET=0x1C
OWNER_INPUT_NODE_OFFSET=0x20
OWNER_OPTION_COPY_OFFSET=0x24
OWNER_ACTIVE_BYTE_OFFSET=0x25
NODE_INITIAL_CACHE_OFFSET=0x08
RECORD_TYPE_OFFSET=0x00
RECORD_SOURCE_BASE_OFFSET=0x08
RECORD_FLAGS_OFFSET=0x12
RECORD_ALIAS_POINTER_OFFSET=0x28
RECORD_LAST_SEQUENCE_OFFSET=0x2C
GATE_FLAGS_SKIP_MASK=0x0200
GATE_FLAGS_MODE_MASK=0x0100

def pack(value:int)->bytes:
    return struct.pack("<I",value)
def rel_call(src:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-src-5)

def witness_chunks(*,base:int,typemap:int,jumptable:int,
                   error_text:int,error_iat:int):
    # Disjoint opcode islands. No whole original executable body.
    return (
       (0x000,bytes.fromhex(
           "8b44240883ec10558be9837d1c0089452075068b400889451c")),
       (0x019,bytes.fromhex(
           "8b44242485c00f8ed90300008b4c2420")),
       (0x02A,bytes.fromhex("83c12856894c240c8944241057")),
       (0x040,bytes.fromhex("8d51d88955180fbf51ea8b0185c089542430")),
       (0x052,bytes.fromhex("74078b4c242489482c8b45188b4c242439482c")),
       (0x06F,bytes.fromhex("83380a7427f6c6020f8568030000")),
       (0x07D,bytes.fromhex("8b4d0083f901750884f10f8558030000")),
       (0x08D,bytes.fromhex("83f9027509f6c6010f844a030000")),
       (0x09B,bytes.fromhex("8b4d0c8b7c88080fbf70108b55108b5c9008")),
       (0x0AD,bytes.fromhex("8a4d15037d04035d08884d24c6452501")),
       (0x0BD,bytes.fromhex("8b0883f91a")),
       (0x0D0,b"\x0F\xB6\x89"+pack(typemap)),
       (0x0D7,b"\xFF\x24\x8D"+pack(jumptable)),
       (0x135,rel_call(base+0x135,base)), # case10 source recursive
       (0x256,bytes.fromhex("c1e9028bf3f3a58bc883e103f3a4")),
       (0x3D7,b"\x68"+pack(error_text)+b"\xFF\x15"+
              pack(error_iat)+b"\x83\xC4\x04"),
       (0x3E5,bytes.fromhex(
           "8b4c241083c134836c241401894c24100f8545fcffff")),
       (0x3FB,bytes.fromhex(
           "5f5e5bc7451c000000005d83c410c21000")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" original whole native function extent differs")
    positions=set()
    for off,value in w:
        check(0<=off and off+len(value)<=len(blob),
              name+" native sparse opcode witness outside method")
        check(blob[off:off+len(value)]==value,
              name+" opcode/operand witness mismatch @"+hex(off))
        positions.update(range(off,off+len(value)))
    return len(positions)

def decode_tables(jumps:bytes,types:bytes)->tuple:
    check(len(jumps)==SWITCH_ENTRY_COUNT*4,
          "jump table length must be 13 native DWORDs")
    check(len(types)==TYPE_COUNT,
          "type lookup length must be 27 native bytes")
    targets=struct.unpack("<13I",jumps)
    case_ids=tuple(types)
    check(targets==SWITCH_TARGETS,
          "13 native switch targets differ from original function starts")
    check(case_ids==TYPE_TO_CASE,
          "27 native type aliases differ from original switch dispatch")
    check(all(0<=x<SWITCH_ENTRY_COUNT for x in case_ids) and
          all(DISPATCH_VA<=x<DISPATCH_VA+DISPATCH_BYTES
              for x in targets),
          "native switch pointers escape executable function")
    return targets,case_ids

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent whole-file original provenance not authenticated")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          OPAQUE_PER_NODE_HELPER_VA==DISPATCH_VA,
          "Phase1671 finalizer callee original address drift")
    f=section_read(pe,LIST_FINALIZER_VA,LIST_FINALIZER_BYTES,".text")
    check(hashlib.sha256(f).hexdigest()==LIST_FINALIZER_SHA,
          "original Phase1671 finalizer body changed")
    for addr in FINALIZER_CALLS:
        check(section_read(pe,addr,5,".text")==rel_call(addr,DISPATCH_VA),
              "original finalizer per-node callsite x86 target differs")
    code=section_read(pe,DISPATCH_VA,DISPATCH_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==DISPATCH_SHA,
          "full original 1036-byte per-node dispatcher SHA256 differs")
    covered=verify_sparse(code,DISPATCH_BYTES,
        witness_chunks(base=DISPATCH_VA,typemap=TYPE_MAP_VA,
                       jumptable=SWITCH_TABLE_VA,
                       error_text=ERROR_TEXT_VA,
                       error_iat=ERROR_PRINT_IAT_VA),
        "original_type_dispatch")
    check(code[-3:]==b"\xC2\x10\x00" and
          DISPATCH_VA+DISPATCH_BYTES==SWITCH_TABLE_VA,
          "original 4-arg RET16 is directly adjacent to jump TABLE")
    jump_raw=section_read(pe,SWITCH_TABLE_VA,
                          SWITCH_ENTRY_COUNT*4,".text")
    type_raw=section_read(pe,TYPE_MAP_VA,TYPE_COUNT,".text")
    check(hashlib.sha256(jump_raw).hexdigest()==SWITCH_TABLE_SHA and
          hashlib.sha256(type_raw).hexdigest()==TYPE_MAP_SHA,
          "complete original case jump table/type aliases changed")
    decode_tables(jump_raw,type_raw)
    check(section_read(pe,TYPE_MAP_VA+TYPE_COUNT,4,".text")==
          b"\xCC\xCC\xCC\xCC",
          "original type map does not terminate before INT3 padding")
    reloc=pe_highlow_relocations(pe)
    own={x for x in reloc
         if DISPATCH_VA<=x<DISPATCH_VA+DISPATCH_BYTES}
    table_sites={x for x in reloc
                 if SWITCH_TABLE_VA<=x<SWITCH_TABLE_VA+SWITCH_ENTRY_COUNT*4}
    map_sites={x for x in reloc
               if TYPE_MAP_VA<=x<TYPE_MAP_VA+TYPE_COUNT}
    check(own==set(ORIGINAL_METHOD_RELOCS),
          "original dispatcher four HIGHLOW pointer operands differ")
    check(table_sites=={
          SWITCH_TABLE_VA+i*4 for i in range(SWITCH_ENTRY_COUNT)}
          and not map_sites,
          "original 13 native jump pointers not HIGHLOW relocated")
    return {
       "phase":1672,
       "provenance":"BOTH full original client.dll+engine.dll SHA256",
       "original_dispatch_method_va":hex(DISPATCH_VA),
       "original_complete_native_bytes":DISPATCH_BYTES,
       "original_whole_method_sha256":DISPATCH_SHA,
       "native_sparse_witness_bytes":covered,
       "original_dispatch_method_highlow_sites":len(ORIGINAL_METHOD_RELOCS),
       "original_finalizer_direct_call_sites":[hex(x) for x in FINALIZER_CALLS],
       "original_thiscall_four_arg_ret_pop_bytes":16,
       "original_count_arg_signed":True,
       "original_entry_node_cache_from_node_plus":hex(NODE_INITIAL_CACHE_OFFSET),
       "original_entry_current_node_saved_at_this_plus":
           hex(OWNER_INPUT_NODE_OFFSET),
       "original_record_stride":hex(RECORD_STRIDE),
       "original_record_last_sequence_offset":hex(RECORD_LAST_SEQUENCE_OFFSET),
       "original_record_alias_pointer_offset":hex(RECORD_ALIAS_POINTER_OFFSET),
       "original_record_signed_flags_offset":hex(RECORD_FLAGS_OFFSET),
       "original_common_flag_gate_mask":hex(GATE_FLAGS_SKIP_MASK),
       "original_mode_flag_gate_mask":hex(GATE_FLAGS_MODE_MASK),
       "original_type10_bypasses_flag_mode_gates":True,
       "original_type_max":TYPE_MAX,
       "original_jump_table_va":hex(SWITCH_TABLE_VA),
       "original_jump_table_whole_sha256":SWITCH_TABLE_SHA,
       "original_jump_table_targets":[hex(x) for x in SWITCH_TARGETS],
       "original_jump_table_relocations":SWITCH_ENTRY_COUNT,
       "original_type_map_va":hex(TYPE_MAP_VA),
       "original_type_map_whole_sha256":TYPE_MAP_SHA,
       "original_type_to_case":list(TYPE_TO_CASE),
       "case_0_skips_and_case_12_emits_opaque_error":True,
       "case_10_native_recursive_dispatch_callsite":
           hex(DISPATCH_VA+0x135),
       "original_end_always_clears_this_plus_0x1c":True,
       "case_specific_original_comparison_helpers_implemented":False,
       "original_native_method_executions":0,
       "retail_windows_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original dispatch accepted: "+label)

def self_test()->None:
    base=0x45110000
    w=witness_chunks(base=base,typemap=0x46200000,
                     jumptable=0x46210000,
                     error_text=0x46300000,
                     error_iat=0x46310000)
    invented=bytearray(b"\xCC"*DISPATCH_BYTES)
    covered=set()
    for off,value in w:
        invented[off:off+len(value)]=value
        covered.update(range(off,off+len(value)))
    check(verify_sparse(bytes(invented),DISPATCH_BYTES,w,
                        "invented_native")==len(covered),
          "synthetic x86 dispatch opcode coverage drift")
    for off in sorted(covered):
        broken=bytearray(invented);broken[off]^=1
        reject(lambda data=bytes(broken):verify_sparse(
            data,DISPATCH_BYTES,w,"invented_native"),
            "synthetic x86 byte corruption "+str(off))
    reject(lambda:verify_sparse(bytes(invented[:-1]),
                  DISPATCH_BYTES,w,"short_method"),"truncated native method")
    reject(lambda:verify_sparse(bytes(invented)+b"\xCC",
                  DISPATCH_BYTES,w,"long_method"),
           "original jump table included in executable method span")
    encoded_jump=struct.pack("<13I",*SWITCH_TARGETS)
    encoded_map=bytes(TYPE_TO_CASE)
    decode_tables(encoded_jump,encoded_map)
    for i in range(SWITCH_ENTRY_COUNT):
        broken=bytearray(encoded_jump)
        broken[i*4]^=1
        reject(lambda value=bytes(broken):decode_tables(
            value,encoded_map),"tampered case target "+str(i))
    for i in range(TYPE_COUNT):
        broken=bytearray(encoded_map);broken[i]=(broken[i]+1)%13
        reject(lambda value=bytes(broken):decode_tables(
            encoded_jump,value),"tampered type alias "+str(i))
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged original whole-DLL image hashes")
    check(DISPATCH_BYTES==0x40C and
          TYPE_TO_CASE[10]==10 and TYPE_TO_CASE[20]==12 and
          TYPE_TO_CASE[11]==0,
          "source-authenticated type alias semantics drift")
    print("PASS Phase1672 original-free: whole-SHA-gated native "
          "1036-byte typed PostNetwork per-node comparison dispatcher, "
          "4-arg __thiscall RET16, 0x34-byte records, signed count and "
          "sequence/flags gates, full 13-target HIGHLOW switch table, "
          "full 27-byte type aliases and default/recursive case edges, "
          "method/table boundary, all sparse opcode byte mutants and "
          "case-target/type-map mutants rejected")

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
