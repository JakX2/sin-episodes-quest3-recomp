#!/usr/bin/env python3
"""Phase530: authenticate the 17 SiN-only NPC factory registrations found
missing in owner Source map runs, using the two exact original 2006 DLLs.

This is factory CLASSNAME metadata and the small registration-call control
flow only. It does not recover NPC allocators, constructors, behavior trees,
navigation, models/props, animation or real map gameplay.

Never publish original DLL bytes. Output contains names, VAs, SHA digests
and registration metadata, no native code, strings longer than classnames,
private paths or user asset blobs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from verify_gameplay_entity_registry import (
    PEImage, VerificationError, REGISTRY_GETTER,
    SERVER_SHA256, CLIENT_SHA256
)

# (original classname, registration VA, string VA, factory object VA,
# SHA256 of the COMPLETE 22-byte original compiled registrar).
# This is the 17-name npc_ subset of the 49 unknown classnames in
# research/phases/sin-original-map-runtime-unknown-entity-logs-nov08.md.
# Source is exact owner SHA-pinned original 2006 server.dll, not guessed
# names from HL2, a game BSP, or the limited earlier 32-class registry.
NPC_FACTORIES=(
    ('npc_druglab_grunt_pistol',0x223C6FE0,0x2249AA20,0x22536840,'b6bf9d05824423a361a5eca5bc3466d12f6a3a40a75f478c4e48fd0362000749'),
    ('npc_druglab_grunt_assault_rifle',0x223C7020,0x22490EA0,0x22536848,'d389c49bb43460e26d952ade7fbcd0cd94aa55c588bb91287ed3f3779a2087f6'),
    ('npc_druglab_grunt_elite',0x223C6940,0x2248E760,0x225346A0,'c1d85ff86f8114d66c6ef3938ea76e886089c30bef00f991becacbd3908b7097'),
    ('npc_druglab_grunt_scattergun',0x223C7000,0x2249AA00,0x22536844,'f2290681719199ad0389925bb2db3d9720fb691efc9e3f96fc206d0a8f4514d4'),
    ('npc_generic_actor',0x223C6B90,0x22490B40,0x22534BDC,'6c01165d80b1cfe80f3de36b7445e307f1b961bfcac7106b38ef15cb8f623ab9'),
    ('npc_civilian',0x223C6630,0x22488490,0x2253342C,'2f4601bc9ae102162a43a46012dbc97e0d2342be42369e23487bf6e3abf4feb8'),
    ('npc_druglab_jetpack',0x223C68B0,0x2248CFB4,0x22534514,'0f7f3d8a70c1aed5590c2e8f2504b48f4d00c8ad0d5d26d149bcbe7f0869b29b'),
    ('npc_recombinant_major',0x223C74E0,0x224A159C,0x225372B8,'af30f578c5a1bf26d1e2acfd2777bdd09456bd2bfbfe3a6b3e1e5cb04ad3e894'),
    ('npc_jessica',0x223C6E10,0x2249821C,0x22535964,'c2904559038f869195a6122009062228fde606e48b9ad492fb481f524b553a0f'),
    ('npc_recombinant_minor',0x223C7630,0x224A3150,0x22537788,'2f0886c9317d6d4d1f9a519bbc2374b808bb7ec990966d492352c4468628b1ae'),
    ('npc_securitybot',0x223C7700,0x224A36BC,0x22537B2C,'5f675103e2218b4a38abed904e5983a20021a30818b07918070c9419e58aaf80'),
    ('npc_druglab_worker',0x223C68E0,0x2248DA54,0x22534544,'060b87faec0e3b16f0e96c545a117f98a22bad7a3810323e0a21e3786a43051c'),
    ('npc_druglab_helicopter',0x223C77E0,0x224A5B18,0x22537DB0,'2839880fbd1bb35611f582cf71e346a53917a388b129a95ae9bca7c3f49fb7f8'),
    ('npc_quadralex',0x223C7350,0x2249F594,0x22536CF8,'31957ba1c8adb54bd34c0644c28d04aed87fecbeef68740f91f00b56fef165c9'),
    ('npc_jc',0x223C6DF0,0x22497780,0x22535938,'e661d2f8f7d7e94ad97b5cea7ec9eac15085f54eb445b3613407f0ad88631f4a'),
    ('npc_hologram',0x223C6D70,0x22496F00,0x22535794,'543f235925bf43886d7c2919d9da76b1b05117edb711484aeef0309c693180c1'),
    ('npc_sintek_grunt',0x223C77A0,0x224A49BC,0x22537CEC,'a39618184fe374743acea63fe5991bafcf039c8928a956bee772f9d4b6447a41'),
)

def verify_registration_shape(
    code:bytes,registered_at:int,classname_va:int,factory_va:int,
    registry_getter_va:int
)->None:
    # Original entity registrars dispatch via:
    # CALL singleton; mov edx,[eax]; push classname; push factory;
    # mov ecx,eax; call [edx]; ret. This is only *registration*.
    if len(code)!=22 or not (
        code[:1]==b'\xE8' and code[5:8]==b'\x8B\x10\x68' and
        code[12:13]==b'\x68' and code[17:]==b'\x8B\xC8\xFF\x12\xC3'
    ):
        raise VerificationError("native NPC registrar signature mismatch")
    target=registered_at+5+struct.unpack_from('<i',code,1)[0]
    name_ptr=struct.unpack_from('<I',code,8)[0]
    factory_ptr=struct.unpack_from('<I',code,13)[0]
    if (target,name_ptr,factory_ptr)!=(
        registry_getter_va,classname_va,factory_va
    ):
        raise VerificationError("native NPC registrar target/factory pointer mismatch")

def verify_three_aliases(u32,objects,vtable,creator,delegate,col)->None:
    # The factory object addresses are different but the vptr they store
    # and its two method pointers are IDENTICAL. This is factory metadata
    # only; class AI behavior or initial weapons are not inferred.
    if len(objects)!=3 or len(set(objects))!=3:
        raise VerificationError("distinct three-variant registrar objects required")
    if any(u32(ptr)!=vtable for ptr in objects):
        raise VerificationError("nonidentical grunt factory virtual tables")
    if (u32(vtable-4),u32(vtable),u32(vtable+4))!=(
        col,creator,delegate):
        raise VerificationError("shared original grunt factory methods changed")

def verify_shared_druglab_grunt_factory(pe)->dict:
    """Authenticate three class aliases and their *shared* original constructor."""
    u32=lambda va:struct.unpack('<I',pe.at(va,4))[0]
    factory_addresses=(0x22536840,0x22536844,0x22536848)
    vtable=0x2249A9F8
    creator=0x2236A320
    delegate=0x22369E30
    constructor=0x2236A340
    if (any(pe.section_of(ptr)!='.data' for ptr in factory_addresses)
        or pe.section_of(vtable)!='.rdata'):
        raise VerificationError("druglab grunt shared factory sections mismatch")
    verify_three_aliases(u32,factory_addresses,vtable,creator,delegate,0x224D23F4)
    methods=(
        ('factory creator wrapper',creator,23,
         '6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9'),
        ('factory delegate',delegate,16,
         'ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21'),
        ('underlying allocation ctor',constructor,87,
         '4df0df1f66082cfa848b96edff3977f7c6cb35e4ff31d2a7918dd244028d7ad6'),
    )
    for label,va,size,expected in methods:
        if hashlib.sha256(pe.at(va,size)).hexdigest()!=expected:
            raise VerificationError("changed complete original "+label)
    for site,target in (
        (0x2236A327,constructor),
        (0x2236A346,0x22138940),
        (0x2236A356,0x22360D80),
    ):
        code=pe.at(site,5)
        if code[0]!=0xE8 or (
            site+5+struct.unpack_from('<i',code,1)[0])!=target:
            raise VerificationError("shared grunt call-edge changed")
    if (pe.at(0x2236A32F,5)!=bytes((0x05,0x60,0x01,0,0)) or
        pe.at(0x2236A341,5)!=bytes((0x68,0xD8,0x0F,0,0)) or
        pe.at(0x2236A37E,3)!=bytes((0xFF,0x50,0x70)) or
        pe.at(0x22369E3A,3)!=bytes((0xFF,0x50,0x18))):
        raise VerificationError("shared grunt allocation and return shape changed")
    if (pe.at(0x2236A35F,2)!=bytes((0xC7,0x06)) or
        u32(0x2236A361)!=0x2249AAA4 or
        pe.at(0x2236A365,2)!=bytes((0xC7,0x86)) or
        u32(0x2236A367)!=0x790 or
        u32(0x2236A36B)!=0x2249AA7C or
        pe.at(0x2236A36F,2)!=bytes((0xC7,0x86)) or
        u32(0x2236A371)!=0xC14 or
        u32(0x2236A375)!=0x2249AA40):
        raise VerificationError("shared grunt source vptr writes changed")
    return {
        'class_aliases':[
            'npc_druglab_grunt_pistol',
            'npc_druglab_grunt_scattergun',
            'npc_druglab_grunt_assault_rifle'
        ],
        'shared_factory_vtable_va':hex(vtable),
        'factory_entry_va':hex(creator),
        'factory_delegate_va':hex(delegate),
        'allocation_constructor_va':hex(constructor),
        'original_allocation_bytes':0xFD8,
        'returned_subobject_offset':0x160,
        'observed_native_vptr_writes':[
            {'object_offset':hex(off),'original_vtable_va':hex(va)}
            for off,va in ((0,0x2249AAA4),(0x790,0x2249AA7C),
                           (0xC14,0x2249AA40))
        ],
        'note':'Not a recovered NPC AI or weapon-selection implementation.'
    }

def verify_npc_registry(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256:
        raise VerificationError("original retail server.dll SHA256 differs")
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
        raise VerificationError("original retail client.dll SHA256 differs")
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError("unexpected server image base")
    names=set()
    factory_objects=set()
    output=[]
    for classname,registered_at,name_va,factory_va,complete_sha in NPC_FACTORIES:
        if classname in names or factory_va in factory_objects:
            raise VerificationError("duplicate NPC classname or native factory object")
        names.add(classname)
        factory_objects.add(factory_va)
        if (pe.section_of(registered_at),pe.section_of(name_va),
            pe.section_of(factory_va))!=('.text','.rdata','.data'):
            raise VerificationError(classname+": registration sections changed")
        if pe.at(name_va,len(classname)+1)!=classname.encode('ascii')+b'\0':
            raise VerificationError(classname+": class literal changed")
        code=pe.at(registered_at,22)
        if hashlib.sha256(code).hexdigest()!=complete_sha:
            raise VerificationError(classname+": whole original registration body changed")
        verify_registration_shape(code,registered_at,name_va,
                                  factory_va,REGISTRY_GETTER)
        output.append({
            'classname':classname,
            'registration_va':hex(registered_at),
            'original_string_va':hex(name_va),
            'factory_object_va':hex(factory_va),
            'registration_full_sha256':complete_sha,
        })
    shared=verify_shared_druglab_grunt_factory(pe)
    if len(output)!=17:
        raise VerificationError("not all 17 missing NPC names checked")
    return {
        'status':'ORIGINAL_2006_NPC_FACTORY_REGISTRATIONS_VERIFIED',
        'total_original_missing_map_npc_registrations':len(output),
        'original_native_registrars_full_sha_verified':len(output),
        'shared_original_registry_getter_va':hex(REGISTRY_GETTER),
        'original_server_sha256':SERVER_SHA256,
        'original_client_sha256':CLIENT_SHA256,
        'entries':output,
        'verified_original_shared_grunt_factory':shared,
        'limits':(
          'Registration metadata only. Neither a constructor ABI nor '
          'an NPC AI, spawn, animation, navigation, map-host entity, '
          'normal server DLLInit or Quest3 gameplay is recovered.'
        )
    }

def self_test()->None:
    # Entirely invented address space. Never materialize original
    # proprietary instruction bytes in the synthetic fixture.
    registrar=0x71010000
    singleton=0x71050000
    name_ptr=0x71081000
    object_ptr=0x710F3000
    code=(b'\xE8'+struct.pack('<i',singleton-(registrar+5))+
          b'\x8B\x10\x68'+struct.pack('<I',name_ptr)+
          b'\x68'+struct.pack('<I',object_ptr)+
          b'\x8B\xC8\xFF\x12\xC3')
    assert len(code)==22
    verify_registration_shape(code,registrar,name_ptr,object_ptr,singleton)
    variants=[]
    for i in range(len(code)):
        changed=bytearray(code)
        changed[i]^=0x01
        variants.append(bytes(changed))
    for bad in variants:
        try:
            verify_registration_shape(bad,registrar,name_ptr,object_ptr,singleton)
        except VerificationError:pass
        else:raise AssertionError("mutated NPC registration accepted")
    for delta in (1,256):
        for args in ((registrar+delta,name_ptr,object_ptr,singleton),
                     (registrar,name_ptr+delta,object_ptr,singleton),
                     (registrar,name_ptr,object_ptr+delta,singleton),
                     (registrar,name_ptr,object_ptr,singleton+delta)):
            try:verify_registration_shape(code,*args)
            except VerificationError:pass
            else:raise AssertionError("forged native NPC ABI address accepted")
    if len(NPC_FACTORIES)!=17 or len({x[0] for x in NPC_FACTORIES})!=17:
        raise AssertionError("wrong original NPC inventory count")
    # No original code or original vtables in CI. Use invented
    # addresses and a fake 5-DWORD pointer lookup.
    objects=(0x71001000,0x71001004,0x71001008)
    vtable=0x72002000
    creator=0x73003000
    delegate=0x73003100
    col=0x74004000
    words={**{p:vtable for p in objects},
           vtable-4:col,vtable:creator,vtable+4:delegate}
    verify_three_aliases(lambda va:words.get(va,0),
                         objects,vtable,creator,delegate,col)
    for key in words:
        mutated=dict(words);mutated[key]^=1
        try:
            verify_three_aliases(lambda va:mutated.get(va,0),
                                 objects,vtable,creator,delegate,col)
        except VerificationError:pass
        else:raise AssertionError("forged shared factory alias accepted")
    try:verify_npc_registry(b'synthetic',b'synthetic')
    except VerificationError:pass
    else:raise AssertionError("untrusted original game DLLs accepted")
    print("PASS 17 NPC metadata entries, all 22 synthetic registrar-byte mutations, 8 offset/target mutations and 6 shared-grunt factory pointer/RTTI mutations with original SHA refusal")

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--self-test',action='store_true')
    a=p.parse_args()
    if a.self_test:
        self_test();return 0
    if a.server is None or a.client is None:
        p.error('--server and --client original 2006 DLLs are required')
    try:
        r=verify_npc_registry(a.server.read_bytes(),a.client.read_bytes())
    except (VerificationError,OSError,struct.error) as e:
        p.exit(1,'ORIGINAL NPC FACTORY VERIFICATION REFUSED: '+str(e)+'\n')
    out=json.dumps(r,indent=2)+'\n'
    if a.output:a.output.write_text(out,encoding='utf-8')
    else:print(out,end='')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
