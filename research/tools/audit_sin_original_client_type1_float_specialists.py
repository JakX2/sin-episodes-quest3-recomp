#!/usr/bin/env python3
"""Phase216: read-only original 2006 SiN client.dll type-1 float specialists.

Requires owner-local complete SHA256-matched original DLLs. Publishes only
method digests, sparse opcode witnesses, CALL targets and source-like findings.
No proprietary complete x86 bodies, raw dumps, or game assets are included.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA, BASE
from audit_sin_original_client_snapshot_record_router import CALL_EDGES

# (entry VA, complete native byte length, native SHA256, INT3 padding,
#  original Phase212 caller, RET cleanup, sparse complete-instruction witnesses)
METHODS = {
    "type1_float_comparison": (
        0x24198AA0, 152,
        "a9234d21d65a6b08005d10cefaa7f76be3733a5cf406734afebd74dc85fa437d",
        8, 0x2419984C, 12, {
            0x01:"80791400", 0x10:"f6401304", 0x18:"bd01000000",
            0x1F:"8b4030", 0x26:"d9442410", 0x2A:"d81df8e22d24",
            0x32:"f6c441", 0x51:"d901", 0x56:"dae9",
            0x5A:"f6c444", 0x5D:"7b1b", 0x63:"d9040e",
            0x68:"d9e1", 0x6A:"d85c2410", 0x70:"f6c441",
            0x73:"7a19", 0x75:"bd02000000",
            0x8B:"c20c00", 0x95:"c20c00",
        }
    ),
    "type1_float_diagnostics": (
        0x24197CA0, 92,
        "1a1bf9db4a3e6b585669372ce3f80b0408033bc0d23b0d2ebad627c0b86fd728",
        4, 0x24199866, 16, {
            0x03:"807f1400", 0x0E:"85db",
            0x1B:"d906", 0x1D:"d820",
            0x22:"dd5c2410", 0x26:"d906",
            0x2C:"d900", 0x31:"68303f2a24",
            0x3F:"d906", 0x47:"68243f2a24",
            0x59:"c21000",
        }
    ),
    "type1_float_preview": (
        0x24198470, 37,
        "0e047eb6c2d6c90164f88e69be9fe7853f57a7268b48cacfd86ab1023ce0d41d",
        11, 0x24199875, 16, {
            0x00:"8b4134", 0x03:"3b4118",
            0x0C:"d902", 0x11:"dd1c24",
            0x14:"6840402a24", 0x1A:"e851fbffff",
            0x22:"c21000",
        }
    ),
}
NESTED_CALLS = (
    (0x24197CD7,0x24197910),
    (0x24197CEE,0x241979C0),
    (0x2419848A,0x24197FE0),
)
FLOAT_ZERO_VA = 0x242DE2F8
STRINGS = {
    0x242A3F24: "float (%f)\n",
    0x242A3F30: "float differs (net %f pred %f) diff(%f)\n",
    0x242A4040: "float (%f)",
}


def rel_call(site: int, dest: int) -> bytes:
    return b"\xe8" + struct.pack("<i", dest - site - 5)


def verify_native(client: PEImage, *, check_hash: bool = True) -> dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase216 incorrect original client PE32 base")
    evidence = {}
    witness_count = 0
    for name, (va, length, digest, pad, site, ret_bytes, witnesses) in METHODS.items():
        if client.section_of(va) != ".text" or client.section_of(va+length-1) != ".text":
            raise VerificationError("Phase216 native function outside original .text: "+name)
        body = client.at(va, length)
        if check_hash and hashlib.sha256(body).hexdigest() != digest:
            raise VerificationError("Phase216 whole original method SHA256 drift: "+name)
        if body[-3:] != b"\xc2" + struct.pack("<H",ret_bytes):
            raise VerificationError("Phase216 native RET arity mismatch: "+name)
        if client.at(va+length,pad) != b"\xcc"*pad:
            raise VerificationError("Phase216 original code/INT3 boundary drift: "+name)
        for off, hx in witnesses.items():
            raw = bytes.fromhex(hx)
            if body[off:off+len(raw)] != raw:
                raise VerificationError("Phase216 sparse native opcode drift: "+name+" +"+hex(off))
            witness_count += 1
        if (site,va) not in CALL_EDGES or client.at(site,5) != rel_call(site,va):
            raise VerificationError("Phase216 original router CALL mismatch: "+name)
        evidence[name] = {
            "entry_va":f"0x{va:08X}", "complete_native_bytes":length,
            "complete_original_native_sha256":digest if check_hash else None,
            "original_router_callsite":f"0x{site:08X}",
            "original_stack_cleanup":ret_bytes,
        }
    for site,dest in NESTED_CALLS:
        if (client.section_of(site) != ".text" or client.section_of(dest) != ".text"
                or client.at(site,5) != rel_call(site,dest)):
            raise VerificationError("Phase216 nested original logging CALL drift")
    for va,value in STRINGS.items():
        if client.section_of(va) != ".rdata" or (
                client.at(va,len(value)+1) != value.encode("ascii")+b"\0"):
            raise VerificationError("Phase216 original float diagnostic string drift")
    if client.section_of(FLOAT_ZERO_VA) != ".rdata" or (
            client.at(FLOAT_ZERO_VA,4) != struct.pack("<f",0.0)):
        raise VerificationError("Phase216 original x87 positive-tolerance zero constant drift")
    return {
        "new_original_verified_client_functions":3 if check_hash else 0,
        "new_complete_original_native_bytes":281 if check_hash else 0,
        "reviewed_methods":evidence,
        "sparse_opcode_witnesses":witness_count,
        "original_router_calls":3,
        "original_nested_trace_calls":3,
        "original_zero_float_constant_va":f"0x{FLOAT_ZERO_VA:08X}",
        "status_values":{"0":"mismatch / disabled comparison",
                         "1":"identical or descriptor forced match",
                         "2":"nonidentical but all within positive tolerance"},
        "limits":[
            "Complete native callee methods are verified, not the entire owning Source engine serializer.",
            "x87 floating point exceptions and host-only memory protections are not emulated.",
            "Log callbacks are modeled but their original engine ownership is not identified.",
            "No original binary, complete x86 function code or gameplay assets are published.",
        ],
    }


def audit(server_bytes:bytes, client_bytes:bytes) -> dict:
    if (hashlib.sha256(server_bytes).hexdigest() != FULL_SHA["server"]
            or hashlib.sha256(client_bytes).hexdigest() != FULL_SHA["client"]):
        raise VerificationError("Phase216 requires both original complete 2006 SiN DLL SHA256s")
    return verify_native(PEImage(client_bytes))


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    report=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+"\n"
    if a.output: a.output.write_text(report,encoding="utf-8")
    else: print(report,end="")


if __name__=="__main__":
    main()
