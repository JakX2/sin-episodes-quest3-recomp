#!/usr/bin/env python3
"""Phase173 owner-only original SiN 2006 registry insert/string interner audit.

Pins four complete original native function hashes (510 bytes total), 55
noncontiguous x86 opcode witnesses, ten direct call targets and seven
incoming factory/registration links, chained through Phase172 and both
private full-file DLL SHA256 checks. No proprietary function bytes are stored.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_registry_registration import audit as phase172_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS={
 "original_registry_table_insert":(0x2228B0A0,102,
  "e09e609fb445ee14e43c8dcd9633b786c25609c1a81be79087cd8b3e11621ecb"),
 "original_registry_slot_acquire":(0x2228A3B0,98,
  "364bc2a8b99b85727e3b1a44260fd397cd89ff2246e909323bd5169fe78c8ab1"),
 "original_string_pool_first_fit":(0x2228EA30,49,
  "4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25"),
 "original_string_symbol_intern":(0x2228F7A0,261,
  "d84e5bbcbf05a7501ef5094690ae0827a991d8e63a33d33fe60d46a32d15268f"),
}
OPCODES={
 0x2228B0A0:"8b442408",0x2228B0A9:"56",0x2228B0B2:"51",
 0x2228B0C0:"8bce",0x2228B0CB:"8bf8",0x2228B0D3:"57",
 0x2228B0DB:"8b4604",0x2228B0E3:"0fb7d7",0x2228B0ED:"85c0",
 0x2228B0F7:"8b5304",0x2228B100:"5f",
 0x2228A3B0:"56",0x2228A3BB:"57",0x2228A3C5:"750a",
 0x2228A3D1:"668b7e16",0x2228A3DC:"eb12",0x2228A3E8:"668b5002",
 0x2228A3F0:"8b4e04",0x2228A3F9:"8d440808",0x2228A401:"66c700ffff",
 0x2228A409:"668bc7",0x2228A411:"c3",
 0x2228EA30:"53",0x2228EA39:"57",0x2228EA43:"8b0a",
 0x2228EA4C:"7d0d",0x2228EA54:"3bc6",0x2228EA5C:"5e",
 0x2228F7A0:"83ec08",0x2228F7A9:"85f6",0x2228F7B3:"5e",
 0x2228F7BD:"c20800",0x2228F7C6:"bfffff0000",
 0x2228F7CE:"897330",0x2228F7D6:"66897c2426",
 0x2228F7E0:"663bc7",0x2228F7E9:"8bc6",0x2228F7F2:"83c001",
 0x2228F7FB:"8d7801",0x2228F80B:"83f8ff",
 0x2228F816:"8bef",0x2228F81F:"8d4d0b",
 0x2228F828:"83c404",0x2228F830:"8d542420",
 0x2228F838:"c7400400000000",0x2228F842:"52",
 0x2228F84C:"8b1481",0x2228F858:"8bcf",
 0x2228F862:"8d7c1708",0x2228F86A:"83e103",
 0x2228F873:"014a04",0x2228F87B:"6689442410",
 0x2228F885:"50",0x2228F892:"5d",0x2228F89C:"8bc1",
}
DIRECT_CALLS={
 0x2228B0BB:0x2228A280,
 0x2228B0C2:0x2228A3B0,
 0x2228B0D6:0x2228AF00,
 0x2228A3CC:0x22288390,
 0x2228A3E3:0x22288300,
 0x2228F7DB:0x2228EB40,
 0x2228F806:0x2228EA30,
 0x2228F823:0x2226E390,
 0x2228F844:0x2228ED80,
 0x2228F88D:0x2228F710,
}
INCOMING_CALLS={
 0x2228B170:0x2228B0A0,
 0x2228B1C1:0x2228B0A0,
 0x2228AEDA:0x2228F7A0,
 0x2228B15A:0x2228F7A0,
 0x2228B1AB:0x2228F7A0,
 0x2228B02E:0x2228F7A0,
 0x2228F8C9:0x2228F7A0,
}

def verify_original_registry_intern_insert(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for va,words in OPCODES.items():
        raw=bytes.fromhex(words)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError("original registry intern opcode mismatch at %#x"%va)
    for va,target in {**DIRECT_CALLS,**INCOMING_CALLS}.items():
        expected=b'\xE8'+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=expected:
            raise VerificationError("original registry intern CALL mismatch at %#x"%va)
    if check_hashes:
        for name,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError("original registry intern whole function SHA mismatch: "+name)
    return {
        "verified_complete_original_function_hashes":len(REGIONS) if check_hashes else 0,
        "verified_native_function_bytes":sum(row[1] for row in REGIONS.values()) if check_hashes else 0,
        "verified_sparse_opcode_witnesses":len(OPCODES),
        "verified_internal_direct_calls":len(DIRECT_CALLS),
        "verified_incoming_calls":len(INCOMING_CALLS),
        "native_missing_symbol_index":"0xFFFF",
        "native_min_string_pool_capacity":0x800,
        "native_pool_raw_allocation_extra_bytes":11,
        "native_registry_record_stride":16,
        "native_registry_record_key_word_offset":8,
        "native_registry_record_extra_dword_offset":12,
        "original_functions":{
            name:{"va":hex(va),"length":n}
            for name,(va,n,_) in REGIONS.items()},
        "limitations":[
            "Native search/evict, slot chaining and string-entry storage are host callbacks",
            "Original first-fit uses signed 32-bit subtraction with x86 wrapping",
            "The native interner assumes successful allocator/string-buffer operations",
            "Portable code is neither original MSVC32 ABI nor a playable DLL pair"
        ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase172_audit(client_bytes,server_bytes)
    report["original_server_registry_intern_insert"]=verify_original_registry_intern_insert(
        PEImage(server_bytes))
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")
if __name__=="__main__":main()