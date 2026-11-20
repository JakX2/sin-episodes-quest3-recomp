#!/usr/bin/env python3
"""Audit derived SHA-pinned 16-type SiN 2006 client value-factory metadata.

Original-free --self-test for public CI; --client is OWNER-ONLY and must
never write original code, bytes, disassembly or private file paths.
No proprietary DLL bytes or extracted source code go into the repository.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SHA256 = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
BASE = 0x24000000
FACTORY = 0x24205BC0
JUMP_TABLE = 0x24205DF0
VTABLE_SLOT_VALUE = 0x4C
# type, original code case VA, constructor VA (zero=inline), virtual
# table VA, observed value-update function VA, reader method slot offset,
# original allocated byte size, original pushed 32-bit config literal.
CASES = (
    (0, 0x24205C00,0,         0x242AF384,0x24205420,0x24,0x10,0),
    (1, 0x24205BD9,0,         0x242AF3DC,0x242054C0,0x20,0x10,0),
    (2, 0x24205C47,0x242054E0,0x242AF434,0x24205580,0x14,0x14,0),
    (3, 0x24205C27,0x24205590,0x242AF48C,0x24205600,0x10,0x14,0),
    (4, 0x24205C67,0x24205620,0x242AF4E4,0x24205670,0x0C,0x14,0),
    (5, 0x24205C87,0x24205690,0x242AF53C,0x242056B0,0x0C,0x14,0),
    (6, 0x24205CA7,0x242056C0,0x242AF594,0x24205710,0x08,0x14,0),
    (7, 0x24205CC7,0x24205720,0x242AF5EC,0x24205780,0x1C,0x14,0),
    (8, 0x24205CE7,0x242057A0,0x242AF644,0x24205820,0x18,0x1C,0x3F800000),
    (9, 0x24205D0C,0x242057A0,0x242AF644,0x24205820,0x18,0x1C,0x3C888889),
    (10,0x24205D31,0x242057A0,0x242AF644,0x24205820,0x18,0x1C,0x3991A2B4),
    (11,0x24205D56,0x24205840,0x242AF69C,0x242058C0,0x04,0x20,0x3F800000),
    (12,0x24205D97,0x242058D0,0x242AF6F4,0x24205960,0x20,0x10,0),
    (13,0x24205D7B,0x24205980,0x242AF74C,0x24205A10,0x20,0x10,0),
    (14,0x24205DCF,0x24205A30,0x242AF7A4,0x24205960,0x20,0x14,0),
    (15,0x24205DB3,0x24205AE0,0x242AF7FC,0x24205A10,0x20,0x14,0),
)
CONSTRUCTOR_BOUNDARY = {
    0x242054E0:0x24205500,0x24205590:0x242055C0,
    0x24205620:0x24205650,0x24205690:0x242056B0,
    0x242056C0:0x242056F0,0x24205720:0x24205750,
    0x242057A0:0x242057D0,0x24205840:0x24205880,
    0x242058D0:0x24205910,0x24205980:0x242059C0,
    0x24205A30:0x24205A70,0x24205AE0:0x24205B20
}
OPCODE_TO_READER_METHOD = bytes.fromhex("8b c1 8b 4c 24 04 8b 11 89 44 24 04 ff 62")

def offset(va: int, length: int, image_length: int) -> int:
    begin=va-BASE
    if begin<0 or length<0 or begin+length>image_length:
        raise ValueError("requested metadata VA outside authenticated image")
    return begin

def raw(data: bytes, va: int, count: int) -> bytes:
    start=offset(va,count,len(data))
    return data[start:start+count]

def u32(data: bytes, va: int) -> int:
    return struct.unpack_from("<I",data,offset(va,4,len(data)))[0]

def check_immutable_metadata() -> None:
    assert len(CASES)==16
    assert [item[0] for item in CASES]==list(range(16))
    assert [item[5] for item in CASES]==[
        0x24,0x20,0x14,0x10,0x0C,0x0C,0x08,0x1C,
        0x18,0x18,0x18,0x04,0x20,0x20,0x20,0x20]
    assert len(set(item[3] for item in CASES))==14
    assert [item[7] for item in CASES[8:12]]==[
        0x3F800000,0x3C888889,0x3991A2B4,0x3F800000]
    assert all(item[5] % 4 == 0 for item in CASES)
    assert len(CONSTRUCTOR_BOUNDARY)==12

def check_original(data: bytes) -> dict:
    if hashlib.sha256(data).hexdigest()!=SHA256:
        raise ValueError("original client SHA256 does not match 2006 retail owner binary")
    if data[:2]!=b"MZ":
        raise ValueError("not a PE32 DLL")
    for entry in CASES:
        index,case,constructor,vtable,method,slot,allocation,config=entry
        if u32(data,JUMP_TABLE+4*index)!=case:
            raise ValueError("factory jump table differs for type "+str(index))
        # Every original switch branch starts with push imm8 for size.
        if raw(data,case,2)!=bytes([0x6a,allocation]):
            raise ValueError("allocation size differs for type "+str(index))
        if constructor:
            end=CONSTRUCTOR_BOUNDARY[constructor]
            code=raw(data,constructor,end-constructor)
            if struct.pack("<I",vtable) not in code:
                raise ValueError("expected object vtable absent from constructor")
            # At least one real CALL rel32 from the factory case to
            # this constructor, not an invented direct function pointer.
            next_branch=min(
                (x[1] for x in CASES if x[1]>case),
                default=JUMP_TABLE)
            body=raw(data,case,next_branch-case)
            targets=[]
            for pos in range(len(body)-4):
                if body[pos]==0xE8:
                    relative=struct.unpack_from("<i",body,pos+1)[0]
                    targets.append(case+pos+5+relative)
            if constructor not in targets:
                raise ValueError("missing case -> constructor CALL for type "+str(index))
        else:
            next_branch=min(
                (x[1] for x in CASES if x[1]>case),
                default=JUMP_TABLE)
            body=raw(data,case,next_branch-case)
            if struct.pack("<I",vtable) not in body:
                raise ValueError("missing inline vtable assignment for type "+str(index))
        if config:
            next_branch=min(
                (x[1] for x in CASES if x[1]>case),
                default=JUMP_TABLE)
            body=raw(data,case,next_branch-case)
            if b"\x68"+struct.pack("<I",config) not in body:
                raise ValueError("factory literal parameter differs for type "+str(index))
        if u32(data,vtable+VTABLE_SLOT_VALUE)!=method:
            raise ValueError("wrong original value virtual method for type "+str(index))
        if raw(data,method,len(OPCODE_TO_READER_METHOD)+1)!=(
                OPCODE_TO_READER_METHOD+bytes([slot])):
            raise ValueError("wrong reader operation thunk for type "+str(index))
    # Original factory dispatch itself must bound type codes to 0..15.
    if raw(data,0x24205BC9,3)!=b"\x83\xf9\x0f":
        raise ValueError("factory did not bound type code to 0..15")
    if raw(data,0x24205BD2,7)!=b"\xff\x24\x8d\xf0\x5d\x20\x24":
        raise ValueError("factory no longer indexes sixteen x86 code addresses")
    # Dynamic-record wrapper vtable +0x4c forwards on to wrapped
    # value object's +0x4c. It is not itself a type-specific read.
    if u32(data,0x242AF854+0x4C)!=0x242063C0:
        raise ValueError("dynamic value wrapper slot changed")
    if raw(data,0x242063D8,3)!=b"\xff\x52\x4c":
        raise ValueError("dynamic wrapper no longer forwards type +0x4c")
    return {
        "client_sha256":SHA256,
        "factory_va":hex(FACTORY),
        "switch_case_count":len(CASES),
        "value_virtual_offset":hex(VTABLE_SLOT_VALUE),
        "dynamic_wrapper_value_virtual_va":"0x242063c0",
        "reader_slot_offsets_by_type":[entry[5] for entry in CASES],
        "case_allocations_by_type":[entry[6] for entry in CASES],
        "config_literal_bits_by_type":[hex(entry[7]) for entry in CASES],
        "status":"SHA-authenticated metadata only; no value type names or runtime dispatch"
    }

def self_test() -> None:
    check_immutable_metadata()
    fake=bytearray(32)
    fake[1:5]=struct.pack("<I",0x24205C00)
    assert u32(fake,BASE+1)==0x24205C00
    assert raw(fake,BASE+1,4)==struct.pack("<I",0x24205C00)
    for invalid in (BASE-1,BASE+32,BASE+30):
        try:
            raw(fake,invalid,4)
        except ValueError:
            pass
        else:
            raise AssertionError("out-of-bounds RVA accepted")
    assert OPCODE_TO_READER_METHOD==bytes.fromhex(
        "8b c1 8b 4c 24 04 8b 11 89 44 24 04 ff 62")
    assert len(CASES)==16
    print("PASS original-free 16-type SiN Stats factory metadata fixtures")

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,help="owner-private original client.dll")
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:self_test()
    elif args.client:
        print(json.dumps(check_original(args.client.read_bytes()),indent=2))
    else:p.error("choose --client or --self-test")
if __name__=="__main__":
    main()
