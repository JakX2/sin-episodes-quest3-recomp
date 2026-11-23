#!/usr/bin/env python3
"""Authenticate original 2006 SiN retail tree rotations and sentinel accessor.

Requires owner-provided original server.dll and client.dll. Public --self-test
uses only invented bytes. No original DLLs are distributed.
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

METHODS=(
 ("original_tree_rotate_left",0x22281180,231,
  "9092a623c08c10b6537a42c5ef8d50ad803fb44e1c5170821b1ca91b73a28888"),
 ("original_tree_rotate_right",0x22281270,234,
  "96aead04d0b3c209f367fbd5fee136a2e9d593ba7e1db18c401f5f20ad87a47a"),
 ("original_sentinel_aware_slot_accessor",0x22280D40,81,
  "af140a33de10123b9607b600d5d08d64bcf62bdd995d02d6779580a4f6d23305"),
)
EDGES=(
 (0x22281B47,0x22281180,"rebalance_calls_left_a"),
 (0x22281BFF,0x22281270,"rebalance_calls_right_a"),
 (0x22281D23,0x22281270,"rebalance_calls_right_b"),
 (0x22281DDB,0x22281180,"rebalance_calls_left_b"),
)
EDGES+=tuple((site,0x22280D40,"rotate_left_resolves_sentinel_slot") for site in (
 0x2228118B,0x22281197,0x222811B0,0x222811BF,0x222811DC,
 0x222811FC,0x22281208,0x22281215,0x2228122A))
EDGES+=tuple((site,0x22280D40,"rotate_right_resolves_sentinel_slot") for site in (
 0x2228127B,0x22281286,0x2228129F,0x222812AF,0x222812CD,
 0x222812ED,0x222812F9,0x22281307,0x2228131D))
# Source-exact, individually address-pinned opcode witnesses.
PIN_WORDS="""22280D40:b801000000 22280D45:840540915822 22280D50:7521
22280D52:090540915822 22280D58:66891538915822 22280D5F:6689153a915822
22280D66:6689153c915822 22280D6D:66a33e915822 22280D73:668b442404
22280D78:663bc2 22280D7B:740c 22280D7D:0fb7c0 22280D80:c1e004
22280D83:034104 22280D86:c20400 22280D89:b838915822 22280D8E:c20400
22281180:53 22281184:8b7c2414 2228118B:e8b0fbffff 22281190:668b5802
2228119C:8b4e04 2228119F:668b00 222811A2:0fb7ef 222811A5:c1e504
222811A8:6689442902 222811B5:668138ffff 222811BA:7416 222811C4:0fb710
222811C7:8b4604 222811CD:66897c0204 222811D2:6681fbffff 222811D7:741a
222811E1:668b4004 222811EE:6689441104 222811F3:663b7e10 222811F7:7447
2228120D:663938 22281213:7515 2228121A:0fb74004 22281224:66891c08
2228122F:0fb75004 22281239:66895c0202 22281240:66895e10
22281244:8b5604 2228124D:6681ffffff 22281252:66893c11 22281256:7408
2228125B:66895c2804 22281264:c20400
22281270:53 22281271:8b5c2408 2228127B:e8c0faffff 22281280:668b38
2228128B:8b4e04 2228128E:668b4002 22281292:0fb7eb 22281295:c1e504
22281298:66890429 222812A4:66817802ffff 222812AA:7417 222812B4:0fb75002
222812B8:8b4604 222812BE:66895c0204 222812C3:6681ffffff 222812C8:741a
222812D2:668b4004 222812DF:6689441104 222812E4:663b5e10 222812E8:7448
222812FE:66395802 22281305:7516 2228130C:0fb74004 22281316:66897c0802
22281322:0fb75004 2228132C:66893c02 22281332:66897e10 22281336:8b5604
2228133F:6681fbffff 22281344:66895c1102 22281349:7408
2228134E:66897c2804 22281357:c20400"""
PINS=tuple((int(entry.split(":")[0],16),entry.split(":")[1])
           for entry in PIN_WORDS.split())
assert len(PINS)==83
def authenticate(server:bytes,client:bytes)->dict:
    if (hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("original SiN 2006 server/client whole DLL identities do not match")
    pe=PEImage(server)
    if pe.base!=0x22000000:raise VerificationError("not original PE32 server.dll VA base")
    for name,va,n,sha in METHODS:
        if pe.section(va)!=".text":raise VerificationError(f"method {name} outside executable .text")
        check_full(pe.at,va,n,sha)
        if pe.at(va+n,1)!=b"\xcc":
            raise VerificationError(f"original {name} boundary no longer INT3 padded")
    if hashlib.sha256(pe.at(0x22281840,1482)).hexdigest()!=(
        "4ef6465536a6a442a842531f09f0d8f0a37f274be6403d540528b24059527ede"):
        raise VerificationError("rebalance caller body not the original verified retail routine")
    for site,target,reason in EDGES:
        if pe.section(site)!=".text" or pe.section(target)!=".text":
            raise VerificationError(f"call edge {reason} outside original .text")
        check_edge(pe.at,site,target)
    for va,code in PINS:check_pin(pe.at,va,code)
    return dict(
        status="PHASE641_AUTHENTIC_RETAIL_TREE_ROTATIONS_VERIFIED",
        original_server_sha256=SERVER_SHA256,
        original_client_sha256=CLIENT_SHA256,
        original_methods=[dict(name=name,va=hex(va),bytes=n,sha256=sha)
                          for name,va,n,sha in METHODS],
        complete_original_method_bytes=sum(m[2] for m in METHODS),
        native_direct_calls=len(EDGES),opcode_witnesses=len(PINS),
        original_rebalance_caller_sha256="4ef6465536a6a442a842531f09f0d8f0a37f274be6403d540528b24059527ede",
        rebalance_fully_reconstructed=False,original_game_process_tested=False,
        quest_gameplay_tested=False)
def self_test():
    for n in (231,234,81):
        invented=bytes(((i*17+n)&255) for i in range(n))
        digest=hashlib.sha256(invented).hexdigest()
        check_full(lambda _va,k:invented[:k],0x71001000,n,digest)
        for i in range(n):
            changed=bytearray(invented);changed[i]^=1
            try:check_full(lambda _va,k:bytes(changed[:k]),0x71001000,n,digest)
            except VerificationError:pass
            else:raise AssertionError("corrupted whole method accepted")
    site,target=0x71001000,0x71002000
    original=b"\xe8"+struct.pack("<i",target-site-5)
    check_edge(lambda _va,n:original[:n],site,target)
    for i in range(5):
        changed=bytearray(original);changed[i]^=1
        try:check_edge(lambda _va,n:bytes(changed[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError("wrong native rel32 accepted")
    check_pin(lambda _va,n:b"\x66\x81\xff\xff\xff"[:n],site,"6681ffffff")
    try:check_pin(lambda _va,n:b"\x81\xff\xff\xff\xff"[:n],site,"6681ffffff")
    except VerificationError:pass
    else:raise AssertionError("accepted wrong WORD-vs-DWORD width")
    try:authenticate(b"not-sin-server",b"not-sin-client")
    except VerificationError:pass
    else:raise AssertionError("accepted wrong original DLLs")
    print("PASS Phase641 invented full-method hashes/rel32/width mutation checks")
def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument("--server",type=Path)
    a.add_argument("--client",type=Path)
    a.add_argument("--self-test",action="store_true")
    a.add_argument("--output",type=Path)
    args=a.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        a.error("requires exact retail original --server and --client DLL paths or --self-test")
    try:result=authenticate(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        a.exit(1,"PHASE641 OWNER RETAIL ORIGINAL AUDIT REFUSED: "+str(exc)+"\n")
    report=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(report,encoding="utf-8")
    else:print(report,end="")
if __name__=="__main__":main()
