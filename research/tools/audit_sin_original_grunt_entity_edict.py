#!/usr/bin/env python3
"""SHA-gated original SiN Grunt factory +0x70 native edict/networkable handoff.

Reads only exact original owner-held 2006 client.dll/server.dll.
Outputs bounded metadata. Original instruction bytes stay private.
The self-test uses completely synthetic fake addresses and rejects
source pointer/rel32 mutations and untrusted original binaries.
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

FULL_METHODS=(
    ("grunt_factory_virtual_70",0x22031B10,27,
     "8b4c539f3d96b95df7c70b4918c8af0d442635b4ac078a8c6e3b51b123c1a6ad"),
    ("base_entity_initialize",0x2213B140,197,
     "46d43167fb58428dafe3d886f42c4f38004c214adfb5268bd87cd50672febe7c"),
    ("native_edict_networkable_attach",0x2227F070,53,
     "8959de0a65a8c2ce49ffbb7b6f79bc8bcbc8e2cf227857ef697fda5e9fa41bb2"),
    ("grunt_npc_postinit_slot_3f8",0x22359210,57,
     "848486f30a6c0d1bf820eecbb544c451f91a362070ca3693f500165b74fbfbc6"),
)
ORIGINAL_EDICT_CONTEXT_GLOBAL=0x22558764
ORIGINAL_NETWORKABLE_SUBOBJECT_OFFSET=0x160
ORIGINAL_EDICT_BACKPTR_OFFSET=0x8
ORIGINAL_BASE_FLAGS_OFFSET=0xB8
# Each entry is a source-authenticated original relative direct call edge.
EDGES=(
    (0x22031B18,0x2213B140),
    (0x2213B151,0x221F3DE0),
    (0x2213B179,0x22164460),
    (0x2213B182,0x2214BF30),
    (0x2213B189,0x2214B280),
    (0x2213B1A6,0x2227F070),
    (0x2213B1D9,0x22164440),
    (0x2213B1F5,0x2214BF30),
    (0x2213B1FC,0x2214B280),
    (0x22359213,0x22031D50),
    (0x22359240,0x22358FA0),
)

def verify_edges(read,edges=EDGES):
    for site,target in edges:
        code=read(site,5)
        if len(code)!=5 or code[0]!=0xE8 or (
            site+5+struct.unpack_from("<i",code,1)[0])!=target:
            raise VerificationError(
                f"original entity/edict initializer CALL mismatch {site:#x}")
    return len(edges)

def verify_structural_witnesses(read,u32):
    # Genuine source primary CDruglabGrunt vtable +0x70 -> wrapper,
    # +0x3F8 -> NPC-specific initialization, and wrapper dispatch.
    if (u32(0x2249AAA4+0x70)!=0x22031B10 or
        u32(0x2249AAA4+0x3F8)!=0x22359210 or
        read(0x22031B21,6)!=bytes((0xFF,0x92,0xF8,0x03,0,0))):
        raise VerificationError("incorrect original Grunt initialization vtable")
    # Source base bit9 special and bit10 skip-edict tests. These
    # are flags at this+0xB8, not a guessed map spawnflags KeyValue.
    if (read(0x2213B15E,6)!=bytes((0x8B,0x86,0xB8,0,0,0)) or
        read(0x2213B166,3)!=bytes((0xC1,0xEA,0x09)) or
        read(0x2213B169,3)!=bytes((0xF6,0xC2,0x01)) or
        read(0x2213B192,3)!=bytes((0xC1,0xE8,0x0A)) or
        read(0x2213B195,2)!=bytes((0xA8,0x01))):
        raise VerificationError("original base entity bit9/bit10 gates changed")
    # Global pending edict context must be passed to native helper
    # on networkable subobject this+0x160 and then cleared.
    if (read(0x2213B199,2)!=bytes((0x8B,0x0D)) or
        u32(0x2213B19B)!=ORIGINAL_EDICT_CONTEXT_GLOBAL or
        read(0x2213B1A0,6)!=bytes((0x8D,0x8E,0x60,0x01,0,0)) or
        read(0x2213B1AB,2)!=bytes((0xC7,0x05)) or
        u32(0x2213B1AD)!=ORIGINAL_EDICT_CONTEXT_GLOBAL):
        raise VerificationError("native edict context/global or +0x160 changed")
    if (read(0x2227F083,2)!=bytes((0x6A,0xFF)) or
        read(0x2227F085,3)!=bytes((0xFF,0x50,0x54)) or
        read(0x2213B1EE,3)!=bytes((0x89,0x48,0x08))):
        raise VerificationError("engine CreateEdict(-1) or +0x08 backlink changed")
    # The NPC postinit vtable is invoked after +0x70 base helper.
    if (read(0x22359220,6)!=bytes((0xFF,0x90,0x6C,0x07,0,0)) or
        read(0x22359232,6)!=bytes((0xFF,0x92,0xAC,0x07,0,0))):
        raise VerificationError("original NPC-specific postinit dispatch changed")
    return 4

def verify_original(client:bytes,server:bytes)->dict:
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
        raise VerificationError("unexpected original client.dll SHA256")
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256:
        raise VerificationError("unexpected original server.dll SHA256")
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError("original server PE32 image base changed")
    for label,va,size,digest in FULL_METHODS:
        if pe.section_of(va)!='.text' or (
            hashlib.sha256(pe.at(va,size)).hexdigest()!=digest):
            raise VerificationError("complete original method changed: "+label)
    dword=lambda va:struct.unpack("<I",pe.at(va,4))[0]
    witness_count=verify_structural_witnesses(pe.at,dword)
    edge_count=verify_edges(pe.at)
    return {
        "status":"ORIGINAL_SIN_GRUNT_ENTITY_EDICT_INIT_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "complete_original_method_bodies_verified":len(FULL_METHODS),
        "complete_original_method_bytes_verified":sum(x[2] for x in FULL_METHODS),
        "native_direct_call_edges_verified":edge_count,
        "source_virtual_table_va":"0x2249AAA4",
        "source_factory_post_ctor_slot":28,
        "source_npc_postinit_virtual_byte_offset":"0x3f8",
        "original_networkable_subobject_offset":hex(ORIGINAL_NETWORKABLE_SUBOBJECT_OFFSET),
        "original_factory_edict_context_global_va":hex(ORIGINAL_EDICT_CONTEXT_GLOBAL),
        "original_edict_backpointer_offset":hex(ORIGINAL_EDICT_BACKPTR_OFFSET),
        "original_base_entity_flags_offset":hex(ORIGINAL_BASE_FLAGS_OFFSET),
        "original_creation_flags":[
            {"mask":"0x200","effect":"special initialization, skips networkable attach"},
            {"mask":"0x400","effect":"skip direct networkable attach"},
        ],
        "structural_witness_groups":witness_count,
        "limits":"Original code/ABI metadata only. No native Source CBaseNPC/edict object was created or initialized in retail by this verifier."
    }

def self_test()->None:
    edges=((0x71001000,0x71050000),(0x71001100,0x71070000))
    fake={site:b'\xE8'+struct.pack("<i",target-site-5)
          for site,target in edges}
    assert verify_edges(lambda va,n:fake.get(va,b'')[:n],edges)==2
    for site in fake:
        for n in range(5):
            broken=dict(fake)
            mutated=bytearray(broken[site])
            mutated[n]^=1
            broken[site]=bytes(mutated)
            try:verify_edges(
                lambda va,size:broken.get(va,b'')[:size],edges)
            except VerificationError:pass
            else:raise AssertionError("forged CALL accepted")
    # Fake native-vtable addresses; no original compiled method bytes.
    fake_vtable={
        0x71001000:0x71005000,
        0x71001388:0x71006000,
    }
    def synthetic_vtable_ok(values,first,second):
        return values.get(0x71001000)==first and (
            values.get(0x71001388)==second)
    assert synthetic_vtable_ok(fake_vtable,0x71005000,0x71006000)
    for key in fake_vtable:
        bad=dict(fake_vtable);bad[key]^=1
        assert not synthetic_vtable_ok(bad,0x71005000,0x71006000)
    try:verify_original(b"invalid",b"invalid")
    except VerificationError:pass
    else:raise AssertionError("untrusted original PE binaries accepted")
    print("PASS original-free entity edict handoff: 10 fake rel32 byte mutations, 2 fake vtable mutations and full-original SHA refusal")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path)
    p.add_argument("--server",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    a=p.parse_args()
    if a.self_test:
        self_test();return 0
    if not a.server or not a.client:
        p.error("both owner-private SHA-pinned original --client/--server required")
    try:
        result=verify_original(a.client.read_bytes(),a.server.read_bytes())
    except (VerificationError,OSError,struct.error) as e:
        p.exit(1,"ORIGINAL NPC EDICT EVIDENCE REFUSED: "+str(e)+"\n")
    text=json.dumps(result,indent=2)+"\n"
    if a.output:a.output.write_text(text,encoding="utf-8")
    else:print(text,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
