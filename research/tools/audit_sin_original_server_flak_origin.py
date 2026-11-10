#!/usr/bin/env python3
"""Phase168 genuine original 2006 server.dll flak spawn / origin code audit.

SHA256-pins whole original function regions, 62 non-overlapping native x86
instruction witnesses, all ten native direct CALL rel32 sites and original
"flak" classname literal. Chains complete dual-DLL audit through Phase167.
Do not commit either proprietary DLL or full disassemblies/regions.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_scattergun_secondary_fire import audit as phase167_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
 "scattergun_spawn_flak":(0x2231B9F0,134,
  "3b8c5a4ad9f86b45224b4e4f06a346254818f1137cf9ae375a69f0ea19315b86"),
 "server_entity_origin_parent_setter":(0x22141A00,307,
  "c5a9e2bb9cef0f468373a3a7205a17a1fefe297c1ddcbcebc140349c873d5c4f"),
}
OPCODES = {
 0x2231B9F0:"56",0x2231B9F2:"6aff",0x2231B9F4:"68d8a14722",
 0x2231B9FE:"8bf0",0x2231BA00:"8b442414",0x2231BA04:"6a00",
 0x2231BA0D:"8b7c2428",0x2231BA14:"85ff",0x2231BA16:"743e",
 0x2231BA1C:"ff92fc000000",0x2231BA22:"85c0",
 0x2231BA2A:"ff90fc000000",0x2231BA37:"85c0",
 0x2231BA3F:"ff5208",0x2231BA44:"898694070000",
 0x2231BA4C:"c78694070000ff",0x2231BA58:"57",
 0x2231BA5B:"ff5250",0x2231BA6E:"ff5264",
 0x2231BA72:"8bc6",0x2231BA75:"c3",
 0x22141A00:"83ec18",0x22141A04:"8b5c2420",
 0x22141A0B:"d98650020000",0x22141A13:"dae9",
 0x22141A17:"f6c444",0x22141A1A:"7a28",
 0x22141A25:"dae9",0x22141A29:"f6c444",
 0x22141A2C:"7a16",0x22141A37:"dae9",
 0x22141A3B:"f6c444",0x22141A3E:"0f8be7000000",
 0x22141A45:"6a04",0x22141A4E:"80a6b9000000ef",
 0x22141A57:"898650020000",0x22141A60:"898e54020000",
 0x22141A69:"899658020000",0x22141A6F:"8b86f0000000",
 0x22141A75:"83f8ff",0x22141A80:"8bc8",
 0x22141A88:"c1e104",0x22141A8F:"c1e80c",
 0x22141A95:"7506",0x22141A9D:"53",
 0x22141AA9:"5f",0x22141AB2:"8b87b8000000",
 0x22141AB8:"c1e80c",0x22141ABD:"7407",
 0x22141AC6:"d903",0x22141ACE:"d95c240c",
 0x22141AD5:"d8a754020000",0x22141AE2:"d8a758020000",
 0x22141AEC:"8b8fb8000000",0x22141AF2:"c1e90b",
 0x22141AF8:"7407",0x22141B06:"81c768020000",
 0x22141B0C:"8d442410",0x22141B17:"83c40c",
 0x22141B1F:"8d8ec8020000",
 0x22141B2B:"5e",0x22141B30:"c20400",
}
CALLS = {
 0x2231B9F9:0x22266B00,0x2231BA08:0x22285AB0,
 0x2231BA32:0x222AA640,0x2231BA65:0x221439C0,
 0x22141A49:0x2214B620,0x22141AA4:0x2213E1A0,
 0x22141AC1:0x2213D750,0x22141AFC:0x2213FA60,
 0x22141B12:0x22268E90,0x22141B25:0x2213E1A0,
}
CALLER = {0x2231CA0B:0x2231B9F0,0x2231CA5C:0x22141A00}
LITERALS={0x2247A1D8:b"flak"+bytes((0,))}

def verify_original_flak_origin(pe: PEImage,*,check_hashes=True) -> dict:
    if pe.base != 0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for va,hex_bytes in OPCODES.items():
        raw=bytes.fromhex(hex_bytes)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError("original flak/origin opcode mismatch at %#x"%va)
    for va,target in {**CALLS,**CALLER}.items():
        wanted=bytes((0xE8,))+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError("original flak/origin CALL mismatch at %#x"%va)
    for va,raw in LITERALS.items():
        if pe.at(va,len(raw))!=raw:
            raise VerificationError("original flak classname literal mismatch")
    if check_hashes:
        for name,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError("original flak/origin full function SHA mismatch: "+name)
    return {
        "original_verified_full_function_hashes":len(REGIONS) if check_hashes else 0,
        "original_sparse_opcode_witnesses":len(OPCODES),
        "original_function_direct_call_edges":len(CALLS),
        "verified_original_scattergun_caller_edges":len(CALLER),
        "original_flak_classname_literal":"flak",
        "flak_spawn_function":"0x2231B9F0",
        "flak_spawn_length":134,
        "entity_origin_setter_function":"0x22141A00",
        "entity_origin_setter_length":307,
        "limitations":[
          "Native object/allocator ABI and parent transform runtime remain host-owned",
          "Original direct engine calls cannot be inferred from portable callback tests",
          "Function 0x22141A00 is an entity origin setter, not a hit/trace routine"
        ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase167_audit(client_bytes,server_bytes)
    report["original_server_flak_spawn_and_origin"]=verify_original_flak_origin(
        PEImage(server_bytes))
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+chr(10)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")
if __name__=="__main__":
    main()
