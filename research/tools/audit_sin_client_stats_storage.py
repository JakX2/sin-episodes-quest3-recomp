#!/usr/bin/env python3
"""Read-only SHA-authenticated original client Stats collection init audit.

Inspect only owner-held 2006 client.dll, emit metadata only. Never emit
original machine-code bytes or include proprietary executable in CI.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE,CLIENT_SHA256

REGIONS={
    "static_records_55":(0x241FF420,171,
        "a4d63f60e7bc440135df208c7bf1c5cf7aabe9f5c9d9ab08c855e0937d40d696"),
    "dynamic_records_11":(0x241FF8E0,166,
        "714148c283d7bb4f41c5501f11e24185444342f957574c8dc6fb28925e67130e"),
    "definition_dispatch":(0x24206430,89,
        "aac276edaceb675ad8a9cc12dd0a014518e396d7f275c21df71f934d92b7c9fc")
}
CALLS={
    0x241FF439:0x241FDBD0, # reserve vector 55
    0x241FF442:0x2416FCF0, # allocate 8-byte static record
    0x241FF45A:0x24206430, # type-dispatch factory
    0x241FF8FA:0x241FDC40, # reserve vector 11
    0x241FF909:0x2416FCF0, # allocate 20-byte dynamic record
}
ANCHORS={
    0x241FF435:bytes.fromhex("6a37"),   # 55
    0x241FF440:bytes.fromhex("6a08"),   # sizeof static element 8
    0x241FF45F:bytes.fromhex("895f04"), # record+4=0
    0x241FF465:bytes.fromhex("8907"),   # record+0=factory result
    0x241FF4BD:bytes.fromhex("83fd37"), # 55 loop iterations
    0x241FF8F6:bytes.fromhex("6a0b"),   # 11
    0x241FF907:bytes.fromhex("6a14"),   # sizeof dynamic 20
    0x241FF915:bytes.fromhex("8918"),   # 5 x 32-bit zeros
    0x241FF917:bytes.fromhex("895804"),
    0x241FF91A:bytes.fromhex("895808"),
    0x241FF91D:bytes.fromhex("89580c"),
    0x241FF920:bytes.fromhex("895810"),
    0x24205BC9:bytes.fromhex("83f90f"), # type dispatch only 0..15
    0x24205DEB:bytes.fromhex("33c05bc3"), # else factory nullptr
}
DISPATCH_VA=0x24205BC0
DISPATCH_SIZE=560
DISPATCH_SHA="3ec859fcfc931d3d695ff389347737f63d65334bcddc57e631cc5bfa876f54ac"
class AuditError(ValueError):pass

def verify(pe,expected=REGIONS,dispatcher_sha=DISPATCH_SHA):
    if pe.base!=0x24000000:
        raise AuditError("original client PE image base mismatch")
    for name,(va,size,sha) in expected.items():
        if hashlib.sha256(pe.read(va,size)).hexdigest()!=sha:
            raise AuditError("original Stats helper body SHA mismatch: "+name)
    if hashlib.sha256(pe.read(DISPATCH_VA,DISPATCH_SIZE)).hexdigest()!=dispatcher_sha:
        raise AuditError("original Stats type dispatch body SHA mismatch")
    for site,target in CALLS.items():
        if pe.read(site,5)!=b"\xe8"+struct.pack("<i",target-site-5):
            raise AuditError("original Stats helper call edge mismatch at %#x"%site)
    for site,raw in ANCHORS.items():
        if pe.read(site,len(raw))!=raw:
            raise AuditError("original Stats helper opcode mismatch at %#x"%site)
    return {
        "owner_original_client_sha256":CLIENT_SHA256,
        "original_helpers":[{"name":name,"va":hex(va),
            "length":size,"sha256":sha} for name,(va,size,sha) in REGIONS.items()],
        "type_dispatch_region_sha256":DISPATCH_SHA,
        "static_entry_count":55,
        "static_entry_bytes":8,
        "dynamic_entry_count":11,
        "dynamic_entry_bytes":20,
        "stat_factory_specialized_types":16,
        "above_type_range_returns_null":True,
        "validated_direct_call_edges":len(CALLS),
        "validated_byte_anchors":len(ANCHORS),
        "independent_engine_boot_proven":False,
        "limitations":["Native stat definition subtype constructors not ported",
            "Original CUtlVector binary ABI deliberately not reused",
            "Remaining Stats owner/layout/registration helpers unimplemented"]
    }
def audit_original(path):
    raw=pathlib.Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=CLIENT_SHA256:
        raise AuditError("original client.dll full SHA mismatch")
    return verify(OriginalPE(raw))
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("client")
    args=p.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
