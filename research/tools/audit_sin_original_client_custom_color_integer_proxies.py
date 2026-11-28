#!/usr/bin/env python3
"""Phase1710 source SHA audit original color and decrement 2006 PE32 callbacks.

Owner-original native code remains PRIVATE and unchanged. Public tests use
a compact source callback inventory and C++ source reconstruction only.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

SOURCE_METHODS=(
    (0x241A8BF0,38,
     "d60b249c0c261174c934cd38641d46656fd299741af2b43b3bc4cd7e944a5a1b",4,0x241A8C20),
    (0x241A8C20,17,
     "bbea8c9b7a7fce6219d595fb3305ce6b6137f5186e171df2ab93d3fe23323582",5,0x241A8C40),
)
SOURCE_COLOR_CODE=bytes.fromhex(
    "8b4424048b40048b4c240c8bd0c1ea1888118bd0"
    "c1ea108851018bd0c1ea08885102884103c3")
SOURCE_DECREMENT_CODE=bytes.fromhex(
    "8b4424048b48048b54240c83e901890ac3")


def require(ok,why):
    if not ok: raise ValueError(why)


def verify_catalog(header:Path):
    addresses=public_header(header)
    for va,length,digest,expected_calls,nextVA in SOURCE_METHODS:
        require(addresses.count(va)==expected_calls,
                f"source callback VA {va:#x} caller count drift")
    require(len(SOURCE_COLOR_CODE)==38 and len(SOURCE_DECREMENT_CODE)==17,
            "original color/dec source instruction length mismatch")


def verify_original(path:Path):
    binary=path.read_bytes()
    require(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
            "original unchanged 2006 SiN client SHA256 changed")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    def at(va,count):
        start=va-base
        require(0<=start<=len(code)-count,
                "original custom callback outside PE32 .text")
        return code[start:start+count]
    for va,length,digest,expected_calls in SOURCE_METHODS:
        body=at(va,length)
        require(hashlib.sha256(body).hexdigest()==digest,
                f"source 2006 complete method SHA changed {va:#x}")
        require(at(va+length,(nextVA-va-length))==b"\xCC"*(nextVA-va-length),
                f"original method padding drift {va:#x}")
    require(at(SOURCE_METHODS[0][0],38)==SOURCE_COLOR_CODE,
            "original received DWORD color byte order changed")
    require(at(SOURCE_METHODS[1][0],17)==SOURCE_DECREMENT_CODE,
            "original received DWORD SUB ECX,1 changed")
    print("PASS Phase1710 38-byte original color and 17-byte subtract-one "
          "PE32 RecvProxies, four/five source callback identities")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_source_proxy_catalog.hpp"))
    args=parser.parse_args()
    require(args.self_test or args.client is not None,
            "choose --self-test or --client /private/client.dll")
    verify_catalog(args.header)
    if args.self_test:
        for va,length,digest,count,nextVA in SOURCE_METHODS:
            require(hashlib.sha256(b"\x90"*length).hexdigest()!=digest,
                    "invented native method unexpectedly authenticated")
        print("PASS Phase1710 source-free 4+5 original custom callback routes "
              "and native SHA length mutation refusal")
    if args.client is not None:verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print("FAIL Phase1710:",exc,file=sys.stderr)
        sys.exit(1)
