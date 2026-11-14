#!/usr/bin/env python3
"""Phase227: original 2006 SiN runtime singleton startup/teardown evidence.

Proves four native .data static-initializer pointers, constructors,
completion-order vptr stores, teardown callbacks, and PE HIGHLOW sites
against BOTH complete original retail PE32 file hashes. The leaf-system
constructor itself is only SPARSELY witnessed; its unreviewed whole body
is not credited to the recovered method count. No original method bytes
or complete disassembly are redistributed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_core_factory_thunks import FACTORIES, pe_highlow_relocations

# (name, entry VA, complete native length, SHA256 of ALL original code bytes,
#  native INT3 trailing pad, class)
METHODS={
 "server":(
  ("server_movement_initializer",0x223C33F0,32,"c4968533bfb17b863ef43b4238b056c1163f73e3c0d7d41c76329c488d03eb73",0,"initializer"),
  ("server_movement_teardown_wrapper",0x223CF2C0,10,"e62c172da3df757cdcb4b417da1711ba42441d265ba6ddd7038f8ee5f8958a9e",6,"teardown"),
  ("server_movement_constructor",0x221EBF40,58,"b41623a8cfab6393ae2b33ae1d89a290cd4a99528c4568fea25e10c58b89edbf",6,"constructor"),
 ),
 "client":(
  ("client_leaf_initializer",0x24268440,22,"ba92fca69ae0bacce3f0eaf0caaba7371b9ca216aff659d76eb8b67bb0582adb",10,"initializer"),
  ("client_movement_initializer",0x2426E940,32,"d0e6bafd640d3ed0dc19c2a1b7d2f50805ab7103c6105de11d6dfa6fa46e64cf",0,"initializer"),
  ("client_prediction_initializer",0x24271430,22,"cd1e62790abbf1083835c043be37815a772d807424a3d0661ce0669380ed7c79",10,"initializer"),
  ("client_leaf_teardown_wrapper",0x24273500,10,"398f89ccc6255fd4c5a31a22092e38bdfc614326cb4ccd90999e352f3886bfe0",6,"teardown"),
  ("client_movement_teardown_wrapper",0x242757A0,10,"2ea1c0a296a9dc53c9c987b679fbd1594db882bfe585af1495f1b68d17ff39b7",6,"teardown"),
  ("client_prediction_teardown_wrapper",0x24275E50,10,"c6a242c2156899d53244328a8edc28bb1d1ae2b79c12cacf44cde38782098651",6,"teardown"),
  ("client_movement_constructor",0x24136F80,58,"4d6388f8810aac5e43bf305ad75231f3e48ab2a5b77e77179e6364083f9fac24",6,"constructor"),
  ("client_prediction_constructor",0x24195A10,50,"9b0b5c77c72ae369a286600971788b3820d79ffff740fc6c7721b3fc5277f8a7",14,"constructor"),
 ),
}

# Native .data function pointer array entry, original startup wrapper VA,
# original singleton, direct constructor, teardown wrapper and cleanup
# method, native original registration helper. Vptr listed before and
# after the initializer's final native instruction.
LIFECYCLES={
 "server":(
  ("GameMovement001",0x224D7CC0,0x223C33F0,0x225934B0,0x221EBF40,
   0x223CF2C0,0x221EAAC0,0x223A5639,
   0x224433D4,0x2245EC04),
 ),
 "client":(
  ("ClientLeafSystem001",0x242F39AC,0x24268440,0x24357F30,0x2410C790,
   0x24273500,0x2410CA80,0x24248788,
   0x2429756C,0x2429756C),
  ("GameMovement001",0x242F4304,0x2426E940,0x24379920,0x24136F80,
   0x242757A0,0x24136270,0x24248788,
   0x2429A684,0x242B1B5C),
  ("VClientPrediction001",0x242F480C,0x24271430,0x2437FA00,0x24195A10,
   0x24275E50,0x24195700,0x24248788,
   0x242A363C,0x242A363C),
 ),
}
# Native direct CALL instructions and cleanup wrapper JMP instructions.
# Direct call to the original leaf constructor does not imply we have
# reviewed its complete function body.
CALLS={
 "server":((0x223C33F5,0x221EBF40),(0x223C3409,0x223A5639)),
 "client":(
  (0x24268445,0x2410C790),(0x2426844F,0x24248788),
  (0x2426E945,0x24136F80),(0x2426E959,0x24248788),
  (0x24271435,0x24195A10),(0x2427143F,0x24248788),
 ),
}
TEARDOWN_JMPS={
 "server":((0x223CF2C5,0x221EAAC0),),
 "client":(
  (0x24273505,0x2410CA80),
  (0x242757A5,0x24136270),
  (0x24275E55,0x24195700),
 ),
}
# Exact selective native constructor initial vptr writes. The client
# leaf constructor is a larger, still-unreviewed method and only these
# three observed vptr-word writes may be claimed.
INITIAL_VPTR_WRITES={
 "server":((0x221EBF6E,0x224433D4),),
 "client":(
  (0x2410C7A0,0x2429756C),
  (0x2410C7A6,0x24297534),
  (0x2410C7AD,0x2429752C),
  (0x24136FAE,0x2429A684),
  (0x24195A14,0x242A363C),
 ),
}
# Each object lies in virtual, unbacked .data; even so its vtable lives
# in the file-backed .rdata, and its first virtual target is executable.
# The original runtime image relocates these absolute address constants.
def call32(site:int,dest:int,op:int=0xE8)->bytes:
    return bytes([op])+struct.pack("<i",dest-site-5)

def checked_u32(pe:PEImage,va:int)->int:
    return struct.unpack("<I",pe.at(va,4))[0]

def verify_module(pe:PEImage,module:str, *,
        original_provenance_verified:bool=False,
        check_hash:bool=True,
        relocated_sites:set[int]|None=None)->dict:
    if module not in METHODS or pe.base!=ORIGINALS[module]["base"]:
        raise VerificationError("Phase227 original PE32 image base mismatch")
    if relocated_sites is None:
        relocated_sites=pe_highlow_relocations(pe)
    summary=[]
    for label,va,n,digest,pad,kind in METHODS[module]:
        if pe.section_of(va)!=".text" or pe.section_of(va+n-1)!=".text":
            raise VerificationError("Phase227 original native body outside .text: "+label)
        raw=pe.at(va,n)
        if check_hash and hashlib.sha256(raw).hexdigest()!=digest:
            raise VerificationError("Phase227 original complete native SHA256 changed: "+label)
        if raw[-1:]!=b"\xc3" and kind!="teardown":
            raise VerificationError("Phase227 original native C3 return drift: "+label)
        if kind=="teardown" and raw[5:6]!=b"\xe9":
            raise VerificationError("Phase227 original x86 teardown tail-JMP drift: "+label)
        if pe.at(va+n,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase227 exact original INT3 code-boundary drift: "+label)
        summary.append({"name":label,"entry_va":f"0x{va:08X}",
                        "complete_original_bytes":n,
                        "whole_native_sha256":digest if check_hash else None,
                        "original_INT3_padding":pad,"kind":kind})
    for site,dest in CALLS[module]:
        if pe.at(site,5)!=call32(site,dest):
            raise VerificationError("Phase227 original initializer/atexit CALL drift: "+hex(site))
    for site,dest in TEARDOWN_JMPS[module]:
        if pe.at(site,5)!=call32(site,dest,0xE9):
            raise VerificationError("Phase227 original teardown tail JMP drift: "+hex(site))
    for site,vptr in INITIAL_VPTR_WRITES[module]:
        prefix=pe.at(site,2)
        if prefix in (b"\xc7\x00",b"\xc7\x02"):
            size=6  # MOV [eax]/[edx], vtable VA
        elif pe.at(site,3) in (b"\xc7\x40\x04",b"\xc7\x40\x08"):
            size=7  # MOV [eax+4]/[eax+8], vtable VA
        else:
            raise VerificationError("Phase227 original constructor vptr MOV opcode drift")
        encoded=int.from_bytes(pe.at(site+size-4,4),"little")
        if encoded!=vptr:
            raise VerificationError("Phase227 constructor initial vptr immediate drift")
        if site+size-4 not in relocated_sites:
            raise VerificationError("Phase227 missing original constructor vptr HIGHLOW relocation")
    rows=[]
    factory_names={n:(factory,singleton) for n,factory,singleton,*_ in FACTORIES[module]}
    for name,cell,initializer,singleton,constructor,shutdown,shutdown_dest,register,
        initial_vptr,final_vptr in LIFECYCLES[module]:
        if (name not in factory_names or factory_names[name][1]!=singleton):
            raise VerificationError("Phase227 original Phase226 factory singleton disagreement")
        if (pe.section_of(cell)!=".data" or
            checked_u32(pe,cell)!=initializer or cell not in relocated_sites):
            raise VerificationError("Phase227 original .data initializer entry/relocation drift: "+name)
        if pe.section_of(singleton)!=".data":
            raise VerificationError("Phase227 runtime singleton not in original virtual .data")
        try:
            pe.at(singleton,4)
        except VerificationError:
            pass
        else:
            raise VerificationError("Phase227 expected singleton to be unbacked, not raw file initialized")
        # Verify init wrapper ECX and destructor callback arguments, and
        # that an actual direct CALL targets original registration code.
        if pe.at(initializer,5)!=b"\xb9"+struct.pack("<I",singleton):
            raise VerificationError("Phase227 original initializer thiscall ECX drift")
        push_offset=10 if initial_vptr==final_vptr else 10
        if pe.at(initializer+push_offset,5)!=b"\x68"+struct.pack("<I",shutdown):
            raise VerificationError("Phase227 native cleanup callback address drift")
        if pe.at(shutdown,5)!=b"\xb9"+struct.pack("<I",singleton):
            raise VerificationError("Phase227 original teardown ECX drift")
        if pe.section_of(final_vptr)!=".rdata":
            raise VerificationError("Phase227 final native vtable outside .rdata")
        method=checked_u32(pe,final_vptr)
        if pe.section_of(method)!=".text":
            raise VerificationError("Phase227 final vtable slot0 not original executable")
        if final_vptr not in relocated_sites:
            raise VerificationError("Phase227 missing HIGHLOW on original final vtable slot0")
        if initial_vptr!=final_vptr:
            # Native MOV DWORD [absolute singleton VA], immediate final
            # vptr, after original constructor and before registration.
            site=initializer+15
            if pe.at(site,2)!=b"\xc7\x05" or (
                checked_u32(pe,site+2)!=singleton or
                checked_u32(pe,site+6)!=final_vptr):
                raise VerificationError("Phase227 original post-ctor vptr overwrite drift")
            if site+2 not in relocated_sites or site+6 not in relocated_sites:
                raise VerificationError("Phase227 native overwrite HIGHLOW relocation drift")
        rows.append({
            "interface":name,"static_initializer_pointer_cell":f"0x{cell:08X}",
            "initializer_va":f"0x{initializer:08X}",
            "runtime_singleton":f"0x{singleton:08X}",
            "constructor_call":f"0x{constructor:08X}",
            "registered_teardown_wrapper":f"0x{shutdown:08X}",
            "teardown_method":f"0x{shutdown_dest:08X}",
            "shutdown_registration_call_target":f"0x{register:08X}",
            "constructor_first_vptr":f"0x{initial_vptr:08X}",
            "vptr_after_initializer":f"0x{final_vptr:08X}",
            "final_vtable_slot0":f"0x{method:08X}",
            "initializer_cell_is_HIGHLOW_relocated":True,
        })
    # Only the three small constructors are individually complete-SHA
    # reviewed. Leaf constructor at 0x2410C790 remains an opaque body.
    return {
        "module":module,
        "new_original_SHA_verified_methods":len(summary)
            if check_hash and original_provenance_verified else 0,
        "new_original_complete_native_bytes":sum(row[2] for row in METHODS[module])
            if check_hash and original_provenance_verified else 0,
        "complete_method_evidence":summary,
        "recovered_initializer_paths":rows,
        "direct_constructor_and_registration_CALLs":len(CALLS[module]),
        "teardown_tail_JMPs":len(TEARDOWN_JMPS[module]),
        "native_initial_vptr_writes":len(INITIAL_VPTR_WRITES[module]),
        "original_relocated_initializer_entries":len(LIFECYCLES[module]),
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    for mod,data in (("server",server_bytes),("client",client_bytes)):
        if hashlib.sha256(data).hexdigest()!=ORIGINALS[mod]["sha256"]:
            raise VerificationError("Phase227 full original DLL SHA256 provenance mismatch: "+mod)
    server=verify_module(PEImage(server_bytes),"server",original_provenance_verified=True)
    client=verify_module(PEImage(client_bytes),"client",original_provenance_verified=True)
    if (server["new_original_SHA_verified_methods"]!=3 or
        client["new_original_SHA_verified_methods"]!=8):
        raise VerificationError("Phase227 expected exactly eleven newly reviewed functions")
    return {
        "phase":227,
        "new_full_SHA_original_native_methods":11,
        "new_native_x86_bytes":314,
        "startup_paths_with_original_data_pointer_and_teardown":4,
        "remaining_untraced_unbacked_singletons":[
            "server HLTVDirector001","client VClientEntityList003"],
        "unreviewed_nested_native_constructor":[
            "client ClientLeafSystem001 full ctor 0x2410C790"],
        "server":server,"client":client,
        "limits":[
            "These are four proven object initializer paths, not complete runtime object/vtable implementation.",
            "The original leaf constructor is only witnessed at selected vptr stores; no whole-method SHA review or full behavior is claimed.",
            "Static .data initializer pointers do not establish exact order of CRT callbacks without tracing the CRT dispatch.",
            "Original shutdown registration routine is identified by native direct CALL, but its internals are not reconstructed.",
            "HLTVDirector001 and VClientEntityList003 still lack a verified equivalent static initializer chain.",
            "Windows original engine loading reconstructed client.dll/server.dll has not been demonstrated.",
        ]
    }

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    source=ap.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    ap.add_argument("--client",type=Path)
    args=ap.parse_args()
    if args.archive:
        if args.client:ap.error("--archive cannot be paired with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner archive requires one original root client/server DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:ap.error("--server requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
