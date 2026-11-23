#!/usr/bin/env python3
"""Phase637: authenticate complete original SiN 2006 LOCALID registrar.

Original 217-byte general range registrar and 31-byte "task" wrapper,
three rel32 edges, original format messages and independent opcode pins.
Original owner-provided retail DLLs remain private. --self-test uses only
invented bytes and MUST reject changed methods, calls or opcodes.
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
    ("original_localid_range_registrar",0x22073760,217,
     "31a68dbb81e3cdd2ea8ac037a1feb1c99028c6b1ab39b61dd663bd1798361356"),
    ("original_task_localid_wrapper",0x22088FA0,31,
     "a582842fd9f4d3c807f486c6f2f0cb8bf5c3074c1337459399c230c68a1150ed")
)
DIRECT_EDGES=(
    (0x2207380E,0x22073700,"register_local_id_via_original_58_byte_converter"),
    (0x22073822,0x22282370,"forward_mapped_symbol_to_original_engine"),
    (0x22088FB7,0x22073760,"original_task_registration_wrapper")
)
ORIGINAL_ERROR_STRINGS=(
    (0x223E91A8,b"ERROR: Adding symbol to uninitialized table %s\n\x00"),
    (0x223E9188,b"ERROR: Bad %s LOCALID for %s\n\x00"),
    (0x223E9154,b"ERROR: %s First added %s must be first LOCALID!\n\x00"),
    (0x223EB17C,b"task\x00")
)
WITNESSES=(
    (0x22073760,"57"), # push EDI
    (0x22073761,"8b7914"), # load original node+0x14 resource pointer
    (0x22073764,"85ff"),
    (0x22073766,"7519"),
    (0x22073768,"8b442414"), # uninitialized log passes display name
    (0x2207376E,"68a8913e22"),
    (0x22073772,"ff154c213d22"), # imported diagnostic
    (0x2207377E,"c21000"), # 4 native stack arguments
    (0x22073781,"8179040f270000"), # low==9999 sentinel
    (0x22073788,"8b44240c"), # original LOCALID arg2
    (0x2207378E,"8b5110"), # original next
    (0x22073793,"894104"), # publish first low BEFORE next ordering check
    (0x22073798,"8b5204"), # next low
    (0x2207379B,"81fa0f270000"),
    (0x220737A3,"3bc2"), # signed low<=next low fails
    (0x220737A5,"7f1e"),
    (0x220737B1,"6888913e22"), # Bad LOCALID diagnostic
    (0x220737C5,"8b5104"),
    (0x220737C8,"3bc2"), # below-range check signed
    (0x220737CA,"7d1e"),
    (0x220737D6,"6854913e22"), # First added diagnostic
    (0x220737EB,"8b7108"), # existing high
    (0x220737EE,"83feff"), # high=-1 uninitialized
    (0x220737F3,"3bc6"),
    (0x220737F5,"7e16"), # don't extend if ID<=high
    (0x220737F7,"8b31"), # mapped start
    (0x220737F9,"2bf2"), # subtract low
    (0x220737FB,"03f0"), # add original new ID (wrap modulo32)
    (0x220737FD,"894108"), # publish high
    (0x22073800,"89710c"), # publish mapped high
    (0x22073805,"895108"), # initial high=first low
    (0x22073808,"8b11"),
    (0x2207380A,"89510c"), # initial mapped high=mapped start
    (0x2207380D,"50"), # push local id for mapper
    (0x22073813,"8bf0"), # mapped result
    (0x22073815,"83feff"), # -1 skips symbol forwarding
    (0x2207381A,"8b44240c"), # original source symbol arg1
    (0x2207381E,"8b0f"), # ECX=[source record]
    (0x22073820,"56"), # push mapped id second arg
    (0x22073821,"50"), # push source symbol first arg
    (0x22073827,"8d4601"), # mapped+1 max
    (0x2207382A,"394704"), # source record +4 signed count check
    (0x2207382D,"7d03"), # no update if >=
    (0x2207382F,"894704"), # record max update AFTER external callback
    (0x22073833,"b001"), # original normal return 1
    (0x22073836,"c21000"), # original ret16
    (0x22088FA0,"8b44240c"), # wrapper arg3 display
    (0x22088FA4,"8b542408"), # wrapper arg2 id
    (0x22088FA9,"8b442408"), # wrapper arg1
    (0x22088FAD,"687cb13e22"), # "task" category, NOT animation
    (0x22088FB4,"83c11c"), # wrapper adds owner+0x1C
    (0x22088FBC,"c20c00"), # wrapper ret12
)

def check_full(read,va,size,expected):
    original=read(va,size)
    if len(original)!=size or hashlib.sha256(original).hexdigest()!=expected:
        raise VerificationError(f"complete original 2006 method SHA mismatch {va:#x}")
def check_direct_call(read,site,target):
    opcode=read(site,5)
    if len(opcode)!=5 or opcode[0]!=0xE8 or (
        site+5+struct.unpack_from("<i",opcode,1)[0]!=target):
        raise VerificationError(f"original rel32 CALL mismatch {site:#x}")
def check_opcode(read,va,expected):
    data=bytes.fromhex(expected)
    if read(va,len(data))!=data:
        raise VerificationError(f"original x86 instruction witness mismatch {va:#x}")
def check_text(read,va,expected):
    if read(va,len(expected))!=expected:
        raise VerificationError(f"original logging/category literal mismatch {va:#x}")
def check_original_originals(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("original 2006 SiN server.dll/client.dll SHA256 identity mismatch")
    pe=PEImage(server)
    if pe.base!=0x22000000 or any(
        pe.section_of(va)!=".text" for _,va,_,_ in METHODS
    ) or any(pe.section_of(site)!=".text" for site,_,_ in DIRECT_EDGES):
        raise VerificationError("unexpected original PE32 machine code layout")
    if any(pe.section_of(va)!=".rdata"
           for va,_ in ORIGINAL_ERROR_STRINGS):
        raise VerificationError("original error messages must be in .rdata")
    for _,va,size,digest in METHODS:
        check_full(pe.at,va,size,digest)
    for site,target,_ in DIRECT_EDGES:
        check_direct_call(pe.at,site,target)
    for va,opcode in WITNESSES:
        check_opcode(pe.at,va,opcode)
    for va,expected in ORIGINAL_ERROR_STRINGS:
        check_text(pe.at,va,expected)
    return {
       "status":"ORIGINAL_2006_ELITE_RANGE_REGISTRATION_PHASE637_VERIFIED",
       "server_sha256":SERVER_SHA256,
       "client_sha256":CLIENT_SHA256,
       "new_original_methods":[
          {"name":name,"native_va":hex(va),"bytes":size,"sha256":digest}
          for name,va,size,digest in METHODS
       ],
       "new_complete_original_bytes":sum(m[2] for m in METHODS),
       "native_direct_calls":[
          {"site":hex(site),"target":hex(target),"reason":role}
          for site,target,role in DIRECT_EDGES
       ],
       "separate_opcode_witnesses":len(WITNESSES),
       "verified_original_error_and_category_literals":len(ORIGINAL_ERROR_STRINGS),
       "wrapper_proves_category":"task",
       "wrapper_node_offset":"0x1C",
       "animation_node_offset":"0x34",
       "not_claimed":"Original animation event-ID registration callsite unknown",
       "original_engine_process_tested":False,
       "replacement_original_callbacks_observed":False
    }
def self_test():
    invented=bytes((27*i+11)&255 for i in range(239))
    digest=hashlib.sha256(invented).hexdigest()
    check_full(lambda _v,n:invented[:n],0x71001000,len(invented),digest)
    for i in range(len(invented)):
        altered=bytearray(invented)
        altered[i]^=1
        try:check_full(lambda _v,n:bytes(altered[:n]),
                       0x71001000,len(invented),digest)
        except VerificationError:pass
        else:raise AssertionError("accepted corrupted invented full method")
    site=0x71002000;dest=0x71003000
    rel=b"\xe8"+struct.pack("<i",dest-site-5)
    check_direct_call(lambda _v,n:rel[:n],site,dest)
    for i in range(5):
        corrupted=bytearray(rel);corrupted[i]^=1
        try:check_direct_call(lambda _v,n:bytes(corrupted[:n]),site,dest)
        except VerificationError:pass
        else:raise AssertionError("accepted altered invented direct CALL")
    check_opcode(lambda _v,n:b"\x83\xc1\x1c"[:n],site,"83c11c")
    try:check_opcode(lambda _v,n:b"\x83\xc1\x34"[:n],site,"83c11c")
    except VerificationError:pass
    else:raise AssertionError("accepted wrong original task/animation offset")
    check_text(lambda _v,n:b"task\x00"[:n],site,b"task\x00")
    try:check_text(lambda _v,n:b"tack\x00"[:n],site,b"task\x00")
    except VerificationError:pass
    else:raise AssertionError("accepted wrong original task literal")
    try:check_original_originals(b"wrong",b"wrong")
    except VerificationError:pass
    else:raise AssertionError("accepted wrong original DLL identity")
    print("PASS Phase637 original-free complete-method, native-rel32, opcode, task-literal mutation and wrong-DLL refusal")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.server or not args.client:
        p.error("specify BOTH original DLLs via --server/--client or --self-test")
    try:
        report=check_original_originals(
            args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        p.exit(1,"PHASE637 ORIGINAL 2006 RANGE AUDIT REFUSED: "+str(exc)+"\n")
    payload=json.dumps(report,indent=2)+"\n"
    if args.output:args.output.write_text(payload,encoding="utf-8")
    else:print(payload,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
