#!/usr/bin/env python3
"""Phase1689: authenticate the ORIGINAL engine's 3 immediate post-prediction
client factory requests, and the original client's separately laid out
secondary entity-list interface. Never execute retail original code.

Original retail engine.dll whole SHA is required for --archive/--engine,
alongside whole client.dll SHA. Public CI --self-test makes ONLY invented
sparse mock instruction windows; it never includes original DLL bytes.

This is source evidence for a new owner-only native empty-state entity
list factory in actual independent client.dll. It does NOT prove retail
game startup or original entity registration correctness.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError,
)
from audit_sin_original_core_factory_thunks import (
    pe_highlow_relocations,
)

ENGINE_SHA="f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
ENGINE_IMAGE_BASE=0x20000000
CLIENT_IMAGE_BASE=0x24000000
WINDOW_VA=0x20023D24
WINDOW_BYTES=100
WINDOW_SHA="9fe9b8d464295f162bfdc7325ed70aff5e8b6ea053207740ade4112356c1037a"
CLIENT_FACTORY_VA=0x241056B0
CLIENT_SECONDARY_OBJECT=0x2434FDDC
CLIENT_PARENT_OBJECT=0x2433FDC8
CLIENT_EMBEDDED_VTABLE_VA=0x24297390
CLIENT_PARENT_VTABLE_VA=0x242973B8
CLIENT_EMBEDDED_VTABLE_SHA="85b1a89b4c59052c027286beed20e00242cbdf428fa1559dbd666a51fe18be75"
CLIENT_PARENT_VTABLE_SHA="f4ccb17ffb03a35df448ebfe6b7fcb29fec20cff98f7403121ac0bc8216bd8d3"
CLIENT_EMBEDDED_SLOT_VAS=(
    0x241056F0,0x24105BB0,0x241074D0,0x24105AF0,
    0x24105BE0,0x24105700,0x24105740,0x24105720,
    0x24105730,
)
CLIENT_PARENT_SLOT_VAS=(0x241089D0,0x24108A70,0x241082A0)
CLIENT_SECONDARY_RTTI_COL_VA=0x242E59D4
CLIENT_SECONDARY_RTTI_VPTR_CELL=0x2429738C
CLIENT_SECONDARY_OWNER_OFFSET=0x10014
ENGINE_FACTORY_GLOBAL=0x203E3F04
STAGES=(
    # engine instruction VA, original name string VA,
    # original exact name, global where returned pointer stored
    (0x20023D24,0x202A0D68,"VClientEntityList003",0x203E4018),
    (0x20023D4A,0x202A0D18,"VCENTERPRINT002",0x203E401C),
    (0x20023D70,0x202A0CCC,"ClientLeafSystem001",0x203E4020),
)
# Original engine invokes VClientPrediction001 virtual +4 at
# 0x20023D21 immediately before the first stage above. Phase1658
# authenticates that caller separately.

def check(ok:bool,why:str)->None:
    if not ok: raise VerificationError("Phase1689 original entitylist startup: "+why)

def pack(v:int)->bytes:
    return struct.pack("<I",v)

def check_window(window:bytes)->int:
    """Check independent sparse instruction operands, no full retail blob."""
    check(len(window)==WINDOW_BYTES,"factory window is not 100 bytes")
    count=0
    for start,name_va,name,destination in STAGES:
        offset=start-WINDOW_VA
        # Push NULL status and requested client interface.
        check(window[offset:offset+3]==b"\x6a\x00\x68",
              name+" no longer push 0 / push native interface name")
        check(window[offset+3:offset+7]==pack(name_va),
              name+" original interface string operand changed")
        # Call through exactly the ORIGINAL captured client factory
        # global, then original caller-owned Add ESP,8.
        check(window[offset+7:offset+9]==b"\xff\x15" and
              window[offset+9:offset+13]==pack(ENGINE_FACTORY_GLOBAL),
              name+" does not call original client factory pointer")
        check(window[offset+13:offset+18]==b"\x83\xc4\x08\x85\xc0",
              name+" native caller cleanup and EAX test changed")
        check(window[offset+18:offset+19]==b"\xa3" and
              window[offset+19:offset+23]==pack(destination),
              name+" original factory-result global changed")
        count+=1
    check(tuple(row[2] for row in STAGES)==(
        "VClientEntityList003","VCENTERPRINT002","ClientLeafSystem001"),
        "original engine startup request order changed")
    return count

def check_client_vtables(pe:PEImage, relocs:set[int]) ->None:
    check(pe.base==CLIENT_IMAGE_BASE,
          "retail client.dll expected original PE32 image base")
    check(CLIENT_SECONDARY_OBJECT-CLIENT_PARENT_OBJECT==
          CLIENT_SECONDARY_OWNER_OFFSET,
          "factory does not return secondary at parent+0x10014")
    creator=pe.at(CLIENT_FACTORY_VA,6)
    check(creator[:1]==b"\xb8" and
          struct.unpack_from("<I",creator,1)[0]==CLIENT_SECONDARY_OBJECT and
          creator[5:]==b"\xc3",
          "original entity-list factory not exact secondary thunk")
    check(CLIENT_FACTORY_VA+1 in relocs,
          "retail secondary-pointer factory HIGHLOW relocation absent")
    for addr,expected,sha in (
        (CLIENT_EMBEDDED_VTABLE_VA,CLIENT_EMBEDDED_SLOT_VAS,CLIENT_EMBEDDED_VTABLE_SHA),
        (CLIENT_PARENT_VTABLE_VA,CLIENT_PARENT_SLOT_VAS,CLIENT_PARENT_VTABLE_SHA),
    ):
        raw=pe.at(addr,4*len(expected))
        check(hashlib.sha256(raw).hexdigest()==sha,
              "entire original parent/secondary vtable SHA changed")
        check(tuple(struct.unpack("<"+"I"*len(expected),raw))==expected,
              "original entity list parent/secondary vtable slots changed")
        for idx in range(len(expected)):
            check(addr+4*idx in relocs,
                  "missing original client vtable HIGHLOW relocation")
    check(struct.unpack("<I",pe.at(CLIENT_SECONDARY_RTTI_VPTR_CELL,4))[0]==
          CLIENT_SECONDARY_RTTI_COL_VA,
          "original secondary RTTI COL pointer changed")
    col=pe.at(CLIENT_SECONDARY_RTTI_COL_VA,20)
    _,offset,cd_offset,type_va,hierarchy_va=struct.unpack("<IIIII",col)
    check(offset==CLIENT_SECONDARY_OWNER_OFFSET and cd_offset==0 and
          type_va!=0 and hierarchy_va!=0,
          "original source RTTI secondary owner offset no longer 0x10014")
    check(CLIENT_SECONDARY_RTTI_VPTR_CELL in relocs and
          CLIENT_SECONDARY_RTTI_COL_VA+12 in relocs and
          CLIENT_SECONDARY_RTTI_COL_VA+16 in relocs,
          "original RTTI relocated type/hierarchy identity changed")

def verify_original(client_bytes:bytes,engine_bytes:bytes)->dict:
    check(hashlib.sha256(client_bytes).hexdigest()==ORIGINALS["client"]["sha256"],
          "whole original retail client SHA mismatch")
    check(hashlib.sha256(engine_bytes).hexdigest()==ENGINE_SHA,
          "whole original retail engine SHA mismatch")
    engine=PEImage(engine_bytes);client=PEImage(client_bytes)
    check(engine.base==ENGINE_IMAGE_BASE,
          "original engine.dll expected native PE32 base")
    engine_relocs=pe_highlow_relocations(engine)
    client_relocs=pe_highlow_relocations(client)
    check(engine.section_of(WINDOW_VA)==".text" and
          engine.section_of(WINDOW_VA+WINDOW_BYTES-1)==".text",
          "full original factory request window not in .text")
    window=engine.at(WINDOW_VA,WINDOW_BYTES)
    check(hashlib.sha256(window).hexdigest()==WINDOW_SHA,
          "full original engine 100-byte window SHA changed")
    n=check_window(window)
    for start,name_va,name,destination in STAGES:
        for site in (start+3,start+9,start+19):
            check(site in engine_relocs,
                  "engine factory source address not HIGHLOW relocated at "+hex(site))
        check(engine.section_of(name_va)==".rdata" and
              engine.at(name_va,len(name)+1)==name.encode("ascii")+b"\0",
              "original retail engine interface name changed: "+name)
        check(engine.section_of(destination)==".data",
              "engine output global no longer points into .data")
    check_client_vtables(client,client_relocs)
    return {
       "phase":1689,
       "original_verified":True,
       "engine_window_va":hex(WINDOW_VA),
       "whole_original_engine_window_sha256":WINDOW_SHA,
       "immediate_post_prediction_factory_order":[s[2] for s in STAGES],
       "original_engine_factory_address":hex(ENGINE_FACTORY_GLOBAL),
       "original_entitylist_secondary_factory_va":hex(CLIENT_FACTORY_VA),
       "original_secondary_offset_from_owner":hex(CLIENT_SECONDARY_OWNER_OFFSET),
       "original_embedded_virtual_slots":len(CLIENT_EMBEDDED_SLOT_VAS),
       "original_parent_virtual_slots":len(CLIENT_PARENT_SLOT_VAS),
       "original_owner_raw_record_count":4096,
       "original_raw_record_stride_bytes":16,
       "original_mirror_record_count":4096,
       "original_mirror_record_stride_bytes":8,
       "scope":"original static PE32 verification only; NO original game execution",
       "factory_windows_checked":n,
    }

def make_synthetic_window()->bytearray:
    # Invented ZERO-filled window with only sparse opcode witnesses
    # placed where they belong. This is NOT a copy or modified
    # version of any original commercial function.
    window=bytearray(WINDOW_BYTES)
    for start,name_va,_,dest in STAGES:
        at=start-WINDOW_VA
        window[at:at+3]=b"\x6a\x00\x68"
        window[at+3:at+7]=pack(name_va)
        window[at+7:at+9]=b"\xff\x15"
        window[at+9:at+13]=pack(ENGINE_FACTORY_GLOBAL)
        window[at+13:at+18]=b"\x83\xc4\x08\x85\xc0"
        window[at+18:at+19]=b"\xa3"
        window[at+19:at+23]=pack(dest)
    return window

def self_test()->dict:
    clean=make_synthetic_window()
    check(check_window(bytes(clean))==3,"synthetic startup stages not found")
    changed=0
    for start,*_ in STAGES:
        at=start-WINDOW_VA
        for offset in range(23):
            mutated=bytearray(clean)
            mutated[at+offset]^=1
            try: check_window(bytes(mutated))
            except VerificationError: changed+=1
            else:
                raise VerificationError("Phase1689 synthetic byte corruption accepted")
    for shortened in (clean[:-1],clean[:-23]):
        try: check_window(bytes(shortened))
        except VerificationError: changed+=1
        else: raise VerificationError("Phase1689 short synthetic source accepted")
    check(changed==3*23+2,"missing original-free mutation probes")
    return {
        "phase":1689,
        "public_synthetic_sparse_window_mutations_rejected":changed,
        "original_game_files_executed":False,
        "retail_original_SHA_claim":False,
        "original-engine-window_bytes_embedded":False,
    }

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--self-test",action="store_true")
    source.add_argument("--archive",type=Path)
    source.add_argument("--client",type=Path)
    parser.add_argument("--engine",type=Path)
    args=parser.parse_args()
    if args.self_test:
        if args.engine:parser.error("--self-test does not take --engine")
        report=self_test()
    elif args.archive:
        if not args.engine:parser.error("--archive needs --engine PE32")
        with zipfile.ZipFile(args.archive) as z:
            check(z.namelist().count("client.dll")==1,
                  "owner archive missing exact original client.dll")
            report=verify_original(z.read("client.dll"),args.engine.read_bytes())
    else:
        if not args.engine:parser.error("--client needs --engine")
        report=verify_original(args.client.read_bytes(),args.engine.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
