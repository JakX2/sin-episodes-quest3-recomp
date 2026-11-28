#!/usr/bin/env python3
"""Phase1681: whole-SHA authentication for original client.dll name
comparator 0x2425F81B (67B) and its no-locale fast path
0x24261C60 (78B), called by the Phase1679 name-finder.

Two cdecl DWORD string arguments, RET0 or TAIL-JMP. The comparator
switches on original process-global DWORD at 0x243895D0. Zero:
tail-JMP into ASCII-specialized casefold comparator. Nonzero:
per-byte case conversion via separate 0x2424C373, then subtract
mapped values. The native case conversion's locale tables and host
CRT are deliberately NOT presented as portable ASCII behavior.

Read-only --client/--engine requires both exactly pinned complete
2006 originals, plus parent SHA evidence. Public --self-test uses
invented opcode fragments and rejects mutations, truncation,
wrong DLL image hashes; never executes or includes retail game code.
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
from audit_sin_original_postnetwork_record_name_lookup import (
    LOOKUP_VA,LOOKUP_BYTES,LOOKUP_SHA,NAME_COMPARE_VA,
    NAME_COMPARE_CALL,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

SLOW_VA=0x2425F81B
SLOW_BYTES=67
SLOW_SHA="0905847aa7a5fa843447c965b7ed17f401401ad60bf6be835038df5637559806"
FAST_VA=0x24261C60
FAST_BYTES=78
FAST_SHA="9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658"
LOCALE_FLAG_VA=0x243895D0
LOCALE_CHAR_CONVERT_VA=0x2424C373
SLOW_TAILJMP_SITE=SLOW_VA+0x09
SLOW_MAP_CALLS=(SLOW_VA+0x1D,SLOW_VA+0x29)
ORIGINAL_LOCALE_GLOBAL_RELOC_SITE=SLOW_VA+0x02
ORIGINAL_FINDER_CALL=NAME_COMPARE_CALL
ORIGINAL_STACK_ARGS=2
ORIGINAL_RET_POP=0

def rel(site:int,target:int,opcode:bytes)->bytes:
    return opcode+struct.pack("<i",target-site-5)

def slow_witnesses(*,base:int,locale_va:int,
                   fast_va:int,map_va:int):
    return (
        (0x00,b"\x83\x3D"+struct.pack("<I",locale_va)+b"\x00"),
        (0x07,bytes.fromhex("7505")),
        (0x09,rel(base+0x09,fast_va,b"\xE9")),
        (0x0E,bytes.fromhex("53568b74240c57")),
        (0x19,bytes.fromhex("0fb60650")),
        (0x1D,rel(base+0x1D,map_va,b"\xE8")),
        (0x24,bytes.fromhex("0fb6075046")),
        (0x29,rel(base+0x29,map_va,b"\xE8")),
        (0x32,bytes.fromhex("85db598bc874043bd974de")),
        (0x3B,bytes.fromhex("5f8bc35e2bc15bc3")),
    )
def fast_witnesses():
    return (
        (0x00,bytes.fromhex("558bec575653")),
        (0x06,bytes.fromhex("8b750c8b7d08")),
        (0x0C,bytes.fromhex("b0ff8bff0ac07432")),
        (0x14,bytes.fromhex("8a0683c6018a2783c701")),
        (0x1E,bytes.fromhex("38c474ee2c413c1a")),
        (0x28,bytes.fromhex("80e12002c1044186e0")),
        (0x31,bytes.fromhex("2c413c1a1ac980e120")),
        (0x3D,bytes.fromhex("38e074ce1ac01cff")),
        (0x47,bytes.fromhex("5b5e5fc9c3")),
    )
def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,name+" complete method length mismatch")
    used=set()
    for off,frag in w:
        check(0<=off and off+len(frag)<=length,
              name+" native instruction witness outside method")
        check(blob[off:off+len(frag)]==frag,
              name+" original source sparse witness changed @"+hex(off))
        used.update(range(off,off+len(frag)))
    return len(used)
def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "whole original engine/client provenance absent")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and SLOW_VA==NAME_COMPARE_VA,
          "original name-finder's comparator VA drift")
    finder=section_read(pe,LOOKUP_VA,LOOKUP_BYTES,".text")
    check(hashlib.sha256(finder).hexdigest()==LOOKUP_SHA,
          "original Phase1679 finder whole SHA changed")
    check(section_read(pe,ORIGINAL_FINDER_CALL,5,".text")==
          rel(ORIGINAL_FINDER_CALL,SLOW_VA,b"\xE8"),
          "original finder no longer invokes comparator")
    slow=section_read(pe,SLOW_VA,SLOW_BYTES,".text")
    fast=section_read(pe,FAST_VA,FAST_BYTES,".text")
    check(hashlib.sha256(slow).hexdigest()==SLOW_SHA and
          hashlib.sha256(fast).hexdigest()==FAST_SHA,
          "original complete slow/fast comparator native SHA drift")
    sw=verify_sparse(slow,SLOW_BYTES,slow_witnesses(
        base=SLOW_VA,locale_va=LOCALE_FLAG_VA,
        fast_va=FAST_VA,map_va=LOCALE_CHAR_CONVERT_VA),
        "locale-gated_cmp")
    fw=verify_sparse(fast,FAST_BYTES,fast_witnesses(),
        "ascii_fast_cmp")
    check(slow[-1:]==b"\xC3" and fast[-1:]==b"\xC3",
          "original native comparison RET0 drift")
    check(section_read(pe,SLOW_VA+SLOW_BYTES,4,".text")==
          bytes.fromhex("837c2408"),
          "original adjacent 2425F85E function boundary drift")
    check(section_read(pe,FAST_VA+FAST_BYTES,1,".text")==b"\x56",
          "original adjacent 24261CAE function boundary drift")
    section_read(pe,LOCALE_FLAG_VA,4,".data")
    check(section_read(pe,LOCALE_CHAR_CONVERT_VA,4,".text")==
          bytes.fromhex("558bec51"),
          "source locale-casefold character helper entry drift")
    reloc=pe_highlow_relocations(pe)
    a={x for x in reloc if SLOW_VA<=x<SLOW_VA+SLOW_BYTES}
    b={x for x in reloc if FAST_VA<=x<FAST_VA+FAST_BYTES}
    check(a=={ORIGINAL_LOCALE_GLOBAL_RELOC_SITE} and not b,
          "original locale global relocation or fast-path relocation drift")
    return {
        "phase":1681,
        "provenance":"BOTH full SHA-authenticated original client.dll and engine.dll",
        "original_finder_name_comparison_call":hex(ORIGINAL_FINDER_CALL),
        "original_locale_gated_comparator_va":hex(SLOW_VA),
        "original_locale_gated_native_bytes":SLOW_BYTES,
        "original_locale_gated_whole_sha256":SLOW_SHA,
        "original_english_ascii_tail_va":hex(FAST_VA),
        "original_ascii_fast_whole_bytes":FAST_BYTES,
        "original_ascii_fast_whole_sha256":FAST_SHA,
        "native_return":"CDECL RET0 (two DWORD args caller cleanup8)",
        "original_process_locale_global_va":hex(LOCALE_FLAG_VA),
        "original_zero_locale_tailjmp_callsite":hex(SLOW_TAILJMP_SITE),
        "original_nonzero_locale_lower_helper":hex(LOCALE_CHAR_CONVERT_VA),
        "original_nonzero_locale_map_calls":[hex(x) for x in SLOW_MAP_CALLS],
        "original_abs_relocations":[hex(x) for x in sorted(a)],
        "original_fast_abs_relocations":0,
        "no_INT3_padding_claim_for_adjacent_CRT_methods":True,
        "sparse_native_witness_bytes":sw+fw,
        "real_original_code_executed":False,
        "unknown_native_locale_table_and_locale_helper":True,
        "windows_gameplay_boot_verified":False,
    }
def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("altered original comparator accepted: "+label)
def self_test()->None:
    tests=(
        ("slow",SLOW_BYTES,slow_witnesses(
            base=0x45110000,locale_va=0x46210000,
            fast_va=0x45120000,map_va=0x45130000)),
        ("fast",FAST_BYTES,fast_witnesses()),
    )
    for name,length,ws in tests:
        fake=bytearray(b"\xCC"*length)
        touched=set()
        for off,fragment in ws:
            fake[off:off+len(fragment)]=fragment
            touched.update(range(off,off+len(fragment)))
        check(verify_sparse(bytes(fake),length,ws,name)==len(touched),
              "invented source witness coverage changed")
        for i in sorted(touched):
            mutated=bytearray(fake);mutated[i]^=1
            reject(lambda data=bytes(mutated):verify_sparse(
                data,length,ws,name),name+" mutated "+str(i))
        reject(lambda:verify_sparse(bytes(fake[:-1]),length,ws,name),
               name+" truncated")
        reject(lambda:verify_sparse(bytes(fake)+b"\xCC",length,ws,name),
               name+" included next method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "forged dual original whole image SHA")
    print("PASS Phase1681 original-free: whole 67B CDECL locale-gated "
          "comparator + 78B ASCII-fast native SHA, original finder "
          "E8 edge, locale flag HIGHLOW absolute relocation, real "
          "fast-path tail E9 and two locale conversion E8 calls, "
          "RET0 without INT3 padding, sparse mutations and "
          "forged-retail image refusal")
def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.client is not None or a.engine is not None:
            p.error("--self-test refuses owner retail binaries")
        self_test()
    else:
        if a.client is None or a.engine is None:
            p.error("retail source audit requires both original DLLs")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
