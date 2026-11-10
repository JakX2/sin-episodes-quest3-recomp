#!/usr/bin/env python3
"""Phase171 original 2006 server.dll entity registry vtable and lookup audit.

Original-only SHA256 validation of three complete function regions, disjoint
x86 instruction witnesses, native direct CALL edges, exact registry vtable
slots and two original diagnostic strings. Chains prior Phase170 evidence and
original client/server fingerprints. No proprietary function bodies committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_entity_factory import audit as phase170_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
    'registry_create_factory_resolver':(0x22285550,46,
        '58f62479791e357d5b220d60803a1a72e2cfc003944000c50f667f30730854f1'),
    'registry_destroy_factory_resolver':(0x22285580,50,
        '83eb972b13abd551e76ba781d13eed5ab614cd013c0d7f8c133020f7a2b2a5db'),
    'registry_internal_name_to_record_lookup':(0x2228B010,94,
        'ba7b2d918b3b72235c9457731f50ebcfb3b6f001c699d981c788d7fed8c9abf5'),
}
# Sparse witnesses are deliberately separated: every full original function
# SHA256 remains private and cannot be recreated from these anchors.
OPCODES={
    0x22285550:'8b01',0x22285553:'8b742408',
    0x22285558:'ff500c',0x2228555B:'85c0',
    0x2228555E:'7514',0x22285560:'68505f4522',
    0x22285565:'ff1548213d22',0x2228556E:'33c0',
    0x22285571:'c20400',0x22285574:'8b10',
    0x22285578:'ff12',0x2228557B:'c20400',
    0x22285580:'8b01',0x22285583:'8b742408',
    0x22285588:'ff500c',0x2228558D:'7513',
    0x22285590:'68805f4522',0x22285595:'ff1548213d22',
    0x2228559F:'c20800',0x222855A2:'8b4c240c',
    0x222855A8:'51',0x222855AB:'ff5204',
    0x222855AF:'c20800',
    0x2228B010:'83ec08',0x2228B013:'8b44240c',
    0x2228B020:'8d7704',0x2228B024:'8d4e1c',
    0x2228B027:'66c7442410ffff',
    0x2228B033:'668b10',0x2228B036:'6a00',
    0x2228B03D:'8bce',0x2228B03F:'6689542410',
    0x2228B049:'663dffff',0x2228B04D:'750a',
    0x2228B050:'33c0',0x2228B056:'c20400',
    0x2228B059:'8b5708',0x2228B05C:'0fb7c8',
    0x2228B05F:'c1e104',0x2228B062:'8b44110c',
    0x2228B06B:'c20400',
}
CALLS={0x2228B02E:0x2228F7A0,0x2228B044:0x2228A760}
# Native vtable created by original ctor at 0x2228AFB3; four exact slots.
VTABLE_START=0x22456324
VTABLE=(0x2228B190,0x22285550,0x22285580,0x2228B010)
LITERALS={
  0x22455F50:b'Attempted to create unknown entity type %s!\n\x00',
  0x22455F80:b'Attempted to destroy unknown entity type %s!\n\x00',
}
# Verification is both function- and owner-grounded: original Phase170
# factory call sites dynamically select vtable+0x04.
INDIRECT_EDGES={
    0x22266B3E:bytes.fromhex('ff5204'),
    0x22266B6E:bytes.fromhex('ff5204'),
}

def verify_original_registry_lookup(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError('original server PE32 image base mismatch')
    for va,witness in OPCODES.items():
        raw=bytes.fromhex(witness)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('original registry lookup opcode mismatch at %#x'%va)
    for va,target in CALLS.items():
        raw=bytes((0xE8,))+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=raw:
            raise VerificationError('original registry lookup CALL mismatch at %#x'%va)
    table=struct.pack('<4I',*VTABLE)
    if pe.at(VTABLE_START,len(table))!=table:
        raise VerificationError('original entity registry vtable slot mismatch')
    for va,raw in LITERALS.items():
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('original registry diagnostic literal mismatch at %#x'%va)
    for va,raw in INDIRECT_EDGES.items():
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('original factory caller virtual dispatch mismatch at %#x'%va)
    if check_hashes:
        for name,(va,size,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
                raise VerificationError('original registry lookup full function SHA mismatch: '+name)
    return {
        'original_full_function_hashes':len(REGIONS) if check_hashes else 0,
        'original_opcode_witnesses':len(OPCODES),
        'original_direct_call_edges':len(CALLS),
        'original_vtable_slots':len(VTABLE),
        'original_parent_factory_indirect_calls':len(INDIRECT_EDGES),
        'original_diagnostic_literals':len(LITERALS),
        'vtable_address':hex(VTABLE_START),
        'native_record_stride_bytes':16,
        'native_record_provider_field_offset':12,
        'original_functions':{
            name:{'address':hex(va),'size':size}
            for name,(va,size,_) in REGIONS.items()},
        'limitations':[
            'Actual symbol interning and index lookup functions remain host callbacks',
            'Registered provider virtual +0 and +4 are not native ABI-reconstructed',
            'This is not an original game DLL replacement or playable game runtime'
        ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    out=phase170_audit(client_bytes,server_bytes)
    out['original_server_entity_registry_lookup']=verify_original_registry_lookup(
        PEImage(server_bytes))
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+chr(10)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding='utf-8')
    else:
        print(result,end='')
if __name__=='__main__':
    main()