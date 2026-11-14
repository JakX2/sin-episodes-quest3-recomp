#!/usr/bin/env python3
"""Phase225: verify original SiN 2006 client shared prediction trace formatters.

Read-only owner-original dual-DLL SHA-gated audit. Never distributes the
retail DLLs, complete native function bytes or raw disassembly.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import BASE, FULL_SHA
from audit_sin_original_allocator_game_callers import named_iat

# (original entry VA, complete bytes, complete original SHA256,
# following INT3 padding, sparse original x86 complete-opcode witnesses)
METHODS={
    "shared_difference_formatter":(
        0x24197910,166,
        "d5d121f8e3a25d66fc8f3814fe2a11b6124d751dd640469e844050df608ce7d9",10,{
            0x00:"b800100000",0x05:"e8c61a0b00",0x13:"bb01000000",
            0x18:"015e28",0x1B:"807e2400",0x21:"807e1600",
            0x27:"837e3000",0x2D:"8b4618",0x33:"bf383e2a24",
            0x43:"bf74012a24",0x48:"8b8c2414100000",
            0x62:"ff155c622724",0x6B:"395e28",0x6E:"8b1dec612724",
            0x76:"688cc52924",0x8E:"68243e2a24",0x93:"ffd3",
            0x98:"c6462400",0x9F:"81c400100000",0xA5:"c3",
        }),
    "shared_status_callback_formatter":(
        0x241979C0,185,
        "026244a82ffe4c257ea97eaacbb7e83c812b4fd6ffbcd064a0fad88dccd52e32",7,{
            0x00:"b800100000",0x05:"e8161a0b00",
            0x12:"807e2500",0x1C:"837e3000",0x36:"0fbf7812",
            0x48:"8b8c2418100000",0x62:"ff155c622724",
            0x6D:"8b842428100000",0x74:"83f802",0x7A:"83f801",
            0x82:"c1e80a",0x8A:"c1e908",
            0x9B:"8b0c85e0da2f24",0xA5:"ff5630",
            0xAC:"c6462500",0xB2:"81c400100000",0xB8:"c3",
        }),
    "shared_preview_msg_formatter":(
        0x24197FE0,93,
        "880a186d02da86b94b953158d2fdaee8a93d59456fdaf4f482031da3bab09cac",3,{
            0x00:"b800100000",0x05:"e8f6130b00",
            0x0A:"8b8c2408100000",0x11:"8d84240c100000",
            0x24:"ff155c622724",0x2A:"8d442410",
            0x2F:"8b842418100000",0x36:"8b4818",0x39:"8b5104",
            0x3C:"8b4038",0x3F:"8b0d58a42f24",
            0x46:"8b5118",0x4B:"68903f2a24",
            0x50:"ff15ec612724",0x56:"81c424100000",0x5C:"c3",
        }),
}
PROBE_CALLS=((0x24197915,0x242493E0),
             (0x241979C5,0x242493E0),
             (0x24197FE5,0x242493E0))
NESTED_CALLS=(
    (0x24197AEE,0x24197910),(0x24197AFE,0x241979C0),
    (0x24197B3E,0x24197910),(0x24197B51,0x241979C0),
    (0x24197B9F,0x24197910),(0x24197BF3,0x241979C0),
    (0x24197C11,0x241979C0),(0x24197C71,0x24197910),
    (0x24197C90,0x241979C0),(0x24198373,0x24197FE0),
    (0x24198645,0x24197910),(0x2419866A,0x241979C0),
    (0x241986E8,0x24197FE0),(0x241988FC,0x24197910),
    (0x24198945,0x241979C0),(0x241989CF,0x24197FE0),
    (0x24199536,0x24197910),(0x241995BB,0x241979C0),
    (0x241995D1,0x241979C0),(0x2419966D,0x24197FE0),
    (0x24199681,0x24197FE0),
)
LITERALS={
    0x242A3E24:"%03i %s::%s - %s",
    0x2429C58C:"\n",
    0x242A3E38:"empty",
    0x242A0174:"NULL",
    0x242A3F90:"%i %s %s : %s\n",
}
FIELD_NAMES=(
    "FIELD_VOID", "FIELD_FLOAT", "FIELD_STRING", "FIELD_VECTOR",
    "FIELD_QUATERNION", "FIELD_INTEGER", "FIELD_BOOLEAN", "FIELD_SHORT",
    "FIELD_CHARACTER", "FIELD_COLOR32", "FIELD_EMBEDDED", "FIELD_CUSTOM",
    "FIELD_CLASSPTR", "FIELD_EHANDLE", "FIELD_EDICT",
    "FIELD_POSITION_VECTOR", "FIELD_TIME", "FIELD_TICK",
    "FIELD_MODELNAME", "FIELD_SOUNDNAME", "FIELD_INPUT",
    "FIELD_FUNCTION", "FIELD_VMATRIX", "FIELD_VMATRIX_WORLDSPACE",
    "FIELD_MATRIX3X4_WORLDSPACE", "FIELD_INTERVALFIELD_MODELINDEX",
)
TYPE_NAME_TABLE_VA=0x242FDAE0
PREVIEW_GLOBAL_PTR_VA=0x242FA458
Q_VSNPRINTF_IAT=0x2427625C
TIER0_MSG_IAT=0x242761EC
NEXT_ENTRIES={
    0x241979C0:bytes.fromhex("b800100000"),
    0x24197A80:bytes.fromhex("8b4118"),
    0x24198040:bytes.fromhex("8bc18b4c2404"),
}

def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,check_imports:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase225 incorrect original PE32 client image base")
    evidence={}
    witness_count=0
    for name,(va,n,digest,padding,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+n-1)!=".text":
            raise VerificationError("Phase225 native method outside original .text: "+name)
        body=client.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase225 whole original native SHA256 drift: "+name)
        if body[-1:]!=b"\xc3" or client.at(va+n,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase225 original RET/INT3 native boundary drift: "+name)
        for off,hx in witnesses.items():
            sig=bytes.fromhex(hx)
            if body[off:off+len(sig)]!=sig:
                raise VerificationError("Phase225 original instruction witness drift: "+name+" "+hex(off))
            witness_count+=1
        evidence[name]={
            "entry_va":f"0x{va:08X}","native_bytes":n,
            "whole_original_sha256":digest if check_hash else None,
            "ret_opcode":"C3","int3_padding":padding,
        }
    for site,dst in PROBE_CALLS+NESTED_CALLS:
        if (client.section_of(site)!=".text" or client.section_of(dst)!=".text"
                or client.at(site,5)!=rel_call(site,dst)):
            raise VerificationError("Phase225 authentic direct CALL drift at "+hex(site))
    for va,literal in LITERALS.items():
        if (client.section_of(va)!=".rdata" or
            client.at(va,len(literal)+1)!=literal.encode("ascii")+b"\0"):
            raise VerificationError("Phase225 original Msg format string drift: "+hex(va))
    if client.section_of(TYPE_NAME_TABLE_VA)!=".data":
        raise VerificationError("Phase225 original field-type table not in .data")
    names=struct.unpack("<27I",client.at(TYPE_NAME_TABLE_VA,27*4))
    if names[-1]!=0:
        raise VerificationError("Phase225 field type name table missing terminal null")
    for i,expected in enumerate(FIELD_NAMES):
        va=names[i]
        if client.section_of(va)!=".rdata" or (
            client.at(va,len(expected)+1)!=expected.encode("ascii")+b"\0"):
            raise VerificationError("Phase225 original FIELD_* type name drift at "+str(i))
    if client.section_of(PREVIEW_GLOBAL_PTR_VA)!=".data":
        raise VerificationError("Phase225 original preview global outside .data")
    for va,head in NEXT_ENTRIES.items():
        if client.section_of(va)!=".text" or client.at(va,len(head))!=head:
            raise VerificationError("Phase225 next original native function entry mismatch")
    if check_imports and (
        named_iat(client,"vstdlib.dll","Q_vsnprintf")!=[Q_VSNPRINTF_IAT] or
        named_iat(client,"tier0.dll","Msg")!=[TIER0_MSG_IAT]):
        raise VerificationError("Phase225 original Q_vsnprintf/Msg import name/IAT drift")
    return {
        "new_whole_SHA_verified_client_methods":len(METHODS) if check_hash else 0,
        "new_complete_original_native_bytes":sum(v[1] for v in METHODS.values()) if check_hash else 0,
        "method_evidence":evidence,
        "sparse_original_opcode_witnesses":witness_count,
        "original_stack_probe_calls":len(PROBE_CALLS),
        "verified_nested_serializer_calls":len(NESTED_CALLS),
        "original_format_literals":len(LITERALS),
        "original_field_type_name_entries":len(FIELD_NAMES),
        "original_field_type_table_terminal_null":True,
        "original_Q_vsnprintf_IAT":f"0x{Q_VSNPRINTF_IAT:08X}" if check_imports else None,
        "original_tier0_Msg_IAT":f"0x{TIER0_MSG_IAT:08X}" if check_imports else None,
        "limits":[
            "Native 4096-byte Q_vsnprintf and tier0 Msg calls are external imports, not recovered implementations.",
            "Original status callback stored at native this+0x30 and global preview context are host-owned.",
            "The C++ host reference models control flow, trace events, and raw vararg words; no original Source logger object is reconstructed.",
            "Passing tests does not establish a playable reconstructed client.dll or server.dll.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or (
        hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase225 requires BOTH original complete 2006 SiN DLL SHA256s")
    return verify_native(PEImage(client))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive cannot accompany --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner archive must contain one root client/server DLL pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
