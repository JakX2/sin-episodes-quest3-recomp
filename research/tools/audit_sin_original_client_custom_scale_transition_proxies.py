#!/usr/bin/env python3
"""Phase1712: SHA-authenticate original Plasma/FireSmoke x87 scale callbacks.

Public --self-test validates the 1110-entry original-source callback
inventory. Private --client authenticates the unchanged owner PE32 and
the full 56/70-byte bodies, precise x87 branch/store witnesses and
original constructor PUSH callback operands. No proprietary executable
bytes are committed to the public repository.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

SOURCE=(
    # native callback VA, executable bytes, whole SHA, original PUSH-name
    # source ctor site, authentic table/slot, next function start address.
    (0x240EAA20,56,
     "5d57913b243190d93e89a433b6cd6168eb08047f1e4db82d171cbc9db251a530",
     0x240EAB87,94,2,0x240EAA60),
    (0x240DA3E0,70,
     "99b2ce915c93019fa10726580b4b971ab35cbdc28403bc171b8309ec4be053b6",
     0x240DA557,111,2,0x240DA430),
)
# Original x87 FUCOMPP compared decoded CRecvProxyData+4 IEEE754
# binary32 with source owned m_flScale at +0x484, then FNSTSW AX,
# TEST AH,0x44 and JNP. PF=0 for one asserted C3 bit (ordered EQ),
# PF=1 for zero bits (less/greater) or two (unordered).
# Therefore JNP is taken on ordered equality, including +0/-0.
# Full original bodies are authenticated by SHA256; only sparse
# x87/conditional field writes are represented here.
WITNESSES=(
    (0x240EAA33,"dae9"), (0x240EAA37,"f6c444"),
    (0x240EAA3A,"7b19"), (0x240EAA42,"d991a4040000"),
    (0x240EAA48,"d99984040000"), (0x240EAA4E,"8991a0040000"),
    (0x240DA3F3,"dae9"), (0x240DA3F7,"f6c444"),
    (0x240DA3FA,"7b23"), (0x240DA404,"dae9"),
    (0x240DA408,"f6c444"), (0x240DA40B,"7b12"),
    (0x240DA413,"d9919c040000"), (0x240DA419,"899198040000"),
    (0x240DA41F,"d99984040000"),
)

def need(condition:bool, message:str)->None:
    if not condition:raise ValueError(message)


def verify_catalog(path:Path) -> None:
    catalog=public_header(path)
    need(len(catalog)==1110,"original 1110-callback source map drift")
    for va,size,sha,site,table,slot,next_va in SOURCE:
        need(catalog.count(va)==1,
             f"original x87 source callback {va:#x} route count drift")
        need(size>0 and len(sha)==64 and next_va>va+size and
             next_va-va-size in (8,10),
             "original x87 method executable bounds changed")
    need({(rec[4],rec[5]) for rec in SOURCE}=={(94,2),(111,2)},
         "original Plasma/FireSmoke table/property references changed")


def verify_original(path:Path)->None:
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "user original 2006 PE32 client.dll whole SHA mismatch")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    def at(va:int,count:int)->bytes:
        offset=va-base
        need(count>=0 and 0<=offset<=len(code)-count,
             "original x87 function VA outside SHA-pinned PE .text")
        return code[offset:offset+count]
    for va,size,digest,source_site,table,slot,next_va in SOURCE:
        full=at(va,size)
        need(hashlib.sha256(full).hexdigest()==digest,
             f"entire original x87 callback SHA mismatch {va:#x}")
        need(full[-1]==0xC3,
             f"original x87 callback terminal RET missing {va:#x}")
        pad=next_va-va-size
        need(at(va+size,pad)==b"\xCC"*pad,
             f"source original method padding/boundary drift {va:#x}")
        # Original type1 direct float constructor has a PUSH-proxy
        # immediate 14 bytes BEFORE PUSH property name/output/CALL.
        need(at(source_site-14,5)==b"\x68"+struct.pack("<I",va),
             f"original PE32 source x87 callback PUSH changed {source_site:#x}")
        need(at(source_site,1)==b"\x68",
             "original float source constructor name PUSH changed")
    for va,opcode in WITNESSES:
        raw=bytes.fromhex(opcode)
        need(at(va,len(raw))==raw,
             f"original 2006 x87 FUCOMPP / conditional member store drift {va:#x}")
    print("PASS Phase1712 private original 56B Plasma and 70B FireSmoke "
          "x87 callback SHA, ordered compare and source constructor PUSHs")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_source_proxy_catalog.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    verify_catalog(args.header)
    if args.self_test:
        for va,size,digest,site,table,slot,next_va in SOURCE:
            need(hashlib.sha256(b"\x90"*size).hexdigest()!=digest,
                 "fabricated native x87 callback unexpectedly authentic")
        print("PASS Phase1712 original-free two x87 custom callback "
              "identities, executable size and invented-byte SHA refusal")
    if args.client is not None:
        verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1712: {exc}",file=sys.stderr)
        sys.exit(1)
