#!/usr/bin/env python3
"""Phase1695 original SiN 2006 ClientLeafSystem001 factory/ABI boundary.

Checks original client+engine PE32 images by BOTH exact whole-file SHA256s.
Only source metadata, hashes and independently authored tests are committed.
Public --self-test uses invented pointer tables, never commercial game bytes.
No renderer, leaf operations, model/prop loading or gameplay is implemented.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_client_engine_entitylist_factory_boundary import (
    verify_original as verify_previous_factory_order,
    ENGINE_SHA, ENGINE_FACTORY_GLOBAL,
)
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

NAME = "ClientLeafSystem001"
ORIGINAL_FACTORY_VA = 0x24108B20
ORIGINAL_SINGLETON_VA = 0x24357F30
ORIGINAL_FACTORY_SHA = "9328e36548ae5755e8637e94a5a3712571f822f49b9ea290e973bef7e9ea6de6"
PRIMARY_VTABLE = 0x2429756C
SECONDARY_VTABLE = 0x24297534
AUXILIARY_VTABLE = 0x2429752C
PRIMARY_SHA = "16fe7aeba8e5d611a640ba3d4bd3419d5a05c5b9c69d19df68244f5cf3417b8b"
SECONDARY_SHA = "f2bd0c8c39b9831d8e2bbc2a817aa48b026945857cc66427a7cade33241ba0fe"
AUXILIARY_SHA = "89db44486f34dddb679b8fff0c6fd8062111505a1c5b86a9b51ed1159ef6b179"
TYPE_DESCRIPTOR = 0x242FA830
PRIMARY_SLOTS = (
  0x2410D8E0, 0x2410D9A0, 0x2410D590, 0x2410A1C0,
  0x2410D940, 0x2410BA60, 0x2410AD20, 0x2410AD50,
  0x2410AD80, 0x2410ADB0, 0x2410BC50, 0x2410BD10,
  0x2410BDA0, 0x2410BE60, 0x2410CC80, 0x24108B90,
  0x24108BA0, 0x2410DA80, 0x2410DAD0, 0x2410D370,
  0x2410D460, 0x2410BB00,
)
SECONDARY_SLOTS = (
  0x2410CA00, 0x2410CA10, 0x2410B870, 0x2410CA30,
  0x24108BB0, 0x2410CB40, 0x2410CA40, 0x2410CA50,
  0x2410CA60, 0x2410D750, 0x2410CA20, 0x2410CA70,
)
AUXILIARY_SLOTS = (0x2410D610,)
SOURCE_VTABLES = (
  (PRIMARY_VTABLE, PRIMARY_SLOTS, PRIMARY_SHA, 0),
  (SECONDARY_VTABLE, SECONDARY_SLOTS, SECONDARY_SHA, 4),
  (AUXILIARY_VTABLE, AUXILIARY_SLOTS, AUXILIARY_SHA, 8),
)
CONSTRUCTOR_STORES = (
  (0x2410C7A0, b"\xC7\x00", PRIMARY_VTABLE, 0),
  (0x2410C7A6, b"\xC7\x40\x04", SECONDARY_VTABLE, 4),
  (0x2410C7AD, b"\xC7\x40\x08", AUXILIARY_VTABLE, 8),
)
ENGINE_LEAF_GLOBAL = 0x203E4020
ENGINE_LEAF_FACTORY = 0x20023D70
ENGINE_LEAF_NAME = 0x202A0CCC
# Engine.dll calls the original client leaf object only in later paths.
# These are source ABI use sites, NOT safe-to-execute reconstructed methods.
ENGINE_LEAF_USERS = (
    (0x2014007A, 0x20140083, 4, 1),
    (0x20141071, 0x2014107C, 0, 2),
    (0x20141634, 0x2014163E, 12, 2),
    (0x20141659, 0x20141669, 12, 2),
    (0x2014166F, 0x2014167A, 12, 2),
)
# All five native callsites return to various engine functions. They do
# not prove the engine invokes any leaf method during initial factory query.


def check(condition: bool, why: str) -> None:
    if not condition:
        raise VerificationError("Phase1695 leaf factory ABI: " + why)


def u32(value: int) -> bytes:
    return struct.pack("<I", value)


def verify_slots(raw: bytes, expected: tuple[int, ...], label: str) -> None:
    check(len(raw) == len(expected)*4, label + " table extent mismatch")
    check(tuple(struct.unpack("<" + "I"*len(expected), raw)) == expected,
          label + " pointer order mismatch")


def verify_original(client_bytes: bytes, engine_bytes: bytes) -> dict:
    check(hashlib.sha256(client_bytes).hexdigest()==ORIGINALS["client"]["sha256"],
          "client original SHA mismatch")
    check(hashlib.sha256(engine_bytes).hexdigest()==ENGINE_SHA,
          "engine original SHA mismatch")
    # Reuse the previously authenticated request order: entitylist,
    # centerprint, then leaf. Do not silently repeat this as NEW code.
    verify_previous_factory_order(client_bytes, engine_bytes)
    client=PEImage(client_bytes)
    engine=PEImage(engine_bytes)
    reloc=pe_highlow_relocations(client)
    engine_reloc=pe_highlow_relocations(engine)
    factory=client.at(ORIGINAL_FACTORY_VA,6)
    check(hashlib.sha256(factory).hexdigest()==ORIGINAL_FACTORY_SHA and
          factory==b"\xB8"+u32(ORIGINAL_SINGLETON_VA)+b"\xC3",
          "original factory not an exact singleton thunk")
    check(ORIGINAL_FACTORY_VA+1 in reloc,"original singleton relocation missing")
    check(client.at(TYPE_DESCRIPTOR+8, len(".?AVCClientLeafSystem@@")+1)==
          b".?AVCClientLeafSystem@@\x00", "original C++ RTTI name mismatch")
    for address,slots,digest,delta in SOURCE_VTABLES:
        check(client.section_of(address)==".rdata","vtable not .rdata")
        raw=client.at(address,len(slots)*4)
        verify_slots(raw,slots,hex(address))
        check(hashlib.sha256(raw).hexdigest()==digest,
              "entire original vtable digest mismatch "+hex(address))
        for index,pointer in enumerate(slots):
            check(client.section_of(pointer)==".text",
                  "original virtual entry not .text")
            check(address+index*4 in reloc,
                  "missing source vtable pointer relocation")
        col_va=struct.unpack("<I",client.at(address-4,4))[0]
        col=struct.unpack("<IIIII",client.at(col_va,20))
        check(col[:3]==(0,delta,0) and col[3]==TYPE_DESCRIPTOR,
              "source secondary-base RTTI displacement changed")
        check(address-4 in reloc and col_va+12 in reloc and col_va+16 in reloc,
              "original RTTI relocation witness missing")
    for site,opcode,vptr_offset in (
        (0x2410C7A0,b"\xC7\x00",PRIMARY_VTABLE),
        (0x2410C7A6,b"\xC7\x40\x04",SECONDARY_VTABLE),
        (0x2410C7AD,b"\xC7\x40\x08",AUXILIARY_VTABLE),
    ):
        check(client.at(site,len(opcode)+4)==opcode+u32(vptr_offset),
              "original +0/+4/+8 constructor vtable store changed")
        check(site+len(opcode) in reloc,
              "original constructor table relocation missing")
    check(engine.at(ENGINE_LEAF_NAME,len(NAME)+1)==NAME.encode()+b"\x00",
          "original engine requested interface literal changed")
    check(engine.at(ENGINE_LEAF_FACTORY,3)==b"\x6A\x00\x68" and
          engine.at(ENGINE_LEAF_FACTORY+3,4)==u32(ENGINE_LEAF_NAME) and
          engine.at(ENGINE_LEAF_FACTORY+7,2)==b"\xFF\x15" and
          engine.at(ENGINE_LEAF_FACTORY+9,4)==u32(ENGINE_FACTORY_GLOBAL),
          "engine leaf CreateInterface request changed")
    check(engine.at(ENGINE_LEAF_FACTORY+13,10)==
          b"\x83\xC4\x08\x85\xC0\xA3"+u32(ENGINE_LEAF_GLOBAL),
          "engine original factory result pointer storage changed")
    for src,call,offset,argc in ENGINE_LEAF_USERS:
        check(engine.at(src,6)==b"\x8B\x0D"+u32(ENGINE_LEAF_GLOBAL),
              "engine leaf consumer global changed "+hex(src))
        check(engine.at(call, 2 if offset==0 else 3)==(
              b"\xFF\x12" if offset==0 else b"\xFF\x52"+bytes((offset,))),
              "engine original leaf caller virtual slot changed "+hex(call))
        check(src+2 in engine_reloc,"engine leaf global read relocation missing")
    return {
        "status":"RETAIL_ENGINE_CLIENTLEAF001_FACTORY_AND_NATIVE_ABI_AUTHENTICATED",
        "original_engine_sha256":ENGINE_SHA,
        "original_client_sha256":ORIGINALS["client"]["sha256"],
        "source_factory_va":hex(ORIGINAL_FACTORY_VA),
        "source_singleton_va":hex(ORIGINAL_SINGLETON_VA),
        "source_primary_vtable_count":len(PRIMARY_SLOTS),
        "source_subobject_offsets":[0,4,8],
        "source_secondary_auto_gamesystem_slots":len(SECONDARY_SLOTS),
        "source_auxiliary_slots":len(AUXILIARY_SLOTS),
        "post_prediction_leaf_request":hex(ENGINE_LEAF_FACTORY),
        "later_engine_leaf_virtual_calls_verified":len(ENGINE_LEAF_USERS),
        "limit":"Nonrendering factory/ABI only. Original leaf rendering methods not reconstructed or invoked; no playable boot."
    }


def self_test() -> None:
    synthetic=b"".join(u32(x) for x in PRIMARY_SLOTS)
    verify_slots(synthetic,PRIMARY_SLOTS,"invented source-pointer vector")
    for i in range(len(synthetic)):
        mutated=bytearray(synthetic);mutated[i]^=1
        try:verify_slots(bytes(mutated),PRIMARY_SLOTS,"mutated")
        except VerificationError:pass
        else:raise AssertionError("altered primary vtable pointer accepted")
    for truncated in (synthetic[:4],synthetic[:-4]):
        try:verify_slots(truncated,PRIMARY_SLOTS,"shortened")
        except VerificationError:pass
        else:raise AssertionError("truncated source pointer vector accepted")
    try:verify_original(b"fake client",b"fake engine")
    except VerificationError:pass
    else:raise AssertionError("unauthenticated original binaries accepted")
    print("PASS original leaf ABI: 88 primary-slot synthetic byte mutations, "
          "short table and wrong original SHA refused")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--bin-zip",type=Path)
    parser.add_argument("--engine",type=Path)
    args=parser.parse_args()
    if not args.self_test and args.bin_zip is None:
        parser.error("supply --self-test or --bin-zip with --engine")
    if args.self_test:self_test()
    if args.bin_zip is not None:
        if args.engine is None:parser.error("--bin-zip requires --engine")
        with zipfile.ZipFile(args.bin_zip) as archive:
            client=archive.read("client.dll")
        print(json.dumps(verify_original(client,args.engine.read_bytes()),indent=2,sort_keys=True))


if __name__=="__main__":
    main()
