#!/usr/bin/env python3
"""Phase170: verify original SiN 2006 entity-factory entry points and registry.

Original owner-only PE32 verifier. SHA-pins the full original 4 function regions,
checks sparse x86 instructions, native direct calls, a genuine incoming flak
call, and the factory warning literal. Chained original client/server hashes
and all earlier native checks are performed by Phase169 auditor. Original
DLLs and complete native function bytes are NOT part of this repository.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_entity_angles import audit as phase169_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS={
  'create_entity_by_name':(0x22266B00,89,
    '3325571079cbbca426a5bbf82b3107161480a5084af1865ced302e7a2f2d236b'),
  'alternate_registry_dispatch':(0x22266B60,29,
    '0ca3b1d05299ea46641fd1e12e7c9c415d649a07be32acd90af6da9c0b44a9db'),
  'entity_factory_registry_singleton':(0x2228B110,48,
    '19b2485323f81a818354e47c14ed282e982f371827a7af4c1556a5293a49d558'),
  'entity_factory_registry_ctor':(0x2228AFB0,96,
    '7fd117f38076655eb4b0e4c2911d8ba3954986ec2488250b1efb4987b009faf6'),
}
OPCODES={
  0x22266B00:'56',
  0x22266B05:'83feff',
  0x22266B0D:'7425',
  0x22266B0F:'8b0d98215722',
  0x22266B18:'ff5054',
  0x22266B1D:'a364875522',
  0x22266B22:'7510',
  0x22266B26:'68a4424522',
  0x22266B2B:'ff1544213d22',
  0x22266B3E:'ff5204',
  0x22266B41:'85c0',
  0x22266B44:'c7056487552200000000',
  0x22266B4F:'7501',
  0x22266B52:'8b10',
  0x22266B56:'ff6224',
  0x22266B65:'8b4c2404',
  0x22266B69:'8b10',
  0x22266B6E:'ff5204',
  0x22266B71:'85c0',
  0x22266B73:'7501',
  0x22266B76:'8b10',
  0x22266B7A:'ff6220',
  0x2228B110:'b801000000',
  0x2228B115:'840504005922',
  0x2228B11B:'751d',
  0x2228B11D:'090504005922',
  0x2228B123:'b9b0ff5822',
  0x2228B12D:'6810ea3c22',
  0x2228B13A:'b8b0ff5822',
  0x2228B13F:'c3',
  0x2228AFB0:'56',
  0x2228AFB3:'c70624634522',
  0x2228AFBA:'c74604c0682822',
  0x2228AFC3:'6800080000',
  0x2228AFC8:'897e08',
  0x2228AFCB:'c7460c80000000',
  0x2228AFD2:'897e10',
  0x2228AFDA:'894608',
  0x2228AFE0:'b8ffff0000',
  0x2228AFE5:'6a01',
  0x2228AFE7:'66894614',
  0x2228AFEB:'66894618',
  0x2228AFF2:'6880000000',
  0x2228AFF8:'8d4e20',
  0x2228AFFB:'66897e16',
  0x2228AFFF:'66897e1a',
  0x2228B003:'89461c',
  0x2228B00C:'8bc6',
  0x2228B00F:'c3',
}
CALLS={
  0x22266B34:0x2228B110,
  0x22266B60:0x2228B110,
  0x2228B128:0x2228AFB0,
  0x2228B132:0x223A5639,
  0x2228AFD5:0x2226E390,
  0x2228B006:0x2228F360,
}
INCOMING={
  0x2231B9F9:0x22266B00,
}
LITERALS={0x224542A4:b"CreateEntityByName("}

def verify_original_factory(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for va,opcode in OPCODES.items():
        raw=bytes.fromhex(opcode)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError("original factory opcode mismatch at %#x"%va)
    for va,target in {**CALLS,**INCOMING}.items():
        raw=bytes((0xE8,))+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=raw:
            raise VerificationError("original factory CALL mismatch at %#x"%va)
    for va,literal in LITERALS.items():
        if pe.at(va,len(literal))!=literal:
            raise VerificationError("original CreateEntityByName diagnostic mismatch")
    if check_hashes:
        for name,(va,length,wanted) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=wanted:
                raise VerificationError("original factory whole-function SHA mismatch: "+name)
    return {
      "verified_complete_original_function_hashes":len(REGIONS) if check_hashes else 0,
      "verified_sparse_opcode_witnesses":len(OPCODES),
      "verified_native_direct_call_edges":len(CALLS),
      "verified_incoming_flak_creation_calls":len(INCOMING),
      "original_diagnostic_literal":"CreateEntityByName(",
      "original_methods":{
          name:{"address":hex(start),"native_length":length}
          for name,(start,length,_) in REGIONS.items()},
      "observed_controls":[
         "explicit edict index != -1 calls engine virtual +0x54",
         "null engine edict logs and still resolves classname",
         "primary factory dispatch clears global edict context before +0x24",
         "alternate factory dispatch tail calls +0x20 without clearing context",
         "singleton bit0 guard set before constructing fixed registry object",
         "constructor allocates 0x800 bytes, initializes +0x20 index with 0/0x80/1"
      ],
      "limitations":[
         "factory lookup, allocator, virtual callbacks, and live engine objects remain host-owned",
         "this is not a complete MSVC x86 ABI or playable original Windows DLL"
      ]
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=phase169_audit(client_bytes,server_bytes)
    result["original_server_entity_factory_registry"]=verify_original_factory(
        PEImage(server_bytes))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    report=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+chr(10)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding="utf-8")
    else:
        print(report,end="")
if __name__=="__main__":
    main()