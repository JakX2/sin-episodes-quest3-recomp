#!/usr/bin/env python3
"""Phase230: verify original SiN client entity-list handles and virtual dispatch.

Private original retail 2006 PE32 DLLs ONLY. Both complete game DLL
SHA256s are required before counting any newly recovered whole function.
No original complete machine-code function bodies or assets are embedded.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
from audit_sin_original_embedded_interface_initializers import CHAINS
from audit_sin_original_embedded_interface_virtual_accessors import CLIENT_VTABLE_VA

# (label, entry VA, exact original code bytes, whole-original SHA256,
# trailing INT3, native RET4 offsets or C3 offsets, sparse opcode witnesses)
# Original RET offsets are relative to each complete function start; all
# tested branches have their own exact native RET instruction.
METHODS = {
    "client_entity_slot3_indexed_vcall_18": (
        0x24105AF0,36,"010be7e7997efd85df59f59260fba5498a35eea7f7d00be9592578987ee80506",
        12,(0x1C,0x21),(),
        {0x00:"8b44240485c0",0x06:"7c17",0x08:"05ffefffff",
         0x0D:"c1e004",0x10:"8b0c08",0x17:"8b01ff5018",0x1F:"33c0"}),
    "client_parent_recalculate_max_live_slot": (
        0x24105B20,48,"c0ca3b059853bb32b91236c0dcdecc0f47d98f1a7157657368071418c704d29a",
        0,(),(0x28,0x2F),
        {0x00:"b8ff070000",0x07:"c78124000100ffffffff",
         0x11:"8d91f47f0000",0x19:"833a00",0x1E:"83e801",
         0x21:"83ea10",0x29:"898124000100"}),
    "client_parent_slot_index_vcall_1c": (
        0x24105B50,34,"8c6620646e8ab5c7aa066bd27631d9dc448144ee40543c27470c4f14c7eaf68c",
        14,(0x1A,0x1F),(),
        {0x00:"8b44240485c0",0x06:"7c15",0x08:"c1e004",
         0x0B:"8b440804",0x15:"8bc8ff521c",0x1D:"33c0"}),
    "client_parent_slot_index_vcall_0c": (
        0x24105B80,34,"eba653c74bdaa08a8fa0d4210f0747e6a49f902711e86965d6e0ed37801cbb79",
        14,(0x1A,0x1F),(),
        {0x00:"8b44240485c0",0x06:"7c15",0x08:"c1e004",
         0x0B:"8b440804",0x15:"8bc8ff520c",0x1D:"33c0"}),
    "client_entity_slot1_handle_vcall_10": (
        0x24105BB0,33,"f7b0f8459b8668297a723f983ccb184b7db31427a00e9ab4d28a16493bbd2c7b",
        15,(0x19,0x1E),(),
        {0x00:"8b54240451",0x05:"8bc48910",0x09:"8b01ff5008",
         0x0E:"85c0",0x14:"8bc8ff5210",0x1C:"33c0"}),
    "client_entity_slot4_handle_vcall_18": (
        0x24105BE0,33,"b905524f103a461177b4c02f4541100c4425c7663b68d8783d7f4870466da56d",
        15,(0x19,0x1E),(),
        {0x00:"8b54240451",0x05:"8bc48910",0x09:"8b01ff5008",
         0x0E:"85c0",0x14:"8bc8ff5218",0x1C:"33c0"}),
    "client_entity_slot2_ehandle_serial_resolver": (
        0x241074D0,44,"4ab8dfccb518e758d2af09b81bed3fcabe80219b0890d885392cc42ac7f55520",
        4,(0x24,0x29),(),
        {0x00:"8b542404",0x04:"83faff",0x09:"8bc225ff0f0000",
         0x10:"2d01100000",0x15:"c1e004",0x18:"03c1c1ea0c",
         0x1D:"395004",0x22:"8b00",0x27:"33c0"}),
}
# Phase228 verified that this final native interface is embedded at
# parent+0x10014. Four more virtual slots have actual .rdata pointer
# entries, each carrying a PE32 HIGHLOW relocation.
VIRTUAL_SLOTS=(
    (1,0x24105BB0),(2,0x241074D0),
    (3,0x24105AF0),(4,0x24105BE0),
)
# Authentic direct calls from other client game code into the parent
# index-lookup and highest-slot recomputation routines.
CALLS=(
    (0x24108AA3,0x24105B20),
    (0x2406F3FD,0x24105B50),
    (0x24132B54,0x24105B50),
)
ORIGINAL_SLOT_MASK=0xFFF
ORIGINAL_SERIAL_SHIFT=12
ORIGINAL_TABLE_STRIDE=16
ORIGINAL_RELATIVE_SLOT_BIAS=0x1001
ORIGINAL_HIGHEST_INDEX=0x7FF
ORIGINAL_HIGHEST_FIELD_OFFSET=0x10024
ORIGINAL_ENTITYLIST_EMBEDDED_OFFSET=0x10014

def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def check_native(client:PEImage, *,
                 check_hash:bool=True,
                 original_provenance_verified:bool=False,
                 relocations:set[int]|None=None)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase230 wrong client PE image base")
    if CHAINS["client"][4]!=ORIGINAL_ENTITYLIST_EMBEDDED_OFFSET:
        raise VerificationError("Phase230 original Phase228 embedded interface offset drift")
    if relocations is None:
        relocations=pe_highlow_relocations(client)
    evidence=[]
    witness_count=0
    for label,(va,n,sha,padding,ret4,ret0,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+n-1)!=".text":
            raise VerificationError("Phase230 function outside authentic .text: "+label)
        code=client.at(va,n)
        if check_hash and hashlib.sha256(code).hexdigest()!=sha:
            raise VerificationError("Phase230 original whole native method SHA256 drift: "+label)
        if client.at(va+n,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase230 original INT3 boundary drift: "+label)
        for off in ret4:
            if code[off:off+3]!=b"\xc2\x04\x00":
                raise VerificationError("Phase230 native RET4 branch drift: "+label)
        for off in ret0:
            if code[off:off+1]!=b"\xc3":
                raise VerificationError("Phase230 native C3 return branch drift: "+label)
        for off,hx in witnesses.items():
            raw=bytes.fromhex(hx)
            if code[off:off+len(raw)]!=raw:
                raise VerificationError("Phase230 sparse original opcode drift: "+label+" "+hex(off))
            witness_count+=1
        evidence.append({
            "name":label,"native_entry":f"0x{va:08X}",
            "original_full_code_bytes":n,
            "complete_native_original_sha256":sha if check_hash else None,
            "native_ret4_branches":len(ret4),
            "native_c3_branches":len(ret0),
            "trailing_INT3_bytes":padding,
        })
    for slot,method in VIRTUAL_SLOTS:
        cell=CLIENT_VTABLE_VA+slot*4
        if (client.section_of(cell)!=".rdata" or
            client.at(cell,4)!=struct.pack("<I",method) or cell not in relocations):
            raise VerificationError("Phase230 original relocated entitylist virtual slot drift: "+str(slot))
    for site,target in CALLS:
        if (client.section_of(site)!=".text" or
            client.at(site,5)!=rel_call(site,target)):
            raise VerificationError("Phase230 original direct game-code CALL drift: "+hex(site))
    return {
        "phase":230,
        "original_client_new_complete_SHA_methods":len(evidence)
            if check_hash and original_provenance_verified else 0,
        "original_client_new_whole_x86_bytes":sum(row[1] for row in METHODS.values())
            if check_hash and original_provenance_verified else 0,
        "methods":evidence,
        "sparse_instruction_witnesses":witness_count,
        "four_original_vtable_slot_HIGHLOW_edges":len(VIRTUAL_SLOTS),
        "direct_game_CALL_edges_to_parent_helpers":len(CALLS),
        "original_entity_index_mask":hex(ORIGINAL_SLOT_MASK),
        "original_handle_serial_shift":ORIGINAL_SERIAL_SHIFT,
        "original_parent_table_stride":ORIGINAL_TABLE_STRIDE,
        "original_entitylist_interface_offset":hex(ORIGINAL_ENTITYLIST_EMBEDDED_OFFSET),
        "original_max_occupied_slot_index":ORIGINAL_HIGHEST_INDEX,
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    for dll,raw in (("server",server_bytes),("client",client_bytes)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[dll]["sha256"]:
            raise VerificationError("Phase230 requires BOTH full original PE32 DLL SHA256s: "+dll)
    report=check_native(PEImage(client_bytes),original_provenance_verified=True)
    if (report["original_client_new_complete_SHA_methods"]!=7 or
            report["original_client_new_whole_x86_bytes"]!=262):
        raise VerificationError("Phase230 authentic native method inventory mismatch")
    report["scope_limits"]=[
        "Four selected embedded VClientEntityList003 methods and three related parent helpers, not complete engine entity-list behavior.",
        "Entity interface virtual dispatch targets invoked by the recovered x86 remain opaque implementation bodies.",
        "Host callbacks use 32-bit pointer ID tokens and safe byte-buffer checks, not native game objects.",
        "Native unchecked pointer arithmetic is refused on invalid host inputs; original crash/wrap behavior is not emulated.",
        "No executable retail Windows gameplay-module replacement is proven by this audit.",
    ]
    return report

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    s=p.add_mutually_exclusive_group(required=True)
    s.add_argument("--archive",type=Path)
    s.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive conflicts with --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("owner archive needs exactly one root server/client.dll pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
