#!/usr/bin/env python3
"""Phase249: authenticate original SiN client intrusive-list & base-vtable ABI.

Original-only credit requires BOTH complete original 2006 retail PE32 DLLs,
all nine exact whole native method SHA256s, and the real PE32 vtable pointer
relocations. No commercial DLLs or full original disassemblies are committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# name, exact original x86 start, complete code bytes, whole code SHA256,
# following INT3 alignment bytes, native returns (offset, encoding),
# sparse original opcode witnesses; intentionally NOT whole original bodies.
METHODS = (
    ("entity_slot_self_link",0x24127C80,7,
     "5a7018f52a7e9b612ef6070e55e66c84d87c73491d4f5ab7a72318192f149305",
     9,((6,"c3"),),{0:"89490c",3:"894908"}),
    ("entity_intrusive_list_clear_heads",0x24127C90,16,
     "9d20fb9737545d0eeb198c79dc5d325beabd6d570a4058d3f1e7f4a887df0403",
     0,((15,"c3"),),{0:"8bc1",2:"c70000000000",8:"c7400400000000"}),
    ("entity_slot_is_linked_by_prev",0x24127CA0,17,
     "1e2f1affbccb04c25e27dafd454fe1ac8c0c0935f714a6dfd7d22ff0c203b67b",
     15,((14,"c20400"),),{
         0:"8b442404",4:"33c9",6:"394008",9:"0f95c1",12:"8ac1"}),
    ("entity_base_virtual_slot0_noop",0x24127CC0,3,
     "1d10894625976bca9d8906f3ecf1e766b94e9219ec3bf1c30e02feea5e18e45a",
     13,((0,"c20800"),),{}),
    ("entity_base_virtual_slot1_noop",0x24127CD0,3,
     "1d10894625976bca9d8906f3ecf1e766b94e9219ec3bf1c30e02feea5e18e45a",
     13,((0,"c20800"),),{}),
    ("entity_slot_entity_pointer_getter",0x24127CE0,3,
     "19d200194d49b34530548575f37a1bb4203ee60f18d42f0084e86b7a8c899faa",
     13,((2,"c3"),),{0:"8b01"}),
    ("entity_intrusive_list_unlink",0x24127CF0,67,
     "74eccc13557d320aae9d1ea67f0f030c3180423ae617a9582ee385c3402cbe44",
     13,((0x31,"c20400"),(0x40,"c20400")),{
         0:"8b442404",4:"8b5008",7:"3bd0",0x13:"89720c",
         0x2B:"89400c",0x3D:"894008"}),
    ("entity_intrusive_list_insert_before",0x24127D40,106,
     "e9faf33f4d1d0dd86a256414e97a9947d7a2f6b1625a70091695833038951359",
     6,((0x61,"c20800"),(0x67,"c20800")),{
         0:"8b442408",4:"8b5008",7:"3bd0",0x38:"8b542408",
         0x43:"8b5104",0x57:"8b5008"}),
    ("entity_intrusive_list_insert_after",0x24127DB0,105,
     "880abd3b6e2202c6f80b688ffc1583ca7e5f0f974e6635b9001f6accc5b6788a",
     7,((0x60,"c20800"),(0x66,"c20800")),{
         0:"8b442408",4:"8b5008",7:"3bd0",0x38:"8b542408",
         0x4C:"8b720c",0x55:"8b500c"}),
)

# Original retail base vtable and the two derived overrides. Also check the
# derived embedded VClientEntityList003 vtable's previously reviewed entries.
VTABLE_EDGES=(
    (0x24298EB4,0x24127CC0),  # base slot0 no-op RET8
    (0x24298EB8,0x24127CD0),  # base slot1 no-op RET8
    (0x242973B8,0x241089D0),  # derived parent slot0 registration
    (0x242973BC,0x24108A70),  # derived parent slot1 unregister
    (0x24297390,0x241056F0),  # embedded entity list virtual slot0
    (0x24297394,0x24105BB0),  # embedded entity list virtual slot1
)

def original_highlow_relocations(pe:PEImage)->set[int]:
    """Parse genuine original PE32 IMAGE_REL_BASED_HIGHLOW relocations."""
    data=pe.data
    if len(data)<0x100 or data[:2]!=b"MZ":
        raise VerificationError("Phase249 original client missing PE DOS header")
    pe_offset=struct.unpack_from("<I",data,0x3C)[0]
    if data[pe_offset:pe_offset+4]!=b"PE\x00\x00":
        raise VerificationError("Phase249 original client missing PE signature")
    optional=pe_offset+24
    if struct.unpack_from("<H",data,optional)[0]!=0x10B:
        raise VerificationError("Phase249 original client is not PE32")
    reloc_rva,reloc_size=struct.unpack_from(
        "<II",data,optional+96+5*8)
    if not reloc_rva or reloc_size<8 or reloc_size>len(data):
        raise VerificationError("Phase249 invalid original relocation directory")
    result:set[int]=set()
    consumed=0
    while consumed+8<=reloc_size:
        entry_va=pe.base+reloc_rva+consumed
        page,block_size=struct.unpack("<II",pe.at(entry_va,8))
        if page==0 and block_size==0:
            break
        if block_size<8 or block_size%2 or consumed+block_size>reloc_size:
            raise VerificationError("Phase249 corrupt original relocation block")
        for pos in range(8,block_size,2):
            value=struct.unpack("<H",pe.at(entry_va+pos,2))[0]
            if (value>>12)==3:
                result.add(pe.base+page+(value&0xFFF))
        consumed+=block_size
    return result

def verify_native(pe:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False,
                  check_relocations:bool=True)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase249 original retail image base mismatch")
    reviewed=[];witness_count=0;exit_count=0;padding_count=0
    for name,va,size,sha,padding,exits,anchors in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+size-1)!=".text":
            raise VerificationError("Phase249 entire function outside native .text: "+name)
        code=pe.at(va,size)
        if check_hash and hashlib.sha256(code).hexdigest()!=sha:
            raise VerificationError("Phase249 whole original method SHA drift: "+name)
        if pe.at(va+size,padding)!=b"\xCC"*padding:
            raise VerificationError("Phase249 original INT3 boundary drift: "+name)
        for off,hex_bytes in exits:
            expected=bytes.fromhex(hex_bytes)
            if code[off:off+len(expected)]!=expected:
                raise VerificationError(f"Phase249 original native return drift: {name}+{off:#x}")
            exit_count+=1
        for off,hex_bytes in anchors.items():
            expected=bytes.fromhex(hex_bytes)
            if code[off:off+len(expected)]!=expected:
                raise VerificationError(f"Phase249 original opcode witness drift: {name}+{off:#x}")
            witness_count+=1
        padding_count+=padding
        reviewed.append({
            "name":name,"entry_va":f"0x{va:08X}",
            "entire_original_code_bytes":size,
            "whole_native_SHA256":sha if check_hash else None,
            "native_exits":len(exits),"following_original_INT3":padding,
        })
    for site,target in VTABLE_EDGES:
        if pe.section_of(site)!=".rdata" or \
                pe.at(site,4)!=struct.pack("<I",target):
            raise VerificationError(f"Phase249 owner-original vtable pointer drift: {site:#x}")
    if check_relocations:
        relocs=original_highlow_relocations(pe)
        for site,target in VTABLE_EDGES:
            if site not in relocs:
                raise VerificationError(f"Phase249 original HIGHLOW vtable relocation missing: {site:#x}")
    return {
        "phase":249,
        "new_original_complete_SHA_methods":len(reviewed)
            if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(m[2] for m in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":reviewed,
        "original_sparse_opcode_witnesses":witness_count,
        "original_native_exit_paths":exit_count,
        "original_INT3_alignment_bytes":padding_count,
        "original_vtable_pointers_verified":len(VTABLE_EDGES),
        "original_HIGHLOW_vtable_relocations_verified":len(VTABLE_EDGES)
            if check_relocations else 0,
        "native_base_vtable_VA":"0x24298EB4",
        "native_derived_parent_vtable_VA":"0x242973B8",
        "native_embedded_entitylist_vtable_VA":"0x24297390",
        "native_slot_stride":16,
        "native_previous_DWORD_offset":8,
        "native_next_DWORD_offset":12,
        "native_list_head_DWORD_offset":0,
        "native_list_tail_DWORD_offset":4,
        "native_base_virtual_slots_0_and_1_are_noop_RET8":True,
        "native_intrusive_list_functions_have_direct_E8_callers":False,
        "limitations":[
            "Native linked-list routines use real 32-bit pointers; host uses safe index tokens.",
            "No original engine virtual methods or client/server independently booted.",
            "The two 3-byte base virtual functions both RET8 without side effects.",
            "No authentic method credit is awarded to synthetic PE fixtures.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for name,blob in (("server",server),("client",client)):
        if hashlib.sha256(blob).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError(
                "Phase249 requires BOTH complete original retail DLL SHA256s: "+name)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=9 or \
            report["new_original_native_code_bytes"]!=327:
        raise VerificationError("Phase249 entire original method accounting drift")
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    g=parser.add_mutually_exclusive_group(required=True)
    g.add_argument("--archive",type=Path)
    g.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    if args.archive:
        if args.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll")!=1 or \
                    z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase249 expects one root original DLL pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:parser.error("--server requires --client")
        result=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
