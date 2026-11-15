#!/usr/bin/env python3
"""Phase243: authenticate original client PVS comparator and dispatch ABI.

This auditor requires the two complete owner-original SiN Episodes: Emergence
(2006) retail PE32 DLL SHA256 hashes before awarding whole-native-method
credit. Never commit original proprietary DLLs or complete x86 function bodies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# label, native VA, entire original code size, complete body SHA256,
# following INT3 alignment, return opcode offsets, sparse opcode witnesses.
METHODS = (
    ("pvs_key_unsigned_first_dword_less", 0x241056C0, 17,
     "77b87ba9017ac5403ea41c488944263dd6475f82ddf3b8f28b36155b9e8f15f4",
     15, {0x10:"c3"}, {
         0x00:"8b442404",0x04:"8b08",0x06:"8b542408",
         0x0A:"3b0a",0x0C:"1bc0f7d8",0x10:"c3",
     }),
    ("pvs_comparator_absolute_global_tail_jump", 0x24105CF0, 6,
     "1c204ed9a7470ce6d00c305cb9c6fe5aede30d0cf03616fc31363d357cb974da",
     10, {}, {0x00:"ff25b8fd3324"}),
    ("pvs_comparator_global_set_or_get", 0x24105760, 20,
     "fe8da08323e50b2b4b05c2b6d87f4798721dce4a1b6d207a000b208a4d792b60",
     12, {0x0D:"c3",0x13:"c3"}, {
         0x00:"8b442404",0x04:"85c0",0x06:"7406",
         0x08:"a3b8fd3324",0x0E:"a1b8fd3324",0x13:"c3",
     }),
    ("pvs_typed_key_composition_and_map_insert_adapter", 0x24108800, 54,
     "90ed1a6166e503620e701e59198499e531b64dec6be6f665edb9fffa6c9bf9d3",
     10, {0x33:"c20800"}, {
         0x00:"83ec08",0x03:"8b44240c",0x07:"8b10",
         0x09:"8b442410",0x0D:"891424",0x10:"668b10",
         0x13:"8b411c",0x18:"6689542404",0x1F:"a3b8fd3324",
         0x24:"6a00",0x2B:"e820ffffff",0x30:"83c408",0x33:"c20800",
     }),
)

CALLS = ((0x2410882B, 0x24108750),)

# Original owner constructor and previously recovered register/unregister.
# The constructor installs a dispatch thunk at the embedded map's first
# DWORD and an unsigned comparator candidate at map+0x1C. The other sites
# copy this candidate to the original global before map operations.
EXTERNAL_WITNESSES = (
    (0x2410822F, "c78660800100f05c1024"),
    (0x24108273, "c7867c800100c0561024"),
    (0x2410881F, "a3b8fd3324"),
    (0x241088EA, "a3b8fd3324"),
    (0x2410894E, "a3b8fd3324"),
)
# Original PE32 HIGHLOW relocation covers each of these VA operands;
# their existence makes the function pointers valid at rebased load sites.
HIGHLOW_SITES = (
    0x24108235,0x24108279,0x24105CF2,
    0x24105769,0x2410576F,0x24108820,
    0x241088EB,0x2410894F,
)

def call_bytes(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def original_pe32_highlow_sites(pe:PEImage)->set[int]:
    data=pe.data
    pe_header=struct.unpack_from("<I",data,0x3C)[0]
    optional=pe_header+24
    reloc_rva,reloc_size=struct.unpack_from("<II",data,optional+96+5*8)
    if not reloc_rva or reloc_size<8 or reloc_size>len(data):
        raise VerificationError("Phase243 missing original PE32 relocation directory")
    start=pe.base+reloc_rva
    consumed=0
    result:set[int]=set()
    while consumed+8<=reloc_size:
        block_va=start+consumed
        page,size=struct.unpack("<II",pe.at(block_va,8))
        if not page and not size:
            break
        if size<8 or size%2 or consumed+size>reloc_size:
            raise VerificationError("Phase243 corrupt original relocation block")
        for off in range(8,size,2):
            value=struct.unpack("<H",pe.at(block_va+off,2))[0]
            reloc_type=value>>12
            if reloc_type==3:
                result.add(pe.base+page+(value&0xFFF))
            elif reloc_type not in (0,):
                # Other PE32 relocation types are not relevant to these
                # absolute 32-bit operands; don't count them as HIGHLOW.
                continue
        consumed+=size
    return result

def verify_native(pe:PEImage, *, check_hash:bool=True,
                  authentic_provenance:bool=False,
                  check_relocations:bool=True)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase243 original client PE image base drift")
    rows=[]
    anchors=0
    for name,va,length,sha,pad,returns,witnesses in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+length-1)!=".text":
            raise VerificationError("Phase243 entire method outside .text: "+name)
        raw=pe.at(va,length)
        if check_hash and hashlib.sha256(raw).hexdigest()!=sha:
            raise VerificationError("Phase243 whole-native-method SHA drift: "+name)
        if pe.at(va+length,pad)!=b"\xCC"*pad:
            raise VerificationError("Phase243 original alignment INT3 drift: "+name)
        for offset,opcodes in returns.items():
            b=bytes.fromhex(opcodes)
            if raw[offset:offset+len(b)]!=b:
                raise VerificationError("Phase243 original return/ABI drift: "+name)
        for offset,hx in witnesses.items():
            b=bytes.fromhex(hx)
            if raw[offset:offset+len(b)]!=b:
                raise VerificationError(f"Phase243 native opcode witness: {name}+{offset:#x}")
            anchors+=1
        rows.append({
            "name":name,"native_entry":f"0x{va:08X}",
            "whole_original_code_bytes":length,
            "whole_original_SHA256":sha if check_hash else None,
            "trailing_INT3_bytes":pad,
            "native_exit_paths":len(returns) if returns else 1,
        })
    for site,target in CALLS:
        if pe.section_of(site)!=".text" or pe.at(site,5)!=call_bytes(site,target):
            raise VerificationError(f"Phase243 original map-insert E8 target drift {site:#x}")
    for site,hx in EXTERNAL_WITNESSES:
        raw=bytes.fromhex(hx)
        if pe.section_of(site)!=".text" or pe.at(site,len(raw))!=raw:
            raise VerificationError(f"Phase243 original owner store drift: {site:#x}")
    if check_relocations:
        relocs=original_pe32_highlow_sites(pe)
        for site in HIGHLOW_SITES:
            if site not in relocs:
                raise VerificationError(f"Phase243 missing original HIGHLOW relocation {site:#x}")
    return {
        "phase":243,
        "new_original_complete_SHA_methods":(
            len(rows) if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes":(
            sum(m[2] for m in METHODS) if check_hash and authentic_provenance else 0
        ),
        "reviewed_original_methods":rows,
        "native_sparse_opcode_witnesses":anchors,
        "native_external_owner_witnesses":len(EXTERNAL_WITNESSES),
        "native_exact_E8_CALLs":len(CALLS),
        "native_PE32_HIGHLOW_sites":len(HIGHLOW_SITES) if check_relocations else 0,
        "map_parent_offset":0x18060,
        "map_comparator_thunk_offset":0x00,
        "map_comparator_candidate_offset":0x1C,
        "global_dispatch_pointer_VA":"0x2433FDB8",
        "original_unsigned_key_field_bytes":4,
        "original_map_stored_key_bytes":8,
        "adapter_initialized_key_bytes":6,
        "adapter_uninitialized_high_key_bytes":2,
        "scope_limits":[
            "Map key comparison uses only first DWORD, unsigned less-than.",
            "Original wrapper copies DWORD+WORD; last two stack key bytes not initialized.",
            "The global thunk may dispatch to other function pointers; host refuses unknown ones.",
            "No independently rebuilt retail Windows gameplay DLL loading demonstrated.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for module,raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[module]["sha256"]:
            raise VerificationError(
                "Phase243 requires BOTH complete original retail DLL SHA256s: "+module)
    result=verify_native(PEImage(client),authentic_provenance=True)
    if result["new_original_complete_SHA_methods"]!=4 or \
            result["new_original_native_code_bytes"]!=97:
        raise VerificationError("Phase243 original review ledger accounting drift")
    return result

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or \
                    z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase243 expects original DLL pair at ZIP root")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server needs --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
