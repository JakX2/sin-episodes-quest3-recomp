#!/usr/bin/env python3
"""Phase228: owner-original embedded Source interface singleton construction.

No original binaries or complete proprietary native function bodies emitted.
Checks BOTH original game DLL SHA256 digests before counting a recovery.
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

# name, complete native region entry, length, SHA256, exact following INT3
METHODS={
  "server":(
    ("hltv_embedded_initializer",0x223C1B40,22,
     "0a8559afff5663dd07f3a188c57dbf67c4400392f671ff5bda47bb26c593db22",10),
    ("hltv_embedded_teardown",0x223CE6A0,10,
     "d828ee3fcabdccea745ae1e1e46471a4758208133e02dc4feee729cd9a959790",6),
    ("hltv_containing_constructor",0x222625E0,158,
     "deb717a3779cf5019a19d8f259009314c670dc5e6bbe4fa36759f8364ab3782a",2),
  ),
  "client":(
    ("client_entitylist_embedded_initializer",0x24268370,22,
     "446b15473c55849da8f446f80f090e459e2e1d6073464871d62803b977e896a6",10),
    ("client_entitylist_embedded_teardown",0x242734C0,10,
     "c6f9ff177371c32624dcd45c17b338cc235c3b9dcc173ddcc1e288bf30e35c08",6),
    ("client_entitylist_containing_constructor",0x24108190,264,
     "c7811e3f10bbf60d9beb9dd54fdc8251fe59445b59d3be0050ae02e3fda209fd",8),
  ),
}
# interface, original relocated .data startup pointer cell, initializer,
# containing object, offset of the returned interface, interface factory,
# complete ctor, teardown wrapper, teardown destination, atexit target,
# final embedded interface vtable, vtable first virtual method.
CHAINS={
 "server":(
  "HLTVDirector001",0x224D7A04,0x223C1B40,0x2257AE98,0x08,
  0x2225F8C0,0x222625E0,0x223CE6A0,0x222626A0,
  0x223A5639,0x22453DA0,0x22262690,
 ),
 "client":(
  "VClientEntityList003",0x242F3998,0x24268370,0x2433FDC8,0x10014,
  0x241056B0,0x24108190,0x242734C0,0x24107430,
  0x24248788,0x24297390,0x241056F0,
 ),
}
# Complete native opcode witnesses for the *full* original constructors.
# Each site writes a relocated i386 vtable address to a subobject.
# (instruction VA, full original MOV encoding, resulting vptr, object offset)
VPTR_WRITES={
 "server":(
  (0x222625E2, "c74004fc134322",0x224313FC,4),
  (0x222625E9, "c74008b43c4522",0x22453CB4,8),
  (0x222625F2, "c700043e4522",0x22453E04,0),
  (0x222625F8, "c74004c83d4522",0x22453DC8,4),
  (0x222625FF, "c74008a03d4522",0x22453DA0,8),
 ),
 "client":(
  (0x24108198, "c7861400010068732924",0x24297368,0x10014),
  (0x241081A2, "c706b8732924",0x242973B8,0),
  (0x241081A8, "c7861400010090732924",0x24297390,0x10014),
 ),
}
NESTED={
 "server":(),
 "client":((0x24108193,0x24127E80),(0x2410828F,0x241061E0)),
}
def branch32(site:int,target:int,opcode:int)->bytes:
    return bytes((opcode,))+struct.pack("<i",target-site-5)

def check_module(pe:PEImage,module:str,*,check_hash:bool=True,
                 full_original_provenance:bool=False,
                 relocated_sites:set[int]|None=None)->dict:
    if module not in CHAINS or pe.base!=ORIGINALS[module]["base"]:
        raise VerificationError("Phase228 original module/base mismatch")
    if relocated_sites is None:
        relocated_sites=pe_highlow_relocations(pe)
    (name,cell,init,parent,offset,factory,ctor,shutdown,destructor,
     registrar,final_vptr,first_method)=CHAINS[module]
    factory_registry={x[0]:x[1:3] for x in FACTORIES[module]}
    if name not in factory_registry or factory_registry[name]!=(factory,parent+offset):
        raise VerificationError("Phase228 original Phase226 interface factory disagreement")
    if (pe.section_of(parent)!=".data" or
        pe.section_of(parent+offset)!=".data"):
        raise VerificationError("Phase228 original embedded interface not in virtual .data")
    for target in (parent,parent+offset):
        try:
            pe.at(target,4)
        except VerificationError:
            pass
        else:
            raise VerificationError("Phase228 original runtime object is not raw file-backed")
    if (pe.section_of(cell)!=".data" or
        pe.at(cell,4)!=struct.pack("<I",init) or cell not in relocated_sites):
        raise VerificationError("Phase228 relocated .data startup pointer drift")
    if pe.at(factory,6)!=b"\xb8"+struct.pack("<I",parent+offset)+b"\xc3":
        raise VerificationError("Phase228 original registered thunk return not embedded interface")
    if factory+1 not in relocated_sites:
        raise VerificationError("Phase228 original factory singleton HIGHLOW missing")
    evidence=[]
    for label,va,n,digest,pad in METHODS[module]:
        if pe.section_of(va)!=".text" or pe.section_of(va+n-1)!=".text":
            raise VerificationError("Phase228 original method outside .text "+label)
        body=pe.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase228 complete native original SHA256 changed: "+label)
        if label.endswith("teardown"):
            if body[:1]!=b"\xb9" or body[5:6]!=b"\xe9":
                raise VerificationError("Phase228 original teardown MOV/JMP shape drift")
        elif body[-1:]!=b"\xc3":
            raise VerificationError("Phase228 original constructor/initializer RET drift")
        if pe.at(va+n,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase228 original INT3 native boundary drift "+label)
        evidence.append({"name":label,"native_entry":f"0x{va:08X}",
                         "complete_original_native_bytes":n,
                         "whole_native_sha256":digest if check_hash else None,
                         "native_INT3_padding":pad})
    if pe.at(init,5)!=b"\xb9"+struct.pack("<I",parent) or init+1 not in relocated_sites:
        raise VerificationError("Phase228 original initializer thiscall pointer changed")
    if pe.at(init+5,5)!=branch32(init+5,ctor,0xE8):
        raise VerificationError("Phase228 original constructor CALL drift")
    if pe.at(init+10,5)!=b"\x68"+struct.pack("<I",shutdown) or init+11 not in relocated_sites:
        raise VerificationError("Phase228 original atexit callback push drift")
    if pe.at(init+15,5)!=branch32(init+15,registrar,0xE8):
        raise VerificationError("Phase228 original atexit registrar CALL drift")
    if pe.at(shutdown,5)!=b"\xb9"+struct.pack("<I",parent) or shutdown+1 not in relocated_sites:
        raise VerificationError("Phase228 teardown uses incorrect containing-object this pointer")
    if pe.at(shutdown+5,5)!=branch32(shutdown+5,destructor,0xE9):
        raise VerificationError("Phase228 teardown native tail JMP drift")
    for site,hex_bytes,ptr,subobject_offset in VPTR_WRITES[module]:
        blob=bytes.fromhex(hex_bytes)
        if pe.at(site,len(blob))!=blob:
            raise VerificationError("Phase228 original native subobject vptr instruction drift "+hex(site))
        imm=site+len(blob)-4
        if pe.at(imm,4)!=struct.pack("<I",ptr) or imm not in relocated_sites:
            raise VerificationError("Phase228 original vptr immediate or HIGHLOW missing")
        if subobject_offset not in (0,4,8,0x10014):
            raise VerificationError("Phase228 invalid original subobject offset")
    if (pe.section_of(final_vptr)!=".rdata" or
        pe.at(final_vptr,4)!=struct.pack("<I",first_method) or
        pe.section_of(first_method)!=".text" or
        final_vptr not in relocated_sites):
        raise VerificationError("Phase228 original final interface vtable slot0 drift")
    for site,dest in NESTED[module]:
        if pe.at(site,5)!=branch32(site,dest,0xE8) or pe.section_of(dest)!=".text":
            raise VerificationError("Phase228 original nested constructor CALL drift "+hex(site))
    return {
        "module":module,"interface":name,
        "embedded_interface_offset":f"0x{offset:X}",
        "containing_object":f"0x{parent:08X}",
        "interface_factory_return":f"0x{parent+offset:08X}",
        "original_initializer_cell":f"0x{cell:08X}",
        "final_vtable":f"0x{final_vptr:08X}",
        "final_vtable_slot0":f"0x{first_method:08X}",
        "complete_method_evidence":evidence,
        "original_vptr_write_witnesses":len(VPTR_WRITES[module]),
        "new_SHA_verified_functions":len(evidence) if check_hash and full_original_provenance else 0,
        "new_whole_native_bytes":sum(x[2] for x in METHODS[module])
           if check_hash and full_original_provenance else 0,
    }

def audit(server:bytes,client:bytes)->dict:
    for mod,buf in (("server",server),("client",client)):
        if hashlib.sha256(buf).hexdigest()!=ORIGINALS[mod]["sha256"]:
            raise VerificationError("Phase228 original whole PE32 provenance failed: "+mod)
    result={mod:check_module(PEImage(buf),mod,full_original_provenance=True)
            for mod,buf in (("server",server),("client",client))}
    if any(x["new_SHA_verified_functions"]!=3 for x in result.values()):
        raise VerificationError("Phase228 original method inventory drift")
    return {
       "phase":228,"new_SHA_verified_functions":6,
       "new_original_native_bytes":486,"remaining_untraced_core_singleton_startup_paths":0,
       **result,
       "limits":[
         "The two factory return values are secondary subobjects, not independent globals.",
         "Complete six native bodies are reviewed, but nested engine constructors' called helpers and full destructors remain outside this review.",
         "CRT startup callback order and actual Windows retail engine gameplay loading are unproven.",
         "Virtual method vtable entries are original pointer evidence, not reconstructed virtual method bodies.",
       ],
    }

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    src=p.add_mutually_exclusive_group(required=True)
    src.add_argument("--archive",type=Path)
    src.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive cannot combine with --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("expected one original root client/server DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        report=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(report,indent=2))
if __name__=="__main__":
    main()
