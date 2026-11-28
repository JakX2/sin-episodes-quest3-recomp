#!/usr/bin/env python3
"""Phase1709: authenticate the two original 2006 PE32 handle RecvProxies.

Both native 49-byte original methods are byte-identical and remap a packed
received 32-bit handle. Their source VA labels are never executed in
an independent Win32/Android runtime. No original executable bytes are
committed to the repository.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

ORIGINAL_METHODS=(0x241A8DB0,0x241A8DF0)
SOURCE_METHOD_BYTES=49
SOURCE_METHOD_SHA="a67004960a3f69d9cfbf0662c7a7f2cc5016d0f14d8f277fbd748507314a8067"
EXPECTED_SOURCE_COUNT=(30,5)
INVALID_RAW=0x001FFFFF
INVALID_DEST=0xFFFFFFFF


def need(yes: bool,message: str) -> None:
    if not yes:raise ValueError(message)


def unsigned_source_result(raw: int) -> int:
    if raw==INVALID_RAW:return INVALID_DEST
    return ((raw<<1)&0xFFFFF000)|(raw&0x7FF)


def x86_shift_result(raw: int) -> int:
    if raw==INVALID_RAW:return INVALID_DEST
    signed=raw if raw<0x80000000 else raw-(1<<32)
    return (((signed>>11)<<12)|(raw&0x7FF))&0xFFFFFFFF


def verify_catalog(header: Path) -> None:
    labels=public_header(header)
    counts=tuple(labels.count(va) for va in ORIGINAL_METHODS)
    need(counts==EXPECTED_SOURCE_COUNT,
         f"original handler/parent source callback counts drift: {counts}")
    for raw in (0,1,0x7FF,0x800,0x801,0x1FFFFF,0x200000,
                0x7FFFFFFF,0x80000000,0xFFFFFFFF):
        need(unsigned_source_result(raw)==x86_shift_result(raw),
             f"original x86 signed SAR/SHL mismatch for raw {raw:#x}")
    # Exhaustively verify first 4096 packed source indices,
    # including the sequence-bit carry across the 11-bit index.
    for raw in range(4096):
        need(unsigned_source_result(raw)==x86_shift_result(raw),
             f"source handle packed bitshift difference for raw {raw:#x}")


def verify_original(path: Path) -> None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original owner 2006 client.dll whole hash mismatch")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def read(va:int,size:int) -> bytes:
        at=va-base
        need(0<=at<=len(code)-size,"source native VA outside .text")
        return code[at:at+size]
    methods=[read(va,SOURCE_METHOD_BYTES) for va in ORIGINAL_METHODS]
    need(methods[0]==methods[1],
         "source handle and moveparent proxy bodies are not identical")
    for va,body in zip(ORIGINAL_METHODS,methods):
        need(hashlib.sha256(body).hexdigest()==SOURCE_METHOD_SHA,
             f"complete 49-byte x86 handle proxy SHA changed at {va:#x}")
        need(read(va+49,15)==b"\xCC"*15,
             f"source original handle method end/padding mismatch {va:#x}")
        # EAX=proxydata+4. Compare to 0x001FFFFF, returning
        # 0xFFFFFFFF only on that exact source sentinel value.
        need(body[:11]==bytes.fromhex("8b442404817804ffff1f00"),
             f"source original incoming value/sentinel compare changed {va:#x}")
        need(body[11:24]==bytes.fromhex(
            "750b8b44240cc700ffffffffc3"),
             f"source invalid-handle output return sequence drift {va:#x}")
        need(body[24:]==bytes.fromhex(
            "8b40048b54240c8bc8c1f90bc1e10c25ff0700000bc8890ac3"),
             f"source handle sign-shift/index remap changed {va:#x}")
    print("PASS Phase1709 owner original: both 49B handle proxy bodies SHA-pin, "
          "invalid sentinel and exact SAR/SHL packed bit remap")


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
        need(hashlib.sha256(b"\x90"*49).hexdigest()!=SOURCE_METHOD_SHA,
             "fabricated 49-byte method unexpectedly authenticated")
        print("PASS Phase1709 original-free source handle mapping: "
              "30 entity handles + 5 moveparents / 35 source custom slots")
    if args.client is not None:
        verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1709: {exc}",file=sys.stderr)
        sys.exit(1)
