#!/usr/bin/env python3
"""SiN 2006 original VGUI keymap and sound-token namespace audit.

Requires all six privately owned game/engine/native binaries; chains full
Phase98 verification. Original executable bytes are never exported.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_factories import PE32, VerificationError
from audit_sourcevr_native_sin_factories import ELF64
from audit_original_engine_sound_vgui_abi import audit as phase98_audit

OLD_VGUI_INIT_VA=0x100227A0
OLD_VGUI_INIT_SIZE=0x403
OLD_VGUI_INIT_SHA256='205064fb73c9f792c651907085e04c740ee7a8f406b845837e18dde957d307c0'
OLD_VGUI_KEY_TABLE_VA=0x10053270
OLD_VGUI_IMMEDIATE_FIRST=0x100227CD
OLD_VGUI_IMMEDIATE_COUNT=98
OLD_VGUI_EXPECTED_OUTPUT_SHA256='6c743e453ce5d6b91214fa2db2fa6b935d7b1ca886c34ebda9538f1ab80a10b0'
OLD_SOUND_VALIDATOR_VA=0x2C005FD0
OLD_SOUND_VALIDATOR_BYTES=23
OLD_SOUND_VALIDATOR_SHA256='4742cc7ff52778ba3ecf9cb0b021116ec4ebcf094068f99b9e01b4a305638145'
NATIVE_SOUND_VALIDATOR_VA=0x13520
NATIVE_SOUND_VALIDATOR_BYTES=76
NATIVE_SOUND_VALIDATOR_SHA256='44f13012e9a0558361a5e4d8ea8fd9b01bddc8f6c04cded69214ea1719272a26'

ORIGINAL_KEY_OPCODE_ANCHORS={
    0x100227A0:'803d743e051000',
    0x100227AD:'57',
    0x100227AE:'ba01000000',
    0x100227B3:'b900010000',
    0x100227B8:'33c0',
    0x100227BA:'bf70320510',
    0x100227BF:'f3ab',
    0x100227C1:'8815743e0510',
    0x100227C7:'891530330510',
    0x10022BA1:'5f',
    0x10022BA2:'c3',
    0x1001CB60:'e83b5c0000',
    0x1001CB69:'85c0',
    0x1001CB6B:'7c11',
    0x1001CB6D:'3d00010000',
    0x1001CB74:'8b048570320510',
    0x1001CB7E:'33c0',
}
ORIGINAL_SOUND_OPCODE_ANCHORS={
    0x2C005FD0:'8b442404', 0x2C005FD4:'85c0',
    0x2C005FD6:'7c0a', 0x2C005FD8:'3b4170',
    0x2C005FDB:'7d05', 0x2C005FDD:'b001',
    0x2C005FE2:'32c0', 0x2C005FE4:'c20400',
}
NATIVE_SOUND_OPCODE_ANCHORS={
    0x13520:0xB9405009, 0x13524:0x12003C28,
    0x13528:0x6B09011F, 0x13530:0x7A48B140,
    0x13534:0x54000062, 0x13540:0xF9402409,
    0x13544:0x8B081129, 0x13548:0x7940112A,
    0x13554:0x79401529, 0x13564:0x52800020,
}
def check_sites(pe,sites,is_arm=False):
    for va,code in sites.items():
        expected=struct.pack('<I',code) if is_arm else bytes.fromhex(code)
        if pe.at(va,len(expected))!=expected:
            raise VerificationError('Phase99 opcode mismatch at %#x'%va)

def check_sha(pe,va,n,expect,strict=True):
    actual=hashlib.sha256(pe.at(va,n)).hexdigest()
    if strict and actual!=expect:
        raise VerificationError('Phase99 code SHA256 mismatch at %#x'%va)
    return actual

def reconstruct_legacy_keymap(old_vgui:PE32,strict=True):
    check_sha(old_vgui,OLD_VGUI_INIT_VA,OLD_VGUI_INIT_SIZE,
              OLD_VGUI_INIT_SHA256,strict)
    check_sites(old_vgui,ORIGINAL_KEY_OPCODE_ANCHORS)
    values=[0]*256
    values[48]=1
    used={48}
    output_values={1}
    for idx in range(OLD_VGUI_IMMEDIATE_COUNT):
        va=OLD_VGUI_IMMEDIATE_FIRST+idx*10
        insn=old_vgui.at(va,10)
        if insn[:2]!=b'\xC7\x05':
            raise VerificationError('Phase99 key initializer not consecutive MOV instructions')
        address,value=struct.unpack_from('<II',insn,2)
        if address<OLD_VGUI_KEY_TABLE_VA or address>=OLD_VGUI_KEY_TABLE_VA+256*4 or (address-OLD_VGUI_KEY_TABLE_VA)%4:
            raise VerificationError('Phase99 key-table store is out of range or unaligned')
        key=(address-OLD_VGUI_KEY_TABLE_VA)//4
        if key in used or value==0 or value>103 or value in output_values:
            raise VerificationError('Phase99 duplicate key/output or invalid key mapping')
        used.add(key);output_values.add(value)
        values[key]=value
    if len(used)!=99 or sum(1 for v in values if v)!=99:
        raise VerificationError('Phase99 legacy key map count mismatch')
    digest=hashlib.sha256(struct.pack('<256I',*values)).hexdigest()
    if strict and digest!=OLD_VGUI_EXPECTED_OUTPUT_SHA256:
        raise VerificationError('Phase99 reconstructed legacy key map SHA256 mismatch')
    return values,{
       'initialization_va':hex(OLD_VGUI_INIT_VA),
       'one_time_flag_va':'0x10053E74','table_bss_va':hex(OLD_VGUI_KEY_TABLE_VA),
       'table_entries':256,'implicit_register_based_store':{'key':48,'value':1},
       'consecutive_immediate_stores':98,'nonzero_entries':99,
       'unmapped_keys':157,'unique_nonzero_values':99,
       'full_table_sha256':digest,
       'full_initializer_sha256':hashlib.sha256(old_vgui.at(OLD_VGUI_INIT_VA,OLD_VGUI_INIT_SIZE)).hexdigest(),
       'original_method':'VGUI_System010 vtable index 24: Init once, then key[0..255] else zero',
       'no_static_image_table':'The original lookup table resides in zero-initialized PE memory, not file-backed bytes'
    }

def verify_sound_namespace(old_sound:PE32,native_sound:ELF64,strict=True):
    old_sha=check_sha(old_sound,OLD_SOUND_VALIDATOR_VA,OLD_SOUND_VALIDATOR_BYTES,
                      OLD_SOUND_VALIDATOR_SHA256,strict)
    native_sha=check_sha(native_sound,NATIVE_SOUND_VALIDATOR_VA,
                         NATIVE_SOUND_VALIDATOR_BYTES,NATIVE_SOUND_VALIDATOR_SHA256,strict)
    check_sites(old_sound,ORIGINAL_SOUND_OPCODE_ANCHORS)
    check_sites(native_sound,NATIVE_SOUND_OPCODE_ANCHORS,True)
    return {
       'old_slot4_validator_va':hex(OLD_SOUND_VALIDATOR_VA),
       'old_validator_sha256':old_sha,
       'old_semantics':'32-bit signed token >=0 AND token < signed count at original this+0x70',
       'native_slot8_validator_va':hex(NATIVE_SOUND_VALIDATOR_VA),
       'native_validator_sha256':native_sha,
       'native_semantics':'first masks argument to 16-bit, then checks native table and generation fields',
       'mismatch':'legacy 32-bit namespace cannot be forwarded by truncating to 16-bit',
       'old_name_to_token_slot':3,'native_candidate_name_to_token_slot':7,
       'old_token_to_name_slot':6,'native_candidate_token_to_name_slot':10,
       'native_candidates_abi_verified':False
    }

def audit(client,server,old_sound,old_vgui,native_sound,native_vgui):
    prior=phase98_audit(client,server,old_sound,old_vgui,native_sound,native_vgui)
    keys,keymeta=reconstruct_legacy_keymap(PE32(old_vgui),True)
    snd=verify_sound_namespace(PE32(old_sound),ELF64(native_sound),True)
    return {
       'legacy_keyboard':keymeta,'sound_token':snd,
       'six_binaries_chained':True,'parent_verdict':prior['verdict'],
       'safe_live_sound_adapter':False,'portable_game_modules_ready':False
    },keys

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('client','server','old-sound','old-vgui','native-sound','native-vgui','output'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--export-keymap',type=Path,
      help='Optional private generated key-table JSON. Do not commit original binaries.')
    a=p.parse_args()
    report,values=audit(a.client.read_bytes(),a.server.read_bytes(),
      a.old_sound.read_bytes(),a.old_vgui.read_bytes(),
      a.native_sound.read_bytes(),a.native_vgui.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if a.export_keymap:
        a.export_keymap.parent.mkdir(parents=True,exist_ok=True)
        a.export_keymap.write_text(json.dumps({'verified_source':report['legacy_keyboard'],
                    'u32_le_entries':values},indent=2)+'\n',encoding='utf-8')
    print('Original VGUI key table: 99/256 mapped; legacy/native sound token namespaces incompatible')
if __name__=='__main__':main()
