#!/usr/bin/env python3
"""Phase621: authenticate the previously omitted DEFAULT npc_druglab_grunt
factory registration and its no-weapon-override Spawn route.

Requires the user's own SHA-pinned unmodified 2006 DLL pair. Retains existing
Phase530 (17 map-missing NPC entries) and Phase532 (three weapon variants)
inventories unchanged. Never publishes source DLL bytes or disassembly.
Self-tests use fabricated x86 memory and synthetic addresses only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from verify_gameplay_entity_registry import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256, REGISTRY_GETTER
)
from audit_sin_original_npc_factories import (
    verify_registration_shape, verify_shared_druglab_grunt_factory
)
from audit_sin_original_druglab_grunt_spawn import (
    verify_source as verify_existing_spawn, VARIANTS
)

BASE_CLASS = "npc_druglab_grunt"
BASE_REGISTER_VA = 0x223C6FC0
BASE_NAME_VA = 0x2249B2F4
BASE_FACTORY_OBJECT_VA = 0x2253683C
BASE_REGISTRATION_BYTES = 22
BASE_REGISTER_SHA = (
    "063e48f768558e1c251b08a9c79ba87af04137351a2b39851504b769bcb2398e"
)
FACTORY_VTABLE_VA = 0x2249A9F8
FACTORY_RTTI_NAME_VA = 0x225367F4
FACTORY_RTTI = ".?AV?$CEntityFactory@VCNPC_MobGrunt@@@@"
CRT_FACTORY_ENTRIES = (
    (0x224D84A8, BASE_REGISTER_VA),
    (0x224D84AC, 0x223C6FE0),
    (0x224D84B0, 0x223C7000),
    (0x224D84B4, 0x223C7020),
)
# Source-authenticated after comparing none of the three weapon-variant
# classnames: jump to common inherited Spawn, skip per-variant scalar writes.
BRANCHES = (
    (0x2236A07F, 0x2236A0D4, "short"), # no assault rifle match
    (0x2236A291, 0x2236A317, "near"),  # no scalar-variant match
)
INHERITED_SPAWN_VA = 0x2235F2A0

def read_u32(pe: PEImage, va: int) -> int:
    return struct.unpack("<I", pe.at(va, 4))[0]

def verify_four_factory_objects(u32, objects, vtable, getter):
    if len(objects) != 4 or len(set(objects)) != 4:
        raise VerificationError("not four independent native MobGrunt factory objects")
    if any(u32(x) != vtable for x in objects):
        raise VerificationError("base / variant factory objects differ")
    if u32(vtable) != getter:
        raise VerificationError("unexpected shared factory constructor callback")

def verify_branch(read, site, destination, width):
    code = read(site, 2 if width == "short" else 6)
    if width == "short":
        if len(code) != 2 or code[0] != 0x75:
            raise VerificationError("short JNE missing from base Spawn path")
        delta = struct.unpack_from("<b", code, 1)[0]
    elif width == "near":
        if len(code) != 6 or code[:2] != b"\x0f\x85":
            raise VerificationError("near JNE missing from base Spawn path")
        delta = struct.unpack_from("<i", code, 2)[0]
    else:
        raise VerificationError("unrecognized Spawn branch width")
    if site + len(code) + delta != destination:
        raise VerificationError("base Spawn branch no longer bypasses variant mutations")

def verify_original(client: bytes, server: bytes) -> dict:
    if hashlib.sha256(client).hexdigest() != CLIENT_SHA256 or (
        hashlib.sha256(server).hexdigest() != SERVER_SHA256
    ):
        raise VerificationError("original 2006 SiN retail DLL fingerprints differ")

    pe = PEImage(server)
    u32 = lambda va: read_u32(pe, va)
    if pe.base != 0x22000000:
        raise VerificationError("unrecognized original SiN PE32 image base")
    if (
        pe.section_of(BASE_REGISTER_VA) != ".text"
        or pe.section_of(BASE_NAME_VA) != ".rdata"
        or pe.section_of(BASE_FACTORY_OBJECT_VA) != ".data"
    ):
        raise VerificationError("default MobGrunt registration sections changed")
    if pe.at(BASE_NAME_VA, len(BASE_CLASS) + 1) != BASE_CLASS.encode() + b"\0":
        raise VerificationError("original default MobGrunt classname literal changed")
    code = pe.at(BASE_REGISTER_VA, BASE_REGISTRATION_BYTES)
    if hashlib.sha256(code).hexdigest() != BASE_REGISTER_SHA:
        raise VerificationError("original full 22-byte default registrar digest differs")
    verify_registration_shape(
        code, BASE_REGISTER_VA, BASE_NAME_VA,
        BASE_FACTORY_OBJECT_VA, REGISTRY_GETTER
    )
    if pe.at(FACTORY_RTTI_NAME_VA, len(FACTORY_RTTI)+1) != (
        FACTORY_RTTI.encode("ascii") + b"\0"
    ):
        raise VerificationError("original CEntityFactory<CNPC_MobGrunt> RTTI differs")
    if pe.section_of(FACTORY_VTABLE_VA) != ".rdata":
        raise VerificationError("unexpected factory vtable section")
    four = (BASE_FACTORY_OBJECT_VA, 0x22536840, 0x22536844, 0x22536848)
    verify_four_factory_objects(u32, four, FACTORY_VTABLE_VA, 0x2236A320)
    # Existing Phase530 independently checks the three variant aliases,
    # creator, constructor, delegate, vtable COL, allocation and vptr writes.
    existing_factory = verify_shared_druglab_grunt_factory(pe)
    if existing_factory["factory_entry_va"] != hex(0x2236A320):
        raise VerificationError("existing three-alias factory audit mismatch")
    for entry_va, method_va in CRT_FACTORY_ENTRIES:
        if u32(entry_va) != method_va:
            raise VerificationError("four adjacent retail CRT factory initializers changed")
    # Phase532 checks the complete 1246-byte original Spawn digest/vtable
    # and all three weapon/global mappings. We add the fourth fallback,
    # NOT an invented fourth weapon.
    verify_existing_spawn(client, server)
    if BASE_CLASS in {p[0] for p in VARIANTS}:
        raise VerificationError("base factory unexpectedly assigned a weapon variant")
    for site,target,width in BRANCHES:
        verify_branch(pe.at, site, target, width)
    if pe.at(0x2236A0D9,3)!=b"\xff\x50\x68" or (
        pe.at(0x2236A0E6,3)!=b"\xff\x52\x1c"
    ) or pe.at(0x2236A0FA,3)!=b"\xff\x52\x6c":
        raise VerificationError("base path common inherited virtual hooks changed")
    ins = pe.at(0x2236A0FF, 5)
    if ins[:1]!=b"\xe8" or (
        0x2236A104 + struct.unpack("<i",ins[1:])[0] != INHERITED_SPAWN_VA
    ):
        raise VerificationError("base Grunt inherited Spawn call differs")
    return {
        "status": "ORIGINAL_2006_DEFAULT_MOBGRUNT_FACTORY_AND_SPAWN_VERIFIED",
        "classname": BASE_CLASS,
        "complete_new_original_registrar_va": hex(BASE_REGISTER_VA),
        "complete_new_original_registrar_bytes": BASE_REGISTRATION_BYTES,
        "complete_new_original_registrar_sha256": BASE_REGISTER_SHA,
        "factory": "CEntityFactory<CNPC_MobGrunt>",
        "factory_vtable_va": hex(FACTORY_VTABLE_VA),
        "factory_objects": [hex(x) for x in four],
        "startup_registration_functions": [hex(x[1]) for x in CRT_FACTORY_ENTRIES],
        "shares_three_weapon_variant_constructor": True,
        "default_has_its_own_weapon_override": False,
        "default_also_uses_inherited_spawn": True,
        "default_skips_per_variant_numeric_field_updates": True,
        "elite_is_different_factory": True,
        "limits": (
            "Original native static factory and common Spawn control flow only. "
            "Does not reconstruct a native Source factory pointer or imply a "
            "live game NPC, engine lifecycle callback, or Android game boot."
        ),
    }

def self_test():
    # Fabricated registration routine, not original bytes.
    base=0x71001100;getter=0x71005000;name=0x71009000;obj=0x7100A000
    registrar=(b"\xe8"+struct.pack("<i",getter-(base+5))+
               b"\x8b\x10\x68"+struct.pack("<I",name)+
               b"\x68"+struct.pack("<I",obj)+b"\x8b\xc8\xff\x12\xc3")
    assert len(registrar)==22
    verify_registration_shape(registrar,base,name,obj,getter)
    for i in range(22):
        mutated=bytearray(registrar);mutated[i]^=1
        try:
            verify_registration_shape(bytes(mutated),base,name,obj,getter)
        except VerificationError:
            pass
        else:
            raise AssertionError("corrupted fabricated factory registrar accepted")
    objs=(0x7100A000,0x7100A004,0x7100A008,0x7100A00C)
    words={p:0x72005000 for p in objs}
    words[0x72005000]=0x73006000
    read_word=lambda va:words.get(va,0)
    verify_four_factory_objects(read_word,objs,0x72005000,0x73006000)
    for key in words:
        altered=dict(words);altered[key]^=1
        try:
            verify_four_factory_objects(lambda va:altered.get(va,0),
                                        objs,0x72005000,0x73006000)
        except VerificationError:
            pass
        else:
            raise AssertionError("corrupted fabricated factory pointers accepted")
    for i,(site,target,width) in enumerate(((0x71000100,0x71000140,"short"),
                                            (0x71001100,0x71001240,"near"))):
        if width=="short":
            b=b"\x75"+struct.pack("<b",target-site-2)
        else:
            b=b"\x0f\x85"+struct.pack("<i",target-site-6)
        verify_branch(lambda _va,n:b[:n],site,target,width)
        for j in range(len(b)):
            altered=bytearray(b);altered[j]^=1
            try:
                verify_branch(lambda _va,n:bytes(altered[:n]),site,target,width)
            except VerificationError:
                pass
            else:
                raise AssertionError("corrupted synthetic fallback branch accepted")
    try:
        verify_original(b"fake-client",b"fake-server")
    except VerificationError:
        pass
    else:
        raise AssertionError("untrusted retail DLL fingerprints accepted")
    print("PASS Phase621 synthetic default MobGrunt registrar, four-object factory identity, inherited Spawn bypass and SHA gate")

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--server",type=Path)
    ap.add_argument("--client",type=Path)
    ap.add_argument("--self-test",action="store_true")
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    if args.self_test:
        self_test();return 0
    if args.server is None or args.client is None:
        ap.error("--server and --client must be owner-provided original 2006 DLLs")
    try:
        result=verify_original(args.client.read_bytes(),args.server.read_bytes())
    except (VerificationError,OSError,struct.error) as exc:
        ap.exit(1,"DEFAULT GRUNT VERIFICATION REFUSED: "+str(exc)+"\n")
    result_json=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(result_json,encoding="utf-8")
    else:print(result_json,end="")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
