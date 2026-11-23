#!/usr/bin/env python3
"""Phase633 authenticate three complete original SiN 2006 Elite 0x47 helpers.

Original owner-only retail server/client DLL SHA256 and PE32 text required.
No commercial bytes copied into CI: --self-test mutates invented data.
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
  ("clock_vs_Elite_9DC",0x220326D0,30,
   "e855a4c4805f0994d12d66af60bb42b6d9d1bea9f7aa9d7a83aca2b39943e1cc"),
  ("Elite_78C_serial_lookup",0x222AA640,46,
   "012fdc0caa1ad7c83f1107ab5e442bf2ce9eae7d87c10e11649998b88aafd5fb"),
  ("target_4EC_dirty_setter",0x2203F8E0,55,
   "355e523bed2535ee30adbe6477d3fdcba0e57d07aa9dcfc1b370028d41b81371")
)
CALL_EDGES=(
  (0x223556C5,0x220326D0,"event47_clock_predicate"),
  (0x223556D4,0x222AA640,"event47_first_78C_lookup"),
  (0x223556E3,0x222AA640,"event47_second_78C_lookup"),
  (0x223556FD,0x222AA640,"event47_third_78C_lookup"),
  (0x2235570A,0x2203F8E0,"event47_target_4EC_setter")
)
WITNESSES=(
  (0x220326D0,"a190215722"),
  (0x220326D5,"d9400c"),
  (0x220326D8,"d899dc090000"),
  (0x220326DE,"dfe0"),
  (0x220326E0,"f6c401"),
  (0x220326E3,"7506"),
  (0x220326E5,"b801000000"),
  (0x220326EB,"33c0"),
  (0x222AA640,"8b818c070000"),
  (0x222AA646,"83f8ff"),
  (0x222AA64B,"8b1520564f22"),
  (0x222AA653,"81e1ff0f0000"),
  (0x222AA659,"c1e104"),
  (0x222AA660,"c1e80c"),
  (0x222AA663,"394104"),
  (0x222AA668,"8b01"),
  (0x222AA66B,"33c0"),
  (0x2203F8E0,"8b542404"),
  (0x2203F8E6,"8b08"),
  (0x2203F8E8,"3b0a"),
  (0x2203F8EA,"7428"),
  (0x2203F8EC,"80b804fdffff00"),
  (0x2203F8F5,"c68014fdffff01"),
  (0x2203F903,"8b8880fcffff"),
  (0x2203F90D,"830901"),
  (0x2203F912,"8910"),
  (0x2203F914,"c20400"),
)
def method_hash(read,va,n,expected):
    data=read(va,n)
    if len(data)!=n or hashlib.sha256(data).hexdigest()!=expected:
        raise VerificationError(f"original complete method SHA mismatch at {va:#x}")
def pin(read,va,hex_string):
    target=bytes.fromhex(hex_string)
    if read(va,len(target))!=target:
        raise VerificationError(f"original native opcode changed at {va:#x}")
def edge(read,va,target):
    data=read(va,5)
    if len(data)!=5 or data[0]!=0xE8 or (
        va+5+struct.unpack_from("<i",data,1)[0]!=target):
        raise VerificationError(f"original event47 call target changed at {va:#x}")
def verify(read):
    for _,va,n,digest in METHODS:
        method_hash(read,va,n,digest)
    for va,value in WITNESSES:
        pin(read,va,value)
    for va,target,_ in CALL_EDGES:
        edge(read,va,target)
def verify_original(server:bytes,client:bytes):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("both owner-original retail DLL SHA256s required")
    pe=PEImage(server)
    if pe.base!=0x22000000 or any(
        pe.section_of(va)!=".text" for _,va,_,_ in METHODS
    ):
        raise VerificationError("expected original PE32 text layout missing")
    verify(pe.at)
    return {
      "status":"ORIGINAL_2006_ELITE_EVENT47_HELPERS_PHASE633_VERIFIED",
      "server_sha256":SERVER_SHA256,
      "client_sha256":CLIENT_SHA256,
      "new_complete_original_methods":[
        {"role":name,"va":hex(va),"bytes":n,"sha256":digest}
        for name,va,n,digest in METHODS
      ],
      "original_executable_bytes_added":sum(n for _,_,n,_ in METHODS),
      "direct_event47_call_edges":[
        {"site":hex(va),"target":hex(target),"role":name}
        for va,target,name in CALL_EDGES
      ],
      "opcode_witness_count":len(WITNESSES),
      "original_game_run":False,
      "original_source_semantics":{
        "predicate":"x87 current engine clock vs Elite +0x9DC, C0 bit",
        "serial":"Elite +0x78C uses original 12-bit index/upper serial global table",
        "setter":"resolved object +0x4EC writes only on change; +0x1F0 selects dirty byte +0x200 versus owner +0x16C OR bit0"
      }
    }
def self_test():
    data=bytes((i*19+7)&0xff for i in range(183))
    digest=hashlib.sha256(data).hexdigest()
    method_hash(lambda _va,n:data[:n],0x71000000,len(data),digest)
    for i in range(len(data)):
        changed=bytearray(data);changed[i]^=1
        try:method_hash(lambda _va,n:bytes(changed[:n]),
                        0x71000000,len(data),digest)
        except VerificationError:pass
        else:raise AssertionError("accepted changed synthetic full body")
    origin=0x71001000;dest=0x71002000
    call=b"\xE8"+struct.pack("<i",dest-origin-5)
    edge(lambda _va,n:call[:n],origin,dest)
    for i in range(5):
        changed=bytearray(call);changed[i]^=1
        try:edge(lambda _va,n:bytes(changed[:n]),origin,dest)
        except VerificationError:pass
        else:raise AssertionError("accepted changed synthetic rel32 edge")
    pin(lambda _va,n:b"\x83\xf8\xff"[:n],origin,"83f8ff")
    try:pin(lambda _va,n:b"\x83\xf8\x00"[:n],origin,"83f8ff")
    except VerificationError:pass
    else:raise AssertionError("accepted changed synthetic native opcode")
    try:verify_original(b"not retail",b"not retail")
    except VerificationError:pass
    else:raise AssertionError("accepted unverified DLL identity")
    print("PASS Phase633: complete-method SHA mutations, rel32, opcode pins, original DLL identity rejection")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.server or not args.client:
        p.error("supply original --server and --client or --self-test")
    try:
        result=verify_original(args.server.read_bytes(),args.client.read_bytes())
    except (VerificationError,OSError,ValueError,struct.error) as exc:
        p.exit(1,"PHASE633 ORIGINAL RETAIL AUDIT REFUSED: "+str(exc)+"\n")
    output=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(output,encoding="utf-8")
    else:print(output,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
