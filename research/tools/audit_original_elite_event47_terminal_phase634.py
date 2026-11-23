#!/usr/bin/env python3
"""Phase634: exact original 2006 Elite event47 terminal-method audit.

Verifies two COMPLETE source method bodies by SHA256, their actual caller
and callee direct-call edges, critical original opcodes, original paired
retail server/client DLL digests, and original PE32 x86 layout.
No retail machine code is shipped to GitHub; --self-test uses invented data.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256
)

METHODS=(
    ("elite_clear_original_flag",0x220398A0,91,
     "1c8c2465a759e27e7eead05d5d31074fc077765f60b8ed55ed44617a86f97327"),
    ("elite_original_state_zero",0x2203D2D0,93,
     "045a0f4fa001ae8c01ae319839067cec5aed1facaa6452174c9cda2d6eece679")
)
DIRECT_CALLS=(
    (0x220398C3,0x22073700,"flag_clear_id_map"),
    (0x2203D2D3,0x2204E420,"mandatory_state_maintenance"),
    (0x2203D2EE,0x22073700,"state_flag21_id_map"),
    (0x22355713,0x220398A0,"event47_flag3_clear"),
    (0x2235571C,0x220398A0,"event47_flag4_clear"),
    (0x22355725,0x220398A0,"event47_flag5_clear"),
    (0x2235572E,0x2203D2D0,"event47_statezero"),
    (0x2235577F,0x2203D2D0,"event43_statezero")
)
# x86 original non-relocated tiny instruction witnesses; independent of SHA.
WITNESSES=(
  (0x220398A0,"56"),
  (0x220398A1,"8b742408"),
  (0x220398A5,"81fe00ca9a3b"), # signed threshold 1,000,000,000
  (0x220398AE,"7c05"), # signed less-than map
  (0x220398B0,"83feff"), # -1 special value
  (0x220398B9,"ff9060070000"), # source virtual slot +0x760
  (0x220398C0,"8d4834"), # source ID map context+0x34
  (0x220398CC,"83f8ff"), # mapped ID = -1
  (0x220398D1,"8d88003665c4"), # mapped ID - 1,000,000,000
  (0x220398D7,"83f9ff"), # subtract result -1 special
  (0x220398DE,"c1fa05"), # original signed SAR 5
  (0x220398E1,"8d8497cc070000"), # Elite +0x7CC DWORD array
  (0x220398E8,"83e11f"), # bit &31
  (0x220398EB,"ba01000000"), # bit base
  (0x220398F0,"d3e2"), # CL shift
  (0x220398F2,"f7d2"), # complement clear mask
  (0x220398F4,"2110"), # AND original DWORD
  (0x220398F8,"c20400"), # original RET 4
  (0x2203D2D0,"568bf1"), # preserve Elite object
  (0x2203D2D8,"807c240800"), # BYTE state argument
  (0x2203D2DD,"7540"), # nonzero skip optional test
  (0x2203D2DF,"8b06"), # Elite virtual table
  (0x2203D2E3,"ff9060070000"), # vtable +0x760
  (0x2203D2E9,"6a21"), # literal test ID 0x21
  (0x2203D2EB,"8d4834"), # map context +0x34
  (0x2203D2F3,"83f8ff"), # unmapped -1
  (0x2203D2F8,"05003665c4"), # subtract 1 billion
  (0x2203D2FD,"83f8ff"), # signed index -1
  (0x2203D302,"8bc8"),
  (0x2203D304,"83e11f"),
  (0x2203D307,"ba01000000"),
  (0x2203D30C,"d3e2"),
  (0x2203D30E,"c1f805"), # arithmetic index /32
  (0x2203D311,"859486cc070000"), # test Elite +0x7CC
  (0x2203D318,"0f95c0"), # SETNE flag present
  (0x2203D31B,"84c0"),
  (0x2203D31D,"750a"), # skip state write if present
  (0x2203D31F,"c786a407000004000000"), # Elite +0x7A4 = 4
  (0x2203D32A,"c20400")
)
def hash_method(read,va,n,digest):
    data=read(va,n)
    if len(data)!=n or hashlib.sha256(data).hexdigest()!=digest:
        raise VerificationError(f"original native complete function digest mismatch at {va:#x}")
def witness(read,va,hexstring):
    b=bytes.fromhex(hexstring)
    if read(va,len(b))!=b:
        raise VerificationError(f"original instruction witness mismatch at {va:#x}")
def rel32(read,site,target):
    data=read(site,5)
    if len(data)!=5 or data[0]!=0xE8 or (
        site+5+struct.unpack_from("<i",data,1)[0]!=target):
        raise VerificationError(f"original direct call edge mismatch at {site:#x}")
def verify(read):
    for _,va,n,digest in METHODS:
        hash_method(read,va,n,digest)
    for va,target,_ in DIRECT_CALLS:
        rel32(read,va,target)
    for va,opcode in WITNESSES:
        witness(read,va,opcode)
def original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("original paired 2006 retail DLL SHA256 mismatch")
    pe=PEImage(server)
    if pe.base!=0x22000000 or any(
        pe.section_of(va)!=".text" for _,va,_,_ in METHODS
    ) or any(pe.section_of(site)!=".text" for site,_,_ in DIRECT_CALLS):
        raise VerificationError("original 2006 PE32 server .text section mismatch")
    verify(pe.at)
    return {
        "status":"ORIGINAL_2006_ELITE_EVENT47_TERMINAL_PHASE634_VERIFIED",
        "server_sha256":SERVER_SHA256,
        "client_sha256":CLIENT_SHA256,
        "two_complete_original_functions":[
            {"role":name,"va":hex(va),"bytes":size,"sha256":digest}
            for name,va,size,digest in METHODS
        ],
        "new_original_executable_bytes":sum(size for _,_,size,_ in METHODS),
        "verified_original_direct_calls":[
            {"site":hex(site),"target":hex(target),"reason":reason}
            for site,target,reason in DIRECT_CALLS
        ],
        "native_opcode_witnesses":len(WITNESSES),
        "important_bounds":{
            "original_bit_base":1000000000,
            "original_flags_array_offset":"0x7CC",
            "state_write_offset":"0x7A4",
            "state_write_value":4,
            "state_test_event":"0x21",
            "virtual_map_offset":"0x760",
            "map_context_adjustment":"0x34"
        },
        "owner_original_game_executed":False,
        "replacement_engine_callback_observed":False
    }
def synthetic_test():
    invented=bytes(((i*79+19)&255) for i in range(193))
    digest=hashlib.sha256(invented).hexdigest()
    hash_method(lambda _va,n:invented[:n],0x71002000,len(invented),digest)
    for i in range(len(invented)):
        altered=bytearray(invented)
        altered[i]^=1
        try:
            hash_method(lambda _va,n:bytes(altered[:n]),
                        0x71002000,len(invented),digest)
        except VerificationError:pass
        else:raise AssertionError("altered synthetic complete native method accepted")
    src=0x71004000
    dst=0x71005000
    raw=b"\xe8"+struct.pack("<i",dst-src-5)
    rel32(lambda _va,n:raw[:n],src,dst)
    for i in range(5):
        altered=bytearray(raw)
        altered[i]^=1
        try:rel32(lambda _va,n:bytes(altered[:n]),src,dst)
        except VerificationError:pass
        else:raise AssertionError("altered synthetic x86 CALL accepted")
    witness(lambda _va,n:b"\x6a\x21"[:n],src,"6a21")
    try:witness(lambda _va,n:b"\x6a\x22"[:n],src,"6a21")
    except VerificationError:pass
    else:raise AssertionError("altered synthetic original event ID accepted")
    try:original(b"not retail",b"not retail")
    except VerificationError:pass
    else:raise AssertionError("non-retail DLL passed identity gate")
    print("PASS Phase634 original-free full method mutation, 8-call source-edge, opcode and identity refusal")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        synthetic_test()
        return 0
    if not args.server or not args.client:
        p.error("provide both original --server and --client, or --self-test")
    try:
        result=original(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        p.exit(1,"PHASE634 ORIGINAL TERMINAL AUDIT REFUSED: "+str(exc)+"\n")
    output=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(output,encoding="utf-8")
    else:print(output,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
