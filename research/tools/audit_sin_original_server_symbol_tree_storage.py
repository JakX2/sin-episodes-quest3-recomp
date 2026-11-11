#!/usr/bin/env python3
"""Phase175 original-only 2006 SiN server symbol table storage auditor.

All five native function regions are verified via SHA256, 56 independent sparse
x86 opcode witnesses, six internal direct native calls and two confirmed
original callers, chaining previous Phase174 full DLL audit. Original
binaries and complete native bodies must remain private.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_registry_tree_search import audit as phase174_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    'native_node12_record_accessor': (0x2228E0E0, 17, '9047b1c9eb80f184fa071a6108f8601051de53ea7ff23b549b7ea28a6233fc18'),
    'native_node12_right_child_setter': (0x2228E450, 24, '3a19f97ad1b4b9d5abf34e4bbd2a7fff9dd645dee2b2aa3519ac2cca35cded05'),
    'native_node12_slot_allocator': (0x2228E620, 86, '9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b'),
    'native_append_capacity_reserve': (0x2228E680, 46, '4723bd92f6896d1f8887b2652295669e8280e5d4373b6cf2947b7947ed0927a4'),
    'native_node12_tree_linker': (0x2228F460, 109, '9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a'),
}

OPCODES = {
    0x2228E0E0: '0fb7442404',
    0x2228E0E5: '8b4904',
    0x2228E0E8: '8d0440',
    0x2228E0EB: '8d0481',
    0x2228E0EE: 'c20400',
    0x2228E450: '0fb7442404',
    0x2228E455: '8b4904',
    0x2228E458: '668b542408',
    0x2228E45D: '8d0440',
    0x2228E460: '6689548102',
    0x2228E465: 'c20800',
    0x2228E620: '56',
    0x2228E623: '668b4614',
    0x2228E62B: '57',
    0x2228E62E: '0fb74616',
    0x2228E635: '750a',
    0x2228E637: '6a01',
    0x2228E645: '8b4604',
    0x2228E64B: '668bc7',
    0x2228E651: '5f',
    0x2228E656: '5e',
    0x2228E658: '50',
    0x2228E65B: '8bf8',
    0x2228E666: '8b4604',
    0x2228E66C: '668bc7',
    0x2228E670: '66895614',
    0x2228E675: 'c3',
    0x2228E680: '56',
    0x2228E683: '8b460c',
    0x2228E686: '8b4e04',
    0x2228E68A: '8b7c240c',
    0x2228E68E: '8d1438',
    0x2228E693: '7e0c',
    0x2228E697: '03c7',
    0x2228E699: '50',
    0x2228E6A1: '017e0c',
    0x2228E6A6: '5f',
    0x2228E6A7: '894610',
    0x2228E6AB: 'c20400',
    0x2228F460: '8b5104',
    0x2228F464: '8b742408',
    0x2228F46B: '8d0440',
    0x2228F471: '668b54240c',
    0x2228F47B: '66895004',
    0x2228F47F: '66c74002ffff',
    0x2228F48A: '66c740060000',
    0x2228F492: '807c241000',
    0x2228F49A: '8b5104',
    0x2228F4A0: '740e',
    0x2228F4A3: '66893482',
    0x2228F4AC: '5e',
    0x2228F4B0: '56',
    0x2228F4B1: '6689748202',
    0x2228F4BB: '5e',
    0x2228F4BF: '56',
    0x2228F4CA: 'c20c00',
}

# Six original CALL rel32 instructions inside the five verified functions.
CALLS = {
    0x2228E63C: 0x2228E200, # symbol slot storage grow
    0x2228E65D: 0x2228E100, # native slot record locator
    0x2228E69C: 0x2228E280, # native reserve growth
    0x2228F4A7: 0x2228EDF0, # left branch balance
    0x2228F4B6: 0x2228EDF0, # right branch balance
    0x2228F4C4: 0x2228EDF0, # root branch balance
}
# Verified original incoming call sites from Phase174 symbol installation.
INCOMING = {
    0x2228F732: 0x2228E620,
    0x2228F746: 0x2228F460,
}

def verify_native_symbol_storage(pe: PEImage, *, check_hashes: bool=True) -> dict:
    if pe.base != 0x22000000:
        raise VerificationError('Phase175: not original server.dll image base')
    for va, wanted in OPCODES.items():
        x=bytes.fromhex(wanted)
        if pe.at(va,len(x))!=x:
            raise VerificationError('Phase175 opcode mismatch at %#x'%va)
    for va,target in {**CALLS,**INCOMING}.items():
        wanted=bytes([0xE8])+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError('Phase175 call rel32 mismatch at %#x'%va)
    if check_hashes:
        for name,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError('Phase175 original complete function hash mismatch: '+name)
    return {
        'verified_complete_original_function_hashes':len(REGIONS) if check_hashes else 0,
        'verified_native_bytes_covered':sum(n for va,n,d in REGIONS.values()) if check_hashes else 0,
        'verified_sparse_original_opcodes':len(OPCODES),
        'verified_native_internal_calls':len(CALLS),
        'verified_native_incoming_calls':len(INCOMING),
        'functions':{name:{'va':hex(va),'native_bytes':length} for name,(va,length,sha) in REGIONS.items()},
        'known_engine_owned_dependencies':['0x2228E200','0x2228E100','0x2228E280','0x2228EDF0'],
        'limitations':[
            'Original 2006 x86 engine ABI and native allocator not yet rebuilt',
            'Original tree rebalance algorithm 0x2228EDF0 remains external',
            'Portable reference helpers are not independently playable DLLs'
        ]
    }

def audit(client_bytes: bytes, server_bytes: bytes) -> dict:
    report=phase174_audit(client_bytes,server_bytes)
    report['original_server_symbol_tree_storage']=verify_native_symbol_storage(PEImage(server_bytes))
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else: print(report,end='')
if __name__=='__main__':main()