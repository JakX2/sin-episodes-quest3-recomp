#!/usr/bin/env python3
"""Phase167 original 2006 SiN server.dll ScatterGun secondary-fire auditor.

SHA-pins two original complete function regions, 54 sparse opcode witnesses,
18 original CALL rel32 edges and three actual original data constants.
Chains original DLL hash and earlier native evidence checks through Phase166.
No proprietary original byte regions are committed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_scattergun_deferred import audit as phase166_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
 "scattergun_secondary_fire":(0x2231C810,612,
  "323253a226cad936b4bc6c1e56c349b39eae8391b5414e203d86ba725fe16d5e"),
 "shared_fire_vector_origin_helper":(0x222CBC20,225,
  "ee1c0a4bf7a3913881d5809a637847b1d3d2ee0251bfb27d0f1df630532f91f3"),
}
OPCODES = {
 0x2231C810:"81ec80000000",0x2231C823:"0f8442020000",
 0x2231C82E:"ff9020010000",0x2231C83C:"83bf9802000003",
 0x2231C84D:"ff92c4030000",0x2231C853:"a190215722",
 0x2231C85F:"dc05d08f3d22",0x2231C866:"8d8ea0040000",
 0x2231C88E:"84c0",0x2231C89A:"ff923c040000",
 0x2231C8A4:"83bef004000000",0x2231C8B1:"ff9080030000",
 0x2231C8CA:"ff92c4030000",0x2231C8D4:"ff9394030000",
 0x2231C8DD:"ff9314030000",0x2231C8E3:"8b86ec030000",
 0x2231C8F5:"a190215722",0x2231C8FA:"d8400c",
 0x2231C911:"8b1590215722",0x2231C917:"d9442410",
 0x2231C923:"8d8ea0040000",0x2231C929:"d95c2418",
 0x2231C936:"ff92d4030000",0x2231C948:"ff9084040000",
 0x2231C952:"ff923c040000",0x2231C96B:"8b86e8040000",
 0x2231C97F:"ff9250040000",0x2231C98D:"898684050000",
 0x2231C997:"ff90a0030000",0x2231C99D:"6a06",
 0x2231C9B4:"bf08000000",0x2231C9C0:"680000803f",
 0x2231C9CA:"8d4c2464",0x2231C9D5:"89542414",
 0x2231CA14:"d80d80a14722",0x2231CA2D:"d80d80a14722",
 0x2231CA43:"d80d80a14722",0x2231CA64:"0f8556ffffff",
 0x2231CA73:"c3",
 0x222CBC20:"83ec0c",0x222CBC2B:"85f6",
 0x222CBC35:"c20800",0x222CBC3C:"ff9020010000",
 0x222CBC4F:"ff9258030000",0x222CBC69:"8b16",
 0x222CBC7D:"8b10",0x222CBC91:"b001",
 0x222CBC97:"c20800",0x222CBCA2:"85ff",
 0x222CBCAD:"c20800",0x222CBCB9:"ff9258030000",
 0x222CBCDD:"ff9234070000",0x222CBCF8:"b001",
 0x222CBCFE:"c20800",
}
CALLS = {
 0x2231C81A:0x222AD370,0x2231C870:0x221732A0,
 0x2231C889:0x222CBC20,0x2231C8EC:0x220EEDE0,
 0x2231C90C:0x22173260,0x2231C92D:0x221732A0,
 0x2231C964:0x2231C740,0x2231C976:0x222AA670,
 0x2231C988:0x222AA770,0x2231C9A1:0x222AAFE0,
 0x2231C9AF:0x22033250,0x2231C9CE:0x22033280,
 0x2231C9F1:0x22269E50,0x2231C9FB:0x222AD370,
 0x2231CA0B:0x2231B9F0,0x2231CA5C:0x22141A00,
 0x222CBC24:0x222AD370,0x222CBC9B:0x22139F60,
}
CONSTANTS = {
 0x223D8FD0:struct.pack("<d",0.2),
 0x2247A180:struct.pack("<f",1500.0),
 0x2252E888:struct.pack("<f",0.13053),
}

def verify_original_secondary(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for va,opcode in OPCODES.items():
        expected=bytes.fromhex(opcode)
        if pe.at(va,len(expected))!=expected:
            raise VerificationError("original secondary-fire opcode mismatch at %#x"%va)
    for va,target in CALLS.items():
        expected=bytes((0xE8,))+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=expected:
            raise VerificationError("original secondary-fire CALL mismatch at %#x"%va)
    for va,expected in CONSTANTS.items():
        if pe.at(va,len(expected))!=expected:
            raise VerificationError("original secondary-fire constant mismatch at %#x"%va)
    if check_hashes:
        for label,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError("original secondary-fire whole-region SHA mismatch: "+label)
    return {
      "verified_complete_original_function_hashes":len(REGIONS) if check_hashes else 0,
      "original_opcode_witnesses":len(OPCODES),
      "verified_direct_call_edges":len(CALLS),
      "verified_original_constants":len(CONSTANTS),
      "secondary_fire_entry":"0x2231C810",
      "secondary_fire_native_bytes":612,
      "vector_helper_entry":"0x222CBC20",
      "vector_helper_native_bytes":225,
      "branch_contract":[
       "owner NULL returns",
       "mode+0x298==3 sets timer+0x4A0 to time+0.2, returns",
       "vector acquisition failure returns",
       "signed secondary+0x4F0 <=0 with virtual+0x43C true invokes v+0x380",
       "normal path sends (3,0), selects animation, writes two independently clocked timers",
       "owner v+0x3D4 always, early v+0x120 controls ammo/animation branch",
       "eight native spread and trace iterations, with x87 1500.0 scale"
      ],
      "limitations":[
       "machine-code-verified original; C++17 host abstraction is not original ABI",
       "engine pellet handling, ray casting and actual firing side effects remain opaque",
       "not playable standalone Windows/Android/Quest 3 replacement binaries"
      ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=phase166_audit(client_bytes,server_bytes)
    result["original_server_scattergun_secondary_fire"]=verify_original_secondary(
        PEImage(server_bytes))
    return result

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+chr(10)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding="utf-8")
    else:
        print(result,end="")
if __name__=="__main__":
    main()
