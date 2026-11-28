#!/usr/bin/env python3
"""Phase1678: SHA-authenticate complete original 93-byte CDECL
PostNetwork diagnostic formatter at client.dll VA 0x24197FE0.

The complete original 2006 PE32 client.dll+engine.dll pair is
SHA-gated before inspecting original code; no proprietary function
body or game binary is published. Exact original function body:
93 bytes, RET (NOT RET12/RET16), 0x1000 stack scratch allocation,
cdecl Q_vsnprintf(dst,4096,format,va_list), then tier0 Msg with
original literal "%i %s %s : %s\\n". Data flow reads the original
this+0x18 record pointer, record+0x04 string pointer, this+0x38
string pointer and global [0x242FA458]+0x18. The meanings of these
strings and global owning class are not invented.

The earlier 30B binary and 33B short format gates pass three
arguments to this cdecl formatter, then each caller cleans 12 bytes.
This auditor preserves only sparse structural instructions and
whole-method SHA256, plus one actual native import and relocation
witness per pointer operand. --self-test uses invented sparse
non-executable bytes and refuses all mutations/forged originals.
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
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
from audit_sin_original_allocator_game_callers import named_iat
from audit_sin_original_postnetwork_scalar_formatters import (
    BINARY_VA,SHORT_VA,BINARY_SHA,SHORT_SHA,
    BINARY_BYTES,SHORT_BYTES,OPAQUE_FORMAT_VA,
)

FORMAT_VA=0x24197FE0
FORMAT_BYTES=93
FORMAT_SHA="880a186d02da86b94b953158d2fdaee8a93d59456fdaf4f482031da3bab09cac"
STACK_SCRATCH_BYTES=4096
STACK_PROBE_VA=0x242493E0
Q_VSNPRINTF_IAT=0x2427625C
MSG_IAT=0x242761EC
GLOBAL_CONTEXT_PTR_VA=0x242FA458
SOURCE_LITERAL_VA=0x242A3F90
SOURCE_LITERAL=b"%i %s %s : %s\n\0"
ORIGINAL_THIS_CURRENT_RECORD=0x18
ORIGINAL_RECORD_NAME=0x04
ORIGINAL_THIS_DIAG_LABEL=0x38
GLOBAL_CONTEXT_OUTPUT_NUMBER=0x18
CALLER_BINARY_SITE=0x24198373
CALLER_SHORT_SITE=0x24198396
RELOCATIONS=(FORMAT_VA+0x26,FORMAT_VA+0x41,
             FORMAT_VA+0x4C,FORMAT_VA+0x52)

def pack(v:int)->bytes:
    return struct.pack("<I",v)
def relcall(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def witness_chunks(*,base:int,probe:int,qsprintf:int,globalptr:int,
                   fmt_literal:int,msg:int):
    return (
        (0x00,b"\xB8"+pack(STACK_SCRATCH_BYTES)),
        (0x05,relcall(base+0x05,probe)),
        (0x0A,bytes.fromhex("8b8c2408100000")),
        (0x11,bytes.fromhex("8d84240c100000")),
        (0x1E,b"\x68"+pack(STACK_SCRATCH_BYTES)),
        (0x24,b"\xFF\x15"+pack(qsprintf)),
        (0x2F,bytes.fromhex("8b8424181000008b48188b51048b4038")),
        (0x3F,b"\x8B\x0D"+pack(globalptr)),
        (0x46,bytes.fromhex("8b5118")),
        (0x4B,b"\x68"+pack(fmt_literal)),
        (0x50,b"\xFF\x15"+pack(msg)),
        (0x56,bytes.fromhex("81c424100000c3")),
    )

def verify_sparse(blob:bytes,length:int,w,name:str)->int:
    check(len(blob)==length,name+" complete method length mismatch")
    covered=set()
    for pos,value in w:
        check(0<=pos and pos+len(value)<=len(blob),
              name+" instruction witness exceeds native extent")
        check(blob[pos:pos+len(value)]==value,
              name+" x86 structural witness mismatch +"+hex(pos))
        covered.update(range(pos,pos+len(value)))
    return len(covered)

def audit(client_bytes:bytes,engine_bytes:bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "source parent original dual complete SHA unavailable")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE and FORMAT_VA==OPAQUE_FORMAT_VA,
          "original formatter address or base changed")
    for va,n,sha in ((BINARY_VA,BINARY_BYTES,BINARY_SHA),
                     (SHORT_VA,SHORT_BYTES,SHORT_SHA)):
        check(hashlib.sha256(section_read(pe,va,n,".text")).hexdigest()==sha,
              "native immediate caller whole-SHA drift "+hex(va))
    for site in (CALLER_BINARY_SITE,CALLER_SHORT_SITE):
        check(section_read(pe,site,5,".text")==
              relcall(site,FORMAT_VA),
              "original caller no longer enters formatter "+hex(site))
    code=section_read(pe,FORMAT_VA,FORMAT_BYTES,".text")
    check(hashlib.sha256(code).hexdigest()==FORMAT_SHA,
          "original 93-byte formatter entire native SHA mismatch")
    covered=verify_sparse(code,FORMAT_BYTES,witness_chunks(
        base=FORMAT_VA,probe=STACK_PROBE_VA,
        qsprintf=Q_VSNPRINTF_IAT,globalptr=GLOBAL_CONTEXT_PTR_VA,
        fmt_literal=SOURCE_LITERAL_VA,msg=MSG_IAT),
        "retail_original_cdecl_formatter")
    check(code[-1:]==b"\xC3" and
          section_read(pe,FORMAT_VA+FORMAT_BYTES,3,".text")==b"\xCC"*3,
          "original cdecl RET / three INT3 padding boundary changed")
    check(section_read(pe,SOURCE_LITERAL_VA,len(SOURCE_LITERAL),
                       ".rdata")==SOURCE_LITERAL,
          "retail Msg formatting literal drift")
    section_read(pe,GLOBAL_CONTEXT_PTR_VA,4,".data")
    check(named_iat(pe,"vstdlib.dll","Q_vsnprintf")==[Q_VSNPRINTF_IAT],
          "original Q_vsnprintf IAT/name drift")
    check(named_iat(pe,"tier0.dll","Msg")==[MSG_IAT],
          "original tier0 Msg IAT/name drift")
    sites={x for x in pe_highlow_relocations(pe)
           if FORMAT_VA<=x<FORMAT_VA+FORMAT_BYTES}
    check(sites==set(RELOCATIONS),
          "native original formatter four HIGHLOW relocations differ")
    return {
        "phase":1678,
        "provenance":"BOTH full original 2006 PE32 client.dll and engine.dll",
        "original_formatter_va":hex(FORMAT_VA),
        "original_method_size":FORMAT_BYTES,
        "original_method_sha256":FORMAT_SHA,
        "native_return_type":"cdecl RET (no stack-pop immediate)",
        "original_stack_scratch_bytes":STACK_SCRATCH_BYTES,
        "original_cdecl_callers_add_esp_bytes":12,
        "original_Q_vsnprintf_iat":hex(Q_VSNPRINTF_IAT),
        "original_Msg_iat":hex(MSG_IAT),
        "source_msg_format_va":hex(SOURCE_LITERAL_VA),
        "source_msg_format":SOURCE_LITERAL[:-1].decode(),
        "original_this_record_pointer_offset":hex(ORIGINAL_THIS_CURRENT_RECORD),
        "original_record_name_pointer_offset":hex(ORIGINAL_RECORD_NAME),
        "original_this_label_pointer_offset":hex(ORIGINAL_THIS_DIAG_LABEL),
        "original_global_context_ptr_va":hex(GLOBAL_CONTEXT_PTR_VA),
        "original_global_context_number_offset":hex(GLOBAL_CONTEXT_OUTPUT_NUMBER),
        "original_highlow_relocations":[hex(x) for x in RELOCATIONS],
        "sparse_native_witness_bytes":covered,
        "original_real_game_method_executions":0,
        "unknown_string_ownership_and_engine_global_identity":True,
        "windows_gameplay_boot_verified":False,
    }

def reject(fn,label:str)->None:
    try:fn()
    except ValueError:return
    raise AssertionError("forged original native formatter accepted: "+label)

def self_test()->None:
    invented_base=0x45110000
    w=witness_chunks(base=invented_base,probe=0x45200000,
                     qsprintf=0x46210000,globalptr=0x46310000,
                     fmt_literal=0x46410000,msg=0x46510000)
    fake=bytearray(b"\xCC"*FORMAT_BYTES)
    covered=set()
    for pos,frag in w:
        fake[pos:pos+len(frag)]=frag
        covered.update(range(pos,pos+len(frag)))
    check(verify_sparse(bytes(fake),FORMAT_BYTES,w,"synthetic")==
          len(covered),"invented formatter sparse evidence mismatch")
    for i in sorted(covered):
        changed=bytearray(fake);changed[i]^=1
        reject(lambda data=bytes(changed):verify_sparse(
            data,FORMAT_BYTES,w,"mutated"),str(i))
    reject(lambda:verify_sparse(bytes(fake[:-1]),FORMAT_BYTES,w,"short"),
           "truncated RET")
    reject(lambda:verify_sparse(bytes(fake)+b"\xCC",
                                FORMAT_BYTES,w,"long"),
           "trailing INT3 counted as method")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "original whole-image SHA forgery")
    print("PASS Phase1678 original-free: whole-SHA native 93B cdecl "
          "formatter, source original this+0x18/0x38 record+0x04 "
          "and global+0x18, Q_vsnprintf 4096B -> tier0 Msg, "
          "four HIGHLOW pointer relocations and two caller rel32 "
          "edges, RET0/caller ESP+12, sparse opcode mutants "
          "and forged-retail SHA refusal")

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
            p.error("retail original audit requires BOTH client and engine")
        print(json.dumps(audit(a.client.read_bytes(),
                               a.engine.read_bytes()),indent=2,sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
