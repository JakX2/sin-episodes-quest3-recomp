#!/usr/bin/env python3
"""Phase248: original SiN client entity-list constructor and registration.

Grants authentic original-method review credit ONLY after both entire owner-
original 2006 retail PE32 DLL SHA256 hashes match all five whole method SHA256
digests. No original game DLLs, disassembly bodies or native code are committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Name, original client VA, exact whole native bytes, entire code SHA256,
# following INT3 padding, complete native exit offset and encoding,
# isolated sparse opcode witnesses. None are whole original code bodies.
METHODS=(
    ("base_entitylist_constructor",0x24127E80,200,
     "3bd028ccda4ac019dbf491fbc834c2fc7713baeae2e4253e84702131e8882d3e",
     8,(0xC7,"c3"),{
         0x000:"53555633ed",0x008:"c707b48e2924",
         0x00E:"89af04000100",0x014:"89af08000100",
         0x01A:"89af0c000100",0x020:"89af10000100",
         0x026:"8d7704",0x029:"bb00100000",
         0x030:"89760c897608",0x036:"e8d50c0800",
         0x03B:"25ff7f0000",0x04B:"75e3",
         0x04D:"8d871c800000",0x053:"beff070000",
         0x0C3:"5f5e5d5bc3",
     }),
    ("register_explicit_entity_slot",0x24127F50,106,
     "9fc4b5d7f510991579d995c6de2ff6f3e4a5685909e99fb59f295ff05874d3e8",
     6,(0x67,"c21000"),{
         0x000:"538b5c240c",0x005:"558b6c2414",
         0x00A:"568bc5c1e004",0x012:"578d7c3004",
         0x017:"8b442420",0x01B:"83f8ff",
         0x01E:"891f",0x020:"7403",0x022:"894704",
         0x025:"578d8e04000100",0x02C:"e89ffeffff",
         0x031:"8b4f04",0x034:"8b13",
         0x036:"c1e10c",0x039:"0bcd",
         0x044:"8bcb",0x046:"ff5204",
     }),
    ("register_explicit_forward_return_output",0x241280A0,32,
     "d94b9d0d4d8e4bc3c2f0b807c25f9e2219cc0c28f279b1e08f2169c2ab05a266",
     0,(0x1D,"c21000"),{
         0x000:"8b442410",0x004:"8b54240c",
         0x008:"56",0x009:"8b742408",
         0x00D:"50",0x015:"e896feffff",
         0x01A:"8bc65e",
     }),
    ("register_from_high_free_slot",0x241280C0,181,
     "e762b3bfac73a8566578848cfc8d7e7fcb8a285b1558789ae29b05aa24870583",
     11,(0xB2,"c20800"),{
         0x000:"53555657",0x004:"8bf9",
         0x006:"8bb70c000100",0x00C:"85f6",0x00E:"750e",
         0x010:"68bc8e2924",0x015:"ff15fc612724",
         0x01B:"83c404",0x01E:"8b4608",
         0x021:"3bc6",0x023:"7433",
         0x058:"8b6c2418",0x076:"e8e5fcffff",
     }),
    ("rand_serial_0_to_32767_vstdlib_import",0x241A8B90,17,
     "13be98bd3c171a6acbe7e5e895939b352b95778c23860569cb51ea9c328739d6",
     15,(0x10,"c3"),{
         0x000:"68ff7f0000",0x005:"6a00",
         0x007:"ff1578622724",0x00D:"83c408",
     }),
)
# Four direct edges within the new original methods, one caller from
# original Phase228 entity-list parent constructor, a registration
# wrapper caller and six real 2006 client gameplay callsites.
CALLS=(
    (0x24127EB6,0x241A8B90),
    (0x24127F7C,0x24127E20),
    (0x241280B5,0x24127F50),
    (0x24128136,0x24127E20),
    (0x24108193,0x24127E80),
    (0x240C9002,0x241280A0),
    (0x240CAD84,0x241280C0),
    (0x240CB0AC,0x241280C0),
    (0x240DBD05,0x241280C0),
    (0x240DC2D0,0x241280C0),
    (0x240E4940,0x241280C0),
    (0x240E5C7A,0x241280C0),
)
# Genuine import-call instruction encodings, not fabricated stubs.
IMPORTED_IAT_CALLS=(
    (0x241A8B97,"ff1578622724"),  # vstdlib.dll RandomInt import
    (0x241280D5,"ff15fc612724"),  # tier0.dll Error on exhausted high free
)

def e8(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def verify_native(pe:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase248 original client PE image base mismatch")
    rows=[];witness_count=0
    for name,va,size,sha,pad,exit_path,witnesses in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+size-1)!=".text":
            raise VerificationError("Phase248 original function outside .text: "+name)
        body=pe.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=sha:
            raise VerificationError("Phase248 whole-original-native-code SHA drift: "+name)
        if pe.at(va+size,pad)!=b"\xCC"*pad:
            raise VerificationError("Phase248 original INT3 alignment drift: "+name)
        off,hx=exit_path
        expected=bytes.fromhex(hx)
        if body[off:off+len(expected)]!=expected:
            raise VerificationError("Phase248 original native return encoding drift: "+name)
        for off,hx in witnesses.items():
            expected=bytes.fromhex(hx)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError(f"Phase248 original sparse opcode drift: {name}+{off:#x}")
            witness_count+=1
        rows.append({"name":name,"original_VA":f"0x{va:08X}",
                     "entire_native_bytes":size,
                     "whole_native_code_sha256":sha if check_hash else None,
                     "following_original_INT3":pad})
    for site,target in CALLS:
        if pe.section_of(site)!=".text" or pe.at(site,5)!=e8(site,target):
            raise VerificationError(f"Phase248 original direct E8 rel32 edge drift: {site:#x}")
    for site,hx in IMPORTED_IAT_CALLS:
        if pe.section_of(site)!=".text" or pe.at(site,len(bytes.fromhex(hx)))!=bytes.fromhex(hx):
            raise VerificationError(f"Phase248 native original IAT call drift: {site:#x}")
    return {
        "phase":248,
        "new_original_complete_SHA_methods":len(rows) if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(m[2] for m in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":rows,
        "original_sparse_opcode_witnesses":witness_count,
        "original_direct_E8_rel32_edges":len(CALLS),
        "original_verified_IAT_calls":len(IMPORTED_IAT_CALLS),
        "slot_stride_bytes":16,
        "slot_count":4096,
        "initial_high_free_index_start":2049,
        "initial_high_free_index_end":4095,
        "initial_high_free_count":2047,
        "serial_mask":0x7FFF,
        "explicit_serial_keep_sentinel":0xFFFFFFFF,
        "original_random_int_implementation":"vstdlib.dll!RandomInt",
        "original_random_int_min":0,
        "original_random_int_max":32767,
        "high_free_exhaustion_original":"tier0.dll!Error; CBaseEntityList::AddNonNetworkableEntity: no free slots!",
        "runtime_game_module_independently_loadable":False,
    }

def audit(server:bytes,client:bytes)->dict:
    for name,blob in (("server",server),("client",client)):
        if hashlib.sha256(blob).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase248 requires BOTH complete original retail DLL SHA256s: "+name)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=5 or \
            report["new_original_native_code_bytes"]!=536:
        raise VerificationError("Phase248 authentic full-code ledger drift")
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    a=parser.parse_args()
    if a.archive:
        if a.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase248 expects owner-original DLL pair at archive root")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:parser.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
