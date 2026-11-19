#!/usr/bin/env python3
"""Read-only owner-original 2006 Stats catalog setup helper audit."""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE,CLIENT_SHA256

VA=0x241FC5A0
SIZE=292
SHA="18bc538a9afd444683914b08aa6bf7d73b2d8dd403e15f6b07be0b32bb2ea6cd"
CALLS={
    0x241FC5B1:0x241FC070, # clear previous vector
    0x241FC5BA:0x241FB810, # reserve 66 pointer slots
    0x241FC5CA:0x2416FCF0, # malloc 12-byte catalog object
    0x241FC65A:0x24164C60, # resources enumeration handle
    0x241FC66E:0x24165010, # enumerator init
    0x241FC68C:0x24164A50, # query enumerator resource set
    0x241FC693:0x24164BC0, # first
    0x241FC6A5:0x241FBD20, # append resource result
    0x241FC6AC:0x24164BE0, # next
    0x241FC6B9:0x24166050, # release enumerator
}
ANCHORS={
    0x241FC5A4:bytes.fromhex("8db184000000"),
    0x241FC5B6:bytes.fromhex("6a42"),
    0x241FC5C8:bytes.fromhex("6a0c"),
    0x241FC5D6:bytes.fromhex("c70000000000"),
    0x241FC5DC:bytes.fromhex("c7400400000000"),
    0x241FC5E3:bytes.fromhex("c74008ffffffff"),
    0x241FC648:bytes.fromhex("b909000000"),
    0x241FC64D:bytes.fromhex("bedce02a24"),
    0x241FC656:bytes.fromhex("6a28"),
    0x241FC6C0:bytes.fromhex("83c42c"),
    0x241FC6C3:bytes.fromhex("c3"),
}
class AuditError(ValueError):pass
def verify(pe,expected_sha=SHA):
    if pe.base!=0x24000000:raise AuditError("Original image base mismatch")
    if hashlib.sha256(pe.read(VA,SIZE)).hexdigest()!=expected_sha:
        raise AuditError("Original catalog full function SHA mismatch")
    for site,target in CALLS.items():
        if pe.read(site,5)!=b"\xe8"+struct.pack("<i",target-site-5):
            raise AuditError("Original catalog direct callsite mismatch")
    for site,original in ANCHORS.items():
        if pe.read(site,len(original))!=original:
            raise AuditError("Original catalog opcode witness mismatch")
    return {
        "original_client_sha256":CLIENT_SHA256,
        "complete_original_helper_va":hex(VA),
        "complete_original_helper_bytes":SIZE,
        "complete_original_helper_sha256":SHA,
        "original_catalog_subobject_offset":0x84,
        "original_catalog_slot_count":66,
        "original_catalog_allocation_bytes_per_entry":12,
        "original_entry_initial_fields":[0,0,-1],
        "resource_enumeration_calls":7,
        "validated_direct_call_sites":len(CALLS),
        "validated_instruction_witnesses":len(ANCHORS),
        "limitations":["Original resource enumeration data model not reconstructed",
                       "Parser and resource records only modeled as host callback",
                       "No complete C_StatsSystem.Init or client gameplay boot"]
    }
def audit_original(path):
    raw=pathlib.Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=CLIENT_SHA256:
        raise AuditError("Owner-original client.dll SHA mismatch")
    return verify(OriginalPE(raw))
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client")
    args=parser.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
