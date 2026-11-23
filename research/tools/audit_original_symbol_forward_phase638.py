#!/usr/bin/env python3
"""Phase638 original PE32 method auditor. --self-test uses invented bytes only."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_elite_range_registration_phase637 import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256,
    check_full,check_direct_call,check_opcode,check_text
)
METHODS=(
 ("condition_owner_34_wrapper",0x2205A760,31,
  "f4834824b68fcbd0c4b0ca540b8b4b354349a3d00404d952b76d66ed47dd3b28"),
 ("symbol_registration_slot_writer",0x22282370,77,
  "1ee09397a760fe3547a48936da90af271eada973a8d2d9d97fa29b89b24769be"),
)
EDGES=(
 (0x2205A777,0x22073760,"condition_wrapper_to_registrar"),
 (0x2228238A,0x2228F7A0,"symbol_tree_lookup"),
 (0x222823A0,0x22282270,"slot_allocator"),
 (0x22073822,0x22282370,"registrar_to_slot_writer"),
)
PINS=(
 (0x2205A760,"8b44240c"),(0x2205A764,"8b542408"),
 (0x2205A76D,"68c8053e22"),(0x2205A774,"83c134"),
 (0x2205A777,"e8e48f0100"),(0x2205A77C,"c20c00"),
 (0x22282370,"83ec08"),(0x22282373,"8b44240c"),
 (0x22282378,"8b31"),(0x2228237B,"8d4c2414"),
 (0x22282380,"8d4e1c"),(0x22282383,"66c744240cffff"),
 (0x2228238A,"e811d40000"),(0x2228238F,"668b10"),
 (0x22282392,"6a00"),(0x22282394,"8d442408"),
 (0x2228239B,"668954240c"),(0x222823A0,"e8cbfeffff"),
 (0x222823A5,"8b5604"),(0x222823A8,"8b742414"),
 (0x222823AC,"0fb7c8"),(0x222823AF,"c1e104"),
 (0x222823B2,"8974110c"),(0x222823B7,"83c408"),
 (0x222823BA,"c20800"),
)
def authenticate(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
       hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("original SiN 2006 DLL SHA256 mismatch")
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError("unexpected server PE32 image base")
    for _,va,n,sha in METHODS:
        if pe.section_of(va)!=".text":
            raise VerificationError("method not original .text")
        check_full(pe.at,va,n,sha)
    for site,target,_ in EDGES:
        if pe.section_of(site)!=".text" or pe.section_of(target)!=".text":
            raise VerificationError("edge outside original .text")
        check_direct_call(pe.at,site,target)
    for va,code in PINS:
        check_opcode(pe.at,va,code)
    if pe.section_of(0x223E05C8)!=".rdata":
        raise VerificationError("category outside original .rdata")
    check_text(pe.at,0x223E05C8,b"condition\x00")
    return {
      "status":"PHASE638_RETAIL_2006_ORIGINAL_SYMBOL_WRITER_VERIFIED",
      "original_method_bytes":sum(m[2] for m in METHODS),
      "methods":[{"name":n,"va":hex(va),"bytes":count,"sha256":sha}
                 for n,va,count,sha in METHODS],
      "native_calls":[{"site":hex(s),"target":hex(t),"role":r}
                      for s,t,r in EDGES],
      "opcode_witnesses":len(PINS),"category":"condition",
      "condition_node_offset":"0x34",
      "tree_lookup_fully_reconstructed":False,
      "slot_allocator_fully_reconstructed":False,
      "actual_gameplay_process_verified":False
    }
def self_test():
    made=bytes((i*13+77)&255 for i in range(108))
    sha=hashlib.sha256(made).hexdigest()
    check_full(lambda _,n:made[:n],0x71001000,len(made),sha)
    for i in range(len(made)):
        bad=bytearray(made);bad[i]^=1
        try:check_full(lambda _,n:bytes(bad[:n]),0x71001000,len(bad),sha)
        except VerificationError:pass
        else:raise AssertionError("accepted changed complete method")
    site,target=0x71002000,0x71003000
    encoded=b"\xe8"+struct.pack("<i",target-site-5)
    check_direct_call(lambda _,n:encoded[:n],site,target)
    for i in range(5):
        bad=bytearray(encoded);bad[i]^=1
        try:check_direct_call(lambda _,n:bytes(bad[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError("accepted changed original call")
    check_opcode(lambda _,n:b"\x83\xc1\x34"[:n],site,"83c134")
    try:check_opcode(lambda _,n:b"\x83\xc1\x1c"[:n],site,"83c134")
    except VerificationError:pass
    else:raise AssertionError("accepted task offset in condition wrapper")
    check_text(lambda _,n:b"condition\x00"[:n],site,b"condition\x00")
    try:check_text(lambda _,n:b"animation\x00"[:n],site,b"condition\x00")
    except VerificationError:pass
    else:raise AssertionError("accepted incorrect original string")
    try:authenticate(b"wrong",b"wrong")
    except VerificationError:pass
    else:raise AssertionError("accepted incorrect DLL identities")
    print("PASS Phase638 SHA256/rel32/opcode/condition/original-DLL refusal (invented bytes)")
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--server",type=Path);ap.add_argument("--client",type=Path)
    ap.add_argument("--self-test",action="store_true");ap.add_argument("--output",type=Path)
    a=ap.parse_args()
    if a.self_test:
        self_test();return
    if not a.server or not a.client:
        ap.error("requires --server and --client, or --self-test")
    try:report=authenticate(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        ap.exit(1,"PHASE638 ORIGINAL AUDIT REFUSED: "+str(exc)+"\n")
    payload=json.dumps(report,indent=2)+"\n"
    if a.output:a.output.write_text(payload,encoding="utf-8")
    else:print(payload,end="")
if __name__=="__main__":
    main()
