#!/usr/bin/env python3
"""Phase1675: original 2006 PE32 PostNetwork scalar diagnostic formatter gates.

Two source-authenticated whole original native methods:
  0x24198360 (30 bytes) binary (%i bytes), four-arg __thiscall RET16
  0x24198380 (33 bytes) short (%i), four-arg __thiscall RET16
Each compares owner DWORD this+0x34 to current record this+0x18.
Only if equal, the first passes argument2 AS IS and second
dereferences argument2 as pointer to signed WORD. Both invoke the
SAME opaque original 0x24197FE0 formatter with original on-disk
format string and ECX=this; no formatted-string result is assumed.

Owner-only audit SHA-gates BOTH whole original 2006 PE32 client.dll
and engine.dll, two entire-method SHA, source caller direct rel32,
native .rdata literals, HIGHLOW relocation sites, x86 instruction
structure and RET16/padding. Public --self-test makes ONLY invented
non-executing sparse x86 chunks and rejects mutation/forged SHA.
No original machine code body/DLL is distributed or executed.
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
from audit_sin_original_postnetwork_type_dispatch_13case import (
    DISPATCH_VA,DISPATCH_BYTES,DISPATCH_SHA,TYPE_TO_CASE,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

BINARY_VA=0x24198360
BINARY_BYTES=30
BINARY_SHA="c654d2ca175b5fad2fd98a18efcbc4709f8feb15190c9ee733e578d63cf6279d"
SHORT_VA=0x24198380
SHORT_BYTES=33
SHORT_SHA="80ed762c28807f23273f2449855fd6063b3b3d37ee4068a007b77e3b2783cb2f"
BINARY_LITERAL_VA=0x242A3FF0
BINARY_LITERAL=b"binary (%i bytes)\0"
SHORT_LITERAL_VA=0x242A4004
SHORT_LITERAL=b"short (%i)\0"
OPAQUE_FORMAT_VA=0x24197FE0
ORIGINAL_THIS_CURRENT_RECORD_OFFSET=0x18
ORIGINAL_THIS_DIAG_SELECTED_RECORD_OFFSET=0x34
ORIGINAL_NATIVE_STACK_ARGS=4
ORIGINAL_NATIVE_RET_POP_BYTES=16
BINARY_CALLS=(0x24199973,0x24199A87)
SHORT_CALLS=(0x24199A1F,)
BINARY_LITERAL_RELOC=BINARY_VA+0x0E
SHORT_LITERAL_RELOC=SHORT_VA+0x11

def pack(v:int)->bytes:
    return struct.pack("<I",v)
def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def binary_witnesses(*,base:int,literal:int,format_va:int):
    return (
       (0x00,bytes.fromhex("8b41343b4118")),
       (0x06,bytes.fromhex("7513")),
       (0x08,bytes.fromhex("8b54240852")),
       (0x0D,b"\x68"+pack(literal)),
       (0x12,bytes.fromhex("51")),
       (0x13,relcall(base+0x13,format_va)),
       (0x18,bytes.fromhex("83c40c")),
       (0x1B,bytes.fromhex("c21000")),
    )

def short_witnesses(*,base:int,literal:int,format_va:int):
    return (
       (0x00,bytes.fromhex("8b41343b4118")),
       (0x06,bytes.fromhex("7516")),
       (0x08,bytes.fromhex("8b5424080fbf0250")),
       (0x10,b"\x68"+pack(literal)),
       (0x15,bytes.fromhex("51")),
       (0x16,relcall(base+0x16,format_va)),
       (0x1B,bytes.fromhex("83c40c")),
       (0x1E,bytes.fromhex("c21000")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,
          name+" original whole-native-method extent differs")
    positions=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" native source witness exceeds function extent")
        check(blob[pos:pos+len(value)]==value,
              name+" native opcode/operand witness mismatch @"+hex(pos))
        positions.update(range(pos,pos+len(value)))
    return len(positions)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "parent whole-file original prediction audit unavailable")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and TYPE_TO_CASE[8]==8
          and TYPE_TO_CASE[9]==9 and TYPE_TO_CASE[7]==7,
          "original typed dispatcher case map changed")
    dispatcher=section_read(pe,DISPATCH_VA,DISPATCH_BYTES,".text")
    check(hashlib.sha256(dispatcher).hexdigest()==DISPATCH_SHA,
          "parent original typed dispatcher whole SHA mismatch")
    for addr,target in (
       *((call,BINARY_VA) for call in BINARY_CALLS),
       *((call,SHORT_VA) for call in SHORT_CALLS)):
        check(section_read(pe,addr,5,".text")==relcall(addr,target),
              "original typed case relative CALL no longer reaches helper "+
              hex(addr))
    for labeladdr,label in (
        (BINARY_LITERAL_VA,BINARY_LITERAL),
        (SHORT_LITERAL_VA,SHORT_LITERAL)):
        check(section_read(pe,labeladdr,len(label),".rdata")==label,
              "native literal format is not owner-original retail")
    for va,size,digest,w,label in (
        (BINARY_VA,BINARY_BYTES,BINARY_SHA,binary_witnesses(
             base=BINARY_VA,literal=BINARY_LITERAL_VA,
             format_va=OPAQUE_FORMAT_VA),"binary"),
        (SHORT_VA,SHORT_BYTES,SHORT_SHA,short_witnesses(
             base=SHORT_VA,literal=SHORT_LITERAL_VA,
             format_va=OPAQUE_FORMAT_VA),"short")):
        body=section_read(pe,va,size,".text")
        check(hashlib.sha256(body).hexdigest()==digest,
              "entire native "+label+" formatter gate SHA differs")
        verify_sparse(body,size,w,label)
        check(section_read(pe,va+size,2,".text")==b"\xCC\xCC",
              "original native "+label+" function not INT3-bounded")
    reloc=pe_highlow_relocations(pe)
    own_binary={x for x in reloc
                if BINARY_VA<=x<BINARY_VA+BINARY_BYTES}
    own_short={x for x in reloc
               if SHORT_VA<=x<SHORT_VA+SHORT_BYTES}
    check(own_binary=={BINARY_LITERAL_RELOC} and
          own_short=={SHORT_LITERAL_RELOC},
          "native formatter literals not exactly HIGHLOW relocated")
    return {
       "phase":1675,
       "provenance":"BOTH SHA-authenticated whole original 2006 client.dll+engine.dll",
       "original_binary_formatter_va":hex(BINARY_VA),
       "original_binary_formatter_size":BINARY_BYTES,
       "original_binary_whole_method_sha256":BINARY_SHA,
       "original_short_formatter_va":hex(SHORT_VA),
       "original_short_formatter_size":SHORT_BYTES,
       "original_short_whole_method_sha256":SHORT_SHA,
       "total_new_whole_sha_original_method_bytes":BINARY_BYTES+SHORT_BYTES,
       "native_thiscall_four_stack_dword_retpop":ORIGINAL_NATIVE_RET_POP_BYTES,
       "original_this_current_record":hex(ORIGINAL_THIS_CURRENT_RECORD_OFFSET),
       "original_this_selected_debug_record":
           hex(ORIGINAL_THIS_DIAG_SELECTED_RECORD_OFFSET),
       "original_guard":"[this+0x34] == [this+0x18]",
       "original_binary_input":"original arg2 full DWORD passed unchanged",
       "original_short_input":"signed 16-bit value read via original arg2 pointer",
       "original_binary_literal_va":hex(BINARY_LITERAL_VA),
       "original_binary_literal":BINARY_LITERAL[:-1].decode(),
       "original_short_literal_va":hex(SHORT_LITERAL_VA),
       "original_short_literal":SHORT_LITERAL[:-1].decode(),
       "original_binary_source_callers":[hex(x) for x in BINARY_CALLS],
       "original_short_source_callers":[hex(x) for x in SHORT_CALLS],
       "original_downstream_opaque_format_va":hex(OPAQUE_FORMAT_VA),
       "formatter_effects_and_string_ownership_opaque":True,
       "retail_native_functions_executed":0,
       "retail_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("corrupt original scalar formatter gate accepted: "+label)

def self_test()->None:
    for label,va,length,w in (
        ("invented_binary",0x45110000,BINARY_BYTES,
         binary_witnesses(base=0x45110000,literal=0x46200000,
                          format_va=0x45120000)),
        ("invented_short",0x45130000,SHORT_BYTES,
         short_witnesses(base=0x45130000,literal=0x46210000,
                         format_va=0x45140000))):
        fake=bytearray(b"\xCC"*length)
        touched=set()
        for pos,frag in w:
            fake[pos:pos+len(frag)]=frag
            touched.update(range(pos,pos+len(frag)))
        check(verify_sparse(bytes(fake),length,w,label)==len(touched),
              "synthetic native opcode coverage differs "+label)
        for pos in sorted(touched):
            bad=bytearray(fake);bad[pos]^=1
            reject(lambda data=bytes(bad):verify_sparse(
                data,length,w,label),
                label+" synthetic byte "+str(pos))
        reject(lambda:verify_sparse(bytes(fake[:-1]),length,w,label),
               label+" truncated method")
        reject(lambda:verify_sparse(bytes(fake)+b"\xCC",length,w,label),
               label+" INT3 included in original method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner retail whole-client/engine SHA forgery")
    print("PASS Phase1675 original-free: whole 30B binary and "
          "33B signed-short native formatter gate SHA, 4arg RET16 "
          "each, this+0x34 selected record vs this+0x18 current, "
          "binary arg2 raw DWORD / short signed *arg2 WORD, exact "
          "retail profile label literals and three dispatcher callers, "
          "one HIGHLOW literal each, opaque 0x24197FE0 dependency, "
          "full sparse x86 mutants and forged-original refusal")

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
