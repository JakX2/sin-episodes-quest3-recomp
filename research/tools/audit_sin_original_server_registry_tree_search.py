#!/usr/bin/env python3
"""Phase174: original 2006 server.dll tree-search and interner registration audit.

Requires private original Windows PE32 client.dll/server.dll, checks BOTH
complete DLL hashes and chains Phase173. Checks four complete native function
hashes, sparse x86 opcode witnesses and verified original rel32 CALL targets.
No proprietary complete executable body or original DLL is committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_registry_intern_insert import audit as phase173_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
 'original_registry_prepare_insertion':(0x2228A280,255,'54592a1af0f32a419dc34900a07841d6f7ddf15e56cc48b80c06da7a998b78eb'),
 'original_registry_guarded_storage_release':(0x2228A380,39,'6913ebe2f4e0b5df55d05094db7e47e45db00f0a3388932c3da16f16ed296a3d'),
 'original_symbol_binary_tree_lookup':(0x2228EB40,266,'e100a658453e531d18ff90797618c5f3d2fa61304b9d8f54c50b7458f8804550'),
 'original_symbol_record_registration':(0x2228F710,94,'7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d'),
}
# Independent, non-contiguous instruction witnesses only, not function bodies.
OPCODES={
 0x2228A280:"8b442408",
 0x2228A28C:"57",
 0x2228A299:"668918",
 0x2228A2A5:"56",
 0x2228A2B4:"8b4c2414",
 0x2228A2C1:"c1e604",
 0x2228A2CD:"83c408",
 0x2228A2DB:"f60590df582201",
 0x2228A2E9:"090590df5822",
 0x2228A2F6:"66891d8adf5822",
 0x2228A304:"66a38edf5822",
 0x2228A312:"668b3c30",
 0x2228A31F:"668b38",
 0x2228A32B:"f60590df582201",
 0x2228A339:"090590df5822",
 0x2228A346:"66891d8adf5822",
 0x2228A354:"66a38edf5822",
 0x2228A362:"03c6",
 0x2228A36F:"663bfb",
 0x2228A37B:"5b",
 0x2228A380:"56",
 0x2228A387:"741c",
 0x2228A38F:"50",
 0x2228A398:"c70600000000",
 0x2228A3A5:"5e",
 0x2228EB40:"55",
 0x2228EB4D:"0f84f1000000",
 0x2228EB59:"8da42400000000",
 0x2228EB67:"8d3440",
 0x2228EB75:"53",
 0x2228EB81:"f6053401592201",
 0x2228EB8F:"b801000000",
 0x2228EBA1:"66890d2e015922",
 0x2228EBAF:"66a332015922",
 0x2228EBBC:"8b4704",
 0x2228EBCC:"668b28",
 0x2228EBD8:"53",
 0x2228EBE4:"84c0",
 0x2228EBF4:"7524",
 0x2228EC01:"66a32c015922",
 0x2228EC0D:"66a330015922",
 0x2228EC1A:"663be8",
 0x2228EC26:"b82c015922",
 0x2228EC34:"0f8526ffffff",
 0x2228EC40:"5d",
 0x2228F710:"8b442408",
 0x2228F719:"56",
 0x2228F722:"51",
 0x2228F729:"8bce",
 0x2228F730:"8bce",
 0x2228F737:"8b4c2410",
 0x2228F741:"50",
 0x2228F74B:"6683461201",
 0x2228F753:"8d1440",
 0x2228F75D:"85c0",
 0x2228F765:"668bc7",
}
# Four exact original internal direct CALL rel32 edges in these regions.
CALLS={
 0x2228A390:0x2226E3E0,
 0x2228F72B:0x2228E470,
 0x2228F732:0x2228E620,
 0x2228F746:0x2228F460,
}
# Confirmed original incoming CALL sites, not inferred function ownership.
INCOMING={
 0x2228B0BB:0x2228A280,
 0x2228F3EC:0x2228EB40,
 0x2228F7DB:0x2228EB40,
 0x2228F88D:0x2228F710,
}
SENTINELS={
  'insertion_words':0x2258DF88,
  'insertion_guard':0x2258DF90,
  'symbol_words':0x2259012C,
  'symbol_guard':0x22590134,
}

def verify_original_tree_search(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError('original 2006 server image base mismatch')
    for va,rawhex in OPCODES.items():
        raw=bytes.fromhex(rawhex)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('original Phase174 opcode mismatch at %#x'%va)
    for va,target in {**CALLS,**INCOMING}.items():
        wanted=bytes((0xe8,))+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError('original Phase174 CALL mismatch at %#x'%va)
    if check_hashes:
        for name,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError('original Phase174 native full function SHA mismatch: '+name)
    return {
      'verified_original_complete_function_hashes':len(REGIONS) if check_hashes else 0,
      'verified_original_native_sparse_witnesses':len(OPCODES),
      'verified_original_internal_direct_calls':len(CALLS),
      'verified_original_incoming_calls':len(INCOMING),
      'native_bytes_covered_by_function_hashes':sum(n for _,n,_ in REGIONS.values()),
      'original_table_stride_insert':16,'original_table_stride_symbol':12,
      'original_sentinel_addresses':{k:hex(v) for k,v in SENTINELS.items()},
      'native_functions':{name:{'va':hex(va),'length':n}
                           for name,(va,n,_) in REGIONS.items()},
      'limitations':[
       'Two tree comparators and native symbol table allocators remain host callbacks',
       'The native insertion search walks until the original 16-bit sentinel',
       'These models are not a playable Windows DLL pair or a Quest 3 gameplay port'
      ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase173_audit(client_bytes,server_bytes)
    report['original_server_registry_tree_search']=verify_original_tree_search(
       PEImage(server_bytes))
    return report

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()