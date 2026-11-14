#!/usr/bin/env python3
"""Phase222: verify three original SiN 2006 client.dll type-3 vec3 helpers.

Read-only original-owner PE32 audit. No original DLL or complete native
function body is published. Original-free synthetic tests are separate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import BASE, FULL_SHA
from audit_sin_original_client_snapshot_record_router import CALL_EDGES, TYPE_CASES
from audit_sin_original_client_vector_copy import METHODS as COPY_METHODS

# entry, complete native bytes, complete body SHA256, trailing INT3 padding,
# Phase212 exact router CALL, i386 RET pop, sparse complete opcode witnesses
METHODS = {
    "type3_vec3_x87_comparison": (
        0x24198E20, 289,
        "1b13b7672ab3ff4932694d934d76d6f7bbdb656c1f09941b827d31d4c7842360",
        15, 0x241998B2, 12, {
            0x0:"83ec1c", 0x3:"80791400", 0x14:"f6401304",
            0x1c:"bd01000000", 0x50:"d902", 0x55:"dae9",
            0x59:"f6c444", 0x5c:"7a1d", 0x7d:"d861fc",
            0x80:"d95c2414", 0xab:"d9442410",
            0xb3:"d81df8e22d24", 0xbb:"f6c441", 0xcc:"f6c441",
            0xf3:"bd02000000", 0x112:"c20c00", 0x11e:"c20c00",
        }
    ),
    "type3_vec3_mismatch_and_status": (
        0x241985A0, 219,
        "d5abf2cbbbf02ba5dac02c32fb7c98f999d363042aae3413f7661808de71ec39",
        5, 0x241998CE, 16, {
            0x3:"83e4f8", 0xe:"807f1400", 0x29:"d906",
            0x2b:"d820", 0x30:"d95c2458", 0x42:"d95c245c",
            0x54:"d95c2460", 0x9f:"6898402a24", 0xa5:"e8c6f2ffff",
            0xad:"d94608", 0xc3:"68cc3f2a24",
            0xca:"e851f3ffff", 0xd8:"c21000",
        }
    ),
    "type3_vec3_preview": (
        0x241986C0, 51,
        "19c07ccae5e728b90a818a248a449719bc02e81bc6e41eccc1b03c4c184bb6a4",
        13, 0x241998DD, 16, {
            0x0:"8b4134", 0x3:"3b4118", 0x8:"8b442408",
            0xc:"d94008", 0x22:"68e4402a24",
            0x28:"e8f3f8ffff", 0x30:"c21000",
        }
    ),
}
NESTED_CALLS = (
    (0x24198645, 0x24197910),
    (0x2419866A, 0x241979C0),
    (0x241986E8, 0x24197FE0),
)
FORMATS = {
    0x242A4098: "vec[] differs (1st diff) (net %f %f %f - pred %f %f %f) delta(%f %f %f)\n",
    0x242A3FCC: "vector (%f %f %f)\n",
    0x242A40E4: "vector (%f %f %f)",
}
FLOAT_ZERO_VA = 0x242DE2F8
TYPE3_COPY_CALL = (0x241998C3, 0x24198DA0)


def call_bytes(site: int, target: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target - site - 5)


def verify_native(client: PEImage, *, check_hash: bool = True) -> dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase222 incorrect original client PE32 image base")
    if TYPE_CASES[3] != 3 or COPY_METHODS["type3_vec3_12byte_copy"][0] != 0x24198DA0:
        raise VerificationError("Phase222 original type3 switch/Phase221 copy drift")
    evidence = {}
    count = 0
    for name, (va, n, sha, pad, caller, pop, witnesses) in METHODS.items():
        if client.section_of(va) != ".text" or client.section_of(va + n - 1) != ".text":
            raise VerificationError("Phase222 native code not entirely original .text: "+name)
        body = client.at(va, n)
        if check_hash and hashlib.sha256(body).hexdigest() != sha:
            raise VerificationError("Phase222 complete native function SHA mismatch: "+name)
        if body[-3:] != b"\xc2" + struct.pack("<H", pop):
            raise VerificationError("Phase222 native RET pop mismatch: "+name)
        if client.at(va+n, pad) != b"\xcc" * pad:
            raise VerificationError("Phase222 exact original INT3 code boundary mismatch: "+name)
        for off, hx in witnesses.items():
            needle=bytes.fromhex(hx)
            if body[off:off+len(needle)] != needle:
                raise VerificationError("Phase222 sparse original opcode drift: "+name+" "+hex(off))
            count += 1
        if (caller,va) not in CALL_EDGES or (
                client.section_of(caller) != ".text" or
                client.at(caller,5) != call_bytes(caller,va)):
            raise VerificationError("Phase222 original Phase212 router CALL drift: "+name)
        evidence[name] = {
            "native_entry": f"0x{va:08X}",
            "complete_original_native_bytes": n,
            "whole_original_method_sha256": sha if check_hash else None,
            "original_router_call": f"0x{caller:08X}",
            "original_ret_pop": pop,
            "trailing_INT3_bytes": pad,
        }

    for site,dest in (TYPE3_COPY_CALL,) + NESTED_CALLS:
        if (client.section_of(site) != ".text" or
                client.section_of(dest) != ".text" or
                client.at(site,5) != call_bytes(site,dest)):
            raise VerificationError("Phase222 original copy/formatter CALL drift: "+hex(site))
    for va,value in FORMATS.items():
        if (client.section_of(va) != ".rdata" or
                client.at(va,len(value)+1) != value.encode("ascii")+b"\0"):
            raise VerificationError("Phase222 original vector format literal drift")
    if (client.section_of(FLOAT_ZERO_VA) != ".rdata" or
            client.at(FLOAT_ZERO_VA,4) != b"\0\0\0\0"):
        raise VerificationError("Phase222 original float zero constant drift")

    return {
        "new_whole_original_SHA_verified_methods": len(METHODS) if check_hash else 0,
        "new_whole_original_native_bytes": sum(m[1] for m in METHODS.values()) if check_hash else 0,
        "methods": evidence,
        "sparse_original_x86_instruction_witnesses": count,
        "original_case3_direct_calls": 4,
        "original_nested_formatter_CALLs": len(NESTED_CALLS),
        "original_format_literals": len(FORMATS),
        "original_float_zero_constant": f"0x{FLOAT_ZERO_VA:08X}",
        "limits": [
            "Native float comparison uses x87 FUCOMPP and three individually single-rounded float deltas.",
            "Host C++ floating-point behavior does not reproduce every possible x87 control-word setting or trap.",
            "Original formatter implementation, record ownership and engine lifecycle remain unresolved.",
            "No original commercial binary, complete function body, or runnable Windows gameplay DLL is distributed.",
        ],
    }


def audit(server: bytes, client: bytes) -> dict:
    if (hashlib.sha256(server).hexdigest() != FULL_SHA["server"] or
            hashlib.sha256(client).hexdigest() != FULL_SHA["client"]):
        raise VerificationError("Phase222 requires both whole SHA-matched original SiN DLLs")
    return verify_native(PEImage(client))


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive cannot accompany --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll") != 1 or z.namelist().count("server.dll") != 1:
                raise VerificationError("owner archive must have one root client/server DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))


if __name__ == "__main__":
    main()
