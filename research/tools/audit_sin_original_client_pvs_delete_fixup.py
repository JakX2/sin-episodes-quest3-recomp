#!/usr/bin/env python3
"""Phase242: authenticate original SiN Episodes client red-black delete fixup.

Counts one authentic original method only when BOTH complete 2006 owner-
original retail PE32 DLLs pass whole-file SHA256 and the entire native method
matches its SHA256. No proprietary original function body, retail DLL or full
disassembly is stored in the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

ENTRY = 0x24107750
LENGTH = 2609
CODE_SHA256 = "7bed48e41a436fb3a0e2fc69e69b3cfe83c42aee02be58f5fd6872e4c7f0c12a"
TRAILING_INT3 = 15
RET4_OFFSETS = (0xA2E,)

# Sparse opcode evidence spread throughout the complete SHA-reviewed method.
# No complete original native method body or complete disassembly is published.
WITNESSES = {
    0x000: "8b542404",
    0x004: "53568bf1",
    0x008: "663b5610",
    0x00C: "bb01000000",
    0x011: "0f84070a0000",
    0x017: "a1c4fd3324",
    0x029: "a3c4fd3324",
    0x02E: "66893dbcfd3324",
    0x04A: "663bd7",
    0x05F: "66395906",
    0x0E5: "663b11",
    0x16A: "6683790600",
    0x175: "8b4e04",
    0x196: "e8d5eeffff",
    0x224: "84c3",
    0x431: "e8baf1ffff",
    0x4FB: "668b4906",
    0x56F: "e8fceaffff",
    0x5F4: "6683790600",
    0x620: "e8cbefffff",
    0x8BA: "e8b1e7ffff",
    0x983: "668b4906",
    0x9F6: "e8f5ebffff",
    0xA1C: "5f5d",
    0xA2E: "c20400",
}
# All actual direct original E8 rel32 edges in the entire original body,
# and the exactly one caller at the end of Phase241 original tree erase.
CALLS = (
    (0x241078E6, 0x241067C0),
    (0x24107B81, 0x24106D40),
    (0x24107CBF, 0x241067C0),
    (0x24107D70, 0x24106D40),
    (0x2410800A, 0x241067C0),
    (0x24108146, 0x24106D40),
    (0x24108669, 0x24107750),
)

def native_e8(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target-site-5)

def verify_native(image: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if image.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase242 original client PE image base mismatch")
    if image.section_of(ENTRY) != ".text" or \
            image.section_of(ENTRY+LENGTH-1) != ".text":
        raise VerificationError("Phase242 complete original method outside .text")
    raw=image.at(ENTRY,LENGTH)
    if check_hash and hashlib.sha256(raw).hexdigest()!=CODE_SHA256:
        raise VerificationError("Phase242 whole original native method SHA256 drift")
    if image.at(ENTRY+LENGTH,TRAILING_INT3)!=b"\xCC"*TRAILING_INT3:
        raise VerificationError("Phase242 original INT3 boundary drift")
    for offset in RET4_OFFSETS:
        if raw[offset:offset+3]!=b"\xC2\x04\x00":
            raise VerificationError("Phase242 original RET4 ABI drift")
    for offset,expected_hex in WITNESSES.items():
        witness=bytes.fromhex(expected_hex)
        if raw[offset:offset+len(witness)]!=witness:
            raise VerificationError(
                f"Phase242 original opcode witness drift: +{offset:#x}")
    for site,target in CALLS:
        if image.section_of(site)!=".text" or \
                image.at(site,5)!=native_e8(site,target):
            raise VerificationError(
                f"Phase242 original E8 CALL target drift: {site:#x}")
    return {
        "phase":242,
        "new_original_complete_SHA_methods":
            1 if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":
            LENGTH if check_hash and authentic_provenance else 0,
        "method_name":"client_pvs_red_black_delete_fixup",
        "original_entry":f"0x{ENTRY:08X}",
        "whole_original_code_bytes":LENGTH,
        "whole_original_code_SHA256":CODE_SHA256 if check_hash else None,
        "RET4_exits":len(RET4_OFFSETS),
        "trailing_INT3":TRAILING_INT3,
        "sparse_opcode_witnesses":len(WITNESSES),
        "native_E8_CALL_edges":len(CALLS),
        "rotation_calls_left":3,
        "rotation_calls_right":3,
        "native_NIL_index":0xFFFF,
        "node_parent_WORD_offset":4,
        "node_color_WORD_offset":6,
        "native_red_state":0,
        "native_black_state":1,
        "scope_limits":[
            "Complete original SHA review does not prove source-level equivalence.",
            "Original global nil sentinel/CRT initialization not recreated.",
            "Host rejection of corrupt trees is not an original x86 behavior.",
            "Native Phase241 eraser skips fixup for null replacements; preserve that gate.",
            "No standalone replacement client/server gameplay DLL boot demonstrated.",
        ],
    }

def audit(server: bytes, client: bytes) -> dict:
    for name,raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError(
                "Phase242 requires BOTH complete original retail DLL SHA256s: "+name)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=1 or \
            report["new_original_native_code_bytes"]!=2609:
        raise VerificationError("Phase242 original function accounting drift")
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    inputs=parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument("--archive",type=Path)
    inputs.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    if args.archive:
        if args.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll")!=1 or \
               z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase242 expects one root original DLL pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:parser.error("--server requires --client")
        result=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
