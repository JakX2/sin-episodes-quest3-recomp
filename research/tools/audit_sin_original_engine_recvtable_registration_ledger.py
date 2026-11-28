#!/usr/bin/env python3
"""Phase1700: authenticate original engine receive-table registration list.

The original SiN 2006 PE32 engine.dll is an owner-private, read-only input
to --engine and SHA256-pinned as a whole image. Original 0x20085630
marks each RecvTable +0x11 before calling a native 102-byte method
0x20086930 that appends the table pointer to a global 8-byte-element
double-linked indexed list. This audit proves the native data writes
without copying proprietary method bodies to the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_dll_interface_abi import PEImage, VerificationError
from audit_sin_original_client_class_registry import need

ORIGINAL_ENGINE_SHA="f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
SOURCE_RECURSION=0x20085630
SOURCE_LIST_APPEND=0x20086930
SOURCE_LIST_ALLOCATE_INDEX=0x20087290
SOURCE_LIST_REMOVE_FREE_INDEX=0x200874D0
SOURCE_GLOBAL_LIST_VA=0x202FDC48
SOURCE_INDEXED_RECORD_BYTES=8
SOURCE_EMPTY_INDEX=0xFFFF
ORIGINAL_SOURCE_FUNCTIONS=(
    (SOURCE_RECURSION,102,
     "09a4446ebba475508d1b87121e2df0c63752e1d60850150fe0e453b7f9f3a347"),
    (SOURCE_LIST_APPEND,102,
     "8e87f3939f7b13cc218bc3ae6d0f8ab43689519fcb58276e359794221a05645e"),
    (SOURCE_LIST_ALLOCATE_INDEX,157,
     "b64128c363fc001d8a747e26301009f6a0987fc27db1a2ebf7f25fea849b5bc8"),
    (SOURCE_LIST_REMOVE_FREE_INDEX,122,
     "052b900155581e9284e4a987bdf02557417fd65ea9a62c6de170f60a0152968d"),
)
SOURCE_INITIAL_COLD_LEDGER=bytes.fromhex(
    "000000000000000000000000ffffffffffff00000000000000000000")
assert len(SOURCE_INITIAL_COLD_LEDGER)==28

SOURCE_WITNESSES=(
    (0x20085658,"51","original caller pushes address of table pointer"),
    (0x20085659,"b848dc2f20","original caller uses global registration list"),
    (0x2008565E,"c6461101","original caller marks table+0x11 before append"),
    (0x20086930,"53566a00","native original append prologue"),
    (0x20086944,"0fb7cb03c903c903c9","native 8-byte record index stride"),
    (0x2008694F,"66c74006ffff","new record next link initialized to FFFF"),
    (0x20086955,"668b560e","native prior tail from global+0x0E"),
    (0x20086959,"66895004","new node previous link at +4"),
    (0x2008695D,"66895e0e","native update global tail at +0x0E"),
    (0x2008696B,"66895e0c","native update initial global head at +0x0C"),
    (0x20086976,"66895cc206","native previous node next link at +6"),
    (0x2008697B,"6683461201","native global active count increment +0x12"),
    (0x20086986,"8b44240c","native caller's pointer-to-table stack argument"),
    (0x2008698A,"8b08","native load original RecvTable* pointer"),
    (0x2008698C,"890e","native store table pointer into record+0"),
    (0x20086993,"c20400","native stdcall-like append return consumes argument"),
)

def rel_call(pe:PEImage,site:int,target:int)->None:
    raw=pe.at(site,5)
    need(raw[0]==0xE8 and site+5+struct.unpack("<i",raw[1:])[0]==target,
         "source engine receive-table list native E8 CALL changed "+hex(site))

def verify(source_engine:bytes)->dict:
    need(hashlib.sha256(source_engine).hexdigest()==ORIGINAL_ENGINE_SHA,
         "full original 2006 engine SHA256 mismatch")
    pe=PEImage(source_engine)
    for va,n,sha in ORIGINAL_SOURCE_FUNCTIONS:
        need(pe.section_of(va)==".text" and
             hashlib.sha256(pe.at(va,n)).hexdigest()==sha and
             pe.at(va+n,2)==b"\xCC\xCC",
             "whole original engine native method SHA or INT3 drift "+hex(va))
    need(pe.section_of(SOURCE_GLOBAL_LIST_VA)==".data" and
         pe.at(SOURCE_GLOBAL_LIST_VA,28)==SOURCE_INITIAL_COLD_LEDGER,
         "original engine global linked registry cold-state changed")
    for va,hx,why in SOURCE_WITNESSES:
        need(pe.at(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),
             "original engine "+why+" opcode changed")
    for site,target in (
       (0x20085662,SOURCE_LIST_APPEND),
       (0x20086936,SOURCE_LIST_ALLOCATE_INDEX),
       (0x2008693D,SOURCE_LIST_REMOVE_FREE_INDEX),
    ):
       rel_call(pe,site,target)
    return {
        "status":"ORIGINAL_ENGINE_RECVTABLE_INDEXED_REGISTRATION_LIST_AUTHENTICATED",
        "original_engine_sha256":ORIGINAL_ENGINE_SHA,
        "source_registered_list_global_va":hex(SOURCE_GLOBAL_LIST_VA),
        "original_native_record_bytes":SOURCE_INDEXED_RECORD_BYTES,
        "original_null_list_index":SOURCE_EMPTY_INDEX,
        "source_record_recvtable_pointer_offset":0,
        "source_record_prev_index_word_offset":4,
        "source_record_next_index_word_offset":6,
        "source_global_list_head_word_offset":12,
        "source_global_list_tail_word_offset":14,
        "source_global_list_free_index_word_offset":16,
        "source_global_list_active_count_word_offset":18,
        "source_original_102B_append_method":hex(SOURCE_LIST_APPEND),
        "source_original_index_allocator":hex(SOURCE_LIST_ALLOCATE_INDEX),
        "source_original_free_list_remove":hex(SOURCE_LIST_REMOVE_FREE_INDEX),
        "original_native_function_whole_SHA_count":len(ORIGINAL_SOURCE_FUNCTIONS),
        "original_total_function_bytes":sum(x[1] for x in ORIGINAL_SOURCE_FUNCTIONS),
        "source_opcode_witness_count":len(SOURCE_WITNESSES),
        "source_original_direct_call_edges":3,
        "limitations":(
           "Records exact native-list bookkeeping and recursion-to-list "
           "callsite. Does not rebuild the original heap/list allocator, "
           "resizing, free-list semantics, callback bodies, executable "
           "RecvTables, user gameplay or a retail engine startup."
        ),
    }

def self_test()->None:
    need(len(SOURCE_INITIAL_COLD_LEDGER)==28,
         "original cold-state list global length changed")
    need(struct.unpack_from("<H",SOURCE_INITIAL_COLD_LEDGER,12)[0]
         ==SOURCE_EMPTY_INDEX and
         struct.unpack_from("<H",SOURCE_INITIAL_COLD_LEDGER,14)[0]
         ==SOURCE_EMPTY_INDEX and
         struct.unpack_from("<H",SOURCE_INITIAL_COLD_LEDGER,16)[0]
         ==SOURCE_EMPTY_INDEX,
         "original empty list head/tail/free sentinel changed")
    need(struct.unpack_from("<H",SOURCE_INITIAL_COLD_LEDGER,18)[0]==0,
         "original empty list active count changed")
    try:
        verify(b"fabricated original engine")
    except VerificationError:pass
    else:raise AssertionError("fabricated game engine accepted as original")
    print("PASS Phase1700 original engine 102B table append and 8B indexed doubly linked list: source call edges, cold global sentinels, invented SHA refusal")

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine",type=Path,
                        help="read-only owner-private unchanged original engine.dll")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.engine is None and not args.self_test:
        parser.error("supply --self-test or --engine")
    if args.self_test:self_test()
    if args.engine is not None:
        print(json.dumps(verify(args.engine.read_bytes()),indent=2,sort_keys=True))

if __name__=="__main__":
    main()
