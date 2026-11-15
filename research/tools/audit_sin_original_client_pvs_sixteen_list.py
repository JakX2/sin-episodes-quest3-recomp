#!/usr/bin/env python3
"""Phase233 original SiN client PVS 16-byte list append/unlink helpers.

Verifies BOTH owner-original retail PE32 DLL SHA256 digests, then two
complete original function body SHA256s. Never emits original proprietary
native bodies, disassembly, or retail game assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Native record: label, entry, full native length, whole-original SHA256,
# exact trailing INT3 pad length, C3 return offsets, RET4 return offsets,
# sparse original machine-instruction byte witnesses.
METHODS=(
    ("sixteen_byte_list_reserve_append",0x241086F0,96,
     "6d86459297ef74bda6ee895e7bff44215f4895c61d0942c5ab555303e0851197",
     0,(0x47,0x5F),(),{
        0x00:"56576a00",
        0x0B:"8bce8bf857",
        0x15:"0fb7c7c1e004",
        0x1D:"66c7400effff",
        0x23:"668b4e0e",
        0x27:"6689480c",
        0x2B:"66897e0e",
        0x33:"663dffff",
        0x39:"6683461201",
        0x4D:"c1e204",
        0x55:"6683461201",
        0x5A:"668bc75f5ec3",
     }),
    ("sixteen_byte_list_unlink",0x24106CB0,140,
     "65dbc842dd6a24b1934b0fd24295b119e0e9c7bcf2d67fb4f775e0daaaaca7ff",
     4,(),(0x6F,0x89),{
        0x00:"53668b5c2408",
        0x06:"663b5914",
        0x11:"c1e004",
        0x14:"66395c020c",
        0x25:"6681ffffff",
        0x30:"668b28",
        0x39:"66896c170e",
        0x43:"6689790c",
        0x5A:"66896c170c",
        0x67:"66814112ffff",
        0x75:"6689510e",
        0x81:"66814112ffff",
     }),
)
# Both true native nested method calls AND the two callers in Phase232.
CALLS=(
    (0x241086F6,0x24106610),  # original reserve_index(0), full body not reviewed
    (0x24108700,0x24106CB0),  # native unlink before append
    (0x241088B2,0x241086F0),  # PVS notifier registration
    (0x241089A2,0x24106CB0),  # PVS notifier unregistration
)
ORIGINAL_RECORD_STRIDE=16
ORIGINAL_NODE_PREV_WORD=0x0C
ORIGINAL_NODE_NEXT_WORD=0x0E
ORIGINAL_HEAD_WORD=0x0C
ORIGINAL_TAIL_WORD=0x0E
ORIGINAL_ACTIVE_COUNT_WORD=0x12
ORIGINAL_CAPACITY_WORD=0x14
ORIGINAL_FREE_MARKER=0xFFFF

def rel_call(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase233 original client base mismatch")
    rows=[]
    witness_count=0
    for name,va,length,digest,padding,c3_offsets,ret4_offsets,anchors in METHODS:
        if (client.section_of(va)!=".text" or
            client.section_of(va+length-1)!=".text"):
            raise VerificationError("Phase233 original complete method outside .text: "+name)
        raw=client.at(va,length)
        if check_hash and hashlib.sha256(raw).hexdigest()!=digest:
            raise VerificationError("Phase233 complete original x86 method SHA256 drift: "+name)
        if client.at(va+length,padding)!=b"\xCC"*padding:
            raise VerificationError("Phase233 original INT3 boundary drift: "+name)
        for offset in c3_offsets:
            if raw[offset:offset+1]!=b"\xC3":
                raise VerificationError("Phase233 original native C3 ABI drift: "+name)
        for offset in ret4_offsets:
            if raw[offset:offset+3]!=b"\xC2\x04\x00":
                raise VerificationError("Phase233 original native RET4 ABI drift: "+name)
        for offset,hx in anchors.items():
            expected=bytes.fromhex(hx)
            if raw[offset:offset+len(expected)]!=expected:
                raise VerificationError("Phase233 original native witness drift: "+name+" "+hex(offset))
            witness_count+=1
        rows.append({"name":name,"native_entry":f"0x{va:08X}",
                     "whole_original_native_bytes":length,
                     "original_body_sha256":digest if check_hash else None,
                     "original_C3_return_paths":len(c3_offsets),
                     "original_RET4_return_paths":len(ret4_offsets),
                     "original_trailing_INT3":padding})
    for site,target in CALLS:
        if (client.section_of(site)!=".text" or
            client.at(site,5)!=rel_call(site,target)):
            raise VerificationError("Phase233 authentic CALL rel32 drift "+hex(site))
    return {
        "phase":233,
        "new_original_complete_SHA_methods":len(rows)
            if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(m[2] for m in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":rows,
        "checked_sparse_opcode_witnesses":witness_count,
        "checked_original_DIRECT_CALL_edges":len(CALLS),
        "original_record_size":ORIGINAL_RECORD_STRIDE,
        "original_node_prev_WORD_offset":ORIGINAL_NODE_PREV_WORD,
        "original_node_next_WORD_offset":ORIGINAL_NODE_NEXT_WORD,
        "original_head_WORD_offset":ORIGINAL_HEAD_WORD,
        "original_tail_WORD_offset":ORIGINAL_TAIL_WORD,
        "original_active_count_WORD_offset":ORIGINAL_ACTIVE_COUNT_WORD,
        "original_capacity_WORD_offset":ORIGINAL_CAPACITY_WORD,
        "original_free_marker":ORIGINAL_FREE_MARKER,
    }

def audit(server:bytes,client:bytes)->dict:
    for name,raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase233 requires BOTH complete original retail DLL SHA256s: "+name)
    result=verify_native(PEImage(client),authentic_provenance=True)
    if result["new_original_complete_SHA_methods"]!=2 or result["new_original_native_code_bytes"]!=236:
        raise VerificationError("Phase233 original method ledger count drift")
    result["scope_limits"]=[
        "The native 16-byte reserve helper 0x24106610 is not yet whole-body reviewed.",
        "Host-managed list nodes preserve link/write ordering but are not original Source engine objects or allocator memory.",
        "Original 16-byte record payload beyond prev/next links belongs to other helpers and is not implemented here.",
        "The eight-byte auxiliary list is a separate structure already reviewed in Phase232.",
        "Not evidence that recreated Windows gameplay DLLs load or run in the retail Source game.",
    ]
    return result

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    source=ap.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    ap.add_argument("--client",type=Path)
    args=ap.parse_args()
    if args.archive:
        if args.client:ap.error("--archive cannot be combined with --client")
        with zipfile.ZipFile(args.archive) as archive:
            if (archive.namelist().count("server.dll")!=1 or
                archive.namelist().count("client.dll")!=1):
                raise VerificationError("Phase233 requires one DLL pair at archive root")
            result=audit(archive.read("server.dll"),archive.read("client.dll"))
    else:
        if not args.client:ap.error("--server requires --client")
        result=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(result,indent=2))
if __name__=="__main__":
    main()
