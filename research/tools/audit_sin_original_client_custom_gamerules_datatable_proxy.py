#!/usr/bin/env python3
"""Phase1714: source-proven original SiN SE1 GameRules RecvProxy.

Whole 13B 2006 PE32 function is hashed in original-only --client mode.
No original executable bytes are committed. Host runtime models a
synthetic opaque singleton token and refuses unknown/retail pointers.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

VA=0x24212E20
LENGTH=13
ORIGINAL_SHA="1b9fa5d21adeab9069a2d97339fed5cfa6e42055c079877dbe5fe7d6a2461043"
NEXT_FUNC=0x24212E30
ORIGINAL_GLOBAL=0x2433F3F8
SOURCE_CONSTRUCTOR_NAME_PUSH=0x24212E83
EXPECTED_PROPERTY=(64,1,"se1_gamerules_data")

def require(valid,message):
    if not valid:raise ValueError(message)

def verify_catalog(path:Path):
    callbacks=public_header(path)
    require(len(callbacks)==1110,
            "original static RecvProp source cardinality changed")
    require(callbacks.count(VA)==1,
            "original GameRules callback no longer has one source route")
    require(LENGTH==13 and NEXT_FUNC-VA-LENGTH==3,
            "native original GameRules function/padding length changed")

def verify_original(path:Path):
    data=path.read_bytes()
    require(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
            "user original 2006 client DLL SHA256 does not match")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def at(va,count):
        off=va-base
        require(count>=0 and 0<=off<=len(code)-count,
                "original PE32 code offset outside .text")
        return code[off:off+count]
    full=at(VA,LENGTH)
    require(hashlib.sha256(full).hexdigest()==ORIGINAL_SHA,
            "complete 13B original GameRules receive callback SHA mismatch")
    require(full[-1]==0xC3 and at(VA+LENGTH,3)==b"\xCC"*3,
            "original method RET or INT3 padding boundary changed")
    # Three sparse 2006 original MOV instructions:
    # arg +8 is pointer-to-DWORD output, global 0x2433F3F8 holds
    # pointer to original GameRules singleton, store output DWORD.
    require(at(VA,4)==bytes.fromhex("8b442408"),
            "original native output argument load changed")
    require(at(VA+4,6)==bytes.fromhex("8b0df8f33324"),
            "original native GameRules global load changed")
    require(at(VA+10,3)==bytes.fromhex("8908c3"),
            "original native GameRules pointer output changed")
    require(at(SOURCE_CONSTRUCTOR_NAME_PUSH-14,5)==
            b"\x68"+struct.pack("<I",VA) and
            at(SOURCE_CONSTRUCTOR_NAME_PUSH,1)==b"\x68",
            "source constructor callback PUSH immediate drift")
    print("PASS Phase1714 owner original 13-byte GameRules global-to-output "
          "pointer RecvProxy, SHA, source constructor and three MOV opcodes")

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
        require(hashlib.sha256(b"\x90"*LENGTH).hexdigest()!=ORIGINAL_SHA,
                "fabricated GameRules PE32 body unexpectedly authentic")
        print("PASS Phase1714 original-free source single-GameRules "
              "callback and original SHA mutation refusal")
    if args.client is not None:
        verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1714: {exc}",file=sys.stderr)
        sys.exit(1)
