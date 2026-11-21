#!/usr/bin/env python3
"""Original SiN 2006 Druglab Grunt Spawn/weapon selector metadata audit.

Requires the exact original owner-held PE32 2006 client and server SHA256s.
Checks the complete original 1246-byte Spawn method, the original primary
vtable slot25, three exact classname/default-weapon pairs, original native
convar globals, network-mode setter, x87 scalar sites and inherited
virtual Spawn sequence. Never outputs original code or proprietary bytes.

--self-test deliberately uses synthetic data, never game DLLs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256
)

SOURCE_METHOD=0x22369E40
SOURCE_BYTES=0x4DE
SOURCE_SHA256="99dbbb09eeebe19d7260e707c4fb917ab4c4941f1b066d6368eba53b485ccff7"
SOURCE_VTABLE=0x2249AAA4
SOURCE_INITIAL_VIRTUAL70=0x22031B10
# (original classname C string VA, corresponding default weapon C string VA,
#  native ConVar-global pointer reference, source field+0x3B0 desired value).
VARIANTS=(
    ("npc_druglab_grunt_pistol",0x2249AA20,
     "weapon_npc_pistol",0x2249A974,0x2259CC50,1),
    ("npc_druglab_grunt_scattergun",0x2249AA00,
     "weapon_scattergun",0x2249A988,0x2259CC98,0),
    ("npc_druglab_grunt_assault_rifle",0x22490EA0,
     "weapon_assault_rifle",0x2249A99C,0x2259CCE0,2),
)

# Exact source x86 rel32 CALL edges witnessed inside the authenticated
# complete method: symbol-string intern, strcmp, netprop setter,
# inherited original NPC Spawn.
CALLS=(
    (0x22369F0C,0x221F3DE0),
    (0x22369FEC,0x221F3DE0),
    (0x2236A096,0x220F5700),
    (0x2236A0AC,0x221F3DE0),
    (0x2236A0CF,0x220F10C0),
    (0x2236A0FF,0x2235F2A0),
)

def verify_call_edges(read,edges=CALLS):
    for site,target in edges:
        code=read(site,5)
        if len(code)!=5 or code[0]!=0xE8 or (
            site+5+struct.unpack_from('<i',code,1)[0])!=target:
            raise VerificationError(
                f"original Druglab Grunt Spawn direct call changed at {site:#x}")
    return len(edges)

def verify_variants(read_text,variants=VARIANTS):
    if len(variants)!=3 or len({v[0] for v in variants})!=3:
        raise VerificationError("not the three original supported Grunt classes")
    for classname,name_va,weapon,weapon_va,global_ptr,mode in variants:
        if not read_text(name_va,classname) or not read_text(weapon_va,weapon):
            raise VerificationError("original Grunt Spawn classname/weapon literal changed")
        if mode not in (0,1,2) or global_ptr not in (
            0x2259CC50,0x2259CC98,0x2259CCE0):
            raise VerificationError("original per-variant mode/global changed")
    return len(variants)

def verify_source(client:bytes,server:bytes)->dict:
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA256 or (
        hashlib.sha256(server).hexdigest()!=SERVER_SHA256):
        raise VerificationError("not the exact original 2006 SiN retail DLL pair")
    pe=PEImage(server)
    if pe.base!=0x22000000 or pe.section_of(SOURCE_METHOD)!='.text':
        raise VerificationError("unexpected original server image/section")
    if hashlib.sha256(pe.at(SOURCE_METHOD,SOURCE_BYTES)).hexdigest()!=SOURCE_SHA256:
        raise VerificationError("original 1246-byte Druglab Grunt Spawn body differs")
    u32=lambda addr:struct.unpack('<I',pe.at(addr,4))[0]
    if (u32(SOURCE_VTABLE+25*4)!=SOURCE_METHOD or
        u32(SOURCE_VTABLE+28*4)!=SOURCE_INITIAL_VIRTUAL70):
        raise VerificationError("wrong CDruglabGrunt vtable Spawn slot 25 or constructor virtual 28")
    checked=verify_variants(
        lambda va,s:pe.at(va,len(s)+1)==s.encode('ascii')+b'\0')
    for _,name_va,_,weapon_va,_,_ in VARIANTS:
        if pe.section_of(name_va)!='.rdata' or (
            pe.section_of(weapon_va)!='.rdata'):
            raise VerificationError("original Grunt source literal outside .rdata")
    call_count=verify_call_edges(pe.at)

    # Original exact class-name stack literal loads (6,7,8 dwords
    # copied to stack); default weapon string PUSH pointers; and three
    # engine ConVar-global pointer MOV and FLD float +0x28 sites.
    for src,ptr in (
        (0x22369E4D,0x2249AA20),(0x22369E63,0x2249AA00),
        (0x22369E74,0x22490EA0),(0x22369F06,0x2249A974),
        (0x22369FE6,0x2249A988),(0x2236A090,0x2249A99C)):
        if pe.at(src,1) not in (b'\xBE',b'\x68') or u32(src+1)!=ptr:
            raise VerificationError("original class/weapon operand changed")
    for source,ptr,fld in (
        (0x2236A141,0x2259CC50,0x2236A14A),
        (0x2236A1ED,0x2259CC98,0x2236A1F6),
        (0x2236A297,0x2259CCE0,0x2236A2A0)):
        if pe.at(source,1)!=b'\xA1' or u32(source+1)!=ptr or (
            pe.at(fld,3)!=bytes((0xD9,0x40,0x28))):
            raise VerificationError("source variant ConVar +0x28 float changed")
    for source,expected in (
        (0x2236A0D9,bytes((0xFF,0x50,0x68))),
        (0x2236A0E6,bytes((0xFF,0x52,0x1C))),
        (0x2236A0FA,bytes((0xFF,0x52,0x6C))),
    ):
        if pe.at(source,3)!=expected:
            raise VerificationError("original inherited NPC virtual order changed")
    for site in (0x2236A175,0x2236A221,0x2236A2CB):
        if pe.at(site,6)!=bytes((0xFF,0x92,0xB0,0x01,0,0)):
            raise VerificationError("source field+0x98 native notification changed")
    if pe.at(0x2236A30B,6)!=bytes((0xFF,0x92,0xB4,0x01,0,0)):
        raise VerificationError("source field+0x9C native notification changed")
    # One of several independent original FISTP truncation witnesses.
    if (pe.at(0x2236A152,3)!=bytes((0x80,0xCC,0x0C)) or
        pe.at(0x2236A159,4)!=bytes((0xD9,0x6C,0x24,0x10))):
        raise VerificationError("original x87 RC=truncate setup changed")
    return {
        "status":"ORIGINAL_SIN_2006_GRUNT_WEAPON_SPAWN_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "spawn_native_va":hex(SOURCE_METHOD),
        "complete_original_method_bytes":SOURCE_BYTES,
        "complete_original_method_sha256":SOURCE_SHA256,
        "original_primary_vtable_va":hex(SOURCE_VTABLE),
        "original_spawn_slot":25,
        "original_creation_postinit_slot":28,
        "verified_direct_call_edges":call_count,
        "variants":[{
            "classname":cn,"weapon_name":weapon,"mode_field_3b0":mode,
            "source_global_convar_ptr_va":hex(glob)
        } for cn,_,weapon,_,glob,mode in VARIANTS],
        "original_mode_field_offset":"0x3b0",
        "original_numeric_fields":["0x98","0x9c"],
        "original_convar_float_offset":"0x28",
        "original_scalar_cast":"x87 FISTP RC=truncate",
        "limits":"This is original native Spawn metadata and a portable host-controlled behavior reference, not playable Source NPC or recovered AI."
    }

def self_test()->None:
    # Invented rel32 call edges in a disjoint, non-original address space.
    edges=((0x71001000,0x71050000),(0x71001100,0x71060000))
    code={src:b'\xE8'+struct.pack('<i',dst-src-5) for src,dst in edges}
    get=lambda va,n:code.get(va,b'')[:n]
    assert verify_call_edges(get,edges)==2
    for site in code:
        for byte_index in range(5):
            broken=dict(code)
            x=bytearray(broken[site]);x[byte_index]^=1
            broken[site]=bytes(x)
            try:verify_call_edges(lambda va,n:broken.get(va,b'')[:n],edges)
            except VerificationError:pass
            else:raise AssertionError("mutated synthetic Spawn rel32 accepted")
    words={}
    for idx,(classname,_,weapon,__,glob,mode) in enumerate(VARIANTS):
        # Intentionally use different fake VAs and fabricated strings.
        words[0x71010000+idx*0x100]=classname
        words[0x71010030+idx*0x100]=weapon
    fake=tuple(
        (v[0],0x71010000+i*0x100,v[2],0x71010030+i*0x100,
         v[4],v[5]) for i,v in enumerate(VARIANTS))
    assert verify_variants(lambda va,x:words.get(va)==x,fake)==3
    for key in words:
        changed=dict(words);changed[key]="forged_classname"
        try:verify_variants(lambda va,x:changed.get(va)==x,fake)
        except VerificationError:pass
        else:raise AssertionError("mutated fake original source string accepted")
    try:verify_source(b'not-original',b'not-original')
    except VerificationError:pass
    else:raise AssertionError("untrusted original DLL hash accepted")
    print("PASS fabricated original-free Grunt Spawn 2 rel32 edges (10 byte mutations), 6 string mutations and full SHA gate refusal")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    a=p.parse_args()
    if a.self_test:
        self_test();return 0
    if not a.server or not a.client:
        p.error("SHA-pinned original --server and --client are required")
    try:
        result=verify_source(a.client.read_bytes(),a.server.read_bytes())
    except (VerificationError,OSError,struct.error) as e:
        p.exit(1,"ORIGINAL SPAWN EVIDENCE REFUSED: "+str(e)+"\n")
    out=json.dumps(result,indent=2)+"\n"
    if a.output:a.output.write_text(out,encoding="utf-8")
    else:print(out,end="")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
