#!/usr/bin/env python3
"""Phase219: original 2006 SiN client.dll type-6 Boolean snapshot helpers.

Read-only verifier requiring both whole original DLL SHA256s. Emits sparse
instruction evidence, never full proprietary x86 method bodies or assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA, BASE
from audit_sin_original_client_snapshot_record_router import CALL_EDGES, TYPE_CASES
from audit_sin_original_client_snapshot_field_copy import METHODS as COPY_METHODS

# (original VA, complete x86 bytes, whole-body SHA256, trailing INT3 length,
#  original Phase212 CALL site, RET stack cleanup, sparse full-instruction witnesses)
METHODS = {
    "type6_boolean_byte_comparison": (
        0x24197E30, 82,
        "ed96e875573440132a3652b4a5a419966e46722da843d655b63ac3efd506b334",
        14, 0x24199982, 12, {
            0x00:"80791400", 0x0B:"8b4118", 0x0E:"f6401304",
            0x16:"8b742414", 0x1A:"33c9", 0x1E:"7e21",
            0x20:"8b442410", 0x24:"8b7c240c", 0x30:"8a1407",
            0x33:"3a10", 0x37:"83c101", 0x3A:"83c001",
            0x42:"b801000000", 0x48:"c20c00", 0x4B:"5f33c05ec20c00",
        }
    ),
    "type6_boolean_diagnostics": (
        0x24197C30, 110,
        "2e200672f44e367c38412b1d9d0e6d19ad6aebfd6b3a4eb923ef04f74a17295b",
        2, 0x2419999E, 16, {
            0x00:"56", 0x03:"807e1400", 0x0A:"8b5c2410",
            0x0F:"8b7c2410", 0x13:"85ff", 0x16:"803b00",
            0x19:"b93c372d24", 0x21:"b914352a24",
            0x26:"8b442418", 0x2A:"803800",
            0x2D:"b83c372d24", 0x34:"b814352a24",
            0x3B:"68043f2a24", 0x41:"e89afcffff",
            0x49:"803b00", 0x53:"b814352a24",
            0x59:"68f83e2a24", 0x60:"e82bfdffff",
            0x6B:"c21000",
        }
    ),
    "type6_boolean_preview": (
        0x24198440, 45,
        "e0cb1517e7cab7e23b5d5fe5d6442515c73dd474ead4050d96b9e0831ae98e74",
        3, 0x241999AD, 16, {
            0x00:"8b4134", 0x03:"3b4118", 0x08:"8b542408",
            0x0C:"803a00", 0x0F:"b83c372d24",
            0x16:"b814352a24", 0x1C:"6834402a24",
            0x22:"e879fbffff", 0x27:"83c40c", 0x2A:"c21000",
        }
    )
}
# Two formatter wrappers in the original diagnostic function and the
# independent preview formatter. Their implementations are NOT claimed.
NESTED_CALLS=(
    (0x24197C71,0x24197910),
    (0x24197C90,0x241979C0),
    (0x24198462,0x24197FE0),
)
FORMAT_LITERALS={
    0x242A3F04:"bool differs (net %s pred %s)\n",
    0x242A3EF8:"bool (%s)\n",
    0x242A4034:"bool (%s)",
    0x242D373C:"true",
    0x242A3514:"false",
}
TRUE_VA=0x242D373C
FALSE_VA=0x242A3514


def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)


def verify_native(client:PEImage,*,check_hash:bool=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase219 incorrect original PE32 client base")
    if TYPE_CASES[6]!=6 or COPY_METHODS["type6_byte_copy"][0]!=0x24198A40:
        raise VerificationError("Phase219 authentic type6 case/copy metadata drift")
    result={}
    anchors=0
    for name,(va,size,sha,pad,caller,stack_pop,witnesses) in METHODS.items():
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase219 Boolean helper outside .text: "+name)
        body=client.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=sha:
            raise VerificationError("Phase219 whole native method SHA256 mismatch: "+name)
        if body[-3:]!=b"\xc2"+struct.pack("<H",stack_pop):
            raise VerificationError("Phase219 original RET arity mismatch: "+name)
        if client.at(va+size,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase219 method boundary / INT3 mismatch: "+name)
        for offset,hx in witnesses.items():
            needle=bytes.fromhex(hx)
            if body[offset:offset+len(needle)]!=needle:
                raise VerificationError("Phase219 original opcode mismatch: "+name+" "+hex(offset))
            anchors+=1
        if ((caller,va) not in CALL_EDGES or client.section_of(caller)!=".text" or
                client.at(caller,5)!=rel_call(caller,va)):
            raise VerificationError("Phase219 original router CALL mismatch: "+name)
        result[name]={
            "original_entry_va":f"0x{va:08X}",
            "original_complete_code_bytes":size,
            "original_full_method_sha256":sha if check_hash else None,
            "original_router_callsite":f"0x{caller:08X}",
            "original_ret_cleanup":stack_pop,
        }
    for site,dest in NESTED_CALLS:
        if (client.section_of(site)!=".text" or client.section_of(dest)!=".text" or
                client.at(site,5)!=rel_call(site,dest)):
            raise VerificationError("Phase219 original nested formatter CALL mismatch")
    for address,literal in FORMAT_LITERALS.items():
        if (client.section_of(address)!=".rdata" or
                client.at(address,len(literal)+1)!=literal.encode("ascii")+b"\0"):
            raise VerificationError("Phase219 original true/false/format literal drift")
    # The type-6 byte-copy method was separately complete-SHA verified
    # in Phase213. Confirm its original CALL site still matches Phase212.
    if (0x24199993,0x24198A40) not in CALL_EDGES or (
            client.at(0x24199993,5)!=rel_call(0x24199993,0x24198A40)):
        raise VerificationError("Phase219 original Phase213 copy integration drift")
    return {
        "new_complete_sha_verified_original_client_functions":len(METHODS) if check_hash else 0,
        "new_complete_original_native_bytes":sum(x[1] for x in METHODS.values()) if check_hash else 0,
        "method_evidence":result,
        "sparse_complete_instruction_witnesses":anchors,
        "original_direct_router_calls":4, # 3 new + Phase213 copy
        "original_nested_formatter_calls":len(NESTED_CALLS),
        "original_format_and_boolean_literals":len(FORMAT_LITERALS),
        "boolean_truth_test":"byte nonzero -> true, zero -> false",
        "limits":[
            "Native Boolean comparisons compare count bytes, not normalized Boolean truth values.",
            "Formatter implementations and original engine object lifetime are unknown.",
            "Host reference adds pointer/length guards not present in the original x86.",
            "Original Windows Source engine has not run reconstructed gameplay DLLs.",
        ],
    }


def audit(server_data:bytes,client_data:bytes)->dict:
    if (hashlib.sha256(server_data).hexdigest()!=FULL_SHA["server"] or
            hashlib.sha256(client_data).hexdigest()!=FULL_SHA["client"]):
        raise VerificationError("Phase219 requires SHA-matched complete original DLL pair")
    return verify_native(PEImage(client_data))


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    sources=p.add_mutually_exclusive_group(required=True)
    sources.add_argument("--archive",type=Path,help="owner-original private bin.zip")
    sources.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive may not be combined with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("owner archive requires one root DLL pair")
            server,client=z.read("server.dll"),z.read("client.dll")
    else:
        if not args.client:p.error("--server requires --client")
        server,client=args.server.read_bytes(),args.client.read_bytes()
    report=json.dumps(audit(server,client),indent=2)+"\n"
    if args.output:args.output.write_text(report,encoding="utf-8")
    else:print(report,end="")


if __name__=="__main__":
    main()
