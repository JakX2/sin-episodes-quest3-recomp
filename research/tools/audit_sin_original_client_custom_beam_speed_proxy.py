#!/usr/bin/env python3
"""Phase1715: authenticate the original 2006 PE32 DT_Beam.m_fSpeed callback.

The private --client path reads the unchanged owner retail client.dll and
pins its whole original callback SHA, original binary64 scale constant,
opcode witnesses, exact x86 factory argument and function boundary.
Public --self-test checks source catalog classification and fake SHA
refusal without embedding executable game bytes or needing a DLL.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

VA=0x2409ED70
NATIVE_LENGTH=44
SOURCE_SHA="77c9e9200d8c3d401d77f13e979c06d0ce35e4b446a65849a92de10f5e01d8d3"
NATIVE_CONSTANT_VA=0x2427B050
NATIVE_BINARY64_TENTH=0x3FB999999999999A
SOURCE_CALLBACK_PUSH=0x2409EFDF
SOURCE_NAME_PUSH=0x2409EFED
SOURCE_DESC_PUSH=0x2409EFF2
SOURCE_HELPER_CALL=0x2409EFF7
SOURCE_HELPER_TARGET=0x24126D80
SOURCE_OWNER_OFFSET=0x508
SOURCE_ROUTE=(128,14,"m_fSpeed")
SOURCE_NEXT_METHOD=0x2409EDA0

# Sparse original evidence; original executable method is validated as
# ONE complete 44-byte SHA, not copied into public research.
OPCODES=(
    (0x2409ED74,"d94004"),          # FLD dword [CRecvProxyData+4]
    (0x2409ED7B,"dc0d50b02724"),    # FMUL qword [0x2427B050]
    (0x2409ED83,"d98108050000"),    # FLD dword [owner+0x508]
    (0x2409ED89,"dae9"),            # FUCOMPP
    (0x2409ED8B,"dfe0"),            # FNSTSW AX
    (0x2409ED8D,"f6c444"),          # TEST AH,0x44
    (0x2409ED90,"7b07"),            # JNP on ordered equality
    (0x2409ED92,"d99908050000"),    # FSTP dword [owner+0x508]
    (0x2409ED98,"c3"),              # non-equal return
    (0x2409ED99,"ddd8c3"),          # equal -> FSTP ST(0); RET
)

def need(valid:bool,message:str)->None:
    if not valid:raise ValueError(message)


def verify_catalog(path:Path)->None:
    source=public_header(path)
    need(len(source)==1110,"original 1110 source callback map drift")
    need(source.count(VA)==1,"original Beam speed has wrong source route count")
    need(NATIVE_LENGTH==44 and SOURCE_NEXT_METHOD-VA-NATIVE_LENGTH==4,
         "source executable length/boundary arithmetic changed")
    need(NATIVE_BINARY64_TENTH==struct.unpack("<Q",struct.pack("<d",0.1))[0],
         "original binary64 factor is not the expected exact 2006 constant")


def verify_original(path:Path)->None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "original 2006 client.dll complete SHA256 mismatch")
    pe=PE(data)
    textbase,textsection=pe.sections[".text"]
    def code(va:int,size:int)->bytes:
        off=va-textbase
        need(size>=0 and 0<=off<=len(textsection)-size,
             "native callback source VA outside original .text")
        return textsection[off:off+size]
    body=code(VA,NATIVE_LENGTH)
    need(hashlib.sha256(body).hexdigest()==SOURCE_SHA,
         "whole original Beam speed 44-byte function SHA mismatch")
    need(body[-1:]==b"\xC3" and
         code(VA+NATIVE_LENGTH,4)==b"\xCC"*4,
         "source Beam speed native return/INT3 boundary changed")
    rd_base,rd_raw=pe.sections[".rdata"]
    off=NATIVE_CONSTANT_VA-rd_base
    need(0<=off<=len(rd_raw)-8,"original x87 factor outside .rdata")
    native_factor,=struct.unpack_from("<Q",rd_raw,off)
    need(native_factor==NATIVE_BINARY64_TENTH,
         "original x87 FMUL factor is not SHA-pinned binary64 0.1")
    for addr,opcode in OPCODES:
        raw=bytes.fromhex(opcode)
        need(code(addr,len(raw))==raw,
             f"original Beam speed x87 compare/store drift at {addr:#x}")
    need(code(SOURCE_CALLBACK_PUSH,5)==
         b"\x68"+struct.pack("<I",VA),
         "source original callback immediate does not reference speed")
    need(code(SOURCE_NAME_PUSH,5)==
         b"\x68"+struct.pack("<I",0x2427B9F0) and
         pe.cstring(0x2427B9F0)==SOURCE_ROUTE[2],
         "original m_fSpeed name/constructor argument changed")
    need(code(0x2409EFE8,5)==
         b"\x68"+struct.pack("<I",SOURCE_OWNER_OFFSET),
         "original RecvProp member +44 offset differs from callback store")
    need(code(SOURCE_HELPER_CALL,1)==b"\xE8" and
         SOURCE_HELPER_CALL+5+
         struct.unpack("<i",code(SOURCE_HELPER_CALL+1,4))[0]
         ==SOURCE_HELPER_TARGET,
         "original 2006 type1 constructor helper call changed")
    print("PASS Phase1715 owner original Beam.m_fSpeed: whole 44B SHA, "
          "binary64 0.1 constant, original x87 comparisons, +0x508 "
          "store and exact constructor source callback")


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
        need(hashlib.sha256(b"\x90"*NATIVE_LENGTH).hexdigest()!=SOURCE_SHA,
             "invented 44-byte speed callback incorrectly authenticated")
        print("PASS Phase1715 original-free Beam callback identity, "
              "44-byte SHA refusal and binary64 tenth factor")
    if args.client is not None:verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError,struct.error) as error:
        print(f"FAIL Phase1715: {error}",file=sys.stderr)
        sys.exit(1)
