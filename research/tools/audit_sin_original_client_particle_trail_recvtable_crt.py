#!/usr/bin/env python3
"""Phase250: authenticate original SiN Episodes (2006) EntityParticleTrailInfo
client RecvTable initialization and CRT constructor/destructor glue.

Only exact owner-original whole client.dll AND server.dll SHA256s permit
native method credit. Proprietary complete method bytes/disassemblies are
never emitted or committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# name, native entry, complete body byte count, whole original code SHA256,
# following INT3, native exit (offset, opcode), distributed sparse witnesses.
METHODS = (
    ("dt_particle_trail_info_lazy_recvtable_initializer",0x241281D0,151,
     "e698bb21d94f55d82c0ba326c696c30b94be1cf7c2f3817f9bf0f2762614173b",
     9,(0x96,"c3"),{
         0x00:"f6054010362401",0x07:"7572",
         0x09:"830d4010362401",0x10:"6a006a006a046a00",
         0x18:"68d8622724",0x1D:"68500f3624",
         0x27:"68406c1224",0x32:"68808e2724",
         0x37:"688c0f3624",0x4C:"68e48d2724",
         0x51:"68c80f3624",0x5B:"83c448",
         0x69:"68d88d2724",0x6E:"6804103624",
         0x78:"83c418",0x7B:"68f88e2924",
         0x80:"6a03",0x82:"688c0f3624",
         0x87:"b944103624",0x91:"b801000000",
         0x96:"c3",
     }),
    ("dt_particle_trail_info_crt_termination_registration",0x24268960,22,
     "24f60e9f71ec970f8cbe34bcd6922716367b5df0e3b7d42337b0e40d3126d9ba",
     10,(0x15,"c3"),{
         0x00:"b944103624",0x0A:"68b0362724",
         0x14:"59",0x15:"c3",
     }),
    ("dt_particle_trail_info_crt_initializer_wrapper",0x24268980,16,
     "9066c780197419033104792af48e29300c27fe14781d802f78bbbc74a81556ec",
     0,(0x0F,"c3"),{
         0x00:"6a00",0x07:"83c404",
         0x0A:"a358103624",0x0F:"c3",
     }),
    ("dt_particle_trail_info_crt_cleanup_thunk",0x242736B0,10,
     "4861e23fe970b18f32c9effba6e5d18711adce11b391326c118c157668179b6c",
     6,(0x05,"e96633ebff"),{0x00:"b944103624"}),
)

# Exact authentic original retail x86 direct rel32 CALL/JMP relationships.
DIRECT_CALLS=(
    (0x241281F2,0x24126DF0), # placeholder property constructor
    (0x2412820C,0x24126D80), # m_flLifetime
    (0x24128226,0x24126D80), # m_flStartSize
    (0x24128243,0x24126D80), # m_flEndSize
    (0x2412825C,0x24126A30), # register table (3 real properties)
    (0x24268965,0x24126D30), # construct destination table object
    (0x2426896F,0x24248788), # register CRT cleanup handler
    (0x24268982,0x241281D0), # static initializer -> network table
)
TAIL_JUMPS=((0x242736B5,0x24126A20),)

# All 16 absolute address operands inside native network initializer,
# 4 operands in CRT wrapper/lifetime/thunk, and 2 CRT initializer-table
# entry pointers. Every one is a real HIGHLOW relocation in retail client.
ABSOLUTE_OPERANDS=(
    (0x241281D2,0x24361040), (0x241281DB,0x24361040),
    (0x241281E9,0x242762D8), (0x241281EE,0x24360F50),
    (0x241281F8,0x24126C40), (0x24128203,0x24278E80),
    (0x24128208,0x24360F8C), (0x24128212,0x24126C40),
    (0x2412821D,0x24278DE4), (0x24128222,0x24360FC8),
    (0x2412822F,0x24126C40), (0x2412823A,0x24278DD8),
    (0x2412823F,0x24361004), (0x2412824C,0x24298EF8),
    (0x24128253,0x24360F8C), (0x24128258,0x24361044),
    (0x24268961,0x24361044), (0x2426896B,0x242736B0),
    (0x2426898B,0x24361058), (0x242736B1,0x24361044),
    (0x242F3A38,0x24268960), (0x242F3A3C,0x24268980),
)
CRT_POINTER_SITES=(0x242F3A38,0x242F3A3C)
ORIGINAL_TEXT_STRINGS=(
    (0x242762D8,"should_never_see_this"),
    (0x24278E80,"m_flLifetime"),
    (0x24278DE4,"m_flStartSize"),
    (0x24278DD8,"m_flEndSize"),
    (0x24298EF8,"DT_EntityParticleTrailInfo"),
)

def rel32(opcode:int,site:int,target:int)->bytes:
    return bytes([opcode])+struct.pack("<i",target-site-5)

def original_pe32_highlow_sites(pe:PEImage)->set[int]:
    data=pe.data
    pe_head=struct.unpack_from("<I",data,0x3C)[0]
    optional=pe_head+24
    if data[pe_head:pe_head+4]!=b"PE\x00\x00" or \
       struct.unpack_from("<H",data,optional)[0]!=0x10B:
        raise VerificationError("Phase250 original client PE32 header drift")
    rva,size=struct.unpack_from("<II",data,optional+96+5*8)
    if not rva or size<8 or size>len(data):
        raise VerificationError("Phase250 original reloc directory absent")
    found:set[int]=set()
    pos=0
    while pos+8<=size:
        va=pe.base+rva+pos
        page,block=struct.unpack("<II",pe.at(va,8))
        if not page and not block:
            break
        if block<8 or block%2 or pos+block>size:
            raise VerificationError("Phase250 malformed original PE32 relocation block")
        for entry in range(8,block,2):
            word=struct.unpack("<H",pe.at(va+entry,2))[0]
            if word>>12==3:
                found.add(pe.base+page+(word&0xFFF))
        pos+=block
    return found

def verify_native(pe:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False,
                  check_relocations:bool=True)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase250 original client image base drift")
    rows=[];witness_count=0;int3_count=0
    for name,va,size,sha,pad,(exit_off,exit_hex),anchors in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+size-1)!=".text":
            raise VerificationError("Phase250 complete native method outside .text: "+name)
        body=pe.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=sha:
            raise VerificationError("Phase250 entire original method SHA drift: "+name)
        if pe.at(va+size,pad)!=b"\xCC"*pad:
            raise VerificationError("Phase250 original method INT3 padding drift: "+name)
        original_exit=bytes.fromhex(exit_hex)
        if body[exit_off:exit_off+len(original_exit)]!=original_exit:
            raise VerificationError("Phase250 original method return/JMP drift: "+name)
        for off,hx in anchors.items():
            b=bytes.fromhex(hx)
            if body[off:off+len(b)]!=b:
                raise VerificationError(f"Phase250 original native witness drift {name}+{off:#x}")
            witness_count+=1
        int3_count+=pad
        rows.append({
            "name":name,"original_VA":f"0x{va:08X}",
            "whole_original_native_code_bytes":size,
            "complete_original_sha256":sha if check_hash else None,
            "original_native_exit_offset":exit_off,
            "trailing_INT3":pad,
        })
    for opcode,edges in ((0xE8,DIRECT_CALLS),(0xE9,TAIL_JUMPS)):
        for site,target in edges:
            if pe.section_of(site)!=".text" or \
               pe.at(site,5)!=rel32(opcode,site,target):
                raise VerificationError(f"Phase250 original E8/E9 edge drift: {site:#x}")
    for site,target in ABSOLUTE_OPERANDS:
        section=".data" if site in CRT_POINTER_SITES else ".text"
        if pe.section_of(site)!=section or \
           pe.at(site,4)!=struct.pack("<I",target):
            raise VerificationError(f"Phase250 original absolute pointer drift at {site:#x}")
    for va,name in ORIGINAL_TEXT_STRINGS:
        raw=name.encode("ascii")+b"\x00"
        if pe.section_of(va)!=".rdata" or pe.at(va,len(raw))!=raw:
            raise VerificationError(f"Phase250 original network-table string drift: {name}")
    if check_relocations:
        relocs=original_pe32_highlow_sites(pe)
        for site,target in ABSOLUTE_OPERANDS:
            if site not in relocs:
                raise VerificationError(f"Phase250 missing original HIGHLOW: {site:#x}")
    return {
        "phase":250,
        "new_original_complete_SHA_methods":len(rows)
            if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(x[2] for x in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":rows,
        "original_sparse_opcode_witnesses":witness_count,
        "original_INT3_alignment_bytes":int3_count,
        "original_exact_E8_CALL_edges":len(DIRECT_CALLS),
        "original_exact_E9_tail_JMP_edges":len(TAIL_JUMPS),
        "original_absolute_pointer_operands":len(ABSOLUTE_OPERANDS),
        "original_HIGHLOW_pointer_relocations":len(ABSOLUTE_OPERANDS)
            if check_relocations else 0,
        "original_recvtable_name":"DT_EntityParticleTrailInfo",
        "original_recvtable_property_names":[
            "m_flLifetime","m_flStartSize","m_flEndSize"],
        "original_recvtable_property_offsets":[8,12,16],
        "original_property_descriptor_stride":0x3C,
        "original_real_network_property_count":3,
        "original_placeholder_is_not_table_member":True,
        "original_registered_table_object_VA":"0x24361044",
        "original_crt_initializer_pointer_table_VAs":[
            "0x242F3A38","0x242F3A3C"],
        "limitations":[
            "RecvProp factories and native receive-proxy internals are callback boundaries.",
            "Global CRT registration order outside the observed code is not modeled.",
            "Host stand-ins do not instantiate actual Source client entity objects.",
            "No original-free rebuilt gameplay DLL loading proved.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for name,blob in (("server",server),("client",client)):
        if hashlib.sha256(blob).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError(
                "Phase250 requires BOTH complete owner-original retail DLL SHA256s: "+name)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=4 or \
       report["new_original_native_code_bytes"]!=199:
        raise VerificationError("Phase250 authentic native function ledger drift")
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    g=parser.add_mutually_exclusive_group(required=True)
    g.add_argument("--archive",type=Path)
    g.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    a=parser.parse_args()
    if a.archive:
        if a.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("client.dll")!=1 or \
               z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase250 needs exact original root DLL pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:parser.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
