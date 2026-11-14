#!/usr/bin/env python3
"""Phase224: full-SHA original 2006 client EHandle type-13 specialist audit.

Owner-local original server/client DLLs are read only. No original DLL,
entire proprietary x86 function body, game content or disassembly is emitted.
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
from audit_sin_original_client_snapshot_record_router import (
    CALL_EDGES, CASE_DESTINATIONS, TYPE_CASES,
)

# (entry VA, full native length, complete authentic body SHA256,
#  exact following INT3 count, original router call, native RET pop,
#  sparse independently reviewed complete opcode witnesses)
METHODS = {
    "type13_ehandle_resolved_pointer_comparison": (
        0x24199060,160,
        "2a5a05257fce45a49037e02d10d537b429f617c8f174bbacc8f471b1fecf343d",
        0,0x24199A93,12,{
            0x00:"80791400", 0x04:"7505", 0x06:"33c0",
            0x08:"c20c00", 0x0B:"8b4118", 0x0E:"f6401304",
            0x18:"33ff", 0x1A:"397c241c", 0x20:"8b442418",
            0x24:"8b6c2414", 0x28:"8b1d34a72f24",
            0x32:"8b042a", 0x35:"83f8ff",
            0x3C:"81e1ff0f0000", 0x42:"c1e104",
            0x49:"c1e80c", 0x4C:"394104",
            0x7B:"3bf1", 0x7F:"83c701",
            0x85:"3b7c241c", 0x8B:"5f", 0x94:"c20c00",
            0x9A:"33c0", 0x9D:"c20c00",
        }
    ),
    "type13_ehandle_forward_dword_copy": (
        0x24197E90,49,
        "fb6a1f47681fbd9b2ab63d77985fe211a4925308b032a83a1306260598fa7794",
        15,0x24199AA2,16,{
            0x00:"80792c00", 0x04:"7428",
            0x06:"837c240401", 0x0B:"7421",
            0x0D:"8b542410", 0x11:"85d2",
            0x13:"7e19", 0x15:"8b442408",
            0x19:"8b4c240c", 0x1D:"2bc8",
            0x20:"8b3401", 0x23:"8930",
            0x25:"83c004", 0x28:"83ea01",
            0x2B:"75f3", 0x2D:"5e", 0x2E:"c21000",
        }
    ),
    "type13_ehandle_diagnostics_and_entity_name": (
        0x241994C0,288,
        "df09716ab9b3746c9e7dfb9b44e8b910d26462de937bfaa8805580cd6a386fa5",
        0,0x24199AB1,16,{
            0x00:"53", 0x03:"807b1400", 0x07:"0f840f010000",
            0x12:"85ed", 0x1C:"8b07",
            0x21:"8b1534a72f24", 0x2B:"81e1ff0f0000",
            0x70:"6830422a24", 0x76:"e8d5e3ffff",
            0x7E:"8b07", 0x83:"0f8481000000",
            0xAE:"e81dc0f2ff", 0xB9:"e887f90a00",
            0xC3:"e80e030b00", 0xF4:"681c422a24",
            0xFB:"e800e4ffff", 0x10A:"680c422a24",
            0x111:"e8eae3ffff", 0x11D:"c21000",
        }
    ),
    "type13_ehandle_preview": (
        0x241995E0,175,
        "ac19b58ac8891a70cdb75e226bf717f1a1b99e8e8f4b62f405393a9c9d90ccb2",
        1,0x24199AC0,16,{
            0x00:"53", 0x03:"8b4334", 0x06:"3b4318",
            0x09:"0f859c000000", 0x15:"8b07",
            0x17:"83f8ff", 0x1C:"8b1534a72f24",
            0x24:"81e1ff0f0000", 0x2A:"c1e104",
            0x31:"c1e80c", 0x34:"394104",
            0x3F:"8bce", 0x41:"e86abff2ff",
            0x4B:"56", 0x4C:"e8d4f80a00",
            0x56:"e85b020b00", 0x87:"681c422a24",
            0x8D:"e86ee9ffff", 0x9B:"680c422a24",
            0xA1:"e85ae9ffff", 0xAC:"c21000",
        }
    ),
}

# Native formatter callbacks and external entity-name methods are only
# exact CALL REL32 evidence, NOT claimed fully reconstructed functions.
NESTED_CALLS = (
    (0x24199536,0x24197910),
    (0x2419956E,0x240C5590),
    (0x24199579,0x24248F05),
    (0x24199583,0x24249896),
    (0x241995BB,0x241979C0),
    (0x241995D1,0x241979C0),
    (0x24199621,0x240C5590),
    (0x2419962C,0x24248F05),
    (0x24199636,0x24249896),
    (0x2419966D,0x24197FE0),
    (0x24199681,0x24197FE0),
)
FORMAT_LITERALS={
    0x242A4230: "EHandles differ (net) 0x%p (pred) 0x%p\n",
    0x242A421C: "EHandle (0x%p->%s)",
    0x242A420C: "EHandle (NULL)",
}
GLOBAL_ENTITY_LIST_PTR=0x242FA734
HANDLE_INDEX_MASK=0xFFF
HANDLE_SERIAL_SHIFT=12
SLOT_STRIDE_BYTES=16
SLOT_BASE_OFFSET=4
FOLLOWING_ENTRIES = {
    0x24199100:bytes.fromhex("8b442404"),
    0x24197ED0:bytes.fromhex("53558b6c2410"),
    0x24199690:bytes.fromhex("80792c00"),
}


def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)


def verify_native(client:PEImage,*,check_hash:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase224 original client PE32 base mismatch")
    if TYPE_CASES[13]!=11 or CASE_DESTINATIONS[11]!=0x24199A8E:
        raise VerificationError("Phase224 original EHandle type13 router case drift")
    if len({x[0] for x in METHODS.values()})!=4:
        raise VerificationError("Phase224 EHandle native address double count")
    evidence={}
    witness_count=0
    for name,(va,n,digest,pad,router,pop,witnesses) in METHODS.items():
        if (client.section_of(va)!=".text" or
            client.section_of(va+n-1)!=".text"):
            raise VerificationError("Phase224 original helper not all in .text "+name)
        body=client.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase224 whole-function native SHA256 mismatch "+name)
        if body[-3:]!=b"\xc2"+struct.pack("<H",pop):
            raise VerificationError("Phase224 original native RET arity mismatch "+name)
        if client.at(va+n,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase224 original native INT3 boundary mismatch "+name)
        for off,hx in witnesses.items():
            signature=bytes.fromhex(hx)
            if body[off:off+len(signature)]!=signature:
                raise VerificationError("Phase224 native opcode witness drift "+name+" "+hex(off))
            witness_count+=1
        if ((router,va) not in CALL_EDGES or
            client.section_of(router)!=".text" or
            client.at(router,5)!=rel_call(router,va)):
            raise VerificationError("Phase224 native router CALL REL32 drift "+name)
        evidence[name]={
            "entry_va":f"0x{va:08X}",
            "original_code_bytes":n,
            "whole_native_sha256":digest if check_hash else None,
            "native_ret_pop":pop,
            "original_router_CALL":f"0x{router:08X}",
            "trailing_INT3_bytes":pad,
        }
    for site,target in NESTED_CALLS:
        if (client.section_of(site)!=".text" or
            client.section_of(target)!=".text" or
            client.at(site,5)!=rel_call(site,target)):
            raise VerificationError("Phase224 original nested engine CALL drift "+hex(site))
    for va,literal in FORMAT_LITERALS.items():
        if (client.section_of(va)!=".rdata" or
            client.at(va,len(literal)+1)!=literal.encode("ascii")+b"\0"):
            raise VerificationError("Phase224 original EHandle format literal drift "+hex(va))
    if client.section_of(GLOBAL_ENTITY_LIST_PTR)!=".data":
        raise VerificationError("Phase224 original CClientEntityList global outside .data")
    for va,head in FOLLOWING_ENTRIES.items():
        if (client.section_of(va)!=".text" or
            client.at(va,len(head))!=head):
            raise VerificationError("Phase224 original unpadded/INT3 neighbor drift "+hex(va))
    if (METHODS["type13_ehandle_resolved_pointer_comparison"][0]+160!=0x24199100
        or METHODS["type13_ehandle_diagnostics_and_entity_name"][0]+288!=0x241995E0):
        raise VerificationError("Phase224 adjacent original x86 method boundaries drift")
    return {
        "new_whole_SHA_verified_original_client_methods":len(METHODS) if check_hash else 0,
        "new_complete_original_native_x86_bytes":sum(x[1] for x in METHODS.values()) if check_hash else 0,
        "method_evidence":evidence,
        "sparse_original_instruction_witnesses":witness_count,
        "verified_original_router_direct_calls":4,
        "verified_original_nested_formatter_and_engine_calls":len(NESTED_CALLS),
        "native_format_literals":len(FORMAT_LITERALS),
        "original_entity_list_global":f"0x{GLOBAL_ENTITY_LIST_PTR:08X}",
        "original_ehandle_slot_index_mask":f"0x{HANDLE_INDEX_MASK:03X}",
        "original_ehandle_serial_shift":HANDLE_SERIAL_SHIFT,
        "original_ehandle_entry_stride_bytes":SLOT_STRIDE_BYTES,
        "original_ehandle_entry_base_offset":SLOT_BASE_OFFSET,
        "limits":[
            "Native comparison resolves two EHandles through a process global entity list and compares ENTITY POINTERS, not raw handle integers.",
            "Actual entity-list object lifetime and concrete native entity class methods remain unverified.",
            "The host uses explicit callbacks for entity list slots and debug-name fallback; no original object is invented.",
            "Forward DWORD copy and diagnostic formatting are post-compare and have host-only safe bounds.",
            "Actual retail Windows game loading reconstructed gameplay DLLs remains unproven.",
        ],
    }


def audit(server:bytes,client:bytes)->dict:
    if (hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or
        hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase224 requires BOTH complete original 2006 DLL SHA256s")
    return verify_native(PEImage(client))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    sources=p.add_mutually_exclusive_group(required=True)
    sources.add_argument("--archive",type=Path)
    sources.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive may not be combined with --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("original archive must contain exactly one root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        report=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(report,indent=2))


if __name__=="__main__":
    main()
