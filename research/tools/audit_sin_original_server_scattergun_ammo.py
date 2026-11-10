#!/usr/bin/env python3
"""Phase165: original 2006 SiN server CWeaponScatterGun ammo-state audit.

Owner-only SHA256 check of both full original DLLs; checks TWO complete
original x86 method regions, MSVC RTTI/vtable identity, 34 sparse opcode
witnesses and original CALL / tail-JMP paths. No proprietary executable
bytes or native full-function dumps are committed or printed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from verify_gameplay_entity_registry import PEImage,VerificationError

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'scattergun_virtual_ammo_snapshot_update':(
  0x2231C3E0,104,'5e5668894afb65fd7dd45078abe9d58cae147714e302b7e5952411b9fb91af43'),
 'scattergun_secondary_clip_decrement':(
  0x2231C740,51,'b88574dc44edf04e41c044514f9a371e8e955c9f1ca9786c24d22fc70eb0ec0e'),
}
ORIGINAL_VTABLE=0x2247A1E4
ORIGINAL_COL=0x224CF8B4
ORIGINAL_RTTI_DESCRIPTOR=0x2252E8A8
TYPE_NAME=b'.?AVCWeaponScatterGun@@\x00'
VTABLE_SLOTS={216:0x2231C3E0,227:0x2231C810}
# Sparse deliberately non-complete x86 witnesses for original behavior.
OPCODES={
 0x2231C3E0:'56',
 0x2231C3E8:'8b867c050000',
 0x2231C3EE:'8b8eec040000',
 0x2231C3F4:'3bc1',
 0x2231C3F6:'b201',
 0x2231C3FA:'83f8ff',
 0x2231C3FF:'889689050000',
 0x2231C405:'898e7c050000',
 0x2231C40B:'8b8680050000',
 0x2231C411:'8b8e84050000',
 0x2231C417:'3bc1',
 0x2231C41B:'80be7805000000',
 0x2231C424:'83f8ff',
 0x2231C429:'889689050000',
 0x2231C42F:'898e80050000',
 0x2231C435:'80be8905000000',
 0x2231C43E:'8bce',
 0x2231C446:'5e',
 0x2231C447:'c3',
 0x2231C740:'8b01',
 0x2231C743:'8d70ff',
 0x2231C746:'3bc6',
 0x2231C74A:'80b900fdffff00',
 0x2231C753:'c68110fdffff01',
 0x2231C75A:'8931',
 0x2231C75C:'5e',
 0x2231C75D:'c20400',
 0x2231C760:'8b917cfcffff',
 0x2231C766:'85d2',
 0x2231C76A:'830a01',
 0x2231C76D:'8931',
 0x2231C76F:'5e',
 0x2231C770:'c20400',
 0x2231C95E:'8d8ef0040000',
}
CALL_EDGES={
 0x2231C3E3:0x222CD050,
 0x2231C964:0x2231C740,
}
TAIL_JUMPS={0x2231C441:0x2231C2F0}

def _rel32(site,target,opcode):
    return bytes((opcode,))+struct.pack('<i',target-site-5)

def verify_original_scattergun(pe:PEImage,*,check_hashes=True):
    if pe.base!=0x22000000:
        raise VerificationError('original server PE32 image base mismatch')
    if pe.at(ORIGINAL_VTABLE-4,4)!=struct.pack('<I',ORIGINAL_COL):
        raise VerificationError('CWeaponScatterGun original RTTI locator mismatch')
    if pe.at(ORIGINAL_COL+12,4)!=struct.pack('<I',ORIGINAL_RTTI_DESCRIPTOR):
        raise VerificationError('CWeaponScatterGun original RTTI TypeDescriptor mismatch')
    if pe.at(ORIGINAL_RTTI_DESCRIPTOR+8,len(TYPE_NAME))!=TYPE_NAME:
        raise VerificationError('CWeaponScatterGun original RTTI class-name mismatch')
    for slot,entry in VTABLE_SLOTS.items():
        if pe.at(ORIGINAL_VTABLE+4*slot,4)!=struct.pack('<I',entry):
            raise VerificationError('CWeaponScatterGun original vtable slot %d mismatch'%slot)
    for addr,raw_hex in OPCODES.items():
        want=bytes.fromhex(raw_hex)
        if pe.at(addr,len(want))!=want:
            raise VerificationError('CWeaponScatterGun original x86 opcode mismatch at %#x'%addr)
    for site,target in CALL_EDGES.items():
        if pe.at(site,5)!=_rel32(site,target,0xe8):
            raise VerificationError('CWeaponScatterGun original CALL mismatch at %#x'%site)
    for site,target in TAIL_JUMPS.items():
        if pe.at(site,5)!=_rel32(site,target,0xe9):
            raise VerificationError('CWeaponScatterGun original tail JMP mismatch at %#x'%site)
    if check_hashes:
        for name,(addr,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(addr,length)).hexdigest()!=digest:
                raise VerificationError('CWeaponScatterGun original full-method SHA mismatch: '+name)
    return {
       'original_class':'CWeaponScatterGun',
       'full_original_method_hashes':len(REGIONS) if check_hashes else 0,
       'sparse_x86_witnesses':len(OPCODES),
       'direct_CALL_edges':len(CALL_EDGES),
       'tail_JMP_edges':len(TAIL_JUMPS),
       'verified_vtable_slots':len(VTABLE_SLOTS),
       'original_methods':{
           'ammo_snapshot_update':'0x2231C3E0',
           'secondary_clip_decrement':'0x2231C740',
           'secondary_fire_caller':'0x2231C810',
           'deferred_action_not_reconstructed':'0x2231C2F0'},
       'verified_native_offsets':{
           'primary_clip':'+0x4EC','secondary_clip_arg':'+0x4F0',
           'tracking_enable':'+0x578','prior_primary':'+0x57C',
           'prior_secondary':'+0x580','live_secondary':'+0x584',
           'pending_action':'+0x589','notification_mode':'+0x1F0',
           'notification_byte':'+0x200','notification_ptr':'+0x16C'},
       'scope':[
           'Base callback 0x222CD050 and deferred callback 0x2231C2F0 remain unresolved.',
           'C++17 model preserves normal field decisions but not native original object ABI.',
           'Secondary clip decrement wraps uint32; no invented zero-ammo guard.',
       ],
    }

def audit(client_bytes:bytes,server_bytes:bytes):
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA:
        raise VerificationError('wrong original client.dll SHA256')
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('wrong original server.dll SHA256')
    return {'original_server_scattergun_ammo':verify_original_scattergun(
        PEImage(server_bytes))}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()
