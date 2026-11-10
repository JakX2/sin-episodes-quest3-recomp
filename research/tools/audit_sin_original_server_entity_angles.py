#!/usr/bin/env python3
"""Phase169 owner-only original SiN 2006 server.dll entity-angle x86 audit.

SHA-pins four complete native original function regions, 96 sparse x86 opcode
witnesses, 13 direct CALL rel32 instructions and the ScatterGun flak-helper
incoming CALL. Chained original server/client SHA verification comes from
Phase168. Never commit any original executable bytes or complete disassembly.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_flak_origin import audit as phase168_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS = {
    "set_absolute_entity_angles": (0x221439C0, 412,
        "6ad0a90de973cdb3fded0dca507c5905d8c40ec3d06c9d3ee0f1cccd3b708205"),
    "native_angle_vector_different": (0x2213A360, 91,
        "0b856a01ccd342b7e18d656d1ac7c8708dd318942fe53d6efd11653ab4ef4410"),
    "native_angle_vector_equal": (0x220E2E70, 60,
        "a6ad19ca3a57bed3a25fdcddaae6a80dbac81ae489f7bba36a39ec52167e04ab"),
    "native_angle_vector_tracked_setter": (0x22140040, 99,
        "c3fcfa2b46be4418d626a752cd75307102200c365d9d1e308cd9afec3d7710e4"),
}
OPCODES = {
    0x220E2E70:"d901",
    0x220E2E7C:"f6c444",
    0x220E2E7F:"7a26",
    0x220E2E89:"dfe0",
    0x220E2E8B:"f6c444",
    0x220E2E96:"dae9",
    0x220E2E98:"dfe0",
    0x220E2E9D:"7a08",
    0x220E2E9F:"b801000000",
    0x220E2EA4:"c20400",
    0x2213A360:"83ec0c",
    0x2213A367:"d901",
    0x2213A371:"8b5004",
    0x2213A383:"f6c444",
    0x2213A386:"7a28",
    0x2213A391:"dfe0",
    0x2213A398:"d94108",
    0x2213A39B:"d9442408",
    0x2213A3A3:"f6c444",
    0x2213A3AA:"83c40c",
    0x2213A3AD:"c20400",
    0x2213A3B5:"83c40c",
    0x22140040:"d901",
    0x2214004C:"f6c444",
    0x2214004F:"7a1e",
    0x2214005E:"7a0f",
    0x22140068:"dfe0",
    0x2214006F:"80b984feffff",
    0x22140076:"7409",
    0x22140078:"c68194feffff",
    0x2214007F:"eb0d",
    0x22140081:"8b8100feffff",
    0x22140087:"85c0",
    0x22140089:"7403",
    0x2214008B:"830801",
    0x2214008E:"8b02",
    0x22140090:"8901",
    0x22140095:"894104",
    0x2214009B:"895108",
    0x221400A0:"c20400",
    0x221439C0:"83ec6c",
    0x221439CD:"d98654030000",
    0x221439DF:"dae9",
    0x221439E3:"f6c444",
    0x221439E6:"7a22",
    0x221439F2:"f6c444",
    0x221439F5:"7a13",
    0x22143A01:"f6c444",
    0x22143A04:"0f8b49010000",
    0x22143A0B:"6a02",
    0x22143A14:"80a6b9000000",
    0x22143A22:"8dae68020000",
    0x22143A30:"895308",
    0x22143A39:"8d86bc020000",
    0x22143A3F:"6a03",
    0x22143A47:"8b86f0000000",
    0x22143A50:"83f8ff",
    0x22143A55:"8b1520564f22",
    0x22143A5D:"81e1ff0f0000",
    0x22143A63:"c1e104",
    0x22143A6A:"c1e80c",
    0x22143A6D:"394104",
    0x22143A70:"7506",
    0x22143A72:"8b29",
    0x22143A76:"7519",
    0x22143A7A:"8b4f04",
    0x22143A80:"89442410",
    0x22143A8C:"e98e000000",
    0x22143A91:"8b85b8000000",
    0x22143A97:"c1e80b",
    0x22143A9A:"a801",
    0x22143AA5:"8d8d54030000",
    0x22143AAC:"8bcb",
    0x22143AB3:"84c0",
    0x22143AB5:"741a",
    0x22143AB7:"c74424100000",
    0x22143AC7:"c74424180000",
    0x22143ACF:"eb4e",
    0x22143AD1:"8b95b8000000",
    0x22143AD7:"c1ea0b",
    0x22143ADA:"f6c201",
    0x22143ADD:"7407",
    0x22143AE6:"8d44241c",
    0x22143AEB:"81c568020000",
    0x22143AF7:"8d4c2454",
    0x22143AFC:"8d8668020000",
    0x22143B03:"8d54242c",
    0x22143B0D:"8d442424",
    0x22143B1C:"83c41c",
    0x22143B23:"8dbe6c030000",
    0x22143B31:"84c0",
    0x22143B34:"741d",
    0x22143B36:"8d44240c",
    0x22143B42:"8b0d90215722",
    0x22143B53:"5f",
    0x22143B59:"c20400",
}
CALLS = {
    0x221439C8:0x2213FA60,
    0x22143A0F:0x2214B620,
    0x22143A33:0x22269F80,
    0x22143A42:0x22269000,
    0x22143AA0:0x2213FA60,
    0x22143AAE:0x220E2E70,
    0x22143AE1:0x2213FA60,
    0x22143AF2:0x22268F10,
    0x22143B08:0x22269520,
    0x22143B17:0x22268A60,
    0x22143B2C:0x2213A360,
    0x22143B3D:0x22140040,
    0x22143B4E:0x22042D70,
}
INCOMING_CALLS = {0x2231BA65:0x221439C0}

def verify_original_angles(pe: PEImage, *, check_hashes: bool=True) -> dict:
    if pe.base != 0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for address, hex_bytes in OPCODES.items():
        expected=bytes.fromhex(hex_bytes)
        if pe.at(address,len(expected))!=expected:
            raise VerificationError("original entity angle opcode mismatch at %#x"%address)
    for address,target in {**CALLS,**INCOMING_CALLS}.items():
        expected=bytes((0xe8,))+struct.pack("<i",target-address-5)
        if pe.at(address,5)!=expected:
            raise VerificationError("original entity angle CALL mismatch at %#x"%address)
    if check_hashes:
        for label,(start,length,expected_digest) in REGIONS.items():
            if hashlib.sha256(pe.at(start,length)).hexdigest()!=expected_digest:
                raise VerificationError("original entity angle full function SHA mismatch: "+label)
    return {
        "native_complete_function_hashes":len(REGIONS) if check_hashes else 0,
        "native_sparse_instruction_witnesses":len(OPCODES),
        "native_direct_call_edges":len(CALLS),
        "native_incoming_flak_call_edges":len(INCOMING_CALLS),
        "original_abs_angles_entry":"0x221439C0",
        "original_abs_angles_bytes":412,
        "native_helpers": ["0x2213A360","0x220E2E70","0x22140040"],
        "original_abs_angles_offsets":{
            "absolute_angles":"+0x354..+0x35C", "local_angles":"+0x36C..+0x374",
            "parent_handle":"+0xF0", "parent_flags":"+0xB8", "entity_flags":"+0xB9",
            "entity_matrix":"+0x268"},
        "behavior_evidence":[
            "always refresh absolute angle cache before equality check",
            "native x87 float equality, signed zeros equal and NaN unordered",
            "changed path emits notification kind 2 and clears entity flag bit 3",
            "absolute angles copied as three raw DWORDs before matrix updates",
            "12-bit parent index + 20-bit serial checked by native registry",
            "parent bit 11 can refresh twice on nonmatching-angle route",
            "matching parent angles become zero local angles",
            "native 0x2213A360 compares but does not store local angles",
            "only if local angles differ 0x22140040 signals and assigns tracked vector",
            "local update triggers clocked 0x22042D70 only if vector differs",
        ],
        "limitations":[
            "Original engine callbacks, matrix routines, and parent registry are not native ABI ports",
            "Standalone C++17 model does not create a playable original Win32/Android/Quest 3 game",
        ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase168_audit(client_bytes,server_bytes)
    report["original_server_entity_absolute_angles"]=verify_original_angles(PEImage(server_bytes))
    return report

def main()->None:
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