#!/usr/bin/env python3
"""Phase160: exact 2006 original SiN CHudElement HUD visibility verifier.

Verifies the complete owner-supplied PE32 DLL SHA256 fingerprints, MSVC
CHudElement RTTI, all 13 original vtable slots, eight full function-region
SHA256s, sparse original opcode witnesses, and four original call edges.
Never writes or commits any proprietary original executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import PEImage, VerificationError

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'hud_element_constructor':(0x24142220,32,'1b26c2de954f86fcc47c355833261a30cd32d9976ae2b9c1a71a89f187c56a14'),
 'hud_element_virtual_slot2_noop':(0x24142240,1,'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e'),
 'global_hud_mask_gate':(0x24142250,125,'d81fb46c0b4c26ad6364969d6f8d99b0e9a479ff3ef181861e2680a5eaf7b3dd'),
 'hud_element_virtual_slot9_should_draw':(0x24142A60,22,'e21dd7b8dfcbbb81a780740afe04a24e02841e2ec2380cafe930f75ec591f3d9'),
 'hud_element_virtual_slot8_get_identity':(0x24140770,4,'2e339cdc5de837ec151dbde90057caeabba18ac3ea32e7c2cffc8f95df41a6ff'),
 'hud_element_virtual_slot10_get_byte04':(0x24140780,4,'e0eb8cffec07b89f095aa1e60d89eed1f3338b76216e649ad3aeb3ba92dc7d04'),
 'hud_element_virtual_slot11_set_byte04':(0x24141830,10,'bd85059e1d7267ad70adf88bb4cea9f4c8413c98ecc8813abe2560acb18983c9'),
 'hud_element_virtual_slot12_set_mask08':(0x24141850,10,'b35caac991eb0d06657c3fbba49c85de5f83e8cf4de0b07b70fa4b90cb5770fd'),
}
RTTI={0x2429B6E8:0x242E72E8,0x242E72F4:0x242FBD14}
VTABLE=[
 0x24144410,0x24140790,0x24142240,0x24140720,0x24140730,
 0x24140740,0x24140750,0x24140760,0x24140770,0x24142A60,
 0x24140780,0x24141830,0x24141850,
]
TYPE_VA=0x242FBD1C
TYPE_NAME=b'.?AVCHudElement@@\0'
# Sparse witnesses only. The original complete binary regions are *not*
# reconstructed in synthetic tests or embedded in public code.
OPCODES={
 0x24142220:'8b542404',0x24142228:'c700ecb62924',
 0x24142234:'89500c',0x24142237:'884810',
 0x2414223D:'c20400',
 0x24142250:'8b0d84f63324',0x24142258:'ff5068',
 0x2414225D:'7505',0x24142265:'8b35f08c3324',
 0x24142275:'8b0da0233624',0x2414227B:'8b412c',
 0x24142281:'8b9e8c0d0000',0x2414228B:'f6c304',
 0x2414229B:'f6c110',0x241422A4:'ff9290010000',
 0x241422AA:'85c0',0x241422B2:'f6c120',
 0x241422B7:'80be150e000000',
 0x241422C2:'23c1',0x241422C6:'1bc0',
 0x241422C9:'f7d8',0x241422CC:'c20400',
 0x24142A60:'8b4108',0x24142A64:'b9d0233624',
 0x24142A6E:'f6d8',0x24142A72:'83c001',0x24142A75:'c3',
 0x24140770:'8b410c',0x24140780:'8a4104',
 0x24141830:'8a442404',0x24141834:'884104',
 0x24141850:'8b442404',0x24141854:'894108',
}
CALLS={
 0x24142A69:0x24142250, # CHudElement slot9 delegates mask to global gate
 0x2422FFA6:0x24142A60, # original client HUD caller of virtual slot9
 0x24235E89:0x24142220, # original subclass calls base constructor
 0x24235F3D:0x24141850, # original subclass sets per-element mask
}

def verify_original_hud_element(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError('original client PE32 image base mismatch')
    for ptr,target in RTTI.items():
        if pe.at(ptr,4)!=struct.pack('<I',target):
            raise VerificationError('original CHudElement RTTI mismatch at %#x'%ptr)
    if pe.at(TYPE_VA,len(TYPE_NAME))!=TYPE_NAME:
        raise VerificationError('original CHudElement type descriptor mismatch')
    for slot,target in enumerate(VTABLE):
        if pe.at(0x2429B6EC+4*slot,4)!=struct.pack('<I',target):
            raise VerificationError('original CHudElement virtual slot %d mismatch'%slot)
    for site,hexcode in OPCODES.items():
        expected=bytes.fromhex(hexcode)
        if pe.at(site,len(expected))!=expected:
            raise VerificationError('original CHudElement opcode mismatch at %#x'%site)
    for site,target in CALLS.items():
        expected=b'\xe8'+struct.pack('<i',target-site-5)
        if pe.at(site,5)!=expected:
            raise VerificationError('original CHudElement rel32 CALL mismatch at %#x'%site)
    if check_hashes:
        for label,(va,length,wanted) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=wanted:
                raise VerificationError('original CHudElement full code SHA mismatch: '+label)
    return {
        'original_class':'CHudElement',
        'original_type_descriptor_va':'0x242FBD14',
        'original_col_va':'0x242E72E8',
        'original_vtable_va':'0x2429B6EC',
        'verified_original_vtable_slots':len(VTABLE),
        'verified_original_full_method_hashes':len(REGIONS) if check_hashes else 0,
        'verified_sparse_opcode_sites':len(OPCODES),
        'verified_original_rel32_call_edges':len(CALLS),
        'base_constructor':'0x24142220',
        'visibility_mask_gate':'0x24142250',
        'slot9_negating_gate':'0x24142A60',
        'gate_sources':{
            'engine_global':'0x2433F684, virtual +0x68',
            'local_player_global':'0x24338CF0',
            'override_global':'0x243623A0, field +0x2C',
            'player_flags':'+0xD8C','player_signed_virtual':'+0x190',
            'player_condition_byte':'+0xE15',
        },
        'scope':'Host-neutral original CHudElement behavior only; engine objects and VGUI drawing are NOT reconstructed.',
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA:
        raise VerificationError('wrong original 2006 client.dll SHA256')
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('wrong original 2006 server.dll SHA256')
    result=verify_original_hud_element(PEImage(client_bytes))
    result['original_dll_sha256']={'client':CLIENT_SHA,'server':SERVER_SHA}
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    text=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(text,encoding='utf-8')
    else:
        print(text,end='')
if __name__=='__main__':
    main()
