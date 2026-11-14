#!/usr/bin/env python3
"""Phase223: original SiN 2006 client.dll type-4 quaternion specialists.

Read-only and gated by BOTH complete original retail DLL SHA256 fingerprints.
Only small, selective x86 instruction witnesses and whole-method hashes
are committed. No original DLL, full x86 body, or commercial asset is included.
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

# (original VA, complete native bytes, complete SHA256, INT3 pad, router CALL,
# original RET pop bytes, independently selected sparse opcode witnesses)
METHODS = {
    "type4_quaternion_x87_compare": (
        0x24198F50, 258,
        "b5d783421c4b15d965de57fde1cb89fe99b659ddac4bfecf2a68aeee3b603fb4",
        14, 0x241998EC, 12, {
            0x00:"83ec18", 0x03:"80791400", 0x14:"f6401304",
            0x24:"0f85be000000", 0x54:"d901",
            0x59:"dae9", 0x5d:"f6c444", 0x60:"7a0d",
            0x6f:"d9042f", 0x72:"d827",
            0x74:"d95cb418",  # FSTP [esp + esi*4 + 0x18], NOT 4 component slots
            0x84:"f6c441", 0x87:"756d",
            0x89:"d9442418", 0x95:"f6c441",
            0x98:"7a5c", 0xcd:"c744241402000000",
            0xf3:"c20c00", 0xff:"c20c00",
        }
    ),
    "type4_quaternion_diagnostics": (
        0x241987C0, 406,
        "20111b64456070863f319501c20ba6c558dba85c4b5d3200f54024dcd1ea4323",
        10, 0x24199908, 16, {
            0x00:"55", 0x03:"83e4f8", 0x0e:"807f1400",
            0x12:"0f8475010000", 0x18:"837d0800",
            0x28:"d9460c", 0x2b:"d8600c",
            0x53:"dd5c2420", 0x86:"83ec10",
            0xdf:"8d5020", 0x133:"6868412a24",
            0x13c:"e80ff0ffff", 0x147:"d94608",
            0x17e:"68f8402a24", 0x185:"e876f0ffff",
            0x193:"c21000",
        }
    ),
    "type4_quaternion_preview": (
        0x241989A0, 58,
        "86c7478c414c10d3e4e9247e47cbdbad416e67b4f2e936df2987cb756b3a0afb",
        6, 0x24199917, 16, {
            0x00:"8b4134", 0x03:"3b4118",
            0x06:"752f", 0x08:"8b442408",
            0x0c:"d9400c", 0x0f:"83ec20",
            0x24:"d900", 0x29:"68c4412a24",
            0x2f:"e80cf6ffff", 0x34:"83c428",
            0x37:"c21000",
        }
    ),
}
FORMAT_STRINGS = {
    0x242A4168: "quaternion[] differs (1st diff) (net %f %f %f %f - pred %f %f %f %f) delta(%f %f %f %f)\n",
    0x242A40F8: "quaternion (%f %f %f %f)\n",
    0x242A41C4: "quaternion (%f %f %f %f)",
}
NESTED_CALLS = (
    (0x241988FC, 0x24197910),
    (0x24198945, 0x241979C0),
    (0x241989CF, 0x24197FE0),
)
COPY_CALL = (0x241998FD, 0x24198DE0)
FLOAT_ZERO = 0x242DE2F8
NEXT_FUNCTIONS = {
    0x24199060: bytes.fromhex("80791400"),  # following native compare
    0x24198960: bytes.fromhex("8b41343b4118"), # neighboring RET 12 preview
    0x241989E0: bytes.fromhex("80792c00"),  # Phase218 word copy
}
# These original instructions warrant an explicit limitation instead of
# falsely substituting four independent, freshly initialized delta floats.
UNINITIALIZED_STACK_SLOT_WRITER = (0x24198FC4, bytes.fromhex("d95cb418"))
DELTA_SLOT_READS = (0x24198FD9,0x24198FEA,0x24198FFB,0x2419900C)


def rel_call(site: int, target: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target - site - 5)


def verify_native(client: PEImage, *, check_hash: bool = True) -> dict:
    if client.base != BASE["client"]:
        raise VerificationError("Phase223 original client PE32 image base mismatch")
    if TYPE_CASES[4] != 4 or COPY_METHODS["type4_stride16_forward_copy"][0] != COPY_CALL[1]:
        raise VerificationError("Phase223 Phase212/221 type4 evidence disagreement")
    summary = {}
    witness_count = 0
    for name, (va, size, digest, pad, caller, pop, anchors) in METHODS.items():
        if client.section_of(va) != ".text" or client.section_of(va+size-1) != ".text":
            raise VerificationError("Phase223 original function outside .text: "+name)
        body=client.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest() != digest:
            raise VerificationError("Phase223 whole native method SHA256 mismatch: "+name)
        if body[-3:] != b"\xc2"+struct.pack("<H",pop):
            raise VerificationError("Phase223 original native RET arity drift: "+name)
        if client.at(va+size,pad) != b"\xcc"*pad:
            raise VerificationError("Phase223 original INT3 boundary drift: "+name)
        for off,hx in anchors.items():
            sig=bytes.fromhex(hx)
            if body[off:off+len(sig)] != sig:
                raise VerificationError("Phase223 sparse opcode drift: "+name+" "+hex(off))
            witness_count+=1
        if (caller,va) not in CALL_EDGES or (
            client.section_of(caller) != ".text" or
            client.at(caller,5) != rel_call(caller,va)):
            raise VerificationError("Phase223 router CALL rel32 drift: "+name)
        summary[name]={
            "entry_va":f"0x{va:08X}", "whole_native_bytes":size,
            "whole_original_sha256":digest if check_hash else None,
            "router_callsite":f"0x{caller:08X}",
            "ret_stack_pop_bytes":pop, "int3_padding_bytes":pad,
        }
    for site,target in (COPY_CALL,)+NESTED_CALLS:
        if (client.section_of(site) != ".text" or
            client.section_of(target) != ".text" or
            client.at(site,5) != rel_call(site,target)):
            raise VerificationError("Phase223 original copy/formatter CALL drift")
    for va,value in FORMAT_STRINGS.items():
        if (client.section_of(va) != ".rdata" or
            client.at(va,len(value)+1) != value.encode("ascii")+b"\0"):
            raise VerificationError("Phase223 native quaternion format literal drift")
    if client.section_of(FLOAT_ZERO) != ".rdata" or client.at(FLOAT_ZERO,4) != b"\0"*4:
        raise VerificationError("Phase223 original tolerance zero constant drift")
    for va,first in NEXT_FUNCTIONS.items():
        if client.section_of(va) != ".text" or client.at(va,len(first)) != first:
            raise VerificationError("Phase223 neighboring native entry drift")
    slot_site,slot_opcode=UNINITIALIZED_STACK_SLOT_WRITER
    if client.at(slot_site,len(slot_opcode)) != slot_opcode:
        raise VerificationError("Phase223 critical indexed x87 FSTP writer drift")
    if any(client.at(site,1) != b"\xd9" for site in DELTA_SLOT_READS):
        raise VerificationError("Phase223 original x87 scratch slot reader drift")
    return {
        "new_SHA_verified_original_native_methods":len(METHODS) if check_hash else 0,
        "new_complete_original_x86_bytes":sum(x[1] for x in METHODS.values()) if check_hash else 0,
        "method_evidence":summary,
        "sparse_native_opcode_witnesses":witness_count,
        "native_type4_router_direct_CALLs":4,
        "native_nested_formatter_CALLs":len(NESTED_CALLS),
        "format_literals":len(FORMAT_STRINGS),
        "native_x87_scratch_witness":"FSTP DWORD [esp+esi*4+0x18] before four fixed stack-slot comparisons",
        "limits":[
            "The original x87 approximate-match branch accesses four stack delta slots but writes only the record-indexed slot; other slots may be uninitialized.",
            "No unconditional 4-component per-axis tolerance semantics can be claimed from this native body.",
            "Only exact-match, enabled/forced/empty gates and callback activation are safely portable without explicit stack scratch inputs.",
            "Native x87 control-word/traps, original formatter implementation, engine object lifetime and gameplay integration are not emulated.",
        ],
    }


def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    if (hashlib.sha256(server_bytes).hexdigest()!=FULL_SHA["server"] or
        hashlib.sha256(client_bytes).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase223 requires both SHA256-matched original retail DLLs")
    return verify_native(PEImage(client_bytes))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive cannot be combined with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("owner original bin.zip requires one root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--client is required with --server")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))


if __name__=="__main__":
    main()
