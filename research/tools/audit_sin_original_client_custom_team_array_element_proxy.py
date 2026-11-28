#!/usr/bin/env python3
"""Phase1713: verify original 2006 PE32 Team player_array_element callback.

Owner-original --client mode is SHA-pinned and read-only. Public CI
uses the immutable original-source callback catalog without the
proprietary executable. No original DLL bytes are exported.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

VA=0x240F7240
LENGTH=24
SOURCE_SHA="491da928d45037adb4f7934e27130a1e5924a1013c447c2a12b30b22ad8ea859"
NEXT_FUNC=0x240F7260
SOURCE_CTOR=0x240F776A
SLOTS=32
TYPE4_SOURCE_OUTER="player_array"
# Sparse opcode witnesses ONLY. The whole 24-byte original function
# is validated against SHA256, not copied into the repo.
WITNESSES=(
    (0x240F7244,"8b4814"),       # [CRecvProxyData+0x14] original m_iElement
    (0x240F724B,"8b9280040000"), # [Team owner+0x480] native array ptr
    (0x240F7251,"8b4004"),       # [CRecvProxyData+0x04] decoded DWORD
    (0x240F7254,"89048a"),       # [arrayptr+element*4] = decoded value
)

def need(valid:bool,message:str)->None:
    if not valid:raise ValueError(message)


def verify_catalog(path:Path)->None:
    callback_list=public_header(path)
    need(len(callback_list)==1110,
         "source original static property callback count changed")
    need(callback_list.count(VA)==1,
         "original Team player_array_element callback count drift")


def verify_original(path:Path)->None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original 2006 SiN client.dll whole hash changed")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def at(va:int,n:int)->bytes:
        loc=va-base
        need(n>=0 and 0<=loc<=len(code)-n,
             "source x86 address outside exact .text range")
        return code[loc:loc+n]
    body=at(VA,LENGTH)
    need(hashlib.sha256(body).hexdigest()==SOURCE_SHA,
         "full original 24-byte Team native RecvProxy SHA changed")
    need(body[-1:]==b"\xC3" and
         at(VA+LENGTH,NEXT_FUNC-VA-LENGTH)==
         b"\xCC"*(NEXT_FUNC-VA-LENGTH),
         "original Team element method boundary/INT3 padding changed")
    need(at(SOURCE_CTOR-11,5)==b"\x68"+struct.pack("<I",VA) and
         at(SOURCE_CTOR,1)==b"\x68",
         "original Team property x86 constructor PUSH callback changed")
    for addr,opcode in WITNESSES:
        raw=bytes.fromhex(opcode)
        need(at(addr,len(raw))==raw,
             f"original array element indexing/owner store drift {addr:#x}")
    print("PASS Phase1713 owner-original 24-byte Team element callback, "
          "native +20 index, owner +0x480 array pointer, 32-element model")


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
        need(hashlib.sha256(b"\x90"*LENGTH).hexdigest()!=SOURCE_SHA,
             "fabricated Team element method unexpectedly authenticated")
        need(NEXT_FUNC-VA-LENGTH==8 and SLOTS==32,
             "original Team source method or element count drift")
        print("PASS Phase1713 original-free Team element source identity "
              "and full function SHA refusal")
    if args.client is not None:
        verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1713: {exc}",file=sys.stderr)
        sys.exit(1)
