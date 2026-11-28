#!/usr/bin/env python3
"""Phase1674: original case8/case9 PostNetwork gated byte-copy blocks.

This is NOT a new complete native method. Source regions are
within the SHA-authenticated original 1036-byte typed dispatcher
0x241996F0, and immediately follow the native 56-byte comparison
helper call for cases 8 and 9.

Both region SHAs and their x86 opcode witnesses prove the
observed conditional side effect: when original dispatcher BYTE
this+0x2C != 0 AND the gated byte comparison returned EAX != 1,
the native code copies exactly case-specific byte count from
RIGHT (EBX) into LEFT (EDI), first REP MOVSD floor(n/4),
then REP MOVSB remainder n&3, restoring register state.
Case8 uses the signed raw WORD+0x10 byte length; case9
multiplies that length by FOUR before comparison and copy.
Remaining post-copy diagnostic helpers remain OPAQUE.

Original full client.dll/engine.dll SHA256 is required via parent
before inspecting either retail instruction region. Public
--self-test constructs only non-executing invented sparse x86
islands and mutated byte windows.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,verify_original,check,section_read,
)
from audit_sin_original_postnetwork_type_dispatch_13case import (
    DISPATCH_VA,DISPATCH_BYTES,DISPATCH_SHA,
    TYPE_TO_CASE,SWITCH_TARGETS,
)
from audit_sin_original_postnetwork_case8_case9_bytecmp import (
    ORIGINAL_BYTE_COMPARE_VA,ORIGINAL_BYTE_COMPARE_BYTES,
    ORIGINAL_BYTE_COMPARE_SHA,CASE8_CALL_VA,CASE9_CALL_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

CASE9_FIXUP_VA=0x24199933
CASE9_FIXUP_BYTES=43
CASE9_FIXUP_SHA="efec6fd7569f9b5b36274b039b2f7063fb50eb8246c5870fca72bf7d37810083"
CASE8_FIXUP_VA=0x24199A33
CASE8_FIXUP_BYTES=41
CASE8_FIXUP_SHA="25a10a5e47d2ab9cd80bec67062f5e49d6d4366dd080a9471e236b8d9fceda75"
CORRECTION_ENABLE_BYTE_THIS_OFFSET=0x2C
LEFT_DESTINATION_REGISTER="EDI"
RIGHT_SOURCE_REGISTER="EBX"
CASE8_BYTE_MULTIPLIER=1
CASE9_BYTE_MULTIPLIER=4
CASE8_COMPARE_CALL=CASE8_CALL_VA
CASE9_COMPARE_CALL=CASE9_CALL_VA

def witnesses(case:str):
    # All fragments are discontinuous: no complete original source
    # instruction body is included. Original region SHAs are verified
    # only in owner read-only mode.
    common=(
        (0x00,bytes.fromhex("807d2c00")),
        (0x08,bytes.fromhex("74")),
        (0x0A,bytes.fromhex("83f801")),
        (0x0F,bytes.fromhex("8bce")),
    )
    if case=="8":
        return common+(
            (0x11,bytes.fromhex("8bd1c1e9028bf3")),
            (0x18,bytes.fromhex("f3a5")),
            (0x1A,bytes.fromhex("8bca83e103")),
            (0x1F,bytes.fromhex("f3a4")),
            (0x21,bytes.fromhex("8b7c242c")),
            (0x25,bytes.fromhex("8b74241c")),
        )
    return common+(
        (0x11,bytes.fromhex("8bc1c1e9028bf3")),
        (0x18,bytes.fromhex("f3a5")),
        (0x1A,bytes.fromhex("8bc883e103")),
        (0x1F,bytes.fromhex("f3a4")),
        (0x21,bytes.fromhex("8b7c242c")),
        (0x25,bytes.fromhex("8bf0")),
        (0x27,bytes.fromhex("8b442430")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" original source code region extent changed")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" sparse x86 witness outside original region")
        check(blob[pos:pos+len(value)]==value,
              name+" original x86 copy witness mismatch @"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent original prediction provenance not authenticated")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and
          SWITCH_TARGETS[8]==0x24199A29 and
          SWITCH_TARGETS[9]==0x24199921 and
          TYPE_TO_CASE[8]==8 and TYPE_TO_CASE[9]==9,
          "parent original case8/case9 dispatch targets drift")
    parent_bytes=section_read(pe,DISPATCH_VA,DISPATCH_BYTES,".text")
    cmp_bytes=section_read(pe,ORIGINAL_BYTE_COMPARE_VA,
                           ORIGINAL_BYTE_COMPARE_BYTES,".text")
    check(hashlib.sha256(parent_bytes).hexdigest()==DISPATCH_SHA and
          hashlib.sha256(cmp_bytes).hexdigest()==ORIGINAL_BYTE_COMPARE_SHA,
          "original full source dispatcher/bytecmp SHA differs")
    for addr,length,digest,case in (
        (CASE8_FIXUP_VA,CASE8_FIXUP_BYTES,CASE8_FIXUP_SHA,"8"),
        (CASE9_FIXUP_VA,CASE9_FIXUP_BYTES,CASE9_FIXUP_SHA,"9")):
        region=section_read(pe,addr,length,".text")
        check(hashlib.sha256(region).hexdigest()==digest,
              "entire original PostNetwork case"+case+" code region SHA changed")
        verify_sparse(region,length,witnesses(case),"case"+case+"_fixup")
        check(CASE8_FIXUP_VA>=DISPATCH_VA and
              CASE9_FIXUP_VA+CASE9_FIXUP_BYTES<=
                   DISPATCH_VA+DISPATCH_BYTES,
              "original copy regions must be INSIDE dispatcher function")
    reloc=pe_highlow_relocations(pe)
    for addr,size in ((CASE8_FIXUP_VA,CASE8_FIXUP_BYTES),
                      (CASE9_FIXUP_VA,CASE9_FIXUP_BYTES)):
        check(not {x for x in reloc if addr<=x<addr+size},
              "original copy regions contain no absolute PE relocations")
    return {
        "phase":1674,
        "provenance":"BOTH owner-original complete client.dll+engine.dll SHA256",
        "original_parent_dispatch_complete_method_sha256":DISPATCH_SHA,
        "original_helper_complete_method_sha256":ORIGINAL_BYTE_COMPARE_SHA,
        "case8_correction_region_va":hex(CASE8_FIXUP_VA),
        "case8_correction_region_bytes":CASE8_FIXUP_BYTES,
        "case8_correction_region_whole_sha256":CASE8_FIXUP_SHA,
        "case9_correction_region_va":hex(CASE9_FIXUP_VA),
        "case9_correction_region_bytes":CASE9_FIXUP_BYTES,
        "case9_correction_region_whole_sha256":CASE9_FIXUP_SHA,
        "case8_multiplier":CASE8_BYTE_MULTIPLIER,
        "case9_multiplier":CASE9_BYTE_MULTIPLIER,
        "original_correction_enabled_this_byte_offset":
            hex(CORRECTION_ENABLE_BYTE_THIS_OFFSET),
        "copy_iff_enable_nonzero_and_compare_EAX_not_one":True,
        "copy_direction":"right EBX -> left EDI",
        "original_native_copy":"REP MOVSD (n>>2) then REP MOVSB (n&3)",
        "no_new_full_native_function_claimed":True,
        "remaining_case_helpers_opaque":True,
        "retail_native_functions_executed":0,
        "gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original gated copy accepted: "+label)

def self_test()->None:
    for case,length in (("8",CASE8_FIXUP_BYTES),
                        ("9",CASE9_FIXUP_BYTES)):
        fake=bytearray(b"\xCC"*length)
        w=witnesses(case)
        covered=set()
        for off,fragment in w:
            fake[off:off+len(fragment)]=fragment
            covered.update(range(off,off+len(fragment)))
        check(verify_sparse(bytes(fake),length,w,
                            "invented_case"+case)==len(covered),
              "invented native copy sparse witness coverage changed")
        for off in sorted(covered):
            broken=bytearray(fake);broken[off]^=1
            reject(lambda data=bytes(broken):verify_sparse(
                data,length,w,"invented_case"+case),
                "case"+case+" byte mutation "+str(off))
        reject(lambda:verify_sparse(bytes(fake[:-1]),length,w,
                  "short_case"+case),"truncated original copy region")
        reject(lambda:verify_sparse(bytes(fake)+b"\xCC",length,w,
                  "long_case"+case),"extended original copy region")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner original complete-file SHA forgery")
    print("PASS Phase1674 original-free: two SHA-gated authentic "
          "case8/case9 code REGIONS within one original native method "
          "(41B + 43B, NOT new independent functions), "
          "this+0x2C correction byte, compare EAX!=1 branch, "
          "source EBX -> dest EDI REP MOVSD/REP MOVSB, "
          "case8 N vs case9 4*N, no HIGHLOW, original source "
          "witness mutations and forged retail SHA refusal")

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
