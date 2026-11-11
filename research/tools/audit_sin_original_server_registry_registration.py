#!/usr/bin/env python3
"""Phase172 original SiN 2006 server classname registration and hash audit.

Owner-original complete PE32 SHA256, 4 entire native functions, sparse opcode
witnesses, direct CALL rel32 edges, registry vtable +0x00 and a genuine hash
caller. Chained through Phase171; no original executable bytes committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_factory_dispatch import audit as phase171_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
 'native_existing_classname_slot':(0x2228AEC0,60,
  '22925b759c92d8e17fea6181e5eb1ac9c6fbb3c68cb081b731592ff10e72c8ec'),
 'native_indirect_provider_registration':(0x2228B140,79,
  'a66c1b2ab5b3ff865fe672c5b02aa2ab10858ed3f867589593c3f6d537b09259'),
 'native_registry_vtable_direct_registration':(0x2228B190,78,
  'cc1922a54b06dc0518f57e6d20c30b0d7bf18fcfea58180fcfdcefbd2de79752'),
 'native_signed_byte_classname_hash':(0x2228B1E0,40,
  '1cbb4f8f3651d9dbfae4235ec60dce656bbf53569dca30ad8af027799571403a'),
}
OPCODES={
 0x2228AEC0:'83ec08',0x2228AEC3:'8b44240c',
 0x2228AEC8:'8bf1',0x2228AED0:'8d4e1c',
 0x2228AED3:'66c744240cffff',0x2228AEDF:'668b10',
 0x2228AEE2:'6a00',0x2228AEE9:'8bce',
 0x2228AEEB:'668954240c',0x2228AEF5:'5e',
 0x2228AEF6:'83c408',0x2228AEF9:'c20400',
 0x2228B140:'83ec08',0x2228B143:'8b44240c',
 0x2228B148:'8bf1',0x2228B150:'8d4e1c',
 0x2228B153:'66c744240cffff',0x2228B15F:'668b10',
 0x2228B162:'6a00',0x2228B169:'8bce',
 0x2228B16B:'668954240c',0x2228B175:'8b5604',
 0x2228B178:'8b742414',0x2228B17C:'8b36',
 0x2228B17E:'0fb7c8',0x2228B181:'c1e104',
 0x2228B184:'8974110c',0x2228B18C:'c20800',
 0x2228B190:'83ec08',0x2228B193:'8b442410',
 0x2228B198:'8d7104',0x2228B1A1:'8d4e1c',
 0x2228B1A4:'66c744240cffff',0x2228B1B0:'668b10',
 0x2228B1B3:'6a00',0x2228B1BA:'8bce',
 0x2228B1BC:'668954240c',0x2228B1C6:'8b5604',
 0x2228B1C9:'0fb7c8',0x2228B1CC:'8b442410',
 0x2228B1D0:'c1e104',0x2228B1D3:'8944110c',
 0x2228B1DB:'c20800',
 0x2228B1E0:'8b542404',0x2228B1E4:'8a0a',
 0x2228B1E6:'84c9',0x2228B1E8:'b805150000',
 0x2228B1ED:'7418',0x2228B1F0:'8bf0',
 0x2228B1F2:'c1e605',0x2228B1F5:'03f0',
 0x2228B1F7:'0fbec1',0x2228B1FA:'8a4a01',
 0x2228B1FD:'83c201',0x2228B200:'03c6',
 0x2228B202:'84c9',0x2228B204:'75ea',
 0x2228B207:'c3',
}
CALLS={
 0x2228AEDA:0x2228F7A0,
 0x2228AEF0:0x2228A760,
 0x2228B15A:0x2228F7A0,
 0x2228B170:0x2228B0A0,
 0x2228B1AB:0x2228F7A0,
 0x2228B1C1:0x2228B0A0,
}
INCOMING={0x222C7A13:0x2228B1E0}
VTABLE_START=0x22456324
VTABLE=(0x2228B190,0x22285550,0x22285580,0x2228B010)

def verify_original_registration(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError('incorrect original server PE32 image base')
    for va,hex_bytes in OPCODES.items():
        expected=bytes.fromhex(hex_bytes)
        if pe.at(va,len(expected))!=expected:
            raise VerificationError('original registry registration opcode mismatch at %#x'%va)
    for va,target in {**CALLS,**INCOMING}.items():
        expected=bytes((0xe8,))+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=expected:
            raise VerificationError('original registry registration CALL mismatch at %#x'%va)
    if pe.at(VTABLE_START,16)!=struct.pack('<4I',*VTABLE):
        raise VerificationError('original registry registration vtable mismatch')
    if check_hashes:
        for label,(va,n,sha) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=sha:
                raise VerificationError('original registry registration whole function SHA mismatch: '+label)
    return {
      'verified_complete_original_function_hashes':len(REGIONS) if check_hashes else 0,
      'verified_sparse_native_opcodes':len(OPCODES),
      'verified_internal_direct_calls':len(CALLS),
      'verified_incoming_hash_call':len(INCOMING),
      'verified_vtable_slots':len(VTABLE),
      'registry_virtual_zero':'0x2228B190 (class provider registration; not destructor)',
      'native_hash_seed':'0x1505',
      'native_hash_multiplier':33,
      'native_hash_byte_type':'signed int8',
      'record_byte_stride':16,
      'record_provider_word_offset':12,
      'original_functions':{
          k:{'va':hex(va),'length':n} for k,(va,n,_) in REGIONS.items()},
      'limitations':[
       'Symbol intern, native find/insert, and source engine storage remain host owned',
       'Native register path does not bound-check returned 16-bit index',
       'No original x86 game ABI or playable Windows/Android/Quest3 DLL replacement'
      ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=phase171_audit(client_bytes,server_bytes)
    result['original_server_registry_registration']=verify_original_registration(
        PEImage(server_bytes))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+chr(10)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()