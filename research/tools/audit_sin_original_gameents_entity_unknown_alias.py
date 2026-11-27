#!/usr/bin/env python3
"""Read-only original SiN 2006 PE32 CBaseEntity/IServerUnknown alias proof.

--server authenticates the owner-private DLL. --self-test synthesizes x86
instruction windows and never depends on commercial game binaries.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_engine_gameents_consumer import PE32

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
BASE=0x22000000
# (role, original VA, size, full-window SHA-256). No source bytes exported.
WINDOWS=(
    ('baseentity_embedded_property_ctor',0x2214671f,13,'b10ac61c981a01aadfcd78d66de8fbbc3d6c948f0eea633f5be24763c3a25381'),
    ('networkproperty_constructor',0x2227efe0,72,'61302018288931809d6d3d724123701771b6ab01e7e78de89fe65d742107b736'),
    ('networkproperty_attach_edict',0x2227f070,53,'8959de0a65a8c2ce49ffbb7b6f79bc8bcbc8e2cf227857ef697fda5e9fa41bb2'),
    ('baseentity_unknown_vtable',0x2240ac2c,24,'4f1ca17465108f08b2bdc68ae454b02bf2c85384b56c3c5e5a8f1b06d25d86b3'),
    ('baseentity_interface_getters',0x22001ac0,35,'5b76fc80e4d718b70c484adb929ff58a7cbbfb3cd4e2d343abf926e5d1867b5f'),
    ('gameents_edict_to_entity',0x221e82f0,28,'189b466407658510bb27d05b63609600b2e338a8c920d5b5997cb2002869bbe3'),
    ('gameents_entity_to_edict',0x221e94b0,22,'c702a6d4ebf9c7ec08865a2dfa7b1010364af129604919721528675c3c3fac53'),
    ('networkproperty_vtable',0x22454f00,40,'c9326cdd3c55f1b9395610f07f9b5ec4a069202fb62040ebb9dbda41eb880413'),
    ('networkproperty_get_outer',0x2227f040,4,'916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689'),
)
# Short independent instruction/virtual-pointer witnesses. Vtable entries
# are pointer data in the original .rdata section, not emitted functions.
WITNESSES=(
    (0,0x2214671f,'8dae60010000','CBaseEntity owns property at +0x160'),
    (0,0x22146725,'8bcd','construct embedded property with thiscall'),
    (0,0x22146727,'e8b4881300','direct property constructor'),
    (1,0x2227efe5,'8d7e04','second vptr is at property+4'),
    (1,0x2227eff6,'c706004f4522','property first interface vptr'),
    (1,0x2227effc,'c707f84e4522','property second interface vptr'),
    (2,0x2227f088,'8b168bce','load property vtable for outer getter'),
    (2,0x2227f08c,'89460c','property records edict pointer at +0x0c'),
    (2,0x2227f08f,'ff5224','virtual getter at property vtable +0x24'),
    (2,0x2227f092,'8b760c','load stored edict pointer'),
    (2,0x2227f095,'89460c','edict unknown field receives base entity'),
    (3,0x2240ac3c,'d01a0022','CBaseEntity vtable +0x10 property getter'),
    (3,0x2240ac40,'e01a0022','CBaseEntity vtable +0x14 unknown GetBaseEntity'),
    (4,0x22001ad0,'8d8160010000c3','GetNetworkable returns property subobject'),
    (4,0x22001ae0,'8bc1c3','GetBaseEntity returns this'),
    (5,0x221e82f8,'8b480c','GameEnts loads edict unknown +0x0c'),
    (5,0x221e82ff,'8b01ff5014','GameEnts invokes virtual +0x14'),
    (6,0x221e94b8,'8b806c010000c20400','CBaseEntity edict field +0x16c'),
    (7,0x22454f24,'40f02722','property vtable +0x24 resolves GetOuter'),
    (8,0x2227f040,'8b4108c3','property outer pointer at +0x08'),
)

def check(blobs,strict=False):
    if len(blobs)!=len(WINDOWS):
        raise ValueError('missing source alias proof windows')
    for blob,(role,va,size,sha) in zip(blobs,WINDOWS):
        if len(blob)!=size:
            raise ValueError('truncated original instruction window '+role)
        if strict and hashlib.sha256(blob).hexdigest()!=sha:
            raise ValueError('original server window SHA mismatch '+role)
    for index,va,hex_bytes,role in WITNESSES:
        start=WINDOWS[index][1]
        b=bytes.fromhex(hex_bytes)
        at=va-start
        if at<0 or at+len(b)>len(blobs[index]) or blobs[index][at:at+len(b)]!=b:
            raise ValueError('original entity unknown alias witness mismatch '+role)
    # This is an instruction-bearing rel32, not merely a coincidental byte.
    start=WINDOWS[0][1]
    call=blobs[0][0x22146727-start:0x2214672c-start]
    if call[0]!=0xe8 or 0x2214672c+struct.unpack_from('<i',call,1)[0]!=0x2227efe0:
        raise ValueError('CBaseEntity did not call the property constructor')
    return {'original_server_entity_unknown_alias_authenticated':bool(strict),
            'windows_authenticated':len(WINDOWS),
            'opcode_and_vtable_witnesses':len(WITNESSES),
            'embedded_networkproperty_offset':0x160,
            'entity_edict_pointer_offset':0x16c,
            'edict_unknown_pointer_offset':0x0c,
            'unknown_equals_outer_entity_in_this_path':True,
            'entity_unknown_exact_base_alias_only':True,
            'physical_owner_lifetime_verified':False,
            'retail_startup_verified':False,
            'original_bytes_exported':False}

def audit(raw):
    if hashlib.sha256(raw).hexdigest()!=SERVER_SHA:
        raise ValueError('wrong original 2006 server.dll SHA')
    pe=PE32(raw)
    if pe.base!=BASE:raise ValueError('wrong original server.dll image base')
    return check([pe.raw(va,size) for _,va,size,_ in WINDOWS],True)

def self_test():
    blobs=[bytearray(b'\xcc'*size) for _,_,size,_ in WINDOWS]
    for index,va,hex_bytes,_ in WITNESSES:
        start=WINDOWS[index][1];b=bytes.fromhex(hex_bytes)
        blobs[index][va-start:va-start+len(b)]=b
    check(blobs)
    for index,va,hex_bytes,role in WITNESSES:
        corrupted=[bytearray(x) for x in blobs]
        corrupted[index][va-WINDOWS[index][1]]^=1
        try:check(corrupted)
        except ValueError:pass
        else:raise AssertionError('mutated source alias accepted: '+role)
    for i in range(len(blobs)):
        short=[bytearray(x) for x in blobs];short[i]=short[i][:-1]
        try:check(short)
        except ValueError:pass
        else:raise AssertionError('truncated window accepted')
    try:check(blobs,True)
    except ValueError:pass
    else:raise AssertionError('fabricated vtable and code passed original SHA')
    print(f'PASS original-free 2006 server entity/unknown alias: {len(WINDOWS)} SHA windows, {len(WITNESSES)} instruction/vtable witnesses, negative mutations')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    g=p.add_mutually_exclusive_group(required=True)
    g.add_argument('--self-test',action='store_true')
    g.add_argument('--server',type=Path)
    args=p.parse_args()
    if args.self_test:self_test()
    else:print(json.dumps(audit(args.server.read_bytes()),indent=2,sort_keys=True))

if __name__=='__main__':main()
