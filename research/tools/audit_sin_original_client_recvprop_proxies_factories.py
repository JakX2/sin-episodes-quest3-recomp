#!/usr/bin/env python3
"""Phase251: verify the 2006 SiN Episodes client RecvProxy functions and
RecvProp / RecvTable native factories from the owner's retail DLLs.

Authentic native-method credit requires SHA256s of BOTH complete original
client.dll/server.dll and each whole original x86 function body. Do not
commit game DLLs or complete proprietary original native instruction bodies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_client_particle_trail_recvtable_crt import original_pe32_highlow_sites

# name, original 2006 client entry VA, whole native code bytes,
# whole-original-body SHA256, following INT3 bytes, terminal offset/encoding,
# distributed sparse original opcode witnesses (NOT complete native bodies).
METHODS=(
    ("recvtable_trivial_destructor",0x24126A20,1,
     "ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e",
     15,(0x00,"c3"),{0x00:"c3"}),
    ("recvtable_register_three_args",0x24126A30,34,
     "6f9e7546053378e2146df443c8244c8250fa214ff339c5404fefd8c291a05555",
     14,(0x1F,"c20c00"),{
         0x00:"8b442404",0x0C:"895104",0x13:"894108",
         0x19:"884110",0x1F:"c20c00"}),
    ("recvproxy_float_word_copy",0x24126C40,14,
     "4c10fc0974c78557d955abfeaa3ce9ffdcec438069cd51b103595d7cdc569d81",
     2,(0x0D,"c3"),{
         0x00:"8b442404",0x04:"8b4804",0x0B:"890a",0x0D:"c3"}),
    ("recvproxy_vector3_bits_copy",0x24126C50,26,
     "da6843b9ae645f72bcba8cd99e4ddd566ad840ddc6856e598fcd5b1b1e763768",
     6,(0x19,"c3"),{
         0x00:"8b442404",0x0D:"8b5008",0x13:"8b400c",
         0x16:"894108",0x19:"c3"}),
    ("recvproxy_byte_copy",0x24126C70,14,
     "a72672d7f606259b258d1d90e5d914ddcd6375fca84ae77714c2df93a4b8be91",
     2,(0x0D,"c3"),{0x04:"8a4804",0x0B:"880a",0x0D:"c3"}),
    ("recvproxy_word_copy",0x24126C80,16,
     "6ab922c6b5f7991f06f8ddf29061ee9e88fe1704531243b5939be6ae3079ff88",
     0,(0x0F,"c3"),{0x04:"668b4804",0x0C:"66890a",0x0F:"c3"}),
    ("recvproxy_int32_word_copy",0x24126C90,14,
     "4c10fc0974c78557d955abfeaa3ce9ffdcec438069cd51b103595d7cdc569d81",
     2,(0x0D,"c3"),{0x04:"8b4804",0x0B:"890a",0x0D:"c3"}),
    ("recvproxy_type_dispatch_table_ctor",0x24126D00,37,
     "782c9edae7e1e020d98681c5286b604b3b2289c2a5383629ce45e428946ce0fa",
     11,(0x24,"c3"),{
         0x00:"8bc1",0x02:"c700706c1224",
         0x08:"c74004806c1224",0x0F:"c74008906c1224",
         0x16:"c7400c406c1224",0x1D:"c74010506c1224",0x24:"c3"}),
    ("recvtable_default_ctor",0x24126D30,22,
     "2af9482667ace8889976660d8d59a14dc9d46b8fc6a26f464bcbd303af8bcefa",
     10,(0x15,"c3"),{0x00:"8bc1",0x04:"8908",0x0C:"89480c",0x12:"884811",0x15:"c3"}),
    ("recvtable_arg_ctor",0x24126D50,36,
     "c6ebab665525b706f20c7c67603ec975f835084052ad0e3a0883029f54305697",
     12,(0x21,"c20c00"),{
         0x00:"8b542408",0x0A:"8908",0x18:"89500c",0x21:"c20c00"}),
    ("recvprop_float_factory",0x24126D80,106,
     "bd1fa7c7ccdc0a278ee0ad889437803cc195d10aa2bd53040b5b66f5f8b97257",
     6,(0x69,"c3"),{
         0x00:"83ec3c",0x25:"b901000000",
         0x32:"894c2438",0x3A:"8b4c2458",
         0x56:"c7442438ffffffff",0x5E:"89542434",
         0x62:"f3a5",0x69:"c3"}),
    ("recvprop_integer_placeholder_factory",0x24126DF0,146,
     "c292491b77e181d88ab5c65d6c49667f99cbd9fcf82a6be39d62f1af23e7c4b5",
     14,(0x91,"c3"),{
         0x00:"83ec3c",0x0B:"894c2414",
         0x17:"c744243401000000",0x1F:"c7442430ffffffff",
         0x33:"7524",0x39:"83f801",0x3E:"83f802",
         0x43:"b8806c1224",0x4A:"83f804",
         0x4D:"b8906c1224",0x54:"b8706c1224",
         0x8A:"f3a5",0x91:"c3"}),
)

# Original Phase250 direct callers of functions now authenticated here.
# These are not inlined host references, but real retail E8 / E9 encodings.
DIRECT_CALLS=(
    (0x241281F2,0x24126DF0), # placeholder constructor
    (0x2412820C,0x24126D80), # lifetime
    (0x24128226,0x24126D80), # start size
    (0x24128243,0x24126D80), # end size
    (0x2412825C,0x24126A30), # register RecvTable
    (0x24268965,0x24126D30), # CRT table constructor
)
DIRECT_TAIL_JMPS=((0x242736B5,0x24126A20),)
# Exact absolute operands from the retail native proxy-dispatch table and
# placeholder factory fallback branches. Each original site is HIGHLOW.
RELOCATED_POINTERS=(
    (0x24126D04,0x24126C70), # byte
    (0x24126D0B,0x24126C80), # word
    (0x24126D12,0x24126C90), # int32
    (0x24126D19,0x24126C40), # float
    (0x24126D20,0x24126C50), # vector3
    (0x24126E34,0x24126C80), # placeholder byte-width 2
    (0x24126E3E,0x24126C90), # placeholder byte-width 4
    (0x24126E45,0x24126C70), # placeholder fallback byte
)

def rel32(op:int,site:int,target:int)->bytes:
    return bytes([op])+struct.pack("<i",target-site-5)

def verify_native(pe:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False,
                  check_relocations:bool=True)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase251 original client PE32 base mismatch")
    reviewed=[];n_witness=0;n_ret=0;n_pad=0
    for name,va,n,sha,padding,(exit_off,exit_hex),anchors in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+n-1)!=".text":
            raise VerificationError("Phase251 whole native method not in .text: "+name)
        body=pe.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=sha:
            raise VerificationError("Phase251 entire original method SHA drift: "+name)
        if pe.at(va+n,padding)!=b"\xCC"*padding:
            raise VerificationError("Phase251 original INT3 boundary drift: "+name)
        return_bytes=bytes.fromhex(exit_hex)
        if body[exit_off:exit_off+len(return_bytes)]!=return_bytes:
            raise VerificationError("Phase251 native original return drift: "+name)
        for off,hx in anchors.items():
            value=bytes.fromhex(hx)
            if body[off:off+len(value)]!=value:
                raise VerificationError(f"Phase251 native opcode witness drift: {name}+{off:#x}")
            n_witness+=1
        n_ret+=1;n_pad+=padding
        reviewed.append({
            "name":name,"original_entry_va":f"0x{va:08X}",
            "whole_original_native_code_bytes":n,
            "whole_original_SHA256":sha if check_hash else None,
            "native_exit_offset":exit_off,"INT3_following_bytes":padding,
        })
    for op,edges in ((0xE8,DIRECT_CALLS),(0xE9,DIRECT_TAIL_JMPS)):
        for site,target in edges:
            if pe.section_of(site)!=".text" or \
               pe.at(site,5)!=rel32(op,site,target):
                raise VerificationError(f"Phase251 native E8/E9 edge drift at {site:#x}")
    for site,target in RELOCATED_POINTERS:
        if pe.section_of(site)!=".text" or \
           pe.at(site,4)!=struct.pack("<I",target):
            raise VerificationError(f"Phase251 native relocated pointer drift: {site:#x}")
    if check_relocations:
        relocs=original_pe32_highlow_sites(pe)
        for site,target in RELOCATED_POINTERS:
            if site not in relocs:
                raise VerificationError(f"Phase251 original HIGHLOW absent: {site:#x}")
    return {
        "phase":251,
        "new_original_complete_SHA_methods":len(reviewed) if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(x[2] for x in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":reviewed,
        "original_sparse_opcode_witnesses":n_witness,
        "original_native_return_paths":n_ret,
        "original_INT3_alignment_bytes":n_pad,
        "original_direct_E8_edges":len(DIRECT_CALLS),
        "original_tail_E9_edges":len(DIRECT_TAIL_JMPS),
        "original_absolute_proxy_pointers":len(RELOCATED_POINTERS),
        "original_HIGHLOW_proxy_relocations":len(RELOCATED_POINTERS)
            if check_relocations else 0,
        "recvprop_descriptor_bytes":60,
        "recvprop_known_fields":{
            "name_DWORD":0,"type_DWORD":4,"extra_DWORD":8,
            "disabled_BYTE":16,"zero_DWORDs":[20,24,28,36,40],
            "proxy_DWORD":32,"member_offset_DWORD":44,
            "array_index_sentinel_DWORD":48,"flag_DWORD":52,
        },
        "recvtable_known_fields":{
            "first_descriptor_DWORD":0,"count_DWORD":4,
            "opaque_aux_DWORD":8,"table_name_DWORD":12,
            "initialized_bytes":[16,17],
        },
        "original_proxy_source_payload_offset":4,
        "original_float_proxy_is_bitwise_DWORD_copy":True,
        "original_float_factory_does_not_read_4th_size_argument":True,
        "original_placeholder_width_fallback":[
            [1,"byte"],[2,"word"],[4,"int32"],[0,"byte"]
        ],
        "limits":[
            "Original 60-byte factory bodies do not initialize all bytes; unknown bytes stay unknown.",
            "Raw x86 pointers are represented by host tokens, not native heap objects.",
            "Original game DLLs are not independently rebuilt/booted.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for name,blob in (("server",server),("client",client)):
        if hashlib.sha256(blob).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase251 requires BOTH original entire DLL SHA256s: "+name)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=12 or \
       report["new_original_native_code_bytes"]!=466:
        raise VerificationError("Phase251 authentic whole-method inventory drift")
    return report

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    args=p.add_mutually_exclusive_group(required=True)
    args.add_argument("--archive",type=Path)
    args.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    a=p.parse_args()
    if a.archive:
        if a.client:p.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("client.dll")!=1 or \
               z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase251 original pair must be present once at ZIP root")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:p.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
