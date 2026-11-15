#!/usr/bin/env python3
"""Phase241: authenticate original SiN Episodes client PVS map tree erase.

Genuine native-method credit is possible ONLY after BOTH entire owner-original
2006 retail PE32 DLL SHA256s match. Never ship complete proprietary x86 method
bytes, retail DLLs, or whole disassemblies in the public repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

ENTRY = 0x241083C0
NATIVE_BYTES = 693
WHOLE_ORIGINAL_CODE_SHA256 = (
    "887ac931d635a7b9e913bb3c68128628eb40a893b626907230f872df0304419d"
)
TRAILING_INT3_BYTES = 11
RET4_OFFSET = 0x2B2

# Only independent sparse opcode witnesses, never a full native code body.
WITNESSES = {
    0x000: "51538b5c240c55bd",
    0x007: "bdffff0000",
    0x00c: "663bdd",
    0x012: "0f8496020000",
    0x01f: "663928",
    0x022: "0f84bd000000",
    0x030: "66396802",
    0x0e5: "8bfb",
    0x0ef: "663928",
    0x117: "6681fdffff",
    0x191: "663bfb",
    0x19c: "0f84f5000000",
    0x1ba: "6689442a04",
    0x1e2: "663b5e10",
    0x22f: "66897e10",
    0x293: "8b6c2418",
    0x297: "6681fdffff",
    0x29f: "837c240c01",
    0x2a6: "558bce",
    0x2ae: "5e5d5b59",
    0x2b2: "c20400",
}

# This entire native body has 24 direct E8 calls to the Phase238 node
# resolver and exactly one final E8 call to the unrecovered delete fixup.
NODE_RESOLVER_CALLS = (
    0x241083DA, 0x241083EB, 0x241083FD, 0x241084AA,
    0x241084B7, 0x241084C5, 0x241084E1, 0x24108501,
    0x2410850D, 0x2410851A, 0x2410852F, 0x2410854C,
    0x24108565, 0x2410857F, 0x24108593, 0x241085AB,
    0x241085B7, 0x241085C4, 0x241085D9, 0x241085F6,
    0x24108605, 0x2410861B, 0x2410862B, 0x24108642,
)
CALLS = (
    tuple((site, 0x24105F40) for site in NODE_RESOLVER_CALLS)
    + (
        (0x24108669, 0x24107750),
        (0x241087D0, 0x241083C0), # original Phase236 release frontend A
        (0x24108850, 0x241083C0), # original Phase236 release frontend B
    )
)

def native_call_bytes(site: int, target: int) -> bytes:
    return b"\xE8" + struct.pack("<i", target - site - 5)

def verify_native(client: PEImage, *, check_hash: bool = True,
                  authentic_provenance: bool = False) -> dict:
    if client.base != ORIGINALS["client"]["base"]:
        raise VerificationError("Phase241 original client DLL image base mismatch")
    if client.section_of(ENTRY) != ".text" or \
            client.section_of(ENTRY + NATIVE_BYTES - 1) != ".text":
        raise VerificationError("Phase241 original method leaves retail .text")
    body = client.at(ENTRY, NATIVE_BYTES)
    if check_hash and hashlib.sha256(body).hexdigest() != WHOLE_ORIGINAL_CODE_SHA256:
        raise VerificationError("Phase241 complete original native method SHA256 drift")
    if body[RET4_OFFSET:RET4_OFFSET+3] != b"\xC2\x04\x00":
        raise VerificationError("Phase241 native RET4 function boundary drift")
    if client.at(ENTRY+NATIVE_BYTES,TRAILING_INT3_BYTES) != \
            b"\xCC"*TRAILING_INT3_BYTES:
        raise VerificationError("Phase241 original trailing INT3 boundary drift")
    for off,hx in WITNESSES.items():
        expected = bytes.fromhex(hx)
        if body[off:off+len(expected)] != expected:
            raise VerificationError(
                f"Phase241 sparse native opcode drift at original +{off:#x}"
            )
    for site,target in CALLS:
        if client.section_of(site) != ".text" or \
                client.at(site,5) != native_call_bytes(site,target):
            raise VerificationError(
                f"Phase241 native E8 rel32 call drift at {site:#x}"
            )
    return {
        "phase":241,
        "new_original_complete_SHA_methods":(
            1 if check_hash and authentic_provenance else 0
        ),
        "new_original_native_code_bytes":(
            NATIVE_BYTES if check_hash and authentic_provenance else 0
        ),
        "original_client_method_VA":f"0x{ENTRY:08X}",
        "entire_original_native_code_SHA256":(
            WHOLE_ORIGINAL_CODE_SHA256 if check_hash else None
        ),
        "original_method_bytes":NATIVE_BYTES,
        "original_RET4_exits":1,
        "original_trailing_INT3_bytes":TRAILING_INT3_BYTES,
        "original_sparse_opcode_witnesses":len(WITNESSES),
        "original_direct_E8_CALL_edges":len(CALLS),
        "original_map_index_sentinel":0xFFFF,
        "original_node_stride":16,
        "original_erase_fixup_VA":"0x24107750",
        "original_erase_fixup_guard":[
            "selected_replacement_index != 0xFFFF",
            "physically_removed_node_color_WORD == 1"
        ],
        "scope_limits":[
            "The original 0x24107750 delete-fixup body is not recovered.",
            "This host reference models mutation ordering, not exact x86 register effects.",
            "Malformed original pointers are unsafe; the host reference rejects them.",
            "Red-black invariants are not promised for black-leaf native early exits.",
            "Rebuilt independent gameplay DLL boot in original Windows engine not shown.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for dll,raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[dll]["sha256"]:
            raise VerificationError(
                "Phase241 requires BOTH complete owner-original retail DLL SHA256s: "+dll
            )
    result=verify_native(PEImage(client),authentic_provenance=True)
    if result["new_original_complete_SHA_methods"]!=1 or \
            result["new_original_native_code_bytes"]!=693:
        raise VerificationError("Phase241 authentic native method count drift")
    return result

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    args=parser.parse_args()
    if args.archive:
        if args.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or \
                    z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase241 needs exactly one root retail DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:parser.error("--server also requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
