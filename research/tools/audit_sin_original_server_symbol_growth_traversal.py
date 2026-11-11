#!/usr/bin/env python3
"""Phase176: original 2006 SiN server.dll symbol storage growth and search.

Original-owner-only SHA256 audit. Checks the exact Windows PE32 client and
server pair (by chained Phase175 audit), four entire native region hashes,
independent sparse opcode witnesses, and exact CALL rel32 destinations.
Never commit owner-provided original DLL bytes or full native function bodies.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_symbol_tree_storage import audit as phase175_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
 'native_twelve_byte_record_reserve':(0x2228E200,115,
 'bf973beee94fe5f7c2bd5e649e41eb973f99bb49dee68e28b9861757796adb2b'),
 'native_four_byte_record_reserve':(0x2228E280,112,
 '1b91b29a1820e36300655fa1f9440dfc0f2653be30b61ca5a6f1ea44d6e8565b'),
 'native_symbol_tree_insertion_traversal':(0x2228E470,261,
 '52909d48c6bde81a72b0e10e3f871e8d13de62ab03ca3a3aa2ffe6b145f3b871'),
 'native_symbol_storage_reset':(0x2228E5B0,58,
 '341f65a09dc5ce440ffbf168cdc6403706b1c409fd77f272c2adc041f1d5754b')
}
# Intentionally sparse original instruction witnesses; these do not recreate
# any complete original proprietary function. Addresses are x86 instruction VAs.
OPCODES={
 0x2228E200:'56',0x2228E203:'8b4e08',0x2228E206:'83f9ff',
 0x2228E209:'7464',0x2228E214:'3bc2',0x2228E21B:'85c0',
 0x2228E223:'03c1',0x2228E22A:'03c0',0x2228E231:'c7460403000000',
 0x2228E238:'395604',0x2228E23D:'8b0e',0x2228E246:'8d1440',
 0x2228E249:'03d2',0x2228E257:'8906',0x2228E25D:'8d0440',
 0x2228E26D:'8906',0x2228E270:'c20400',
 0x2228E280:'56',0x2228E283:'8b4e08',0x2228E286:'83f9ff',
 0x2228E289:'7461',0x2228E294:'3bc2',0x2228E2A3:'03c1',
 0x2228E2AA:'03c0',0x2228E2B1:'c7460408000000',
 0x2228E2B8:'395604',0x2228E2BD:'8b06',0x2228E2C3:'8b4e04',
 0x2228E2C6:'03c9',0x2228E2D4:'8906',0x2228E2DA:'8b5604',
 0x2228E2EA:'8906',0x2228E2ED:'c20400',
 0x2228E470:'8b44240c',0x2228E474:'53',0x2228E479:'55',
 0x2228E47D:'668b7d10',0x2228E481:'6681ffffff',
 0x2228E486:'66c703ffff',0x2228E48B:'c60000',
 0x2228E48E:'0f84db000000',0x2228E494:'56',
 0x2228E499:'0fb7c7',0x2228E49F:'66893b',
 0x2228E4A9:'8d440e08',0x2228E4B4:'ff5500',
 0x2228E4BA:'84c0',0x2228E4C2:'c60101',
 0x2228E4C5:'f6053401592201',
 0x2228E4D8:'090534015922',0x2228E4DE:'66890d2c015922',
 0x2228E4F9:'6681ffffff',0x2228E503:'668b3c30',
 0x2228E519:'c60200',0x2228E51C:'f6053401592201',
 0x2228E52F:'090d34015922',0x2228E54E:'663bf8',
 0x2228E55F:'668b7802',0x2228E568:'0f8527ffffff',
 0x2228E572:'c21000',
 0x2228E5B0:'56',0x2228E5B3:'837e08ff',
 0x2228E5B7:'c7460c00000000',0x2228E5BE:'7423',
 0x2228E5C0:'8b06',0x2228E5C4:'740f',
 0x2228E5CF:'c70600000000',0x2228E5D5:'8b06',
 0x2228E5D7:'c7460400000000',0x2228E5DE:'894610',
 0x2228E5E3:'8b0e',0x2228E5E5:'894e10'
}
CALLS={
 0x2228E24F:0x2226E400, # original 12-byte array reallocation
 0x2228E265:0x2226E390, # original 12-byte allocation
 0x2228E2CC:0x2226E400, # original 4-byte array reallocation
 0x2228E2E2:0x2226E390, # original 4-byte allocation
 0x2228E5C7:0x2226E3E0, # guarded cleanup free
}
INCOMING={
 0x2228E63C:0x2228E200, # Phase175 slot allocator resize
 0x2228F72B:0x2228E470, # Phase174 symbol preparation search
}
SENTINEL_WORDS=(0x2259012C,0x2259012E,0x22590130,0x22590132)
SENTINEL_GUARD=0x22590134

def verify_original_symbol_growth(pe:PEImage, *, check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError('Phase176 expected genuine original server PE32 image base')
    for va,rawhex in OPCODES.items():
        raw=bytes.fromhex(rawhex)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('Phase176 original opcode witness mismatch at %#x'%va)
    for va,target in {**CALLS,**INCOMING}.items():
        wanted=b'\xe8'+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError('Phase176 original direct CALL mismatch at %#x'%va)
    if check_hashes:
        for name,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError('Phase176 whole original native function SHA mismatch: '+name)
    return {
       'verified_complete_function_hashes':len(REGIONS) if check_hashes else 0,
       'verified_sparse_opcode_witnesses':len(OPCODES),
       'verified_internal_direct_calls':len(CALLS),
       'verified_incoming_direct_calls':len(INCOMING),
       'native_bytes_sha256_pinned':sum(length for _,length,_ in REGIONS.values()),
       'record_strides_bytes':[12,4],
       'original_lazy_sentinel_words':[hex(x) for x in SENTINEL_WORDS],
       'original_lazy_sentinel_guard':hex(SENTINEL_GUARD),
       'original_functions':{name:{'va':hex(va),'length':length}
           for name,(va,length,_) in REGIONS.items()},
       'limits':[
         'Live native comparator, engine x86 allocators and object ABI are callbacks',
         'The portable host adds invalid index and growth-loop guards that the native code does not have',
         'Neither reconstructed Windows playable game DLLs nor Android/Quest3 gameplay achieved'
       ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase175_audit(client_bytes,server_bytes)
    report['original_server_symbol_growth_and_tree']=verify_original_symbol_growth(PEImage(server_bytes))
    return report

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    output=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(output,encoding='utf-8')
    else:print(output,end='')
if __name__=='__main__':main()