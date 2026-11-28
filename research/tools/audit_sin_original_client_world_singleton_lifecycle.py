#!/usr/bin/env python3
"""Phase1725: source-authenticate original CWorld singleton native lifetime.

Original CWorld ClientClass creator at 0x240FC920 does not allocate;
a distinct original startup callback at 0x240FE590 invokes the
complete 74-byte source initializer 0x240FCDD0 (zero-allocate 1200
bytes, constructor, four RTTI vtables and global pointer store).
Original client teardown at 0x240FF3D4 calls the complete 27-byte
source 0x240FC900 virtual-destructor/global-clear. Getter at
0x240FCD40 reads the original singleton source process global.

No original executable bytes or heap pointers are committed. This
is a source ABI/lifetime audit, not a 2006 engine implementation.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_seven_creator_exceptions import (
    verify_public as verify_seven_original_source,
)

SOURCE_GLOBAL=0x2433F3F4
SOURCE_CREATOR_VA=0x240FC920
SOURCE_STARTUP_CALL=0x240FE590
SOURCE_STARTUP_VA=0x240FCDD0
SOURCE_SHUTDOWN_CALL=0x240FF3D4
SOURCE_SHUTDOWN_VA=0x240FC900
SOURCE_GETTER_VA=0x240FCD40
SOURCE_SIZE=1200
SOURCE_ALLOCATOR_VA=0x240C4FF0
SOURCE_CONSTRUCTOR_VA=0x240CCE30
SOURCE_CREATOR_LENGTH=40
SOURCE_CREATOR_SHA="8b06decbe2712135eef0780b69446b5147fbf52c618fa78e7c46aa3fcffde274"
SOURCE_STARTUP_LENGTH=74
SOURCE_STARTUP_SHA="8a9a82e9e7a8bbd9eb61c80802270817f00c8053bff3b21ba941e9c46311d289"
SOURCE_SHUTDOWN_LENGTH=27
SOURCE_SHUTDOWN_SHA="71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88"
SOURCE_GETTER_LENGTH=6
SOURCE_GETTER_SHA="b07f64b7fbb5a63cfd4816fcc0481ce9f61be29ae5eae77d80f758bd319a0d06"
SOURCE_NATIVE_VTABLES=(0x2429632C,0x242962C4,0x2429628C,0x24296274)
SOURCE_WORLD_VIRTUAL94_WRAPPER_VA=0x240FCB10
SOURCE_WORLD_VIRTUAL94_WRAPPER_BYTES=44
SOURCE_WORLD_VIRTUAL94_WRAPPER_SHA="305044e8173fc81a50952318532ec1a5638e4c956dc056285e048ef92448b35d"
SOURCE_WORLD_LOCAL_CLEAR_OFFSET=0x480
SOURCE_WORLD_PREINIT_FIRST_VA=0x24085750
SOURCE_WORLD_PREINIT_SECOND_VA=0x24129480
SOURCE_COMMON_NETWORK_INIT_VA=0x240C8FE0
SOURCE_INIT_VIRTUAL94=0x240FCB10
SOURCE_INIT_VIRTUAL44=0x24054E80
SOURCE_SECONDARY_VIRTUAL0=0x24054EC0

def need(yes:bool,message:str)->None:
    if not yes:raise ValueError(message)


def verify_public(special_manifest:Path,special_header:Path,
                  creator_catalog:Path,common_manifest:Path,
                  common_header:Path):
    data=verify_seven_original_source(
        special_manifest,special_header,creator_catalog,
        common_manifest,common_header)
    world=[r for r in data if int(r["class_ordinal"])==75]
    need(len(world)==1,"source CWorld exceptional creator absent")
    orig=world[0]
    need(orig["network_class"]=="CWorld" and
         int(orig["creator_va"],16)==SOURCE_CREATOR_VA and
         orig["path_kind"]=="global_singleton" and
         int(orig["original_allocator_or_global_va"],16)==SOURCE_GLOBAL and
         int(orig["allocation_request_bytes"])==0 and
         int(orig["returned_subobject_offset"])==8 and
         int(orig["original_virtual_call_offset"])==0x94 and
         int(orig["complete_native_function_bytes"])==SOURCE_CREATOR_LENGTH and
         orig["complete_native_function_sha256"]==SOURCE_CREATOR_SHA,
         "source registered CWorld factory no longer uses the global")
    return orig


def verify_original(path:Path):
    original=path.read_bytes()
    need(hashlib.sha256(original).hexdigest()==ORIGINAL_CLIENT_SHA,
         "source user-original retail 2006 PE32 client.dll whole SHA mismatch")
    pe=PE(original)
    base,code=pe.sections[".text"]
    database,rd=pe.sections[".rdata"]
    def read(va,n):
        off=va-base
        need(n>=0 and 0<=off<=len(code)-n,
             f"original CWorld native code address out of .text: {va:#x}")
        return code[off:off+n]
    def method(va,n,sha,pad,ret8=False):
        block=read(va,n)
        # World virtual +0x94 is a thiscall-style RET 8; the
        # original bootstrap/getter/shutdown functions use RET.
        correct_return=(block[-3:]==b"\xC2\x08\x00") if ret8 else (
            block[-1:]==b"\xC3")
        need(hashlib.sha256(block).hexdigest()==sha and
             correct_return and read(va+n,pad)==b"\xCC"*pad,
             f"complete source original CWorld method/padding drift {va:#x}")
        need(read(va+n+pad,1)!=b"\xCC",
             "original CWorld native method boundary too short")
        return block
    creator=method(SOURCE_CREATOR_VA,SOURCE_CREATOR_LENGTH,
                   SOURCE_CREATOR_SHA,8)
    startup=method(SOURCE_STARTUP_VA,SOURCE_STARTUP_LENGTH,
                   SOURCE_STARTUP_SHA,6)
    shutdown=method(SOURCE_SHUTDOWN_VA,SOURCE_SHUTDOWN_LENGTH,
                    SOURCE_SHUTDOWN_SHA,5)
    getter=method(SOURCE_GETTER_VA,SOURCE_GETTER_LENGTH,
                  SOURCE_GETTER_SHA,10)
    wrapper=method(SOURCE_WORLD_VIRTUAL94_WRAPPER_VA,
                   SOURCE_WORLD_VIRTUAL94_WRAPPER_BYTES,
                   SOURCE_WORLD_VIRTUAL94_WRAPPER_SHA,4,ret8=True)
    need(wrapper[3:13]==bytes.fromhex("c7868004000000000000") and
         wrapper[-3:]==b"\xC2\x08\x00",
         "original world-specific virtual94 wrapper lost +0x480 clear or RET8")
    for off,target in ((13,SOURCE_WORLD_PREINIT_FIRST_VA),
                       (18,SOURCE_WORLD_PREINIT_SECOND_VA),
                       (35,SOURCE_COMMON_NETWORK_INIT_VA)):
        originalcall=wrapper[off:off+5]
        need(originalcall[0]==0xE8 and
             SOURCE_WORLD_VIRTUAL94_WRAPPER_VA+off+5+
             struct.unpack_from("<i",originalcall,1)[0]==target,
             "world-only +0x94 virtual wrapper dependency E8 target changed")
    need(getter==b"\xA1"+struct.pack("<I",SOURCE_GLOBAL)+b"\xC3",
         "original CWorld getter not exactly MOV EAX,global;RET")
    need(creator[:10]==bytes.fromhex("8b5424088b0df4f33324") and
         creator[18:24]==bytes.fromhex("ff9094000000"),
         "CWorld ClientClass original creator loses global/dynamic virtual+0x94")
    need(startup[:2]==b"\x56\x68" and
         struct.unpack_from("<I",startup,2)[0]==SOURCE_SIZE and
         startup[6]==0xE8 and startup[22]==0xE8,
         "CWorld original init does not allocate 0x4B0 and call constructor")
    for off,target in ((6,SOURCE_ALLOCATOR_VA),(22,SOURCE_CONSTRUCTOR_VA)):
        rel,=struct.unpack_from("<i",startup,off+1)
        need(SOURCE_STARTUP_VA+off+5+rel==target,
             "CWorld original bootstrap allocator/constructor target drift")
    for off,vt in zip((29,36,43,50),SOURCE_NATIVE_VTABLES):
        # Four original RTTI pointer immediates after constructors.
        native,=struct.unpack_from("<I",startup,off)
        need(native==vt,
             "CWorld original four vtable addresses changed")
    need(b"\x89\x35"+struct.pack("<I",SOURCE_GLOBAL) in startup and
         b"\xC7\x05"+struct.pack("<I",SOURCE_GLOBAL)+b"\x00"*4 in startup,
         "CWorld original global success/failure assignments changed")
    need(shutdown[:6]==b"\x8B\x0D"+struct.pack("<I",SOURCE_GLOBAL) and
         b"\x6A\x01\xFF\x10" in shutdown and
         b"\xC7\x05"+struct.pack("<I",SOURCE_GLOBAL)+b"\x00"*4 in shutdown,
         "CWorld original virtual-destructor/null-global lifecycle drift")
    for caller,callee in ((SOURCE_STARTUP_CALL,SOURCE_STARTUP_VA),
                          (SOURCE_SHUTDOWN_CALL,SOURCE_SHUTDOWN_VA)):
        originalcall=read(caller,5)
        need(originalcall[0]==0xE8 and caller+5+
             struct.unpack_from("<i",originalcall,1)[0]==callee,
             "original real client init/shutdown no longer CALL source world method")
    primary,secondary=SOURCE_NATIVE_VTABLES[:2]
    def vtable_at(va,off):
        offset=va+off-database
        need(0<=offset<=len(rd)-4,
             "source native CWorld vtable pointer outside original .rdata")
        return struct.unpack_from("<I",rd,offset)[0]
    need(vtable_at(primary,0x94)==SOURCE_INIT_VIRTUAL94 and
         vtable_at(primary,0x44)==SOURCE_INIT_VIRTUAL44 and
         vtable_at(secondary,0)==SOURCE_SECONDARY_VIRTUAL0,
         "CWorld primary/secondary original virtual target drift")
    print("PASS Phase1725 owner-original CWorld 74B/27B/6B/40B/44B "
          "startup/shutdown/getter/creator methods, original 1200B "
          "zeroing allocation, RTTI vtables and client lifecycle callers")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--special-manifest",type=Path,default=Path(
        "research/manifests/original_client_seven_creator_exceptions_phase1721.csv"))
    parser.add_argument("--special-header",type=Path,default=Path(
        "include/sin_compat/original_client_seven_creator_exceptions.hpp"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--common-manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--common-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    verify_public(args.special_manifest,args.special_header,
                  args.creator_catalog,args.common_manifest,args.common_header)
    if args.self_test:
        for length,sha in ((74,SOURCE_STARTUP_SHA),(27,SOURCE_SHUTDOWN_SHA),
                           (6,SOURCE_GETTER_SHA),(40,SOURCE_CREATOR_SHA),
                           (44,SOURCE_WORLD_VIRTUAL94_WRAPPER_SHA)):
            need(hashlib.sha256(b"\x90"*length).hexdigest()!=sha,
                 "fabricated source CWorld native method falsely authenticated")
        print("PASS Phase1725 original-free source CWorld singleton global "
              "creator identity, 74B startup/27B shutdown/6B getter SHA refusal")
    if args.client is not None:verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1725: {exc}",file=sys.stderr)
        sys.exit(1)
