#!/usr/bin/env python3
"""Phase226: original core Source interface factory thunks and relocations.

Read-only, owner-original dual-DLL SHA gate. Reuses Phase177 full registry
manifest verifier. Phase178's NINE previously catalogued factory thunks are
NOT counted again. Checks PE32 HIGHLOW relocation sites, not just opcodes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError, scan_static_registration_wrappers,
    verify_original_interface_abi,
)

# (original version, thunk VA, static singleton VA, complete SIX-byte
# authentic native function SHA256, initialized vptr VA / zero,
# first vtable function VA / zero, NEW method vs. Phase178 prior review).
# A missing initial vptr means the singleton is in the virtual, unbacked
# portion of .data, NOT that the runtime pointer is necessarily null.
FACTORIES = {
 "server": (
  ("IEffects001",0x22129680,0x224ECE00,"2eac16d4c0cfda2bcb592a089d6762c3c6697740eac955659c0abe3362ed83ef",0x224059B8,0x22129810,True),
  ("ServerGameDLL003",0x221E7630,0x22510650,"d878272797e03ad3c82d0a18ffaceef5be1f25fb3d33dbaf1493c240fefc369b",0x22442EFC,0x221E7640,False),
  ("ServerGameEnts001",0x221E82C0,0x22510654,"81f3c5629ce13db9b52fddf5328c9d8301d28f7d02a510d2108ede5d9d3ea532",0x22443030,0x221E9360,False),
  ("ServerGameClients003",0x221E8320,0x22510658,"c811e01f62d7f1a8ec30e417c67c5e99dbf5f6dd9fa6a7112f92790e4bdece9c",0x22443054,0x221E63F0,False),
  ("PluginHelpersCheck001",0x221FC4F0,0x22511A14,"9171cdfc432102e603561d2be6b8ab08815e29993474946b32f7b5bf03e83d72",0x2244688C,0x221FC500,True),
  ("PlayerInfoManager001",0x22257780,0x2251A048,"e828c0b8f05a5fb5ae09c15fab91f334910e50d6084e79e4faddc65d47addb10",0x22452E64,0x22257860,True),
  ("PlayerInfoManager002",0x222577A0,0x2251A048,"e828c0b8f05a5fb5ae09c15fab91f334910e50d6084e79e4faddc65d47addb10",0x22452E64,0x22257860,True),
  ("BotManager001",0x222577B0,0x2251A04C,"c9d32770c94fb85fde54f6d6d895aa092924ab0574d4242fc104013bbd0b13c5",0x22452E70,0x222578A0,True),
  ("HLTVDirector001",0x2225F8C0,0x2257AEA0,"9a50234d1328f0c8cfd361e0d497e1df4d3a5310eaeb5e7f18f97c5bfb4d689a",0,0,True),
  ("GameMovement001",0x222CFFE0,0x225934B0,"70c5729215c8f7f0d736f6cd9f1c6c780eaa4e2763c0db5d6c881628720f6117",0,0,False),
 ),
 "client": (
  ("GameClientExports001",0x240FD810,0x242FA4B0,"e65c1e17c05a9b36167fe1e9bee4af19b9055e0b9764c299ee84d053f73d2d26",0x24296B18,0x240FE9B0,False),
  ("VClient011",0x240FD840,0x242FA4B8,"72b920d06b2ea5b37dc9fe0b0a8821128ab5d7604d4d4bae35a9f6e01a86aa6c",0x24296C6C,0x240FE0E0,False),
  ("VClientEntityList003",0x241056B0,0x2434FDDC,"af71d0de06e3cbfaf91c55fd1a68e973d49c6998f6dea1b6e834d33aaa465905",0,0,False),
  ("ClientLeafSystem001",0x24108B20,0x24357F30,"9328e36548ae5755e8637e94a5a3712571f822f49b9ea290e973bef7e9ea6de6",0,0,True),
  ("IEffects001",0x24127780,0x242FB554,"13b0ec8347529f2c88a47912819ef52895a47e95b6da8dcd0e85a66a930a99e4",0x24298E54,0x24127890,True),
  ("VCENTERPRINT002",0x241EC540,0x2431DBF0,"81267ad71e6a917e5251cb606e3180b7d36d219e6be716ed03bc97674e9c0aca",0x242ABC4C,0x241EC430,True),
  ("GameMovement001",0x2420FA40,0x24379920,"2d707f00df3baa85babb91c2ae1f759df7e49cb70ce82bc7dc25dbfa57066cde",0,0,False),
  ("VClientPrediction001",0x2422EA30,0x2437FA00,"72a64b5a986505d8284d4487eb23c37528a0d79f7ba3a000a2c484790b33d1d8",0,0,False),
 )
}
INITIALIZED=12
UNBACKED=6

def pe_highlow_relocations(pe:PEImage)->set[int]:
    # PE32 base relocation Data Directory is entry 5 at Optional+96+5*8.
    data=pe.data
    nt=struct.unpack_from("<I",data,0x3c)[0]
    opt=nt+24
    rva,byte_count=struct.unpack_from("<II",data,opt+96+5*8)
    if not rva or byte_count<8:
        raise VerificationError("Phase226 original image has no .reloc directory")
    raw=pe.at(pe.base+rva,byte_count)
    cursor=0
    sites=set()
    while cursor<byte_count:
        if cursor+8>byte_count:
            raise VerificationError("Phase226 truncated relocation block")
        page,size=struct.unpack_from("<II",raw,cursor)
        if size<8 or size%2 or cursor+size>byte_count:
            raise VerificationError("Phase226 malformed relocation block")
        for off in range(cursor+8,cursor+size,2):
            word=struct.unpack_from("<H",raw,off)[0]
            typ=word>>12
            if typ==3:
                sites.add(pe.base+page+(word&0x0fff))
            elif typ!=0:
                raise VerificationError("Phase226 unexpected original PE relocation kind")
        cursor+=size
    return sites

def verify_native_factories(
    pe:PEImage,module:str, *,
    registrations:list[dict]|None=None,
    relocated_sites:set[int]|None=None,
    check_hash:bool=True,
    original_provenance_verified:bool=False
)->dict:
    if module not in FACTORIES or pe.base!=ORIGINALS[module]["base"]:
        raise VerificationError("Phase226 incorrect original DLL or image base")
    if registrations is None:
        registrations=scan_static_registration_wrappers(
            pe,ORIGINALS[module]["constructor"])
    if relocated_sites is None:
        relocated_sites=pe_highlow_relocations(pe)
    by_name={item["interface"]:item for item in registrations}
    if len(by_name)!=len(registrations) or len(registrations)!=ORIGINALS[module]["registrations"]:
        raise VerificationError("Phase226 original registration manifest cardinality drift")
    selected={r[0] for r in FACTORIES[module]}
    expected_other=0 if module=="server" else 47
    if len(selected)!=len(FACTORIES[module]) or len(registrations)-len(selected)!=expected_other:
        raise VerificationError("Phase226 core interfaces / dynamic material proxies classification drift")
    if set(by_name)-selected != {n for n in by_name if n.endswith("_IMaterialProxy003")}:
        raise VerificationError("Phase226 unexpected unclassified native interface factory")
    result=[]
    backed=0
    new=0
    for name,va,singleton,whole_sha,vptr,first_method,is_new in FACTORIES[module]:
        if name not in by_name or by_name[name]["factory_va"]!=va:
            raise VerificationError("Phase226 native interface registration/factory address drift: "+name)
        if pe.section_of(va)!=".text" or pe.section_of(singleton)!=".data":
            raise VerificationError("Phase226 native factory or singleton in wrong PE section: "+name)
        original=pe.at(va,6)
        if original[0]!=0xB8 or original[-1]!=0xC3:
            raise VerificationError("Phase226 singleton thunk is not MOV EAX, immediate; RET: "+name)
        if struct.unpack_from("<I",original,1)[0]!=singleton:
            raise VerificationError("Phase226 original return singleton VA changed: "+name)
        if pe.at(va+6,10)!=b"\xcc"*10:
            raise VerificationError("Phase226 original 10-byte INT3 function boundary drift: "+name)
        if va+1 not in relocated_sites:
            raise VerificationError("Phase226 missing HIGHLOW relocation of singleton return VA: "+name)
        if check_hash and hashlib.sha256(original).hexdigest()!=whole_sha:
            raise VerificationError("Phase226 original complete 6-byte SHA256 drift: "+name)
        if vptr:
            backed+=1
            if (pe.section_of(vptr)!=".rdata" or
                    pe.section_of(first_method)!=".text" or
                    struct.unpack("<I",pe.at(singleton,4))[0]!=vptr or
                    struct.unpack("<I",pe.at(vptr,4))[0]!=first_method):
                raise VerificationError("Phase226 original static singleton/vtable slot0 mismatch: "+name)
            if singleton not in relocated_sites or vptr not in relocated_sites:
                raise VerificationError("Phase226 missing native vptr / virtual slot0 HIGHLOW: "+name)
        else:
            # For these originals the target is in virtual .data beyond
            # SizeOfRawData. Its vptr is not available from the file; do
            # not assert it is zero or that the object is initialized.
            try:
                pe.at(singleton,4)
            except VerificationError:
                pass
            else:
                raise VerificationError("Phase226 runtime-only singleton unexpectedly file-backed: "+name)
        if is_new:new+=1
        result.append({
            "interface":name,
            "factory_va":f"0x{va:08X}",
            "static_singleton_va":f"0x{singleton:08X}",
            "original_factory_full_sha256":whole_sha if check_hash else None,
            "code_bytes":6,"trailing_INT3":10,
            "singleton_imm32_highlow":True,
            "file_backed_vptr":f"0x{vptr:08X}" if vptr else None,
            "vtable_slot0":f"0x{first_method:08X}" if vptr else None,
            "new_function":is_new,
        })
    return {
        "module":module,
        "checked_static_core_factories":len(result),
        "new_whole_SHA_verified_original_methods":new if check_hash and original_provenance_verified else 0,
        "previously_reviewed_Phase178_methods":len(result)-new,
        "new_whole_original_x86_bytes":new*6 if check_hash and original_provenance_verified else 0,
        "file_backed_vptr_and_slot0_evidence":backed,
        "runtime_only_unbacked_singletons":len(result)-backed,
        "all_factory_immediates_highlow_relocated":True,
        "core_factory_metadata":result,
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    # Phase177 authenticates both full original file hashes, export
    # tables, 10/55 registrations, and complete ordered manifest SHA256.
    verify_original_interface_abi(server_bytes,"server")
    verify_original_interface_abi(client_bytes,"client")
    server=verify_native_factories(PEImage(server_bytes),"server",original_provenance_verified=True)
    client=verify_native_factories(PEImage(client_bytes),"client",original_provenance_verified=True)
    if (server["new_whole_SHA_verified_original_methods"]!=6 or
        client["new_whole_SHA_verified_original_methods"]!=3 or
        server["file_backed_vptr_and_slot0_evidence"]!=8 or
        client["file_backed_vptr_and_slot0_evidence"]!=4):
        raise VerificationError("Phase226 original 18-factory evidence inventory drift")
    return {
        "phase":226,"original_PE32_provenance":"Phase177 dual full-SHA and ordered factory manifest",
        "new_whole_SHA_verified_original_methods":9,
        "new_complete_original_x86_bytes":54,
        "core_factories_checked":18,"already_reviewed_Phase178_factories":9,
        "original_STATIC_singleton_vptrs_and_vtables":INITIALIZED,
        "original_runtime_only_unbacked_singletons":UNBACKED,
        "server":server,"client":client,
        "alias":"server PlayerInfoManager001 and PlayerInfoManager002 both return original 0x2251A048",
        "limits":[
            "A singleton return address is NOT a completed native C++ interface object or working engine vtable.",
            "Unbacked .data singleton vptrs cannot be inferred from on-disk DLL bytes.",
            "47 additional client material-proxy registrations allocate objects; they are not singleton thunks.",
            "Native PE HIGHLOW relocations require the Windows loader to rebase absolute addresses.",
            "Real Windows retail game startup with reconstructed gameplay DLLs is still unproven.",
        ],
    }

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive and --client cannot be combined")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner archive requires one original root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        report=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
