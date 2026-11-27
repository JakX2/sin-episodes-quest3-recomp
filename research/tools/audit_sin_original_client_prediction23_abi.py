#!/usr/bin/env python3
"""Phase1658: SHA-authenticated original VClientPrediction001 and engine startup.

--client AND --engine read owner-original PE32 files, verify both full SHA256s,
then inspect only file-backed instructions, RTTI, relocations and 23 pointers.
--self-test only constructs fabricated byte windows, no game images or code.
No original method is executed; no replacement prediction factory is enabled.

Previous Phase226 already identified this original singleton factory. This
phase authenticates newly bounded original prediction ABI/table evidence,
NOT 23 new function decompilations or a playable Windows/Android/VR client.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError,
)
from audit_sin_original_core_factory_thunks import (
    FACTORIES, pe_highlow_relocations,
)

CLIENT_SHA = ORIGINALS["client"]["sha256"]
ENGINE_SHA = "f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
CLIENT_BASE = 0x24000000
ENGINE_BASE = 0x20000000

NAME = "VClientPrediction001"
CLIENT_REG_WRAPPER_VA = 0x24271450
CLIENT_REG_WRAPPER_SHA = "d39365be71be631383edf67ea843adfb38c469f268f1609fc715a24cadb4330c"
CLIENT_NAME_VA = 0x242C6824
CLIENT_REG_CTOR_VA = 0x24164160
CLIENT_REG_RECORD_VA = 0x2437FA34
CLIENT_FACTORY_VA = 0x2422EA30
CLIENT_FACTORY_SHA = "72a64b5a986505d8284d4487eb23c37528a0d79f7ba3a000a2c484790b33d1d8"
CLIENT_SINGLETON_VA = 0x2437FA00  # virtual .data, NOT on-disk vptr
CLIENT_CONSTRUCTOR_VA = 0x24195A10
CLIENT_CONSTRUCTOR_BYTES = 50
CLIENT_CONSTRUCTOR_SHA = "9b0b5c77c72ae369a286600971788b3820d79ffff740fc6c7721b3fc5277f8a7"
CLIENT_VTABLE_VA = 0x242A363C
CLIENT_VTABLE_SHA = "17c4d590cbf557162939e041589450821200671285adf245d3d3f456350806de"
CLIENT_NEXT_RDATA = b"pred error %6.3f units ("

CLIENT_VIRTUAL_TARGETS = (
    0x24195A50, 0x24195F80, 0x24195710, 0x24197210,
    0x24195F90, 0x24196040, 0x241960E0, 0x241957A0,
    0x24195DD0, 0x24195DF0, 0x24195E30, 0x24195E80,
    0x24195EC0, 0x24195F10, 0x24195F50, 0x24195970,
    0x24195980, 0x24195990, 0x241968A0, 0x241964F0,
    0x24196790, 0x24196AE0, 0x241977B0,
)
VIRTUAL_COUNT = len(CLIENT_VIRTUAL_TARGETS)
assert VIRTUAL_COUNT == 23
CLIENT_DTOR_VA = CLIENT_VIRTUAL_TARGETS[0]
CLIENT_DTOR_BYTES = 31
CLIENT_DTOR_SHA = "93ddcd6e073f64faf09cd08ed68214da156d0b38db3ef3989471aab733e15111"
CLIENT_DTOR_PARENT_VPTR = 0x242A3524
CLIENT_DELETE_TARGET = 0x2416FD60
CLIENT_INIT_VA = CLIENT_VIRTUAL_TARGETS[1]
CLIENT_INIT_BYTES = 16
CLIENT_INIT_SHA = "7829179302e1698746d72b1e4bdfdc51704ef8c90bd8d25ea03e6b54331eab11"
# This is NOT a missing engine service: it is cl_predict's m_pParent
# field, at object address +0x18, initialized to self by the ConVar
# constructor. Prediction slot1 reads m_pParent->m_nValue (+0x2C).
CLIENT_INIT_GLOBAL_VA = 0x24374540
CL_PREDICT_OBJECT_VA = 0x24374528
CL_PREDICT_PARENT_FIELD_OFFSET = 0x18
CL_PREDICT_CVAR_VALUE_OFFSET = 0x2C
CL_PREDICT_NAME_VA = 0x242A3A04
CL_PREDICT_DESCRIPTION_VA = 0x242A3A10
CL_PREDICT_DEFAULT_STRING_VA = 0x242D6CCC
CL_PREDICT_DEFAULT_STRING = b"1\0"
CL_PREDICT_REGISTER_WRAPPER_VA = 0x2426C550
CL_PREDICT_REGISTER_WRAPPER_SHA = "17281254628e8b8b69d1def3b311502cc351d08583b5c92b977fc74357ad6858"
CL_PREDICT_CVAR_CTOR_VA = 0x241216C0
CL_PREDICT_CVAR_CTOR_BYTES = 64
CL_PREDICT_CVAR_CTOR_SHA = "e1ddea23dbd98b86e8a02f0b56494b364e1a9102e4824f466b51f1a0bcbd5196"
CL_PREDICT_BASE_CTOR_VA = 0x241212E0
CL_PREDICT_BASE_CTOR_BYTES = 244
CL_PREDICT_BASE_CTOR_SHA = "510247221f012afd3817fb65b99122d8907e752d7d472ad46cf6d0a93d3ab761"
CL_PREDICT_CONVAR_VPTR_VA = 0x242981CC
CL_PREDICT_REGISTER_ATEXIT_VA = 0x24248788
CL_PREDICT_ATEXIT_THUNK_VA = 0x24274990
CL_PREDICT_FLAGS = 0x200
CLIENT_INIT_GLOBAL_FIELD_OFFSET = 0x2C
CLIENT_INIT_DESTINATION_OFFSET = 0x10

CLIENT_RTTI_CELL_VA = CLIENT_VTABLE_VA - 4
CLIENT_RTTI_COL_VA = 0x242E8E94
CLIENT_RTTI_TYPE_VA = 0x242FDAC4
CLIENT_RTTI_CHD_VA = 0x242E8E84
CLIENT_RTTI_BASE_ARRAY_VA = 0x242E8E78
CLIENT_RTTI_BASE_DESCRIPTOR_VAS = (0x242E8E60, 0x242E8E1C)
CLIENT_RTTI_BASE_TYPE_VAS = (0x242FDAC4, 0x242FDAA8)
CLIENT_RTTI_BASE_NAMES = (
    b".?AVCPrediction@@\0", b".?AVIPrediction@@\0",
)

ENGINE_FACTORY_CALL_VA = 0x20023CF2
ENGINE_FACTORY_CALL_BYTES = 50
ENGINE_FACTORY_CALL_SHA = "51531be82672348be6b725dedab1145cc085db88498634fc9e19add2a069ac22"
ENGINE_PREDICTION_NAME_VA = 0x202A0DB8
ENGINE_ENTITYLIST_NAME_VA = 0x202A0D68
ENGINE_FACTORY_GLOBAL_VA = 0x203E3F04
ENGINE_PREDICTION_GLOBAL_VA = 0x203E4014
ENGINE_ERROR_STRING_VA = 0x202A0D80
ENGINE_ERROR_HANDLER_VA = 0x2015D5E0
ENGINE_PREDICTION_VCALL_VA = 0x20023D21
ENGINE_ENTITYLIST_FACTORY_CALL_VA = 0x20023D24

def check(ok: bool, why: str) -> None:
    if not ok:
        raise VerificationError("Phase1658 VClientPrediction001: " + why)

def pack(value: int) -> bytes:
    return struct.pack("<I", value)

def u32(blob: bytes, off: int = 0) -> int:
    return struct.unpack_from("<I", blob, off)[0]

def section_read(pe: PEImage, addr: int, n: int, section: str) -> bytes:
    check(pe.section_of(addr) == section
          and pe.section_of(addr+n-1) == section,
          "expected entire original range in " + section)
    return pe.at(addr, n)

def verify_virtual_table(blob: bytes,
                         targets: tuple[int, ...],
                         boundary: bytes) -> int:
    check(len(targets) == 23 and len(set(targets)) == 23,
          "expected 23 DISTINCT original prediction function addresses")
    check(len(blob) == 23*4+len(boundary),
          "unexpected exact 23-pointer table extent")
    for slot, expected in enumerate(targets):
        check(u32(blob,slot*4) == expected,
              "original prediction vtable method changed at slot "+str(slot))
    check(blob[23*4:] == boundary,
          "23-slot prediction table non-pointer terminator changed")
    return len(targets)

def verify_factory_thunk(blob: bytes, destination: int) -> None:
    check(blob == b"\xB8" + pack(destination) + b"\xC3",
          "original prediction singleton factory not MOV EAX, address; RET")

def verify_registration(blob: bytes, start: int,
                        name_va: int, creator_va: int,
                        record_va: int, ctor_va: int) -> None:
    check(len(blob)==21 and
          blob[0:1]==b"\x68" and blob[5:6]==b"\x68" and
          blob[10:11]==b"\xB9" and blob[15:16]==b"\xE8" and
          blob[20:21]==b"\xC3",
          "original static InterfaceReg constructor wrapper shape")
    check((u32(blob,1),u32(blob,6),u32(blob,11)) ==
          (name_va,creator_va,record_va),
          "original InterfaceReg address/creator/record operands")
    destination=start+20+struct.unpack_from("<i",blob,16)[0]
    check(destination==ctor_va,
          "original static InterfaceReg target constructor differs")

def verify_cl_predict_registration(blob: bytes,va: int,
                                   description: int, flags: int,
                                   default_string: int, name: int,
                                   object_va: int, convar_ctor: int,
                                   atexit_thunk: int,
                                   atexit_register: int) -> None:
    check(len(blob)==42 and
          all(blob[at:at+1]==b"\x68" for at in (0,5,10,15,30))
          and blob[20:21]==b"\xB9" and
          blob[25:26]==b"\xE8" and blob[35:36]==b"\xE8" and
          blob[40:42]==b"\x59\xC3",
          "cl_predict static ConVar registration wrapper changed")
    check((u32(blob,1),u32(blob,6),u32(blob,11),
           u32(blob,16),u32(blob,21),u32(blob,31)) ==
          (description,flags,default_string,name,object_va,atexit_thunk),
          "cl_predict actual name/default/ConVar object flags differ")
    check(va+30+struct.unpack_from("<i",blob,26)[0]==convar_ctor and
          va+40+struct.unpack_from("<i",blob,36)[0]==atexit_register,
          "cl_predict ConVar constructor/atexit target changed")


def verify_convar_constructor(blob: bytes,base_constructor: int,
                              address: int, vptr: int) -> None:
    check(len(blob)==CL_PREDICT_CVAR_CTOR_BYTES,
          "cl_predict delegating constructor must have 64 bytes")
    off=0x2F
    check(blob[off:off+2]==b"\xC7\x06"
          and u32(blob,off+2)==vptr and blob[off+6:off+7]==b"\xE8"
          and blob[-3:]==b"\xC2\x10\x00",
          "ConVar delegating constructor does not install vptr/RET16")
    check(address+off+11+struct.unpack_from("<i",blob,off+7)[0] ==
          base_constructor,
          "cl_predict ConVar constructor did not call parent constructor")


def verify_convar_base_constructor(blob: bytes) -> None:
    check(len(blob)==CL_PREDICT_BASE_CTOR_BYTES,
          "ConVar base constructor must have 244 bytes")
    # Full retail method SHA checks happen separately in owner-only mode.
    # These are the two relevant ABI semantics, not full decompilation:
    # ebp=this; +0x18 self parent; +0x2c numeric integer from FISTP.
    check(blob[9:12]==bytes.fromhex("896d18") and
          blob[0xAB:0xAE]==bytes.fromhex("db5d2c") and
          blob[-3:]==b"\xC2\x24\x00",
          "ConVar self parent or +0x2C numeric DWORD store differs")


def verify_prediction_ctor(blob: bytes, vtable: int) -> None:
    check(len(blob)==CLIENT_CONSTRUCTOR_BYTES,
          "original prediction constructor expected 50 bytes")
    check(blob[:6] == bytes.fromhex("8b c1 33 c9 c7 00") and
          u32(blob,6)==vtable and
          blob[10:16] == bytes.fromhex("83 ca ff 89 50 04") and
          blob[-1:] == b"\xC3",
          "constructor does not install primary vptr and default -1")
    # Original fields +8 and +9 zero; +0xC/+0x14..+0x20 zero;
    # +0x24=-1, +0x28/+0x2C/+0x30 zero. +0x10 is not set here.
    check(blob[16:] == bytes.fromhex(
        "88 48 08 88 48 09 89 48 0c 89 48 14 "
        "89 48 18 89 48 1c 89 48 20 89 50 24 "
        "89 48 28 89 48 2c 89 48 30 c3"),
        "original constructor field-store sequence differs")

def verify_prediction_dtor(blob: bytes, parent_vtable: int,
                           va: int, free_va: int) -> None:
    check(len(blob)==CLIENT_DTOR_BYTES,
          "original prediction deleting-destructor length differs")
    check(blob[:10] == bytes.fromhex("f644240401568bf1c706") and
          u32(blob,10)==parent_vtable and
          blob[14:18] == bytes.fromhex("740956e8") and
          blob[22:] == bytes.fromhex("83c4048bc65ec20400"),
          "original slot 0 is not a scalar deleting-destructor RET 4")
    delta=struct.unpack_from("<i",blob,18)[0]
    check(va+22+delta == free_va,
          "original prediction deleting-destructor free target differs")

def verify_prediction_init(blob: bytes, global_va: int) -> None:
    """Original slot1 is a no-argument RET, not a full prediction engine.

    MOV EAX,[cl_predict.m_pParent]; CMP DWORD PTR[EAX+2C],0;
    SETNE DL; MOV BYTE PTR[ECX+10],DL; RET.
    This is the ConVar numeric value; no m_pParent null guard.
    """
    check(len(blob)==CLIENT_INIT_BYTES and
          blob[0:1]==b"\xA1" and u32(blob,1)==global_va and
          blob[5:] == bytes.fromhex("83782c000f95c2885110c3"),
          "original prediction slot1 global/field/return ABI differs")

def verify_engine_prediction_window(blob: bytes, start: int,
                                    pred_name: int, factory_global: int,
                                    stored_global: int, error_name: int,
                                    error_handler: int) -> None:
    check(len(blob)==ENGINE_FACTORY_CALL_BYTES,
          "engine prediction factory/init window must be 50 bytes")
    check(blob[:3]==bytes.fromhex("6a0068") and
          u32(blob,3)==pred_name and
          blob[7:9]==bytes.fromhex("ff15") and
          u32(blob,9)==factory_global and
          blob[13:18]==bytes.fromhex("83c40c85c0") and
          blob[18:19]==b"\xA3" and u32(blob,19)==stored_global,
          "engine did not fetch VClientPrediction001 through client factory")
    check(blob[23:26]==bytes.fromhex("751268") and
          u32(blob,26)==error_name and blob[30:31]==b"\xE8" and
          start+35+struct.unpack_from("<i",blob,31)[0]==error_handler and
          blob[35:36]==b"\xA1" and u32(blob,36)==stored_global and
          blob[40:43]==bytes.fromhex("83c404") and
          blob[43:47]==bytes.fromhex("8b108bc8") and
          blob[47:50]==bytes.fromhex("ff5204"),
          "engine failed to invoke prediction __thiscall slot1 (+4)")
    check(blob[47:50]==bytes.fromhex("ff5204"),
          "engine no-argument prediction slot1 call differs")

def verify_engine_next_interface(blob: bytes,
                                 name_va: int, factory_global: int) -> None:
    check(len(blob)==13 and blob[:3]==bytes.fromhex("6a0068")
          and u32(blob,3)==name_va
          and blob[7:9]==bytes.fromhex("ff15")
          and u32(blob,9)==factory_global,
          "engine does not request entity-list immediately after prediction init")

def verify_rtti_col(blob: bytes, type_va: int, hierarchy_va: int) -> None:
    check(len(blob)==20 and struct.unpack("<IIIII",blob) ==
          (0,0,0,type_va,hierarchy_va),
          "prediction original MSVC RTTI COL must be primary offset zero")

def verify_rtti_hierarchy(blob: bytes, array_va: int) -> None:
    check(len(blob)==16 and struct.unpack("<IIII",blob) ==
          (0,0,2,array_va),
          "prediction original RTTI hierarchy not simple two-base shape")

def verify_rtti_base(blob: bytes, type_va: int, contained: int) -> None:
    check(len(blob)==24 and struct.unpack("<IIiiiI",blob) ==
          (type_va,contained,0,-1,0,0),
          "prediction original RTTI base PMD or type differs")

def verify_original(client_bytes: bytes, engine_bytes: bytes) -> dict:
    check(hashlib.sha256(client_bytes).hexdigest()==CLIENT_SHA,
          "complete owner original client.dll SHA-256 mismatch")
    check(hashlib.sha256(engine_bytes).hexdigest()==ENGINE_SHA,
          "complete owner original engine.dll SHA-256 mismatch")
    client=PEImage(client_bytes); engine=PEImage(engine_bytes)
    check(client.base==CLIENT_BASE and engine.base==ENGINE_BASE,
          "original retail engine/client are not expected PE32 images")
    reloc=pe_highlow_relocations(client)
    engine_reloc=pe_highlow_relocations(engine)
    rows={x[0]:x for x in FACTORIES["client"]}
    check(NAME in rows and rows[NAME][1]==CLIENT_FACTORY_VA and
          rows[NAME][2]==CLIENT_SINGLETON_VA,
          "Phase226 prediction factory/singleton contracts drifted")
    wrapper=section_read(client,CLIENT_REG_WRAPPER_VA,21,".text")
    check(hashlib.sha256(wrapper).hexdigest()==CLIENT_REG_WRAPPER_SHA,
          "original prediction registration complete method SHA changed")
    verify_registration(wrapper,CLIENT_REG_WRAPPER_VA,
                        CLIENT_NAME_VA,CLIENT_FACTORY_VA,
                        CLIENT_REG_RECORD_VA,CLIENT_REG_CTOR_VA)
    for operand in (CLIENT_REG_WRAPPER_VA+1,
                    CLIENT_REG_WRAPPER_VA+6,
                    CLIENT_REG_WRAPPER_VA+11):
        check(operand in reloc,"original client registration HIGHLOW missing")
    check(section_read(client,CLIENT_NAME_VA,len(NAME)+1,".rdata") ==
          NAME.encode("ascii")+b"\0",
          "original prediction interface version string differs")
    check(client.section_of(CLIENT_REG_RECORD_VA)==".data" and
          client.section_of(CLIENT_SINGLETON_VA)==".data",
          "original singleton / registration record not in virtual .data")
    creator=section_read(client,CLIENT_FACTORY_VA,6,".text")
    check(hashlib.sha256(creator).hexdigest()==CLIENT_FACTORY_SHA,
          "original prediction singleton creator complete method SHA changed")
    verify_factory_thunk(creator,CLIENT_SINGLETON_VA)
    check(CLIENT_FACTORY_VA+1 in reloc,
          "original creator address not HIGHLOW relocated")
    # This original prediction singleton is in unbacked .data. Never
    # fabricate a static initializer vptr from PE file raw bytes.
    try:
        client.at(CLIENT_SINGLETON_VA,4)
    except VerificationError:
        pass
    else:
        raise VerificationError("Phase1658 retail prediction singleton was unexpectedly file-backed")
    ctor=section_read(client,CLIENT_CONSTRUCTOR_VA,
                      CLIENT_CONSTRUCTOR_BYTES,".text")
    check(hashlib.sha256(ctor).hexdigest()==CLIENT_CONSTRUCTOR_SHA,
          "original prediction constructor full-method SHA changed")
    verify_prediction_ctor(ctor,CLIENT_VTABLE_VA)
    check(CLIENT_CONSTRUCTOR_VA+6 in reloc,
          "original prediction constructor vtable immediate not relocated")
    count=VIRTUAL_COUNT
    table=section_read(client,CLIENT_VTABLE_VA,
                       4*count+len(CLIENT_NEXT_RDATA),".rdata")
    check(hashlib.sha256(table[:4*count]).hexdigest()==CLIENT_VTABLE_SHA,
          "whole original 23-slot prediction vtable SHA changed")
    verify_virtual_table(table, CLIENT_VIRTUAL_TARGETS,
                         CLIENT_NEXT_RDATA)
    for slot,addr in enumerate(CLIENT_VIRTUAL_TARGETS):
        check(CLIENT_VTABLE_VA+4*slot in reloc,
              "original prediction virtual pointer missing relocation "+str(slot))
        check(client.section_of(addr)==".text",
              "original prediction slot target not executable "+str(slot))
    dtor=section_read(client,CLIENT_DTOR_VA,CLIENT_DTOR_BYTES,".text")
    check(hashlib.sha256(dtor).hexdigest()==CLIENT_DTOR_SHA,
          "original prediction slot0 method SHA mismatch")
    verify_prediction_dtor(dtor,CLIENT_DTOR_PARENT_VPTR,
                           CLIENT_DTOR_VA,CLIENT_DELETE_TARGET)
    check(CLIENT_DTOR_VA+10 in reloc,
          "original prediction deleting-destructor vptr not relocated")
    init=section_read(client,CLIENT_INIT_VA,CLIENT_INIT_BYTES,".text")
    check(hashlib.sha256(init).hexdigest()==CLIENT_INIT_SHA,
          "complete original prediction slot1 method SHA mismatch")
    verify_prediction_init(init,CLIENT_INIT_GLOBAL_VA)
    check(CLIENT_INIT_VA+1 in reloc and
          client.section_of(CLIENT_INIT_GLOBAL_VA)==".data",
          "original prediction slot1 global pointer relocation missing")
    # Resolve the previously ambiguous +0x2C dependency to the ACTUAL
    # original cl_predict ConVar registration and its m_pParent->m_nValue.
    check(CL_PREDICT_OBJECT_VA+CL_PREDICT_PARENT_FIELD_OFFSET ==
          CLIENT_INIT_GLOBAL_VA and
          CLIENT_INIT_GLOBAL_FIELD_OFFSET==CL_PREDICT_CVAR_VALUE_OFFSET,
          "original cl_predict ConVar parent/value offsets drifted")
    check(client.section_of(CL_PREDICT_OBJECT_VA)==".data",
          "cl_predict ConVar singleton not in virtual .data")
    check(section_read(client,CL_PREDICT_NAME_VA,11,".rdata")==b"cl_predict\0"
          and section_read(client,CL_PREDICT_DEFAULT_STRING_VA,2,".rdata")
              ==b"1\0"
          and section_read(client,CL_PREDICT_DESCRIPTION_VA,
                           len(b"Perform client side prediction.\0"),
                           ".rdata")==b"Perform client side prediction.\0",
          "original cl_predict name/default/help text differs")
    cv=section_read(client,CL_PREDICT_REGISTER_WRAPPER_VA,42,".text")
    check(hashlib.sha256(cv).hexdigest()==CL_PREDICT_REGISTER_WRAPPER_SHA,
          "original cl_predict static constructor full SHA mismatch")
    verify_cl_predict_registration(
        cv,CL_PREDICT_REGISTER_WRAPPER_VA,
        CL_PREDICT_DESCRIPTION_VA,CL_PREDICT_FLAGS,
        CL_PREDICT_DEFAULT_STRING_VA,CL_PREDICT_NAME_VA,
        CL_PREDICT_OBJECT_VA,CL_PREDICT_CVAR_CTOR_VA,
        CL_PREDICT_ATEXIT_THUNK_VA,CL_PREDICT_REGISTER_ATEXIT_VA)
    for site in (CL_PREDICT_REGISTER_WRAPPER_VA+1,
                 CL_PREDICT_REGISTER_WRAPPER_VA+11,
                 CL_PREDICT_REGISTER_WRAPPER_VA+16,
                 CL_PREDICT_REGISTER_WRAPPER_VA+21,
                 CL_PREDICT_REGISTER_WRAPPER_VA+31):
        check(site in reloc,"original cl_predict ConVar HIGHLOW relocation missing")
    cvctor=section_read(client,CL_PREDICT_CVAR_CTOR_VA,
                        CL_PREDICT_CVAR_CTOR_BYTES,".text")
    check(hashlib.sha256(cvctor).hexdigest()==CL_PREDICT_CVAR_CTOR_SHA,
          "original ConVar delegation complete function SHA changed")
    verify_convar_constructor(cvctor,CL_PREDICT_BASE_CTOR_VA,
                              CL_PREDICT_CVAR_CTOR_VA,
                              CL_PREDICT_CONVAR_VPTR_VA)
    check(CL_PREDICT_CVAR_CTOR_VA+0x31 in reloc,
          "original ConVar constructor vtable pointer is not relocated")
    cvbase=section_read(client,CL_PREDICT_BASE_CTOR_VA,
                        CL_PREDICT_BASE_CTOR_BYTES,".text")
    check(hashlib.sha256(cvbase).hexdigest()==CL_PREDICT_BASE_CTOR_SHA,
          "original ConVar base constructor full SHA changed")
    verify_convar_base_constructor(cvbase)

    check(client.section_of(CLIENT_RTTI_CELL_VA)==".rdata"
          and CLIENT_RTTI_CELL_VA in reloc
          and section_read(client,CLIENT_RTTI_CELL_VA,4,".rdata") ==
          pack(CLIENT_RTTI_COL_VA),
          "original prediction vtable[-1] RTTI COL missing")
    rtti_col=section_read(client,CLIENT_RTTI_COL_VA,20,".rdata")
    verify_rtti_col(rtti_col,CLIENT_RTTI_TYPE_VA,CLIENT_RTTI_CHD_VA)
    for address in (CLIENT_RTTI_COL_VA+12,CLIENT_RTTI_COL_VA+16):
        check(address in reloc,"original prediction RTTI COL relocation missing")
    hierarchy=section_read(client,CLIENT_RTTI_CHD_VA,16,".rdata")
    verify_rtti_hierarchy(hierarchy,CLIENT_RTTI_BASE_ARRAY_VA)
    check(CLIENT_RTTI_CHD_VA+12 in reloc,
          "original prediction RTTI CHD array pointer not relocated")
    for i,(descriptor_va,type_va,name) in enumerate(zip(
            CLIENT_RTTI_BASE_DESCRIPTOR_VAS,CLIENT_RTTI_BASE_TYPE_VAS,
            CLIENT_RTTI_BASE_NAMES)):
        cell=CLIENT_RTTI_BASE_ARRAY_VA+4*i
        check(cell in reloc and
              section_read(client,cell,4,".rdata")==pack(descriptor_va),
              "prediction RTTI array entry missing / modified "+str(i))
        base=section_read(client,descriptor_va,24,".rdata")
        verify_rtti_base(base,type_va,1 if i==0 else 0)
        check(descriptor_va in reloc,
              "original prediction base-type pointer not relocated")
        check(section_read(client,type_va+8,len(name),".data")==name,
              "original RTTI base name changed "+str(i))

    # Original ENGINE code, NOT original client startup simulation.
    call=section_read(engine,ENGINE_FACTORY_CALL_VA,
                      ENGINE_FACTORY_CALL_BYTES,".text")
    check(hashlib.sha256(call).hexdigest()==ENGINE_FACTORY_CALL_SHA,
          "original engine prediction request/init complete window changed")
    check(ENGINE_FACTORY_CALL_VA+47==ENGINE_PREDICTION_VCALL_VA,
          "original prediction vcallsite offset differs")
    verify_engine_prediction_window(
        call,ENGINE_FACTORY_CALL_VA,
        ENGINE_PREDICTION_NAME_VA,ENGINE_FACTORY_GLOBAL_VA,
        ENGINE_PREDICTION_GLOBAL_VA,ENGINE_ERROR_STRING_VA,
        ENGINE_ERROR_HANDLER_VA)
    check(ENGINE_FACTORY_CALL_VA+3 in engine_reloc
          and ENGINE_FACTORY_CALL_VA+9 in engine_reloc
          and ENGINE_FACTORY_CALL_VA+19 in engine_reloc
          and ENGINE_FACTORY_CALL_VA+36 in engine_reloc,
          "engine prediction request/global HIGHLOW references missing")
    check(section_read(engine,ENGINE_PREDICTION_NAME_VA,
                       len(NAME)+1,".rdata")==NAME.encode()+b"\0",
          "engine prediction interface string differs")
    check(engine.section_of(ENGINE_FACTORY_GLOBAL_VA)==".data"
          and engine.section_of(ENGINE_PREDICTION_GLOBAL_VA)==".data",
          "engine prediction factory/pointer storage outside global data")
    check(section_read(engine,ENGINE_ENTITYLIST_NAME_VA,
                       len("VClientEntityList003")+1,".rdata") ==
          b"VClientEntityList003\0",
          "engine next entity-list version string differs")
    verify_engine_next_interface(
        section_read(engine,ENGINE_ENTITYLIST_FACTORY_CALL_VA,13,".text"),
        ENGINE_ENTITYLIST_NAME_VA,ENGINE_FACTORY_GLOBAL_VA)
    check(ENGINE_ENTITYLIST_FACTORY_CALL_VA+3 in engine_reloc
          and ENGINE_ENTITYLIST_FACTORY_CALL_VA+9 in engine_reloc,
          "engine entity-list next factory call HIGHLOW refs missing")
    return {
        "phase": 1658,
        "provenance": "client and engine original full SHA-256 matched",
        "original_interface": NAME,
        "singleton_rva": hex(CLIENT_SINGLETON_VA-CLIENT_BASE),
        "singleton_initial_static_vptr_file_backed": False,
        "constructor_va": hex(CLIENT_CONSTRUCTOR_VA),
        "registered_factory_va": hex(CLIENT_FACTORY_VA),
        "original_vtable_va": hex(CLIENT_VTABLE_VA),
        "original_vtable_sha256": CLIENT_VTABLE_SHA,
        "original_virtual_slots": VIRTUAL_COUNT,
        "original_target_vas": [hex(x) for x in CLIENT_VIRTUAL_TARGETS],
        "slot0_scalar_deleting_destructor_ret4": True,
        "slot1_parameterless_init_ret0": True,
        "slot1_reads_cl_predict_parent_slot_va": hex(CLIENT_INIT_GLOBAL_VA),
        "cl_predict_original_convar_va": hex(CL_PREDICT_OBJECT_VA),
        "cl_predict_parent_member_offset": hex(CL_PREDICT_PARENT_FIELD_OFFSET),
        "cl_predict_parent_numeric_value_offset": hex(CL_PREDICT_CVAR_VALUE_OFFSET),
        "cl_predict_default_numeric_value": 1,
        "cl_predict_registration_full_sha_verified": True,
        "cl_predict_convar_constructor_full_sha_verified": True,
        "slot1_reads_global_field_offset": hex(CLIENT_INIT_GLOBAL_FIELD_OFFSET),
        "slot1_sets_prediction_byte_offset": hex(CLIENT_INIT_DESTINATION_OFFSET),
        "slot1_uses_null_guard": False,
        "original_RTTI_primary_class": "CPrediction",
        "original_RTTI_base_class": "IPrediction",
        "original_engine_prediction_callsite_va": hex(ENGINE_FACTORY_CALL_VA),
        "original_engine_prediction_init_vcallsite_va": hex(ENGINE_PREDICTION_VCALL_VA),
        "original_engine_next_interface": "VClientEntityList003",
        "engine_calls_prediction_init_before_entitylist_fetch": True,
        "original_native_functions_executed": 0,
        "new_original_native_functions_decompiled": 0,
        "production_prediction_factory_registered": False,
        "retail_windows_startup_verified": False,
    }

def reject(function, label: str) -> None:
    try:
        function()
    except VerificationError:
        return
    raise AssertionError("corruption incorrectly accepted: "+label)

def self_test() -> None:
    # Invented pointer targets, never copied original game code.
    invented_targets=tuple(0x49110000+i*0x41 for i in range(23))
    invented_rdata=b"completely invented data boundary"
    table=b"".join(pack(x) for x in invented_targets)+invented_rdata
    check(verify_virtual_table(table,invented_targets,invented_rdata)==23,
          "synthetic 23-slot vtable pass failed")
    for offset in range(len(table)):
        corrupt=bytearray(table);corrupt[offset]^=1
        reject(lambda data=bytes(corrupt):verify_virtual_table(
            data,invented_targets,invented_rdata), "vtable cell/boundary byte "+str(offset))
    reject(lambda:verify_virtual_table(table[:88],invented_targets,
                                       invented_rdata),"truncated 22-slot table")
    reject(lambda:verify_virtual_table(table+b"extra",invented_targets,
                                       invented_rdata),"fake 24th slot")
    fake_reg_start=0x45110000
    fake_name=0x45320000
    fake_factory=0x45120000
    fake_record=0x45500000
    fake_ctor=0x45101000
    fake_reg=(b"\x68"+pack(fake_name)+b"\x68"+pack(fake_factory)+
              b"\xB9"+pack(fake_record)+b"\xE8"+
              struct.pack("<i",fake_ctor-(fake_reg_start+20))+b"\xC3")
    verify_registration(fake_reg,fake_reg_start,fake_name,
                        fake_factory,fake_record,fake_ctor)
    for i in range(len(fake_reg)):
        corrupt=bytearray(fake_reg);corrupt[i]^=1
        reject(lambda data=bytes(corrupt):verify_registration(
            data,fake_reg_start,fake_name,fake_factory,
            fake_record,fake_ctor),"registration byte "+str(i))
    fake_singleton=0x45500030
    thunk=b"\xB8"+pack(fake_singleton)+b"\xC3"
    verify_factory_thunk(thunk,fake_singleton)
    for i in range(len(thunk)):
        corrupt=bytearray(thunk);corrupt[i]^=1
        reject(lambda data=bytes(corrupt):verify_factory_thunk(
            data,fake_singleton),"factory thunk byte "+str(i))
    # Constructor/dtor contain compiler machine-code PATTERN with INVENTED
    # virtual addresses. None of these are original retail instruction bytes.
    fake_vtable=0x452a1000
    fake_ctor_code=(bytes.fromhex("8bc133c9c700")+pack(fake_vtable)+
                    bytes.fromhex("83caff89500488480888480989480c894814"
                                  "89481889481c89482089502489482889482c"
                                  "894830c3"))
    verify_prediction_ctor(fake_ctor_code,fake_vtable)
    for i in range(len(fake_ctor_code)):
        corrupt=bytearray(fake_ctor_code);corrupt[i]^1
        reject(lambda data=bytes(corrupt):verify_prediction_ctor(
            data,fake_vtable),"constructor byte "+str(i))
    fake_dtor_va=0x45150000
    fake_parent_vptr=0x452a1100
    fake_free=0x45160000
    fake_dtor=(bytes.fromhex("f644240401568bf1c706")+pack(fake_parent_vptr)+
               bytes.fromhex("740956e8")+
               struct.pack("<i",fake_free-(fake_dtor_va+22))+
               bytes.fromhex("83c4048bc65ec20400"))
    verify_prediction_dtor(fake_dtor,fake_parent_vptr,fake_dtor_va,fake_free)
    for i in range(len(fake_dtor)):
        corrupt=bytearray(fake_dtor);corrupt[i]^1
        reject(lambda data=bytes(corrupt):verify_prediction_dtor(
            data,fake_parent_vptr,fake_dtor_va,fake_free),
            "deleting dtor byte "+str(i))
    # Validate that our newly identified synthetic ConVar object
    # constructor sets m_pParent at +0x18, and an integer at +0x2C.
    fake_cv_start=0x47001000
    fake_cvobj=0x47102000
    fake_cvdefault=0x47110000
    fake_cvname=0x47111000
    fake_cvdesc=0x47112000
    fake_cv_ctor=0x47010000
    fake_cv_atexit=0x47020000
    fake_cv_thunk=0x47022000
    fake_cv=(b"\x68"+pack(fake_cvdesc)+b"\x68"+pack(0x200)+
             b"\x68"+pack(fake_cvdefault)+b"\x68"+pack(fake_cvname)+
             b"\xB9"+pack(fake_cvobj)+b"\xE8"+
             struct.pack("<i",fake_cv_ctor-(fake_cv_start+30))+
             b"\x68"+pack(fake_cv_thunk)+b"\xE8"+
             struct.pack("<i",fake_cv_atexit-(fake_cv_start+40))+
             b"\x59\xC3")
    verify_cl_predict_registration(
        fake_cv,fake_cv_start,fake_cvdesc,0x200,fake_cvdefault,
        fake_cvname,fake_cvobj,fake_cv_ctor,fake_cv_thunk,fake_cv_atexit)
    for i in range(len(fake_cv)):
        corrupt=bytearray(fake_cv);corrupt[i]^=1
        reject(lambda data=bytes(corrupt):verify_cl_predict_registration(
            data,fake_cv_start,fake_cvdesc,0x200,fake_cvdefault,
            fake_cvname,fake_cvobj,fake_cv_ctor,fake_cv_thunk,fake_cv_atexit),
            "synthetic cl_predict ConVar wrapper byte "+str(i))
    fake_base_ctor=0x47030000
    fake_cv_vtable=0x47200000
    fake_delegate=(bytearray(64))
    fake_delegate[0x2F:0x35]=b"\xC7\x06"+pack(fake_cv_vtable)
    fake_delegate[0x35:0x3A]=b"\xE8"+struct.pack(
        "<i",fake_base_ctor-(fake_cv_ctor+0x3A))
    fake_delegate[-3:]=b"\xC2\x10\x00"
    verify_convar_constructor(bytes(fake_delegate),fake_base_ctor,
                              fake_cv_ctor,fake_cv_vtable)
    for i in range(0x2F,0x3A):
        corrupt=bytearray(fake_delegate);corrupt[i]^=1
        reject(lambda data=bytes(corrupt):verify_convar_constructor(
            data,fake_base_ctor,fake_cv_ctor,fake_cv_vtable),
            "synthetic ConVar delegating constructor byte "+str(i))
    fake_base=bytearray(244)
    fake_base[9:12]=bytes.fromhex("896d18")
    fake_base[0xAB:0xAE]=bytes.fromhex("db5d2c")
    fake_base[-3:]=b"\xC2\x24\x00"
    verify_convar_base_constructor(bytes(fake_base))
    for pos in list(range(9,12))+list(range(0xAB,0xAE))+list(range(241,244)):
        corrupt=bytearray(fake_base);corrupt[pos]^=1
        reject(lambda data=bytes(corrupt):verify_convar_base_constructor(data),
               "synthetic cl_predict parent/nValue byte "+str(pos))
    fake_engine_global=0x45610000
    fake_init=b"\xA1"+pack(fake_engine_global)+bytes.fromhex(
        "83782c000f95c2885110c3")
    verify_prediction_init(fake_init,fake_engine_global)
    for i in range(len(fake_init)):
        corrupt=bytearray(fake_init);corrupt[i]^1
        reject(lambda data=bytes(corrupt):verify_prediction_init(
            data,fake_engine_global),"slot1 init byte "+str(i))
    # Artificial engine call: exact dispatch shape, invented addresses.
    fake_engine_site=0x46120000
    fake_eng_name=0x46220000
    fake_eng_factory=0x46510000
    fake_stored=0x46520000
    fake_error=0x46230000
    fake_handler=0x46170000
    fake_eng=(b"\x6A\x00\x68"+pack(fake_eng_name)+
              b"\xFF\x15"+pack(fake_eng_factory)+bytes.fromhex("83c40c85c0")+
              b"\xA3"+pack(fake_stored)+b"\x75\x12\x68"+pack(fake_error)+
              b"\xE8"+struct.pack("<i",fake_handler-(fake_engine_site+35))+
              b"\xA1"+pack(fake_stored)+bytes.fromhex("83c4048b108bc8ff5204"))
    # Real byte-accurate engine request+slot1 dispatch verifier, using
    # ENTIRELY invented addresses and call targets in this self-test.
    verify_engine_prediction_window(fake_eng,fake_engine_site,
                                    fake_eng_name,fake_eng_factory,
                                    fake_stored,fake_error,fake_handler)
    for i in range(len(fake_eng)):
        corrupt=bytearray(fake_eng);corrupt[i]^1
        reject(lambda data=bytes(corrupt):verify_engine_prediction_window(
            data,fake_engine_site,fake_eng_name,fake_eng_factory,
            fake_stored,fake_error,fake_handler),
            "engine prediction request/call byte "+str(i))
    fake_entitylist_call=b"\x6A\x00\x68"+pack(0x46240000)+b"\xFF\x15"+pack(fake_eng_factory)
    verify_engine_next_interface(fake_entitylist_call,0x46240000,fake_eng_factory)
    for i in range(len(fake_entitylist_call)):
        corrupt=bytearray(fake_entitylist_call);corrupt[i]^1
        reject(lambda data=bytes(corrupt):verify_engine_next_interface(
            data,0x46240000,fake_eng_factory),
            "next original client interface request byte "+str(i))
    fake_col=struct.pack("<IIIII",0,0,0,0x47310000,0x47320000)
    verify_rtti_col(fake_col,0x47310000,0x47320000)
    fake_hierarchy=struct.pack("<IIII",0,0,2,0x47330000)
    verify_rtti_hierarchy(fake_hierarchy,0x47330000)
    for label,check_fn,blob in (
        ("RTTI COL",lambda d:verify_rtti_col(d,0x47310000,0x47320000),fake_col),
        ("RTTI hierarchy",lambda d:verify_rtti_hierarchy(d,0x47330000),fake_hierarchy)):
        for i in range(len(blob)):
            corrupt=bytearray(blob);corrupt[i]^1
            reject(lambda data=bytes(corrupt),fn=check_fn:fn(data),
                   label+" byte "+str(i))
    for contained in (0,1):
        base=struct.pack("<IIiiiI",0x47340000,contained,0,-1,0,0)
        verify_rtti_base(base,0x47340000,contained)
        for i in range(len(base)):
            corrupt=bytearray(base);corrupt[i]^1
            reject(lambda data=bytes(corrupt),n=contained:verify_rtti_base(
                data,0x47340000,n),"RTTI base "+str(contained)+" byte "+str(i))
    reject(lambda:verify_original(b"fabricated client",b"fabricated engine"),
           "forged full client/engine original SHA accepted")
    print("PASS Phase1658 synthetic original-free prediction23: exact 23-slot "
          "pointer extent and nonvirtual boundary; creator, InterfaceReg, "
          "constructor, deleting destructor RET4 and slot1 RET0/service "
          "dependency through cl_predict ConVar parent/nValue; "
          "engine slot1 before entity-list; RTTI two-base class "
          "metadata; mutation and fake original-SHA rejection")

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    p.add_argument("--client",type=Path,
                   help="owner-private unchanged SiN client.dll")
    p.add_argument("--engine",type=Path,
                   help="owner-private unchanged SiN engine.dll")
    args=p.parse_args()
    if args.self_test:
        if args.client is not None or args.engine is not None:
            p.error("--self-test must not be mixed with retail DLL paths")
        self_test()
        return 0
    if args.client is None or args.engine is None:
        p.error("original audit needs BOTH --client and --engine")
    print(json.dumps(verify_original(args.client.read_bytes(),
                                     args.engine.read_bytes()),
                     sort_keys=True,indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
