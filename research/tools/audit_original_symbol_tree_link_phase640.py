#!/usr/bin/env python3
"""Phase640: authenticate original 2006 SiN symbol-tree link and slot accessor.

Requires the unchanged owner's *two* retail DLLs. Public --self-test uses only
invented bytes; none of the retail DLL contents is distributed in this tool.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_symbol_slot_allocation_phase639 import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256,
    check_full, check_edge, check_pin
)

METHODS = (
    ("symbol_tree_link_and_rebalance_dispatch", 0x22281F30, 106,
     "42db6c305a6dda67117bfd276134dd609f397af1de0e4e79e7a9d98ae5dcedc5"),
    ("direct_16byte_slot_accessor", 0x22280D30, 14,
     "d24e0ecdd1504637ccce7f2bc4cbcb165a1773826729ffa8f96748273dadd406"),
)
EDGES = (
    (0x222822A6, 0x22281F30, "original_Phase639_register_to_link"),
    (0x22281F74, 0x22281840, "left_child_rebalance"),
    (0x22281F83, 0x22281840, "right_child_rebalance"),
    (0x22281F91, 0x22281840, "root_rebalance"),
)
# Independently address-pinned exact x86 instruction bytes. In addition to
# exact SHA of each ENTIRE original method, these enforce semantics/provenance.
PINS = (
 (0x22281F30,"668b542408"),  # second argument low WORD: parent index
 (0x22281F35,"56"),
 (0x22281F36,"8b742408"),  # original full DWORD index preserved in ESI
 (0x22281F3A,"0fb7c6"),  # slot address uses low WORD index
 (0x22281F3D,"c1e004"),  # 16-byte slot stride
 (0x22281F40,"034104"),  # original [table+4] slot array
 (0x22281F43,"6681faffff"),  # parent sentinel comparison
 (0x22281F48,"66895004"),  # slot+4 parent FIRST
 (0x22281F4C,"66c74002ffff"),  # slot+2=FFFF second
 (0x22281F52,"66c700ffff"),  # slot+0=FFFF third
 (0x22281F57,"66c740060000"),  # slot+6=0 fourth
 (0x22281F5D,"742d"),  # sentinel root branch
 (0x22281F5F,"0fb7c2"),  # parent index zero extend
 (0x22281F62,"8b5104"),  # array pointer re-read AFTER child init
 (0x22281F65,"c1e004"),
 (0x22281F68,"807c241000"),  # low BYTE side flag
 (0x22281F6D,"740e"),  # zero -> right child
 (0x22281F6F,"56"),  # pushes ORIGINAL 32-bit index for rebalance
 (0x22281F70,"66893410"),  # side nonzero: parent slot +0
 (0x22281F79,"5e"),
 (0x22281F7A,"c20c00"),
 (0x22281F7D,"56"),
 (0x22281F7E,"6689741002"),  # side zero: parent slot +2
 (0x22281F88,"5e"),
 (0x22281F89,"c20c00"),
 (0x22281F8C,"56"),
 (0x22281F8D,"66897110"),  # root at table+0x10
 (0x22281F96,"5e"),
 (0x22281F97,"c20c00"),
 (0x22280D30,"0fb7442404"),  # index argument low WORD
 (0x22280D35,"c1e004"),
 (0x22280D38,"034104"),
 (0x22280D3B,"c20400"),
)
def authenticate(server: bytes, client: bytes) -> dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("wrong exact original SiN 2006 DLL identity")
    pe=PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original server PE32 image base")
    for name,va,length,digest in METHODS:
        if pe.section(va)!=".text":
            raise VerificationError(f"original method outside .text: {name}")
        check_full(pe.at,va,length,digest)
    for site,target,role in EDGES:
        if pe.section(site)!=".text" or pe.section(target)!=".text":
            raise VerificationError(f"original rel32 edge outside .text: {role}")
        check_edge(pe.at,site,target)
    for va,hexbytes in PINS:check_pin(pe.at,va,hexbytes)
    if pe.at(0x22281F9A,1)!=b"\xcc" or pe.at(0x22280D3E,1)!=b"\xcc":
        raise VerificationError("original method end boundary not INT3 padding")
    return {
       "status":"PHASE640_RETAIL_2006_ORIGINAL_SYMBOL_TREE_LINK_VERIFIED",
       "server_sha256":SERVER_SHA256,
       "client_sha256":CLIENT_SHA256,
       "complete_native_methods":[
           {"name":name,"va":hex(va),"bytes":length,"sha256":digest}
           for name,va,length,digest in METHODS],
       "complete_original_executable_bytes":sum(m[2] for m in METHODS),
       "native_edges":[{"site":hex(site),"target":hex(target),"role":role}
                       for site,target,role in EDGES],
       "instruction_witnesses":len(PINS),
       "mandatory_unrecovered_rebalancer":"0x22281840",
       "original_game_process_tested":False,
       "android_quest_gameplay_tested":False,
    }
def self_test():
    for size in (106,14):
        invented=bytes((i*19+size)&0xff for i in range(size))
        digest=hashlib.sha256(invented).hexdigest()
        check_full(lambda _va,n:invented[:n],0x71000000,size,digest)
        for i in range(size):
            altered=bytearray(invented);altered[i]^=1
            try:check_full(lambda _va,n:bytes(altered[:n]),0x71000000,size,digest)
            except VerificationError:pass
            else:raise AssertionError("accepted corrupted complete method")
    site,target=0x71001000,0x71002000
    original=b"\xe8"+struct.pack("<i",target-site-5)
    check_edge(lambda _va,n:original[:n],site,target)
    for i in range(5):
        altered=bytearray(original);altered[i]^=1
        try:check_edge(lambda _va,n:bytes(altered[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError("accepted changed rel32 edge")
    check_pin(lambda _va,n:b"\x80\x7c\x24\x10\x00"[:n],site,"807c241000")
    try:check_pin(lambda _va,n:b"\x83\x7c\x24\x10\x00"[:n],site,"807c241000")
    except VerificationError:pass
    else:raise AssertionError("accepted DWORD compare instead of BYTE side")
    try:authenticate(b"not-original",b"not-original")
    except VerificationError:pass
    else:raise AssertionError("accepted unrelated DLLs")
    print("PASS Phase640 invented full-method SHA, branch-width, rel32, wrong-DLL mutation refusals")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    if a.self_test:
        self_test();return
    if not a.server or not a.client:
        p.error("requires original --server and --client, or --self-test")
    try:payload=authenticate(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        p.exit(1,"PHASE640 ORIGINAL RETAIL AUDIT REFUSED: "+str(exc)+"\n")
    text=json.dumps(payload,indent=2)+"\n"
    if a.output:a.output.write_text(text,encoding="utf-8")
    else:print(text,end="")
if __name__=="__main__":main()
