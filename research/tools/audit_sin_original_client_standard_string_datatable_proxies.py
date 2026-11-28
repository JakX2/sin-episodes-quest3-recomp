#!/usr/bin/env python3
"""Phase1707 authenticate original 2006 PE32 standard STRING/DATATABLE proxies.

Original DLL is read privately and whole SHA256 pinned. Public CI self-test
does not embed copyrighted executable bytes and cannot substitute for real
original native runtime or network packet decoding.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA

STR_VA=0x24126CA0
STR_LEN=59
STR_SHA="ce29a1a4701a85ae6174f1410b4fb592d58312adf6fc39a24b8ef508ebaf28d5"
TABLE_VA=0x24126CE0
TABLE_LEN=11
TABLE_SHA="016529308820e753867bd2c2dc5176a2ef2d947df9772b7b58425b0192893ec1"

def need(condition,message):
    if not condition:raise ValueError(message)

def verify_original(path: Path):
    original=path.read_bytes()
    need(hashlib.sha256(original).hexdigest()==ORIGINAL_CLIENT_SHA,
         "owner-original 2006 PE32 client.dll whole hash drift")
    pe=PE(original)
    base,code=pe.sections[".text"]
    def section(va,n):
        at=va-base
        need(at>=0 and at+n<=len(code),"original proxy VA outside source .text")
        return code[at:at+n]
    need(hashlib.sha256(section(STR_VA,STR_LEN)).hexdigest()==STR_SHA,
         "native 59-byte standard string receiver source changed")
    need(hashlib.sha256(section(TABLE_VA,TABLE_LEN)).hexdigest()==TABLE_SHA,
         "native 11-byte standard datatable receiver source changed")
    need(section(STR_VA+STR_LEN,5)==b"\xCC"*5,
         "original string receiver post-RET method boundary changed")
    need(section(TABLE_VA+TABLE_LEN,5)==b"\xCC"*5,
         "original datatable receiver post-RET method boundary changed")
    # String source reads decoded value pointer at incoming RecvProxyData+4,
    # buffer capacity from pData->RecvProp+12, and forcibly appends a
    # terminator at destination+capacity-1 without clearing other bytes.
    witnesses={
        0x24126CA0:bytes.fromhex("8b5424048b028b480c"),
        0x24126CB8:bytes.fromhex("8b4a048a0c08"),
        0x24126CC0:bytes.fromhex("880c30740a"),
        0x24126CC7:bytes.fromhex("83c001"),
        0x24126CD1:bytes.fromhex("8b420c"),
        0x24126CD4:bytes.fromhex("c64430ff00"),
        0x24126CE0:bytes.fromhex("8b44240c8b4c24088901c3")
    }
    for va,want in witnesses.items():
        need(section(va,len(want))==want,
             f"original string/datatable proxy x86 witness drift {va:#x}")
    print("PASS Phase1707 owner-original 59B STRING and 11B DATATABLE "
          "proxy native methods / copy, NUL bound, object-pointer assignment")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    if args.self_test:
        # A made-up incomplete original method cannot authenticate.
        need(hashlib.sha256(b"\x90"*STR_LEN).hexdigest()!=STR_SHA and
             hashlib.sha256(b"\x90"*TABLE_LEN).hexdigest()!=TABLE_SHA,
             "fabricated original executable SHA unexpectedly matched")
        need(STR_VA+STR_LEN==0x24126CDB and
             TABLE_VA+TABLE_LEN==0x24126CEB,
             "original method-end bookkeeping drift")
        print("PASS Phase1707 source-free original proxy SHA-length/boundary refusal")
    if args.client is not None:verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print("FAIL Phase1707:",exc,file=sys.stderr)
        sys.exit(1)
