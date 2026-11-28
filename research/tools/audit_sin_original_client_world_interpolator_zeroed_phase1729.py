#!/usr/bin/env python3
"""Phase1729: source-verify zeroed CWorld embedded variable's no-op virtual.

Checks unchanged owner 2006 PE32 client.dll, not a reconstruction that can
execute original code. No original executable bytes are committed.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, executable_text, need, read, sha_ok,
)
from audit_sin_original_client_world_ctor_prefix_phase1728 import (
    EMBEDDED_CTOR_VA, EMBEDDED_CTOR_LEN, EMBEDDED_CTOR_SHA,
    EMBEDDED_HELPER_VA, EMBEDDED_HELPER_LEN, EMBEDDED_HELPER_SHA,
)
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8

EMBEDDED_VTABLE=0x2429806C
LATER_CWORLD_VTABLE=0x24288E3C
VIRTUAL_BYTE_OFFSET=0x48
EMBEDDED_VIRTUAL_TARGET=0x24119750
LATER_CWORLD_VIRTUAL_TARGET=0x240C6F20
RET_SHA="ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e"
ZERO_FLOAT_CONSTANT_VA=0x242DE2F8
SOURCE_BSS_GLOBAL_VEC_VA=0x24372818
SOURCE_WORLD_SUBOBJECT_OFFSET=0x16C


def pe_section(original:bytes,name:str)->tuple[int,bytes,int,int]:
    p=struct.unpack_from("<I",original,0x3c)[0]
    need(original[p:p+4]==b"PE\0\0","missing original PE signature")
    count=struct.unpack_from("<H",original,p+6)[0]
    optsize=struct.unpack_from("<H",original,p+20)[0]
    imagebase=struct.unpack_from("<I",original,p+24+28)[0]
    for i in range(count):
        entry=original[p+24+optsize+i*40:p+24+optsize+(i+1)*40]
        if entry[:8].rstrip(b"\0")!=name.encode():continue
        virtual, rva, rawsize, raw=struct.unpack_from("<IIII",entry,8)
        need(raw+rawsize<=len(original) and rawsize<=virtual+0x1000,
             f"invalid {name} raw bounds")
        return imagebase+rva, original[raw:raw+rawsize], virtual,rawsize
    raise ValueError(f"missing original {name} section")


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         "unchanged owner original PE32 client.dll whole SHA mismatch")
    base,code=executable_text(original)
    rd_base,rd,_,_=pe_section(original,".rdata")
    data_base,_,data_virtual,data_raw=pe_section(original,".data")
    need(data_base+data_raw <= SOURCE_BSS_GLOBAL_VEC_VA and
         SOURCE_BSS_GLOBAL_VEC_VA+12 <= data_base+data_virtual,
         "source origin vector is no longer three DWORDs in original .data BSS")
    def rdata32(va:int)->int:
        idx=va-rd_base
        need(0<=idx<=len(rd)-4,f"not source .rdata: {va:#x}")
        return struct.unpack_from("<I",rd,idx)[0]
    need(rdata32(EMBEDDED_VTABLE+VIRTUAL_BYTE_OFFSET)==EMBEDDED_VIRTUAL_TARGET,
         "26B subobject ctor's +0x48 vtable entry no longer points at 1B RET")
    need(rdata32(LATER_CWORLD_VTABLE+VIRTUAL_BYTE_OFFSET)==LATER_CWORLD_VIRTUAL_TARGET,
         "CWorld followup subobject vtable +0x48 no longer has no-op")
    for target in (EMBEDDED_VIRTUAL_TARGET,LATER_CWORLD_VIRTUAL_TARGET):
        method=read(base,code,target,1)
        need(sha_ok(method,1,RET_SHA) and method==b"\xc3" and
             read(base,code,target+1,15)==b"\xcc"*15,
             f"original virtual method is not exactly one-byte RET: {target:#x}")
    need(rdata32(ZERO_FLOAT_CONSTANT_VA)==0,
         "zeroed path's original x87 comparison uses nonzero constant")
    ctor=read(base,code,EMBEDDED_CTOR_VA,EMBEDDED_CTOR_LEN)
    nested=read(base,code,EMBEDDED_HELPER_VA,EMBEDDED_HELPER_LEN)
    need(sha_ok(ctor,EMBEDDED_CTOR_LEN,EMBEDDED_CTOR_SHA) and
         sha_ok(nested,EMBEDDED_HELPER_LEN,EMBEDDED_HELPER_SHA),
         "original complete 26B or 210B embedded constructor SHA drift")
    need(ctor[5:7]==b"\xc7\x06" and
         struct.unpack_from("<I",ctor,7)[0]==EMBEDDED_VTABLE and
         ctor[11:17]==bytes.fromhex("66c74626ffff") and
         ctor[3:5]==b"\x6a\x00",
         "source embedded subobject must install first vtable and 0xFFFF flag")
    expected_e8(base,code,EMBEDDED_CTOR_VA+17,EMBEDDED_HELPER_VA)
    need(nested[0:4]==bytes.fromhex("8b442404") and
         nested[8:13]==bytes.fromhex("8946048b11") and
         nested[-3:]==b"\xc2\x04\x00" and
         b"\xe8" not in nested,
         "source nested 210B helper must take zero arg and have no direct calls")
    # The source does 8 vtable+0x48 calls, which all resolve to a 1B RET
    # during this constructor, not a general engine callback.
    virtual_offsets=(13,31,68,84,100,115,177,193)
    for offset in virtual_offsets:
        need(nested[offset:offset+3] in (b"\xff\x52\x48",b"\xff\x50\x48"),
             f"unexpected embedded method virtual target at helper +{offset:#x}")
    # The original copies runtime-supplied BSS triple twice, at subobj
    # +0x4C..+0x54 and +0x58..+0x60; the six reads/writes are SHA-bound.
    need(nested.count(struct.pack("<I",SOURCE_BSS_GLOBAL_VEC_VA))==2 and
         nested.count(struct.pack("<I",SOURCE_BSS_GLOBAL_VEC_VA+4))==2 and
         nested.count(struct.pack("<I",SOURCE_BSS_GLOBAL_VEC_VA+8))==2,
         "original nested helper no longer duplicates global Vec3 bits twice")
    need(SOURCE_WORLD_SUBOBJECT_OFFSET+0x64<=1200,
         "source embedded object exceeds zeroed original owner")
    print("PASS Phase1729 original 26B+210B nested ctor SHA, eight +0x48 "
          "virtual calls resolved to source 1B RET in first vtable, "
          "later CWorld vtable also no-op, original zero x87 constant, "
          "BSS Vec3 duplicate copies, private 2006 PE32 SHA")


def self_test()->None:
    need(EMBEDDED_VTABLE+0x48==0x242980B4,
         "original initial embedded vtable slot arithmetic drift")
    need(EMBEDDED_CTOR_LEN==26 and EMBEDDED_HELPER_LEN==210,
         "native source helper sizes drift")
    need(sha_ok(b"\xc3",1,RET_SHA) and not sha_ok(b"\x90",1,RET_SHA),
         "source RET fingerprint or mutation refusal invalid")
    print("PASS Phase1729 original-free embedded virtual no-op/zeroed path metadata")


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,
                   help="private unchanged source 2006 PE32 client.dll")
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1729: {exc}",file=sys.stderr)
        sys.exit(1)
