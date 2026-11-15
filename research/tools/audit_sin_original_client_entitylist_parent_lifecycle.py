#!/usr/bin/env python3
"""Phase231: verify original client.dll entity-list parent vtable lifecycle.

Read-only owner-original SHA256 gate across BOTH 2006 SiN Episodes PE32
DLLs. Never outputs full original proprietary method bytes/disassembly.
Three methods belong to parent vtable 0x242973B8, NOT the separate
VClientEntityList003 embedded-interface vtable at 0x24297390.
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

PARENT_VTABLE_VA=0x242973B8
EMBEDDED_VTABLE_VA=0x24297390
CONSTRUCTOR_FINAL_PARENT_VPTR_STORE=0x241081A2
PARENT_NATIVE_OBJECT_VA=0x2433FDC8

# label, original native method VA, complete original x86 region length,
# SHA256 of ENTIRE original method body, INT3 padding length, native RET4
# instruction offsets, selected SPARSE complete-opcode witnesses.
METHODS=(
 ("parent_slot0_register_entity",0x241089D0,157,
  "4c3874fcb23e149e035f7b7c853ae45b2c89c4cb5f9c64d7dc95ba4debfddd4e",3,
  (0x8A,0x9A),{
    0x00:"53",0x07:"8b7c2414",0x0B:"81e7ff0f0000",
    0x1D:"83861800010001",0x36:"ff5010",
    0x39:"8984fe28000100",0x4C:"ff521c",
    0x69:"668984fe2c000100",0x75:"ff92c4000000",
    0x7B:"f6c404",0x80:"83862000010001",
    0x8D:"66c784fe2c000100ffff",
  }),
 ("parent_slot1_unregister_entity",0x24108A70,161,
  "0d6734b30784e88c1e0aee96c4eef76cae0aa221f95086a62838bd37743996f0",15,
  (0x9E,),{
    0x00:"535657",0x07:"81e7ff0f0000",
    0x11:"81ff00080000",0x19:"c784fe2800010000000000",
    0x24:"838618000100ff",0x2B:"3bbe24000100",
    0x48:"ff501c",0x53:"ff92c4000000",
    0x59:"f6c404",0x5E:"838620000100ff",
    0x65:"8d9cfe2c000100",0x6C:"668b3b",
    0x98:"66c703ffff",
  }),
 ("parent_slot2_deleting_destructor",0x241082A0,30,
  "069ece106e6326af9c4449dc188f4a08c0d1b2e5479cad2a53dd79ea29a2d191",2,
  (0x1B,),{
    0x00:"568bf1",0x08:"f644240801",
    0x0D:"7409",0x18:"8bc65e",
  }),
)

# Real original direct E8 CALL rel32 instructions inside these 3 whole
# methods. Targets are NOT newly reviewed original bodies here.
DIRECT_CALLS=(
 (0x24108A13,0x24108880), # registry/callback helper
 (0x24108A34,0x24108680), # secondary entity auxiliary slot acquisition
 (0x24108AA3,0x24105B20), # already SHA-reviewed Phase230 highest index
 (0x24108AAF,0x24108910), # unregister helper
 (0x24108AEF,0x24106C20), # secondary auxiliary slot release
 (0x241082A3,0x24107430), # parent destructor, not new whole review
 (0x241082B0,0x2416FD60), # conditional memory deallocation
)
VIRTUAL_SLOTS=((0,0x241089D0),(1,0x24108A70),(2,0x241082A0))
NATIVE_INDEX_MASK=0xFFF
MIRROR_LIVE_SLOT_LIMIT=0x800
MIRROR_ENTRY_STRIDE=8
MIRROR_POINTER_OFFSET=0x10028
MIRROR_AUX_HANDLE_OFFSET=0x1002C
LIVE_COUNT_OFFSET=0x10018
HIGH_WATER_OFFSET=0x10024
SECONDARY_FLAG_COUNT_OFFSET=0x10020
SECONDARY_FLAG_BIT=0x400
AUXILIARY_STORAGE_OFFSET=0x18028

def direct_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage, *,
                  check_hash:bool=True,
                  original_provenance_verified:bool=False,
                  relocated_sites:set[int]|None=None)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase231 requires original SiN PE32 client base")
    chain=CHAINS["client"]
    if chain[3]!=PARENT_NATIVE_OBJECT_VA or chain[4]!=0x10014 or (
        chain[10]!=EMBEDDED_VTABLE_VA):
        raise VerificationError("Phase231 Phase228 embedded-parent factory contract drift")
    if relocated_sites is None:
        relocated_sites=pe_highlow_relocations(client)
    if PARENT_VTABLE_VA==EMBEDDED_VTABLE_VA:
        raise VerificationError("Phase231 parent and embedded vtables must be distinct")
    if (client.section_of(PARENT_VTABLE_VA)!=".rdata" or
        client.at(CONSTRUCTOR_FINAL_PARENT_VPTR_STORE,6)!=(
            b"\xc7\x06"+struct.pack("<I",PARENT_VTABLE_VA)) or
        CONSTRUCTOR_FINAL_PARENT_VPTR_STORE+2 not in relocated_sites):
        raise VerificationError("Phase231 constructor parent vptr store/HIGHLOW drift")
    summary=[]
    witness_count=0
    for label,entry,n,digest,padding,ret4,witnesses in METHODS:
        if (client.section_of(entry)!=".text" or
            client.section_of(entry+n-1)!=".text"):
            raise VerificationError("Phase231 original body outside .text: "+label)
        body=client.at(entry,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase231 complete original native SHA drift: "+label)
        if client.at(entry+n,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase231 original INT3 method boundary changed: "+label)
        for off in ret4:
            if body[off:off+3]!=b"\xc2\x04\x00" and (
                # register/unregister use RET 8, deleting destructor RET 4
                body[off:off+3]!=b"\xc2\x08\x00"):
                raise VerificationError("Phase231 native callee cleanup RET drift: "+label)
            expected=b"\xc2\x04\x00" if "destructor" in label else b"\xc2\x08\x00"
            if body[off:off+3]!=expected:
                raise VerificationError("Phase231 native stack-pop ABI mismatch: "+label)
        for off,hex_witness in witnesses.items():
            w=bytes.fromhex(hex_witness)
            if body[off:off+len(w)]!=w:
                raise VerificationError("Phase231 sparse original instruction witness drift: "+
                                        label+" "+hex(off))
            witness_count+=1
        summary.append({
           "name":label,
           "entry_va":f"0x{entry:08X}",
           "whole_original_native_bytes":n,
           "complete_native_sha256":digest if check_hash else None,
           "native_ret_cleanup":4 if "destructor" in label else 8,
           "native_return_paths":len(ret4),
           "trailing_INT3_padding":padding,
        })
    for slot,va in VIRTUAL_SLOTS:
        cell=PARENT_VTABLE_VA+4*slot
        if (client.section_of(cell)!=".rdata" or
            client.at(cell,4)!=struct.pack("<I",va) or
            cell not in relocated_sites):
            raise VerificationError("Phase231 original relocated parent vtable slot drift: "+str(slot))
    for site,dest in DIRECT_CALLS:
        if (client.section_of(site)!=".text" or
            client.at(site,5)!=direct_call(site,dest)):
            raise VerificationError("Phase231 original real game CALL rel32 drift: "+hex(site))
    return {
       "phase":231,"original_parent_vtable":f"0x{PARENT_VTABLE_VA:08X}",
       "original_embedded_interface_vtable":f"0x{EMBEDDED_VTABLE_VA:08X}",
       "new_original_full_SHA_methods":len(summary)
           if check_hash and original_provenance_verified else 0,
       "new_original_complete_x86_bytes":sum(m[2] for m in METHODS)
           if check_hash and original_provenance_verified else 0,
       "method_evidence":summary,
       "original_native_opcode_witnesses":witness_count,
       "original_parent_vtable_HIGHLOW_slots":len(VIRTUAL_SLOTS),
       "native_direct_CALLs_into_other_methods":len(DIRECT_CALLS),
       "observed_original_offsets":{
           "handle_index_mask":hex(NATIVE_INDEX_MASK),
           "mirror_guard_max_exclusive":hex(MIRROR_LIVE_SLOT_LIMIT),
           "mirror_entry_stride":MIRROR_ENTRY_STRIDE,
           "mirror_entity_ptr":hex(MIRROR_POINTER_OFFSET),
           "mirror_aux_index":hex(MIRROR_AUX_HANDLE_OFFSET),
           "entity_count":hex(LIVE_COUNT_OFFSET),
           "highest_live_slot":hex(HIGH_WATER_OFFSET),
           "secondary_flag_count":hex(SECONDARY_FLAG_COUNT_OFFSET),
           "secondary_flag_bit":hex(SECONDARY_FLAG_BIT),
           "auxiliary_storage":hex(AUXILIARY_STORAGE_OFFSET),
       },
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    for dll,raw in (("server",server_bytes),("client",client_bytes)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[dll]["sha256"]:
            raise VerificationError("Phase231 requires BOTH original complete DLL SHA256s: "+dll)
    result=verify_native(PEImage(client_bytes),
                         check_hash=True,original_provenance_verified=True)
    if result["new_original_full_SHA_methods"]!=3 or (
            result["new_original_complete_x86_bytes"]!=348):
        raise VerificationError("Phase231 authentic method inventory mismatch")
    result["scope_limits"]=[
       "Parent vtable three methods mapped; this does not complete all entity virtual callee implementations.",
       "Registration/removal delegate substantial state changes to independently unrecovered native helpers and entity vtables.",
       "Host reference models observable field writes, branch gates and callback event ordering only.",
       "Original PE32 pointer arithmetic and real engine memory are not executed or modeled on invalid input.",
       "Windows retail Source engine loading replacement gameplay DLLs remains unproven.",
    ]
    return result

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    s=p.add_mutually_exclusive_group(required=True)
    s.add_argument("--archive",type=Path)
    s.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive cannot accompany --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("owner archive needs exactly one root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        report=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
