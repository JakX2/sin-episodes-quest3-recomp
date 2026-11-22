#!/usr/bin/env python3
"""Phase623: original PE32 source-authenticated Elite Grunt Spawn method.

The --server/--client audit accepts only the user's exact unmodified 2006
binary pair; it emits *metadata only*, never proprietary code/pointers.
The original-free --self-test uses independently invented test addresses.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256
)

SPAWN_VA=0x22355820
SPAWN_SIZE=505
SPAWN_SHA256="6d56850edc4675dc55273a9ff476e6e8cf0c425e187f9c527423dedabe115b99"
ELITE_VTABLE=0x2248DF04
SPAWN_VTABLE_SLOT=25
CHAINGUN_STRING_VA=0x224781A4
CHAINGUN_STRING="weapon_chaingun"
WEAPON_CURRENT_FIELD_OFFSET=0xA24
CONVAR_PTR_GLOBAL=0x2259C208
CONVAR_SCALAR_OFFSET=0x28
CONDITION_SOURCENAME="npc_druglab_grunt_elite"
ELITE_RTTI_VA=0x2252DA38
BASE_COMBAT_WEAPON_RTTI_VA=0x224DE300
RTTI_ELITE_NAME=".?AVCWeaponChaingun@@"
RTTI_BASE_NAME=".?AVCBaseCombatWeapon@@"
# Direct x86 CALLs witnessed inside the complete original 505-byte method.
CALLS=(
    (0x22355897,0x221F3DE0), # intern original name symbol
    (0x223558CF,0x2235F2A0), # inherited native NPC Spawn
    (0x223559E4,0x222AA640), # resolve native active weapon handle
    (0x223559EA,0x223A5BAB), # RTTI runtime cast
)
# Original absolute pointer loads at these instruction sites. They reassemble
# 16 bytes of the exact chaingun string from four DWORD constants.
CHAINGUN_DWORD_LOADS=(
    (0x22355823,b"\xA1",CHAINGUN_STRING_VA),
    (0x22355828,b"\x8B\x15",CHAINGUN_STRING_VA+8),
    (0x22355832,b"\xA1",CHAINGUN_STRING_VA+12),
    (0x2235583A,b"\x8B\x0D",CHAINGUN_STRING_VA+4),
)
# Each witness is an original function-local instruction, never an entire
# original bytecode body distributed to CI or repo. The complete body is
# verified by the SHA above. Tests only emit signatures and metadata.
OPCODE_WITNESSES=(
    (0x22355844,b"\x8B\x86\x24\x0A\x00\x00"),
    (0x223558A1,b"\x89\x96\x24\x0A\x00\x00"),
    (0x223558AB,b"\xFF\x50\x68"),
    (0x223558B7,b"\xFF\x52\x1C"),
    (0x223558CA,b"\xFF\x52\x6C"),
    (0x223558DA,b"\xC6\x86\xD9\x0F\x00\x00\x00"),
    (0x223558E1,b"\xFF\x90\x98\x07\x00\x00"),
    (0x223558F0,b"\x8B\x0D\x08\xC2\x59\x22"),
    (0x223558F9,b"\xD9\x41\x28"),
    (0x22355904,b"\xDB\x5C\x24\x0C"),
    (0x2235590C,b"\x39\xBE\x9C\x00\x00\x00"),
    (0x2235591C,b"\xFF\x92\xB4\x01\x00\x00"),
    (0x22355931,b"\xD9\x40\x28"),
    (0x22355944,b"\xDB\x5C\x24\x0C"),
    (0x2235594C,b"\x39\xBE\x98\x00\x00\x00"),
    (0x2235595C,b"\xFF\x92\xB0\x01\x00\x00"),
    (0x22355970,b"\x0D\x00\x00\x10\x00"),
    (0x2235597F,b"\x80\xBE\xF0\x01\x00\x00\x00"),
    (0x22355988,b"\x88\x96\x00\x02\x00\x00"),
    (0x2235599A,b"\x09\x11"),
    (0x2235599C,b"\x89\x86\xB4\x00\x00\x00"),
    (0x223559AA,b"\x80\xCD\x20"),
    (0x223559B1,b"\x80\xBE\xF0\x01\x00\x00\x00"),
    (0x223559BA,b"\x88\x96\x00\x02\x00\x00"),
    (0x223559CC,b"\x09\x10"),
    (0x223559CE,b"\x89\x8E\xB4\x00\x00\x00"),
    (0x223559D4,b"\x6A\x00"),
    (0x223559D6,b"\x68\x38\xDA\x52\x22"),
    (0x223559DB,b"\x68\x00\xE3\x4D\x22"),
    (0x223559E0,b"\x6A\x00"),
    (0x223559FA,b"\xFF\x52\x08"),
    (0x223559FD,b"\x8B\x00"),
    (0x223559FF,b"\x89\x86\xDC\x0F\x00\x00"),
    (0x22355A0A,b"\xC7\x86\xDC\x0F\x00\x00\xFF\xFF\xFF\xFF"),
    (0x22355A18,b"\xC3"),
)

def verify_hash(reader,va,size,expected):
    b=reader(va,size)
    if len(b)!=size or hashlib.sha256(b).hexdigest()!=expected:
        raise VerificationError("complete original Elite Spawn method SHA differs")

def verify_call_edges(reader,edges=CALLS):
    for at,destination in edges:
        ins=reader(at,5)
        if len(ins)!=5 or ins[:1]!=b"\xe8" or (
            at+5+struct.unpack_from("<i",ins,1)[0]!=destination
        ):
            raise VerificationError("original Elite Spawn CALL edge changed at "+hex(at))
    return len(edges)

def verify_opcodes(reader,witnesses=OPCODE_WITNESSES):
    for at,original in witnesses:
        if reader(at,len(original))!=original:
            raise VerificationError("original Elite Spawn source instruction changed at "+hex(at))
    return len(witnesses)

def verify_pointer_loads(reader,loads=CHAINGUN_DWORD_LOADS):
    for at,prefix,ptr in loads:
        ins=reader(at,len(prefix)+4)
        if ins[:len(prefix)]!=prefix or len(ins)!=len(prefix)+4 or (
            struct.unpack_from("<I",ins,len(prefix))[0]!=ptr
        ):
            raise VerificationError("original Elite Spawn chaingun literal load differs")
    return len(loads)

def verify_original(client,server):
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA256 or (
        hashlib.sha256(server).hexdigest()!=SERVER_SHA256
    ):
        raise VerificationError("incorrect original retail 2006 SiN DLL pair")
    pe=PEImage(server)
    if pe.base!=0x22000000 or pe.section_of(SPAWN_VA)!=".text":
        raise VerificationError("original Elite Spawn PE section/image base differs")
    verify_hash(pe.at,SPAWN_VA,SPAWN_SIZE,SPAWN_SHA256)
    def word(at):return struct.unpack("<I",pe.at(at,4))[0]
    if pe.section_of(ELITE_VTABLE)!=".rdata" or (
        word(ELITE_VTABLE+SPAWN_VTABLE_SLOT*4)!=SPAWN_VA
    ):
        raise VerificationError("original EliteGrunt vtable Spawn slot differs")
    if pe.at(CHAINGUN_STRING_VA,len(CHAINGUN_STRING)+1)!=(CHAINGUN_STRING.encode()+b"\0"):
        raise VerificationError("original weapon_chaingun literal differs")
    if pe.at(ELITE_RTTI_VA+8,len(RTTI_ELITE_NAME)+1)!=(
        RTTI_ELITE_NAME.encode()+b"\0"
    ) or pe.at(BASE_COMBAT_WEAPON_RTTI_VA+8,len(RTTI_BASE_NAME)+1)!=(
        RTTI_BASE_NAME.encode()+b"\0"
    ):
        raise VerificationError("CWeaponChaingun / CBaseCombatWeapon cast RTTI differs")
    verify_pointer_loads(pe.at)
    verify_opcodes(pe.at)
    verify_call_edges(pe.at)
    return {
        "status":"ORIGINAL_2006_ELITE_GRUNT_SPAWN_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "original_function_va":hex(SPAWN_VA),
        "complete_original_method_bytes":SPAWN_SIZE,
        "complete_original_method_sha256":SPAWN_SHA256,
        "elite_primary_vtable":hex(ELITE_VTABLE),
        "native_spawn_vtable_slot":SPAWN_VTABLE_SLOT,
        "registered_classname":CONDITION_SOURCENAME,
        "selected_weapon":CHAINGUN_STRING,
        "native_weapon_symbol_field":hex(WEAPON_CURRENT_FIELD_OFFSET),
        "original_global_convar_ptr":hex(CONVAR_PTR_GLOBAL),
        "original_global_convar_float_offset":hex(CONVAR_SCALAR_OFFSET),
        "independent_native_float_reads":2,
        "x87_control_word":"temporary RC=truncate; restored after each conversion",
        "native_updates_in_order":["weapon symbol if different",
             "virtual+0x68","virtual+0x1c to +0x6c","inherited Spawn",
             "clear byte+0xfd9","virtual+0x798 with zero",
             "field+0x9c via virtual+0x1b4","field+0x98 via virtual+0x1b0",
             "set bit 0x00100000","set bit 0x00200000",
             "native active weapon lookup and CWeaponChaingun RTTI cast",
             "virtual+0x08 return word0 -> field+0xfdc or 0xffffffff"],
        "native_flags_offset":"0xb4",
        "native_network_dirty_flag_byte":"0x200",
        "native_edict_field":"0x16c",
        "native_chaingun_rtti_va":hex(ELITE_RTTI_VA),
        "native_base_weapon_rtti_va":hex(BASE_COMBAT_WEAPON_RTTI_VA),
        "native_chaingun_handle_storage_offset":"0xfdc",
        "original_direct_calls":len(CALLS),
        "additional_native_opcode_witnesses":len(OPCODE_WITNESSES),
        "limits":"No original DLL instruction bytes, native Source methods or entity objects are exported. Native gameplay integration remains unverified."
    }

def self_test():
    fakeva=0x71001000
    synthetic=b"arbitrary synthetic not original"
    digest=hashlib.sha256(synthetic).hexdigest()
    verify_hash(lambda at,n:synthetic[:n],fakeva,len(synthetic),digest)
    for i in range(len(synthetic)):
        mutated=bytearray(synthetic);mutated[i]^=1
        try:verify_hash(lambda at,n:bytes(mutated)[:n],fakeva,len(synthetic),digest)
        except VerificationError:pass
        else:raise AssertionError("modified synthetic function passed SHA gate")
    calls=((0x71002000,0x71003000),(0x71002100,0x71004100))
    mapping={at:b"\xe8"+struct.pack("<i",dst-at-5) for at,dst in calls}
    def read_call(at,n):return mapping.get(at,b"")[:n]
    assert verify_call_edges(read_call,calls)==2
    for at in mapping:
        for i in range(5):
            copy=dict(mapping);mutated=bytearray(copy[at]);mutated[i]^=1
            copy[at]=bytes(mutated)
            try:verify_call_edges(lambda at,n:copy.get(at,b"")[:n],calls)
            except VerificationError:pass
            else:raise AssertionError("changed synthetic call target accepted")
    pointers=(
        (0x71005000,b"\xa1",0x72001000),
        (0x71005010,b"\x8b\x15",0x72002000)
    )
    synthetic_words={at:opcode+struct.pack("<I",ptr) for at,opcode,ptr in pointers}
    assert verify_pointer_loads(lambda at,n:synthetic_words[at][:n],pointers)==2
    for at in synthetic_words:
        for i in range(len(synthetic_words[at])):
            copy=dict(synthetic_words)
            invalid=bytearray(copy[at]);invalid[i]^=1
            copy[at]=bytes(invalid)
            try:verify_pointer_loads(lambda at,n:copy[at][:n],pointers)
            except VerificationError:pass
            else:raise AssertionError("modified synthetic literal load accepted")
    opcodes=((0x71007000,b"\x80\xc0\x01"),
             (0x71007010,b"\xff\x50\x04"))
    opcode_memory=dict(opcodes)
    assert verify_opcodes(lambda at,n:opcode_memory[at][:n],opcodes)==2
    for at in opcode_memory:
        for i in range(len(opcode_memory[at])):
            copy=dict(opcode_memory)
            corrupted=bytearray(copy[at]);corrupted[i]^1
            copy[at]=bytes(corrupted)
            try:verify_opcodes(lambda at,n:copy[at][:n],opcodes)
            except VerificationError:pass
            else:raise AssertionError("modified synthetic opcode accepted")
    try:verify_original(b"fake-client",b"fake-server")
    except VerificationError:pass
    else:raise AssertionError("untrusted original DLLs accepted")
    print("PASS Phase623 synthetic method SHA, x86 direct calls, pointer loads, opcode mutations and original SHA refusal")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path)
    p.add_argument("--server",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    a=p.parse_args()
    if a.self_test:
        self_test();return 0
    if not a.client or not a.server:
        p.error("provide exact owner-held original --client and --server DLLs")
    try:
        result=verify_original(a.client.read_bytes(),a.server.read_bytes())
    except (VerificationError,struct.error,OSError) as e:
        p.exit(1,"ORIGINAL ELITE SPAWN REFUSED: "+str(e)+"\n")
    report=json.dumps(result,indent=2)+"\n"
    if a.output:a.output.write_text(report,encoding="utf-8")
    else:print(report,end="")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
